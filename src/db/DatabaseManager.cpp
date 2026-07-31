// DatabaseManager.cpp
#include "DatabaseManager.h"
#include "db/postgres/PgQtDatabaseManager.h"

#include <QSqlError>
#include <QSqlRecord>
#include <QThread>
#include <QTimer>
#include <QUuid>

namespace {

QString quoteSqliteIdent(const QString& ident)
{
    return QStringLiteral("\"%1\"").arg(QString(ident).replace(QLatin1Char('\"'), QStringLiteral("\"\"")));
}

} // namespace

DatabaseManager& DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initializeLibpq(const DbInfo& dbInfo, int maxConnections)
{
    PgDbInfo info;
    info.connMode = dbInfo.connMode;
    info.timeout = dbInfo.timeout;
    info.connName = dbInfo.connName.toStdString();
    info.dbName = dbInfo.dbName.toStdString();
    info.hostName = dbInfo.hostName.toStdString();
    info.hostPort = dbInfo.hostPort;
    info.userName = dbInfo.userName.toStdString();
    info.userPwd = dbInfo.userPwd.toStdString();
    info.applicationName = "AlarmSys-linux";
    info.sslMode = "disable";

    auto& pgQt = PgQtDatabaseManager::getInstance();
    pgQt.rawManager().setDebugCallback([](const std::string&) {});
    pgQt.rawManager().setErrorCallback([](const std::string& msg) {
        qWarning().noquote() << "[libpq]" << QString::fromStdString(msg);
    });

    m_libpqReady = pgQt.initialize(info, maxConnections);
    if (m_libpqReady) {
        qInfo() << "Database access mode=libpq (AccessMode=1)"
                << "host" << dbInfo.hostName << "port" << dbInfo.hostPort
                << "db" << dbInfo.dbName;
    } else {
        qWarning() << "libpq initialize failed:" << pgQt.lastError();
    }
    return m_libpqReady;
}

bool DatabaseManager::initialize(const DbType &dbType, const DbInfo &dbInfo, int maxConnections)
{
    m_accessMode = dbInfo.accessMode;
    if (m_accessMode != 0 && m_accessMode != 1) {
        qWarning() << "Invalid Database/AccessMode" << m_accessMode << ", fallback to 1 (libpq)";
        m_accessMode = 1;
    }

    if (useLibpq()) {
        return initializeLibpq(dbInfo, maxConnections);
    }

    m_libpqReady = false;
    qInfo() << "Database access mode=Qt QPSQL (AccessMode=0)"
            << "host" << dbInfo.hostName << "port" << dbInfo.hostPort
            << "db" << dbInfo.dbName;
    return m_connectionPool.initializePool(dbType, dbInfo, maxConnections);
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent), 
      m_connectionPool(DbConnectionPool::getInstance()),
      m_queryTimeoutMs(10000),
      m_shuttingDown(false),
      m_accessMode(1),
      m_libpqReady(false)
{
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
}

DatabaseManager::~DatabaseManager()
{
    cancelAsyncOperations();
    cleanup();
}

void DatabaseManager::cleanup()
{
    if (useLibpq() && m_libpqReady) {
        PgQtDatabaseManager::getInstance().cleanup();
        m_libpqReady = false;
    }
    m_connectionPool.closeAllConnections();
}

QSqlQuery DatabaseManager::materializeRowsToSqlQuery(const QVector<QVariantMap>& rows)
{
    if (rows.isEmpty()) {
        return QSqlQuery();
    }

    // 取第一行推断列：优先 _c0,_c1,... 否则用所有非 _c 键
    const QVariantMap& first = rows.first();
    QStringList colNames;
    for (int i = 0; ; ++i) {
        const QString key = QStringLiteral("_c%1").arg(i);
        if (!first.contains(key)) {
            break;
        }
        // 优先找真实列名（同序、非 _c）
        QString realName = key;
        for (auto it = first.constBegin(); it != first.constEnd(); ++it) {
            if (it.key().startsWith(QLatin1String("_c"))) {
                continue;
            }
            if (it.value() == first.value(key)) {
                // 可能误匹配同值列；用 PgQt 的顺序约定：先写入 _c，再写入名
                // 这里仅作缓存列名：尝试用 keys 中“第 i 个非_c”
            }
        }
        colNames << key;
    }
    if (colNames.isEmpty()) {
        colNames = first.keys();
        colNames.removeAll(QString());
    }

    // 若有 _cN，用调用方可 value(i)；同时为名字段建别名列
    // 重建为：尽量从 rows 的命名键取展示列，空则用 _c
    QStringList displayCols;
    if (!colNames.isEmpty() && colNames.first().startsWith(QLatin1String("_c"))) {
        // 保持索引列；额外扫描命名列挂到同表会复杂，索引访问已够大多数 DAL
        displayCols = colNames;
        // 同时加入命名列（去重）
        for (auto it = first.constBegin(); it != first.constEnd(); ++it) {
            if (!it.key().startsWith(QLatin1String("_c")) && !displayCols.contains(it.key())) {
                displayCols << it.key();
            }
        }
    } else {
        displayCols = colNames;
    }

    // 唯一化列名（SQLite 禁止 duplicate）
    QStringList uniqueCols;
    uniqueCols.reserve(displayCols.size());
    for (int i = 0; i < displayCols.size(); ++i) {
        QString name = displayCols.at(i);
        if (name.isEmpty()) {
            name = QStringLiteral("_c%1").arg(i);
        }
        QString unique = name;
        int suffix = 0;
        while (uniqueCols.contains(unique)) {
            unique = QStringLiteral("%1_%2").arg(name).arg(++suffix);
        }
        uniqueCols << unique;
    }

    const QString connName = QStringLiteral("alarmsys_libpq_buf_%1")
        .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);
    if (!QSqlDatabase::contains(connName)) {
        QSqlDatabase memDb = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
        memDb.setDatabaseName(QStringLiteral(":memory:"));
        if (!memDb.open()) {
            qWarning() << "libpq materialize open failed:" << memDb.lastError().text();
            return QSqlQuery();
        }
    }
    QSqlDatabase memDb = QSqlDatabase::database(connName);
    static thread_local int s_seq = 0;
    const QString table = QStringLiteral("qbuf_%1").arg(++s_seq);

    QStringList defs;
    for (const QString& c : uniqueCols) {
        defs << quoteSqliteIdent(c) + QStringLiteral(" TEXT");
    }
    QSqlQuery setup(memDb);
    if (!setup.exec(QStringLiteral("CREATE TEMP TABLE %1 (%2)")
                        .arg(table, defs.join(QLatin1Char(','))))) {
        qWarning() << "libpq materialize create failed:" << setup.lastError().text();
        return QSqlQuery();
    }

    QStringList ph;
    for (int i = 0; i < uniqueCols.size(); ++i) {
        ph << QStringLiteral("?");
    }
    const QString insertSql = QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(table,
             [&]() {
                 QStringList q;
                 for (const QString& c : uniqueCols) {
                     q << quoteSqliteIdent(c);
                 }
                 return q.join(QLatin1Char(','));
             }(),
             ph.join(QLatin1Char(',')));

    for (const QVariantMap& row : rows) {
        setup.prepare(insertSql);
        for (int i = 0; i < uniqueCols.size(); ++i) {
            const QString srcKey = displayCols.at(i);
            setup.bindValue(i, row.value(srcKey));
        }
        if (!setup.exec()) {
            qWarning() << "libpq materialize insert failed:" << setup.lastError().text();
            return QSqlQuery();
        }
    }

    QSqlQuery result(memDb);
    QStringList selectCols;
    for (const QString& c : uniqueCols) {
        selectCols << quoteSqliteIdent(c);
    }
    if (!result.exec(QStringLiteral("SELECT %1 FROM %2")
                         .arg(selectCols.join(QLatin1Char(',')), table))) {
        qWarning() << "libpq materialize select failed:" << result.lastError().text();
        return QSqlQuery();
    }
    return result;
}

QSqlQuery DatabaseManager::executeQueryLibpq(const QString& query, const QVariantList& params, int timeoutMs)
{
    const QVector<QVariantMap> rows =
        PgQtDatabaseManager::getInstance().executeQuery(query, params, timeoutMs);
    return materializeRowsToSqlQuery(rows);
}

bool DatabaseManager::executeNonQueryLibpq(const QString& query, const QVariantList& params, int timeoutMs)
{
    return PgQtDatabaseManager::getInstance().executeNonQuery(query, params, timeoutMs);
}

QVariant DatabaseManager::executeScalarLibpq(const QString& query, const QVariantList& params, int timeoutMs)
{
    return PgQtDatabaseManager::getInstance().executeScalar(query, params, timeoutMs);
}

QSqlQuery DatabaseManager::executeQuery(const QString& query, const QVariantList& params, int timeoutMs)
{
    if (useLibpq()) {
        if (!m_libpqReady) {
            emit error(QStringLiteral("libpq not initialized"));
            return QSqlQuery();
        }
        return executeQueryLibpq(query, params, timeoutMs);
    }
  
    // 如果设置了硬超时时间，使用硬超时功能
    if (timeoutMs > 0) {
        qDebug() << "执行带硬超时的查询SQL操作:" << query << "超时时间:" << timeoutMs << "毫秒";
        
        QSqlDatabase db = m_connectionPool.getConnection();
        if (!db.isValid()) {
            emit error("Failed to get valid connection for timed query operation");
            return QSqlQuery();
        }
        
        // 复制参数列表，避免在异步操作中使用引用
        QVariantList paramsCopy = params;
        
        // 创建一个future来异步执行查询
        QFuture<std::pair<QSqlQuery, QString>> future = QtConcurrent::run([this, query, paramsCopy, db]() {
            try {
                QSqlQuery sqlQuery(db);
                if (!sqlQuery.prepare(query)) {
                    return std::make_pair(QSqlQuery(), sqlQuery.lastError().text());
                }
                
                for (const auto& param : paramsCopy) {
                    sqlQuery.addBindValue(param);
                }
                
                // 设置查询选项，包括转发结果集
                sqlQuery.setForwardOnly(true);
                
                if (!sqlQuery.exec()) {
                    return std::make_pair(QSqlQuery(), sqlQuery.lastError().text());
                }
                
                return std::make_pair(sqlQuery, QString());
            } catch (const std::exception& e) {
                return std::make_pair(QSqlQuery(), QString("Exception: %1").arg(e.what()));
            } catch (...) {
                return std::make_pair(QSqlQuery(), QString("Unknown exception in query execution"));
            }
        });
        
        // 设置future watcher和超时
        QFutureWatcher<std::pair<QSqlQuery, QString>> watcher;
        watcher.setFuture(future);
        
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        
        QEventLoop loop;
        connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(&watcher, &QFutureWatcher<std::pair<QSqlQuery, QString>>::finished, &loop, &QEventLoop::quit);
        
        timeoutTimer.start(timeoutMs);
        loop.exec();
        
        QSqlQuery resultQuery;
        
        if (future.isFinished()) {
            // 查询正常完成
            auto result = future.result();
            resultQuery = result.first;
            
            if (result.second.isEmpty()) {
               
            } else {
                
                qWarning() << "Query execution failed:" << result.second;
            }
        } else {
            // 查询超时
            emit queryTimedOut(query);
            qWarning() << "Query execution hard timeout after" << timeoutMs << "ms:" << query;
            
            // 尝试中断操作，通过使用数据库的特定功能或关闭连接
            try {
                if (db.driverName().toLower() == "qmysql") {
                    // MySQL特定的取消方式
                    QSqlQuery killQuery(db);
                    killQuery.exec("SELECT CONNECTION_ID()");
                    if (killQuery.next()) {
                        int connectionId = killQuery.value(0).toInt();
                        // 使用单独的QSqlDatabase连接，避免参数过多
                        QSqlDatabase adminDb = QSqlDatabase::database("admin");
                        if (adminDb.isValid()) {
                            QSqlQuery adminQuery(adminDb);
                            adminQuery.exec(QString("KILL QUERY %1").arg(connectionId));
                        }
                    }
                } else if (db.driverName().toLower() == "qpsql") {
                    // PostgreSQL特定的取消方式
                    QSqlQuery pidQuery(db);
                    pidQuery.exec("SELECT pg_backend_pid()");
                    if (pidQuery.next()) {
                        int pid = pidQuery.value(0).toInt();
                        // 使用单独的QSqlDatabase连接，避免参数过多
                        QSqlDatabase adminDb = QSqlDatabase::database("admin");
                        if (adminDb.isValid()) {
                            QSqlQuery adminQuery(adminDb);
                            adminQuery.exec(QString("SELECT pg_cancel_backend(%1)").arg(pid));
                        }
                    }
                }
                // 对于SQLite和其他数据库，目前无法优雅地取消
            } catch (...) {
                // 忽略取消查询时的异常
            }
        }
        
        // 等待future完成
        future.waitForFinished();
        
        m_connectionPool.releaseConnection(db);
        return resultQuery;
    }
    else
    {
        QSqlDatabase db = m_connectionPool.getConnection();

        // 检查连接是否为空或无效
        if (!db.isValid()) {
            emit error("Failed to get a valid database connection");
            return QSqlQuery();
        }

        // 检查连接是否有效，如果无效则尝试重新连接
        if (!m_connectionPool.isConnectionValid(db)) {
            if (!m_connectionPool.reopenConnection(db)) {
                emit error("Failed to reopen database connection");
                m_connectionPool.releaseConnection(db);
                return QSqlQuery();
            }
        }

        try {
            QSqlQuery result = prepareQuery(db, query, params);

            if (!result.exec()) {
                emit error(QString("Query execution failed: %1").arg(result.lastError().text()));
                qWarning() << QString("Query execution failed: %1").arg(result.lastError().text());
            }

            m_connectionPool.releaseConnection(db);
            return result;
        }
        catch (const std::exception& e) {
            emit error(QString("Exception in executeQuery: %1").arg(e.what()));
            qWarning() << QString("Exception in executeQuery: %1").arg(e.what());
            m_connectionPool.releaseConnection(db);
            return QSqlQuery();
        }
        catch (...) {
            emit error("Unknown exception in executeQuery");
            qWarning() << "Unknown exception in executeQuery";
            m_connectionPool.releaseConnection(db);
            return QSqlQuery();
        }
     }
     
}

bool DatabaseManager::executeNonQuery(const QString& query, const QVariantList& params, int timeoutMs)
{
    if (useLibpq()) {
        if (!m_libpqReady) {
            emit error(QStringLiteral("libpq not initialized"));
            return false;
        }
        const bool ok = executeNonQueryLibpq(query, params, timeoutMs);
        if (!ok) {
            emit error(QStringLiteral("libpq Non-query failed: %1 | SQL: %2")
                           .arg(PgQtDatabaseManager::getInstance().lastError(), query));
        }
        return ok;
    }

    QElapsedTimer timer;
    timer.start();
    
    // 如果设置了硬超时时间，使用硬超时功能
    if (timeoutMs > 0) {
        qDebug() << "执行带硬超时的SQL操作:" << query << "超时时间:" << timeoutMs << "毫秒";
        
        QSqlDatabase db = m_connectionPool.getConnection();
        if (!db.isValid()) {
            emit error("Failed to get valid connection for timed operation");
            return false;
        }
        
        // 复制参数列表，避免在异步操作中使用引用
        QVariantList paramsCopy = params;
        
        // 创建一个future来异步执行查询
        QFuture<std::pair<bool, QString>> future = QtConcurrent::run([this, query, paramsCopy, db]() {
            try {
                QSqlQuery sqlQuery(db);
                if (!sqlQuery.prepare(query)) {
                    return std::make_pair(false, sqlQuery.lastError().text());
                }
                
                for (const auto& param : paramsCopy) {
                    sqlQuery.addBindValue(param);
                }
                
                bool execResult = sqlQuery.exec();
                if (!execResult) {
                    return std::make_pair(false, sqlQuery.lastError().text());
                }
                
                return std::make_pair(true, QString());
            } catch (const std::exception& e) {
                return std::make_pair(false, QString("Exception: %1").arg(e.what()));
            } catch (...) {
                return std::make_pair(false, QString("Unknown exception in query execution"));
            }
        });
        
        // 设置future watcher和超时
        QFutureWatcher<std::pair<bool, QString>> watcher;
        watcher.setFuture(future);
        
        QTimer timeoutTimer;
        timeoutTimer.setSingleShot(true);
        
        QEventLoop loop;
        connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(&watcher, &QFutureWatcher<std::pair<bool, QString>>::finished, &loop, &QEventLoop::quit);
        
        timeoutTimer.start(timeoutMs);
        loop.exec();
        
        bool success = false;
        
        if (future.isFinished()) {
            // 查询正常完成
            auto result = future.result();
            success = result.first;
            
            if (!success) {
                emit error(QString("Query execution failed: %1").arg(result.second));
                qWarning() << "Query execution failed:" << result.second;
            }
            
            qDebug() << "SQL操作完成，耗时:" << timer.elapsed() << "毫秒，结果:" << (success ? "成功" : "失败");
        } else {
            // 查询超时
            emit queryTimedOut(query);
            qWarning() << "Query execution hard timeout after" << timeoutMs << "ms:" << query;
            
            // 尝试中断操作，通过使用数据库的特定功能或关闭连接
            try {
                if (db.driverName().toLower() == "qmysql") {
                    // MySQL特定的取消方式 - 需要知道连接ID
                    QSqlQuery killQuery(db);
                    killQuery.exec("SELECT CONNECTION_ID()");
                    if (killQuery.next()) {
                        int connectionId = killQuery.value(0).toInt();
                        // 使用单独的QSqlDatabase连接，避免参数过多
                        QSqlDatabase adminDb = QSqlDatabase::database("admin");
                        if (adminDb.isValid()) {
                            QSqlQuery adminQuery(adminDb);
                            adminQuery.exec(QString("KILL QUERY %1").arg(connectionId));
                        }
                    }
                } else if (db.driverName().toLower() == "qpsql") {
                    // PostgreSQL特定的取消方式
                    QSqlQuery pidQuery(db);
                    pidQuery.exec("SELECT pg_backend_pid()");
                    if (pidQuery.next()) {
                        int pid = pidQuery.value(0).toInt();
                        // 使用单独的QSqlDatabase连接，避免参数过多
                        QSqlDatabase adminDb = QSqlDatabase::database("admin");
                        if (adminDb.isValid()) {
                            QSqlQuery adminQuery(adminDb);
                            adminQuery.exec(QString("SELECT pg_cancel_backend(%1)").arg(pid));
                        }
                    }
                }
                // 对于SQLite和其他数据库，目前无法优雅地取消
            } catch (...) {
                // 忽略取消查询时的异常
            }
            
            qDebug() << "SQL操作超时，已中断，耗时超过:" << timeoutMs << "毫秒";
        }
        
        // 等待future完成
        future.waitForFinished();
        
        m_connectionPool.releaseConnection(db);
        return success;
    }
    else
    {
        QSqlDatabase db = m_connectionPool.getConnection();

        // 检查连接是否为空或无效
        if (!db.isValid()) {
            emit error("Failed to get a valid database connection");
            return false;
        }

        // 检查连接是否有效，如果无效则尝试重新连接
        if (!m_connectionPool.isConnectionValid(db)) {
            if (!m_connectionPool.reopenConnection(db)) {
                emit error("Failed to reopen database connection");
                m_connectionPool.releaseConnection(db);
                return false;
            }
        }

        try {
            QSqlQuery sqlQuery(db);
            if (!sqlQuery.prepare(query)) {
                const QString err = sqlQuery.lastError().text();
                emit error(QString("Non-query prepare failed: %1 | SQL: %2").arg(err, query));
                qWarning() << "Non-query prepare failed:" << err << "SQL:" << query;
                m_connectionPool.releaseConnection(db);
                return false;
            }
            bindParams(sqlQuery, params);

            const bool success = sqlQuery.exec();
            if (!success) {
                emit error(QString("Non-query execution failed: %1").arg(sqlQuery.lastError().text()));
                qWarning() << QString("Non-query execution failed: %1").arg(sqlQuery.lastError().text())
                           << "SQL:" << query;
            }

            m_connectionPool.releaseConnection(db);
            return success;
        }
        catch (const std::exception& e) {
            emit error(QString("Exception in executeNonQuery: %1").arg(e.what()));
            qWarning() << QString("Exception in executeNonQuery: %1").arg(e.what());
            m_connectionPool.releaseConnection(db);
            return false;
        }
        catch (...) {
            emit error("Unknown exception in executeNonQuery");
            qWarning() << "Unknown exception in executeNonQuery";
            m_connectionPool.releaseConnection(db);
            return false;
        }
    }
     
}

QVariant DatabaseManager::executeScalar(const QString& query, const QVariantList& params, int timeoutMs)
{
    if (useLibpq()) {
        if (!m_libpqReady) {
            emit error(QStringLiteral("libpq not initialized"));
            return {};
        }
        return executeScalarLibpq(query, params, timeoutMs);
    }

    QSqlDatabase db = m_connectionPool.getConnection();
    QSqlQuery result = prepareQuery(db, query, params);

    QVariant returnValue;
    if (result.exec() && result.next()) {
        returnValue = result.value(0);
    }

    m_connectionPool.releaseConnection(db);
    return returnValue;
}

QFuture<QSqlQuery> DatabaseManager::executeQueryAsync(const QString& query, const QVariantList& params)
{
    // 检查是否正在关闭
    QMutexLocker locker(&m_mutex);
    if (m_shuttingDown) {
        qWarning() << "Async operation rejected: system is shutting down";
        return QFuture<QSqlQuery>();
    }
    locker.unlock();
    
    return QtConcurrent::run(&m_threadPool, [this, query, params]() {
        if (m_shuttingDown) {
            qWarning() << "Async operation aborted: system is shutting down";
            return QSqlQuery();
        }
        return executeQuery(query, params);
    });
}

QFuture<bool> DatabaseManager::executeNonQueryAsync(const QString& query, const QVariantList& params)
{
    // 检查是否正在关闭
    QMutexLocker locker(&m_mutex);
    if (m_shuttingDown) {
        qWarning() << "Async operation rejected: system is shutting down";
        return QFuture<bool>();
    }
    locker.unlock();
    
    return QtConcurrent::run(&m_threadPool, [this, query, params]() {
        if (m_shuttingDown) {
            qWarning() << "Async operation aborted: system is shutting down";
            return false;
        }
        return executeNonQuery(query, params);
    });
}

QFuture<QVariant> DatabaseManager::executeScalarAsync(const QString& query, const QVariantList& params)
{
    // 检查是否正在关闭
    QMutexLocker locker(&m_mutex);
    if (m_shuttingDown) {
        qWarning() << "Async operation rejected: system is shutting down";
        return QFuture<QVariant>();
    }
    locker.unlock();
    
    return QtConcurrent::run(&m_threadPool, [this, query, params]() {
        if (m_shuttingDown) {
            qWarning() << "Async operation aborted: system is shutting down";
            return QVariant();
        }
        return executeScalar(query, params);
    });
}

QString DatabaseManager::buildInsertSql(const QString& tableName, const QVariantMap& data)
{
    QStringList columns;
    QStringList placeholders;

    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        columns << it.key();
        placeholders << "?";
    }

    return QString("INSERT INTO %1 (%2) VALUES (%3)")
        .arg(tableName)
        .arg(columns.join(", "))
        .arg(placeholders.join(", "));
}

QFuture<int> DatabaseManager::executeBatchInsertAsync(const QString& table, const QList<QVariantMap>& dataList)
{
    // 检查是否正在关闭
    QMutexLocker locker(&m_mutex);
    if (m_shuttingDown) {
        qWarning() << "Batch insert rejected: system is shutting down";
        return QFuture<int>();
    }
    
    // 创建一个数据列表的副本，避免捕获引用导致的问题
    auto dataListCopy = dataList;
    locker.unlock();
    
    return QtConcurrent::run(&m_threadPool, [this, table, dataListCopy]() {
        if (m_shuttingDown) {
            qWarning() << "Batch insert aborted: system is shutting down";
            return 0;
        }
        
        QElapsedTimer timer;
        timer.start();

        int successCount = 0;
        QSqlDatabase db = m_connectionPool.getConnection();
        
        // 检查连接
        if (!db.isValid()) {
            emit error("Failed to get a valid database connection for batch insert");
            return 0;
        }
        
        // 检查连接是否有效
        if (!m_connectionPool.isConnectionValid(db)) {
            if (!m_connectionPool.reopenConnection(db)) {
                emit error("Failed to reopen database connection for batch insert");
                m_connectionPool.releaseConnection(db);
                return 0;
            }
        }

        // 空列表直接返回
        if (dataListCopy.isEmpty()) {
            qWarning() << "Batch insert called with empty data list";
            m_connectionPool.releaseConnection(db);
            return 0;
        }

        try {
            if (!db.transaction()) {
                emit error("Failed to start transaction for batch insert");
                m_connectionPool.releaseConnection(db);
                return 0;
            }

            QSqlQuery query(db);

            // 构建批量插入的SQL语句
            QStringList columns = dataListCopy.first().keys();
            QStringList placeholderList;
            placeholderList.reserve(columns.size());
            for (int i = 0; i < columns.size(); ++i) {
                placeholderList << "?";
            }
            QString placeholders = QString("(%1)").arg(placeholderList.join(", "));
            
            // 每次插入100条记录
            const int batchSize = 100;

            for (int i = 0; i < dataListCopy.size(); i += batchSize) {
                // 检查是否超时或正在关闭
                if (m_shuttingDown || isQueryTimedOut(timer)) {
                    if (m_shuttingDown) {
                        qWarning() << "Batch insert interrupted: system is shutting down";
                    } else {
                        emit queryTimedOut("Batch insert");
                        qWarning() << "Batch insert timed out after" << timer.elapsed() << "ms";
                    }
                    db.rollback();
                    m_connectionPool.releaseConnection(db);
                    return successCount;
                }
                
                QStringList valuePlaceholders;
                int currentBatchSize = qMin(batchSize, dataListCopy.size() - i);
                for (int j = 0; j < currentBatchSize; ++j) {
                    valuePlaceholders << placeholders;
                }

                QString sql = QString("INSERT INTO %1 (%2) VALUES %3")
                                  .arg(table)
                                  .arg(columns.join(", "))
                                  .arg(valuePlaceholders.join(", "));

                if (!query.prepare(sql)) {
                    emit error(QString("Failed to prepare batch insert query: %1").arg(query.lastError().text()));
                    db.rollback();
                    m_connectionPool.releaseConnection(db);
                    return successCount;
                }

                QVariantList values;
                try {
                    for (int j = i; j < i + currentBatchSize; ++j) {
                        const QVariantMap& data = dataListCopy[j];
                        for (const QString& column : columns) {
                            values << data.value(column, QVariant()); // 使用value()安全访问
                        }
                    }
                } catch (const std::exception& e) {
                    emit error(QString("Exception preparing batch values: %1").arg(e.what()));
                    db.rollback();
                    m_connectionPool.releaseConnection(db);
                    return successCount;
                } catch (...) {
                    emit error("Unknown exception preparing batch values");
                    db.rollback();
                    m_connectionPool.releaseConnection(db);
                    return successCount;
                }

                for (const QVariant& value : values) {
                    query.addBindValue(value);
                }

                if (query.exec()) {
                    successCount += currentBatchSize;
                } else {
                    emit error(QString("Batch insert failed: %1").arg(query.lastError().text()));
                    qWarning() << "SQL:" << query.lastQuery();
                    db.rollback();
                    m_connectionPool.releaseConnection(db);
                    return successCount;
                }
            }

            if (db.commit()) {
                qDebug() << "Batch insert completed. Inserted" << successCount << "out of" << dataListCopy.size()
                         << "records in" << timer.elapsed() << "ms";
            } else {
                emit error("Failed to commit transaction for batch insert");
                db.rollback();
            }

            m_connectionPool.releaseConnection(db);
            return successCount;
        } catch (const std::exception& e) {
            emit error(QString("Exception in batch insert: %1").arg(e.what()));
            qWarning() << QString("Exception in batch insert: %1").arg(e.what())
                     << "Execution time:" << timer.elapsed() << "ms";
            try {
                db.rollback();
            } catch (...) {
                // 忽略回滚错误
            }
            m_connectionPool.releaseConnection(db);
            return successCount;
        } catch (...) {
            emit error("Unknown exception in batch insert");
            qWarning() << "Unknown exception in batch insert"
                     << "Execution time:" << timer.elapsed() << "ms";
            try {
                db.rollback();
            } catch (...) {
                // 忽略回滚错误
            }
            m_connectionPool.releaseConnection(db);
            return successCount;
        }
    });
}

QSqlQuery DatabaseManager::prepareQuery(QSqlDatabase& db,const QString& query, const QVariantList& params)
{
    QSqlQuery sqlQuery(db);

    if (!sqlQuery.prepare(query)) {
        const QString err = sqlQuery.lastError().text();
        emit error(QString("Query preparation failed: %1 | SQL: %2").arg(err, query));
        qWarning() << "Query preparation failed:" << err << "SQL:" << query;
        return sqlQuery;
    }

    bindParams(sqlQuery, params);
    return sqlQuery;
}

void DatabaseManager::bindParams(QSqlQuery& query, const QVariantList& params)
{
    // QPSQL 下 addBindValue 在 prepare 异常时可能只发出 EXECUTE (values...) 导致语法错误，改用位置 bindValue
    for (int i = 0; i < params.size(); ++i) {
        query.bindValue(i, params.at(i));
    }
}

QSqlDatabase DatabaseManager::beginTransaction()
{
    if (useLibpq()) {
        if (!m_libpqReady) {
            emit error(QStringLiteral("libpq not initialized"));
            return QSqlDatabase();
        }
        auto& pgQt = PgQtDatabaseManager::getInstance();
        if (!pgQt.beginTransaction()) {
            emit error(QStringLiteral("Failed to start libpq transaction: %1").arg(pgQt.lastError()));
            return QSqlDatabase();
        }
        // DAL 仅用 isValid() 判断；实际事务在 PgQt 线程本地
        const QString connName = QStringLiteral("libpq_txn_marker_%1")
            .arg(reinterpret_cast<quintptr>(QThread::currentThread()), 0, 16);
        if (!QSqlDatabase::contains(connName)) {
            QSqlDatabase marker = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connName);
            marker.setDatabaseName(QStringLiteral(":memory:"));
            if (!marker.open()) {
                qWarning() << "libpq txn marker open failed:" << marker.lastError().text();
            }
        }
        return QSqlDatabase::database(connName);
    }

    QSqlDatabase db = m_connectionPool.getConnection();
    if (!db.transaction()) {
        emit error("Failed to start transaction");
        m_connectionPool.releaseConnection(db);
        return QSqlDatabase();
    }
    return db;
}

bool DatabaseManager::commitTransaction(QSqlDatabase& db)
{
    if (useLibpq()) {
        Q_UNUSED(db);
        auto& pgQt = PgQtDatabaseManager::getInstance();
        const bool success = pgQt.commitTransaction();
        if (!success) {
            emit error(QStringLiteral("Failed to commit libpq transaction: %1").arg(pgQt.lastError()));
        }
        return success;
    }

    bool success = db.commit();
    if (!success) {
        emit error("Failed to commit transaction");
    }
    m_connectionPool.releaseConnection(db);
    return success;
}

bool DatabaseManager::rollbackTransaction(QSqlDatabase& db)
{
    if (useLibpq()) {
        Q_UNUSED(db);
        auto& pgQt = PgQtDatabaseManager::getInstance();
        const bool success = pgQt.rollbackTransaction();
        if (!success) {
            emit error(QStringLiteral("Failed to rollback libpq transaction: %1").arg(pgQt.lastError()));
        }
        return success;
    }

    bool success = db.rollback();
    if (!success) {
        emit error("Failed to rollback transaction");
    }
    m_connectionPool.releaseConnection(db);
    return success;
}

// 添加超时设置方法
void DatabaseManager::setQueryTimeout(int milliseconds)
{
    QMutexLocker locker(&m_mutex);
    m_queryTimeoutMs = milliseconds;
}

int DatabaseManager::queryTimeout() const
{
    return m_queryTimeoutMs; // 常量方法不需要锁
}

bool DatabaseManager::isQueryTimedOut(const QElapsedTimer& timer) const
{
    return m_queryTimeoutMs > 0 && timer.elapsed() > m_queryTimeoutMs;
}

// 实现取消异步操作
void DatabaseManager::cancelAsyncOperations()
{
    QMutexLocker locker(&m_mutex);
    m_shuttingDown = true;
    m_threadPool.clear();  // 清除未开始的任务
    m_threadPool.waitForDone(); // 等待所有任务完成
}
