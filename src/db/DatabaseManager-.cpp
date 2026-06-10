// databasemanager.cpp
#include "databasemanager.h"



DatabaseManager& DatabaseManager::getInstance()
{
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::initialize(const DbType &dbType, const DbInfo &dbInfo, int maxConnections)
{
    bool result = m_connectionPool.initializePool(dbType, dbInfo, maxConnections);
    return result;
}

DatabaseManager::DatabaseManager(QObject* parent)
    : QObject(parent), 
      m_connectionPool(DbConnectionPool::getInstance()),
      m_queryTimeoutMs(10000),
      m_shuttingDown(false)
{
    m_threadPool.setMaxThreadCount(QThread::idealThreadCount());
}

DatabaseManager::~DatabaseManager()
{
    cancelAsyncOperations();
    m_connectionPool.closeAllConnections();
}

void DatabaseManager::cleanup()
{
    m_connectionPool.closeAllConnections();
}

QSqlQuery DatabaseManager::executeQuery(const QString& query, const QVariantList& params, int timeoutMs)
{  
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
            QSqlQuery sqlQuery = prepareQuery(db, query, params);

            bool success = sqlQuery.exec();
            if (!success) {
                emit error(QString("Non-query execution failed: %1").arg(sqlQuery.lastError().text()));
                qWarning() << QString("Non-query execution failed: %1").arg(sqlQuery.lastError().text());
                QString error = sqlQuery.lastError().text();
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
        emit error(QString("Query preparation failed: %1").arg(sqlQuery.lastError().text()));
    }

    bindParams(sqlQuery, params);
    return sqlQuery;
}

void DatabaseManager::bindParams(QSqlQuery& query, const QVariantList& params)
{
    for (const auto& param : params) {
        query.addBindValue(param);
    }
}

QSqlDatabase DatabaseManager::beginTransaction()
{
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
    bool success = db.commit();
    if (!success) {
        emit error("Failed to commit transaction");
    }
    m_connectionPool.releaseConnection(db);
    return success;
}

bool DatabaseManager::rollbackTransaction(QSqlDatabase& db)
{
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

// 在 DatabaseManager.cpp 的头部添加使用新连接池的示例方法

// 推荐的使用方式 - 使用RAII ConnectionGuard
bool DatabaseManager::executeNonQueryWithGuard(const QString& query, const QVariantList& params)
{
    // 使用RAII连接管理，自动处理连接的获取和释放
    auto connectionGuard = m_connectionPool.getConnectionGuard();
    if (!connectionGuard->isValid()) {
        emit error("无法获取有效的数据库连接");
        return false;
    }
    
    QSqlDatabase& db = connectionGuard->get();
    
    try {
        QSqlQuery sqlQuery = prepareQuery(db, query, params);
        bool success = sqlQuery.exec();
        
        if (!success) {
            emit error(QString("查询执行失败: %1").arg(sqlQuery.lastError().text()));
            qWarning() << "查询执行失败:" << sqlQuery.lastError().text();
        }
        
        return success;
        // connectionGuard 自动析构，连接自动释放
    }
    catch (const std::exception& e) {
        emit error(QString("查询执行异常: %1").arg(e.what()));
        qWarning() << "查询执行异常:" << e.what();
        return false;
    }
}

// 获取连接池统计信息的方法
QString DatabaseManager::getConnectionPoolStats() const
{
    int total = m_connectionPool.getTotalConnectionCountSafe();
    int active = m_connectionPool.getActiveConnectionCountSafe(); 
    int available = m_connectionPool.getAvailableConnectionCountSafe();
    
    return QString("连接池统计 - 总连接数: %1, 活动连接: %2, 可用连接: %3")
           .arg(total).arg(active).arg(available);
}
