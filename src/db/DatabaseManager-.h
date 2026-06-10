#pragma once

#include <QObject>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QFuture>
#include <QtConcurrent>
#include <QThreadStorage>
#include <QThreadPool>
#include <QElapsedTimer>
#include <QMutex>
#include "dbconnectionpool.h"

//// RAII风格的事务类，确保事务自动提交或回滚
//class DbTransaction {
//public:
//    DbTransaction(DatabaseManager& manager, int timeoutMs = 10000);  // 默认超时30秒
//    ~DbTransaction();
//
//    bool isValid() const;
//    QSqlDatabase& database();
//    bool commit();
//    bool rollback();
//
//private:
//    DatabaseManager& m_manager;
//    QSqlDatabase m_db;
//    bool m_committed;
//    bool m_valid;
//    QElapsedTimer m_timer;
//    int m_timeoutMs;
//};
//
//// DbTransaction的实现
//DbTransaction::DbTransaction(DatabaseManager& manager, int timeoutMs)
//    : m_manager(manager), m_committed(false), m_valid(false), m_timeoutMs(timeoutMs)
//{
//    m_timer.start();
//    m_db = manager.beginTransaction();
//    m_valid = m_db.isValid();
//}
//
//DbTransaction::~DbTransaction()
//{
//    if (m_valid && !m_committed) {
//        rollback();
//    }
//}
//
//bool DbTransaction::isValid() const
//{
//    return m_valid;
//}
//
//QSqlDatabase& DbTransaction::database()
//{
//    return m_db;
//}
//
//bool DbTransaction::commit()
//{
//    if (!m_valid) {
//        return false;
//    }
//
//    if (m_timer.elapsed() > m_timeoutMs) {
//        qWarning() << "Transaction timed out before commit attempt";
//        rollback();
//        return false;
//    }
//
//    bool result = m_manager.commitTransaction(m_db);
//    m_committed = result;
//    m_valid = false;
//    return result;
//}
//
//bool DbTransaction::rollback()
//{
//    if (!m_valid) {
//        return false;
//    }
//
//    bool result = m_manager.rollbackTransaction(m_db);
//    m_committed = false;
//    m_valid = false;
//    return result;
//}

class DatabaseManager : public QObject
{
    Q_OBJECT
public:
    static DatabaseManager& getInstance();
    DbConnectionPool& m_connectionPool;
    bool initialize(const DbType& dbType, const DbInfo& dbInfo, int maxConnections = 10);

    // 超时设置
    void setQueryTimeout(int milliseconds);
    int queryTimeout() const;

    // 同步方法（集成了硬超时功能）
    QSqlQuery executeQuery(const QString& query, const QVariantList& params = QVariantList(), int timeoutMs = 0);
    bool executeNonQuery(const QString& query, const QVariantList& params = QVariantList(), int timeoutMs = 0);
    QVariant executeScalar(const QString& query, const QVariantList& params = QVariantList(), int timeoutMs = 0);

    // 异步方法
    QFuture<QSqlQuery> executeQueryAsync(const QString& query, const QVariantList& params = QVariantList());
    QFuture<bool> executeNonQueryAsync(const QString& query, const QVariantList& params = QVariantList());
    QFuture<QVariant> executeScalarAsync(const QString& query, const QVariantList& params = QVariantList());

    QFuture<int> executeBatchInsertAsync(const QString& table, const QList<QVariantMap>& dataList);
    QString buildInsertSql(const QString& tableName, const QVariantMap& data);
    
    // RAII连接管理方法
    bool executeNonQueryWithGuard(const QString& query, const QVariantList& params = QVariantList());
    
    // 连接池统计信息
    QString getConnectionPoolStats() const;

    // 显式事务管理
    QSqlDatabase beginTransaction();
    bool commitTransaction(QSqlDatabase& db);
    bool rollbackTransaction(QSqlDatabase& db);

    // 取消正在执行的异步操作
    void cancelAsyncOperations();

    void cleanup();
private:
    DatabaseManager(QObject* parent = nullptr);
    ~DatabaseManager();
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    QSqlQuery prepareQuery(QSqlDatabase& db, const QString& query, const QVariantList& params);
    void bindParams(QSqlQuery& query, const QVariantList& params);
    bool isQueryTimedOut(const QElapsedTimer& timer) const;

    
    QThreadPool m_threadPool;
    int m_queryTimeoutMs;
    QMutex m_mutex;
    bool m_shuttingDown;

signals:
    void error(const QString& errorMessage);
    void queryTimedOut(const QString& query);
};

