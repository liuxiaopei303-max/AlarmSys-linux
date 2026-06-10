// DbConnectionPool.cpp
#include "dbconnectionpool.h"
#include <QSqlError>
#include <QUuid>

DbConnectionPool& DbConnectionPool::getInstance()
{
    static DbConnectionPool instance;
    return instance;
}

DbConnectionPool::DbConnectionPool(QObject* parent)
    : QObject(parent), m_maxConnections(50), m_connectionTimeout(300), m_idleTimeout(3000)
{
    m_checkTimer = new QTimer(this);
    connect(m_checkTimer, &QTimer::timeout, this, &DbConnectionPool::checkConnections);
    m_checkTimer->start(30000); // 每30秒检查一次连接
}

DbConnectionPool::~DbConnectionPool()
{
    closeAllConnections();
}

bool DbConnectionPool::initializePool(const DbType& dbType, const DbInfo& dbInfo, int maxConnections, int minConnections)
{
    QMutexLocker locker(&m_mutex);
    m_dbType = dbType;
    m_dbInfo = dbInfo;
    m_maxConnections = maxConnections;
    m_minConnections = minConnections;

    // 初始化时只创建一个连接
    PooledConnection connection = createConnection(0);
    if (connection.connection.isOpen())
    {
        m_connections.append(connection);
        return true;
    }
    return false;
}

QSqlDatabase DbConnectionPool::getConnection()
{
    QMutexLocker locker(&m_mutex);

    // 如果没有可用连接且未达到最大连接数，创建新连接
    //qDebug() << "dbconnectionsize" << m_connections.size() << endl;
    // 查找可用的连接
    for (auto& conn : m_connections)
    {
        if (!conn.inUse)
        {
            if (!conn.connection.isOpen() || !conn.connection.isValid())
            {
                // 如果连接已关闭或无效，尝试重新打开
                if (!openConnection(conn.connection))
                {
                    continue;
                }
            }
            conn.inUse = true;
            conn.lastUsed = QDateTime::currentDateTime();
            return conn.connection;
        }
    }

    if (m_connections.size() < m_maxConnections)
    {
        PooledConnection newConn = createConnection(m_connections.size());
        if (newConn.connection.isOpen())
        {
            newConn.inUse = true;
            newConn.lastUsed = QDateTime::currentDateTime();
            m_connections.append(newConn);
            return newConn.connection;
        }
    }

    emit error("No available connections in the pool");
    qWarning() << "No available connections in the pool";
    return QSqlDatabase();
}

void DbConnectionPool::releaseConnection(QSqlDatabase& connection)
{
    QMutexLocker locker(&m_mutex);
    for (auto& conn : m_connections)
    {
        if (conn.connection.connectionName() == connection.connectionName())
        {
            conn.inUse = false;
            conn.lastUsed = QDateTime::currentDateTime();
            break;
        }
    }
}

void DbConnectionPool::closeAllConnections()
{
    QMutexLocker locker(&m_mutex);
    for (auto& conn : m_connections)
    {
        conn.connection.close();
    }
    m_connections.clear();
}

bool DbConnectionPool::isConnectionValid(QSqlDatabase& connection)
{
    if (!connection.isOpen())
    {
        return false;
    }

    try
    {
        QSqlQuery query(connection);
        bool valid = query.exec("SELECT 1");
        return valid;
    }
    catch (const SqlException& e) {
        qCritical() << "Database error:" << e.what();
        qCritical() << "Detailed error:" << e.getError().text();
        // 处理错误（如回滚事务、清理资源等）
    }
    catch (const std::exception& e) {
        qCritical() << "General error:" << e.what();
    }
}

bool DbConnectionPool::reopenConnection(QSqlDatabase& connection)
{
    connection.close();
    return connection.open();
}

void DbConnectionPool::ensureMinConnections()
{
    int currentConnections = m_connections.size();
    int connectionsToAdd = qMax(0, m_minConnections - currentConnections);

    for (int i = 0; i < connectionsToAdd; ++i)
    {
        PooledConnection newConn = createConnection(currentConnections + i);
        if (newConn.connection.isOpen())
        {
            m_connections.append(newConn);
        }
        else
        {
            qDebug() << "无法创建新连接";
        }
    }
}
PooledConnection DbConnectionPool::createConnection(int connectionNumber)
{
    QString connectionName = QString("Connection_%1_%2")
        .arg(QUuid::createUuid().toString(QUuid::WithoutBraces))
        .arg(connectionNumber);

    QSqlDatabase connection = QSqlDatabase::addDatabase("QPSQL", connectionName);
    connection.setDatabaseName(m_dbInfo.dbName);
    connection.setHostName(m_dbInfo.hostName);
    connection.setPort(m_dbInfo.hostPort);
    connection.setUserName(m_dbInfo.userName);
    connection.setPassword(m_dbInfo.userPwd);

    PooledConnection pooledConn;
    pooledConn.connection = connection;
    pooledConn.lastUsed = QDateTime::currentDateTime();
    pooledConn.inUse = false;

    if (!openConnection(pooledConn.connection))
    {
        emit error(QString("Failed to open database connection %1").arg(connectionNumber));
    }

    return pooledConn;
}

bool DbConnectionPool::openConnection(QSqlDatabase& connection)
{
    if (!connection.open())
    {
        emit error(QString("Failed to open database connection: %1").arg(connection.lastError().text()));
        return false;
    }
    return true;
}

void DbConnectionPool::checkConnections()
{
    QMutexLocker locker(&m_mutex);
    QDateTime now = QDateTime::currentDateTime();

    for (auto it = m_connections.begin(); it != m_connections.end();)
    {
        if (!it->inUse)
        {
            // 检查空闲连接是否超时
            if (it->lastUsed.secsTo(now) > m_idleTimeout)
            {
                qDebug() << "数据库回收空闲连接：" << it->connection.connectionName();
                it->connection.close();
                it = m_connections.erase(it);
                continue;
            }

            // 检查连接是否有效
            if (!it->connection.isValid())
            {
                if (!openConnection(it->connection))
                {
                    it = m_connections.erase(it);
                    continue;
                }
            }
        }
        else
        {
            // 检查使用中的连接是否超时
            if (it->lastUsed.secsTo(now) > m_connectionTimeout)
            {
                qDebug() << "数据库连接超时：" << it->connection.connectionName();
                emit error(QString("Connection timeout: %1").arg(it->connection.connectionName()));
                it->connection.close();
                it = m_connections.erase(it);
                continue;
            }
            else
            {
                qDebug() << "数据库连接正常：" << it->connection.connectionName();
            }
        }
        ++it;
    }
}

void DbConnectionPool::removeStaleConnections()
{
    QMutexLocker locker(&m_mutex);
    QDateTime now = QDateTime::currentDateTime();

    m_connections.erase(
        std::remove_if(m_connections.begin(), m_connections.end(),
            [this, now](const PooledConnection& conn)
            {
                return !conn.inUse && conn.lastUsed.secsTo(now) > m_idleTimeout;
            }),
        m_connections.end()
                );
}
