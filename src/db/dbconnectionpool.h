// dbconnectionpool.h
#ifndef DBCONNECTIONPOOL_H
#define DBCONNECTIONPOOL_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QSqlDatabase>
#include <QVector>
#include <QTimer>
#include <QDateTime>
#include "dbhelper.h"
#include <QSqlError>

struct PooledConnection {
    QSqlDatabase connection;
    QDateTime lastUsed;
    bool inUse;
};

class SqlException : public std::runtime_error {
public:
    // 构造函数：接受错误消息和 QSqlError 对象
    SqlException(const QString& message, const QSqlError& error)
        : std::runtime_error(message.toStdString()),  // 将 QString 转换为标准异常消息
        m_error(error) {}                            // 保存 Qt 的 SQL 错误细节

  // 获取详细的数据库错误信息
    QSqlError getError() const {
        return m_error;
    }

private:
    QSqlError m_error;  // 保存 Qt 的 SQL 错误对象
};

class DbConnectionPool : public QObject
{
    Q_OBJECT
public:
    static DbConnectionPool& getInstance();

    bool initializePool(const DbType& dbType, const DbInfo& dbInfo, int maxConnections = 50, int minConnections = 1);
    QSqlDatabase getConnection();
    void releaseConnection(QSqlDatabase& connection);
    void closeAllConnections();

    bool isConnectionValid(QSqlDatabase& connection);
    bool reopenConnection(QSqlDatabase& connection);
    void ensureMinConnections();

private:
    DbConnectionPool(QObject* parent = nullptr);
    ~DbConnectionPool();
    DbConnectionPool(const DbConnectionPool&) = delete;
    DbConnectionPool& operator=(const DbConnectionPool&) = delete;

    QMutex m_mutex;
    DbType m_dbType;
    DbInfo m_dbInfo;
    int m_maxConnections;
    int m_minConnections;  // 最小连接数
    QVector<PooledConnection> m_connections;
    QTimer* m_checkTimer;
    int m_connectionTimeout; // 连接超时时间（秒）
    int m_idleTimeout; // 空闲连接超时时间（秒）

    PooledConnection createConnection(int connectionNumber);
    bool openConnection(QSqlDatabase& connection);
    void removeStaleConnections();

private slots:
    void checkConnections();

signals:
    void debug(const QString& msg);
    void error(const QString& msg);
};

#endif // DBCONNECTIONPOOL_H
