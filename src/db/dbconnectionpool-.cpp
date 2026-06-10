// DbConnectionPool.cpp
#include "dbconnectionpool.h"
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QtConcurrent>
#include <QApplication>

// ===== ConnectionGuard 实现 =====

ConnectionGuard::ConnectionGuard(SimpleConnectionPool* pool)
    : m_pool(pool), m_isValid(false)
{
    if (m_pool) {
        m_database = m_pool->getConnection();
        m_isValid = m_database.isValid() && m_database.isOpen();
    }
}

ConnectionGuard::~ConnectionGuard()
{
    if (m_isValid && m_pool && m_database.isValid()) {
        m_pool->releaseConnection(m_database);
    }
}

QSqlDatabase& ConnectionGuard::getDatabase()
{
    return m_database;
}

bool ConnectionGuard::isValid() const
{
    return m_isValid && m_database.isValid() && m_database.isOpen();
}

// ===== SimpleConnectionPool 实现 =====

SimpleConnectionPool& SimpleConnectionPool::getInstance()
{
    static SimpleConnectionPool instance;
    return instance;
}

SimpleConnectionPool::SimpleConnectionPool(QObject* parent)
    : QObject(parent)
    , m_maxConnections(20)
    , m_minConnections(2)
    , m_connectionTimeoutSeconds(300) // 5分钟超时
    , m_maintenanceTimer(new QTimer(this))
{
    // 每5分钟进行一次维护
    connect(m_maintenanceTimer, &QTimer::timeout, this, &SimpleConnectionPool::performMaintenance);
    m_maintenanceTimer->start(300000); // 5分钟
}

SimpleConnectionPool::~SimpleConnectionPool()
{
    closeAll();
}

bool SimpleConnectionPool::initialize(const DbType& dbType, const DbInfo& dbInfo, 
                                     int maxConnections, int minConnections)
{
    QMutexLocker locker(&m_mutex);
    
    // 清理现有连接
    closeAll();
    
    // 设置参数
    m_dbType = dbType;
    m_dbInfo = dbInfo;
    m_maxConnections = qMax(1, maxConnections);
    m_minConnections = qMax(1, qMin(minConnections, m_maxConnections));
    
    // 创建初始连接
    bool success = false;
    for (int i = 0; i < m_minConnections; ++i) {
        QSqlDatabase db = createNewConnection();
        if (db.isValid() && db.isOpen()) {
            ConnectionInfo info;
            info.database = db;
            info.connectionName = db.connectionName();
            info.createdTime = QDateTime::currentDateTime();
            info.lastUsedTime = QDateTime::currentDateTime();
            info.isValid = true;
            
            m_availableConnections.enqueue(info);
            success = true;
            
            emit debug(QString("创建初始连接: %1").arg(info.connectionName));
        } else {
            emit error(QString("创建初始连接失败: %1").arg(i));
        }
    }
    
    m_isInitialized = success;
    
    if (success) {
        emit debug(QString("连接池初始化成功，创建了 %1 个连接").arg(m_availableConnections.size()));
    } else {
        emit error("连接池初始化失败");
    }
    
    return success;
}

QSqlDatabase SimpleConnectionPool::getConnection(int timeoutMs)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isInitialized) {
        emit error("连接池未初始化");
        return QSqlDatabase();
    }
    
    QElapsedTimer timer;
    timer.start();
    
    while (timer.elapsed() < timeoutMs) {
        // 1. 尝试从可用连接队列获取连接
        if (!m_availableConnections.isEmpty()) {
            ConnectionInfo info = m_availableConnections.dequeue();
            
            // 验证连接有效性
            if (validateConnection(info.database)) {
                info.lastUsedTime = QDateTime::currentDateTime();
                m_activeConnections.append(info);
                
                qDebug() << "获取连接成功:" << info.connectionName 
                        << "耗时:" << timer.elapsed() << "ms";
                return info.database;
            } else {
                // 连接无效，关闭并丢弃
                qWarning() << "发现无效连接，已丢弃:" << info.connectionName;
                info.database.close();
                QSqlDatabase::removeDatabase(info.connectionName);
            }
        }
        
        // 2. 如果没有可用连接且未达到最大连接数，创建新连接
        int totalConnections = m_availableConnections.size() + m_activeConnections.size();
        if (totalConnections < m_maxConnections) {
            QSqlDatabase newDb = createNewConnection();
            if (newDb.isValid() && newDb.isOpen()) {
                ConnectionInfo info;
                info.database = newDb;
                info.connectionName = newDb.connectionName();
                info.createdTime = QDateTime::currentDateTime();
                info.lastUsedTime = QDateTime::currentDateTime();
                info.isValid = true;
                
                m_activeConnections.append(info);
                
                emit debug(QString("创建新连接: %1").arg(info.connectionName));
                qDebug() << "创建新连接成功:" << info.connectionName 
                        << "耗时:" << timer.elapsed() << "ms";
                return newDb;
            } else {
                emit error("创建新连接失败");
            }
        }
        
        // 3. 连接池已满，等待连接释放
        if (m_condition.wait(&m_mutex, 100)) {
            // 有连接被释放，继续循环尝试
            continue;
        }
    }
    
    // 超时
    emit error(QString("获取连接超时，等待时间: %1ms").arg(timeoutMs));
    qWarning() << "获取连接超时:" << timeoutMs << "ms";
    return QSqlDatabase();
}

void SimpleConnectionPool::releaseConnection(const QSqlDatabase& connection)
{
    if (!connection.isValid()) {
        return;
    }
    
    QString connectionName = connection.connectionName();
    QMutexLocker locker(&m_mutex);
    
    // 从活动连接列表中查找并移除
    for (auto it = m_activeConnections.begin(); it != m_activeConnections.end(); ++it) {
        if (it->connectionName == connectionName) {
            ConnectionInfo info = *it;
            m_activeConnections.erase(it);
            
            // 验证连接是否仍然有效
            if (validateConnection(info.database)) {
                info.lastUsedTime = QDateTime::currentDateTime();
                m_availableConnections.enqueue(info);
                
                qDebug() << "连接已释放并返回池中:" << connectionName;
            } else {
                // 连接无效，关闭并丢弃
                qWarning() << "释放无效连接:" << connectionName;
                info.database.close();
                QSqlDatabase::removeDatabase(connectionName);
            }
            
            // 通知等待的线程
            m_condition.wakeOne();
            return;
        }
    }
    
    qWarning() << "尝试释放不存在的连接:" << connectionName;
}

std::unique_ptr<ConnectionGuard> SimpleConnectionPool::getConnectionGuard()
{
    return std::make_unique<ConnectionGuard>(this);
}

int SimpleConnectionPool::getTotalConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_availableConnections.size() + m_activeConnections.size();
}

int SimpleConnectionPool::getActiveConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_activeConnections.size();
}

int SimpleConnectionPool::getAvailableConnections() const
{
    QMutexLocker locker(&m_mutex);
    return m_availableConnections.size();
}

void SimpleConnectionPool::closeAll()
{
    // 注意：调用者必须已持有锁
    
    // 关闭可用连接
    while (!m_availableConnections.isEmpty()) {
        ConnectionInfo info = m_availableConnections.dequeue();
        info.database.close();
        QSqlDatabase::removeDatabase(info.connectionName);
    }
    
    // 关闭活动连接
    for (const auto& info : m_activeConnections) {
        info.database.close();
        QSqlDatabase::removeDatabase(info.connectionName);
    }
    m_activeConnections.clear();
    
    // 通知所有等待的线程
    m_condition.wakeAll();
    
    m_isInitialized = false;
    
    emit debug("所有数据库连接已关闭");
}

QSqlDatabase SimpleConnectionPool::createNewConnection()
{
    // 生成唯一连接名
    int counter = m_connectionCounter.fetchAndAddOrdered(1);
    QString connectionName = QString("Connection_%1_%2_%3")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
        .arg(counter)
        .arg(QDateTime::currentMSecsSinceEpoch());
    
    // 根据数据库类型创建连接
    QString driverName;
    switch (m_dbType) {
        case DbType::PostgreSQL:
            driverName = "QPSQL";
            break;
        case DbType::MySQL:
            driverName = "QMYSQL";
            break;
        case DbType::SQLite:
            driverName = "QSQLITE";
            break;
        default:
            driverName = "QPSQL";
            break;
    }
    
    QSqlDatabase db = QSqlDatabase::addDatabase(driverName, connectionName);
    db.setDatabaseName(m_dbInfo.dbName);
    db.setHostName(m_dbInfo.hostName);
    db.setPort(m_dbInfo.hostPort);
    db.setUserName(m_dbInfo.userName);
    db.setPassword(m_dbInfo.userPwd);
    
    // 设置连接选项
    db.setConnectOptions("connect_timeout=10");
    
    // 尝试打开连接
    if (!db.open()) {
        QString errorText = db.lastError().text();
        emit error(QString("打开数据库连接失败: %1").arg(errorText));
        qWarning() << "打开数据库连接失败:" << errorText;
        QSqlDatabase::removeDatabase(connectionName);
        return QSqlDatabase();
    }
    
    qDebug() << "成功创建数据库连接:" << connectionName;
    return db;
}

bool SimpleConnectionPool::validateConnection(const QSqlDatabase& db)
{
    if (!db.isValid() || !db.isOpen()) {
        return false;
    }
    
    try {
        QSqlQuery query(db);
        QString testQuery = "SELECT 1";
        
        // 根据数据库类型调整测试查询
        if (db.driverName() == "QPSQL") {
            testQuery = "SELECT 1";
        } else if (db.driverName() == "QMYSQL") {
            testQuery = "SELECT 1";
        } else if (db.driverName() == "QSQLITE") {
            testQuery = "SELECT 1";
        }
        
        bool result = query.exec(testQuery);
        
        if (!result) {
            qDebug() << "连接验证失败:" << query.lastError().text() 
                    << "连接:" << db.connectionName();
        }
        
        return result;
    }
    catch (const std::exception& e) {
        qWarning() << "连接验证异常:" << e.what();
        return false;
    }
    catch (...) {
        qWarning() << "连接验证未知异常";
        return false;
    }
}

void SimpleConnectionPool::cleanupExpiredConnections()
{
    QDateTime now = QDateTime::currentDateTime();
    QQueue<ConnectionInfo> validConnections;
    
    // 检查可用连接队列中的过期连接
    while (!m_availableConnections.isEmpty()) {
        ConnectionInfo info = m_availableConnections.dequeue();
        
        // 检查是否过期（超过超时时间未使用）
        if (info.lastUsedTime.secsTo(now) < m_connectionTimeoutSeconds) {
            // 连接未过期，验证有效性
            if (validateConnection(info.database)) {
                validConnections.enqueue(info);
            } else {
                // 连接无效，关闭并丢弃
                qDebug() << "清理无效连接:" << info.connectionName;
                info.database.close();
                QSqlDatabase::removeDatabase(info.connectionName);
            }
        } else {
            // 连接已过期，关闭并丢弃
            qDebug() << "清理过期连接:" << info.connectionName;
            info.database.close();
            QSqlDatabase::removeDatabase(info.connectionName);
        }
    }
    
    // 重新加入有效连接
    m_availableConnections = validConnections;
}

void SimpleConnectionPool::ensureMinimumConnections()
{
    int totalConnections = m_availableConnections.size() + m_activeConnections.size();
    int connectionsToCreate = m_minConnections - totalConnections;
    
    if (connectionsToCreate > 0) {
        qDebug() << "补充连接，需要创建:" << connectionsToCreate;
        
        for (int i = 0; i < connectionsToCreate; ++i) {
            QSqlDatabase db = createNewConnection();
            if (db.isValid() && db.isOpen()) {
                ConnectionInfo info;
                info.database = db;
                info.connectionName = db.connectionName();
                info.createdTime = QDateTime::currentDateTime();
                info.lastUsedTime = QDateTime::currentDateTime();
                info.isValid = true;
                
                m_availableConnections.enqueue(info);
                emit debug(QString("补充连接: %1").arg(info.connectionName));
            } else {
                emit error(QString("补充连接失败: %1").arg(i));
                break; // 创建失败就停止尝试
            }
        }
    }
}

void SimpleConnectionPool::performMaintenance()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isInitialized) {
        return;
    }
    
    qDebug() << "开始数据库连接池维护...";
    
    // 1. 清理过期连接
    cleanupExpiredConnections();
    
    // 2. 确保最小连接数
    ensureMinimumConnections();
    
    // 3. 输出统计信息
    int total = m_availableConnections.size() + m_activeConnections.size();
    int active = m_activeConnections.size();
    int available = m_availableConnections.size();
    
    qDebug() << QString("连接池维护完成 - 总连接: %1, 活动: %2, 可用: %3")
                .arg(total).arg(active).arg(available);
    
    emit debug(QString("连接池状态: 总连接=%1, 活动=%2, 可用=%3")
               .arg(total).arg(active).arg(available));
}
