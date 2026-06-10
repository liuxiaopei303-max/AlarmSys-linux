// dbconnectionpool.h
#ifndef DBCONNECTIONPOOL_H
#define DBCONNECTIONPOOL_H

#include <QObject>
#include <QSqlDatabase>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QDateTime>
#include <memory>
#include <atomic>

// 数据库类型枚举
enum class DbType {
    PostgreSQL,
    MySQL,
    SQLite
};

// 数据库连接信息
struct DbInfo {
    QString hostName;
    int hostPort;
    QString dbName;
    QString userName;
    QString userPwd;
};

// 简化的连接信息
struct ConnectionInfo {
    QSqlDatabase database;
    QDateTime createdTime;
    QDateTime lastUsedTime;
    QString connectionName;
    std::atomic<bool> isValid{true};
    
    ConnectionInfo() = default;
    ConnectionInfo(const ConnectionInfo& other) 
        : database(other.database)
        , createdTime(other.createdTime)
        , lastUsedTime(other.lastUsedTime)
        , connectionName(other.connectionName)
        , isValid(other.isValid.load())
    {}
};

// 前置声明
class SimpleConnectionPool;

// 连接保护类 - 使用RAII管理连接生命周期
class ConnectionGuard {
public:
    explicit ConnectionGuard(SimpleConnectionPool* pool);
    ~ConnectionGuard();
    
    ConnectionGuard(const ConnectionGuard&) = delete;
    ConnectionGuard& operator=(const ConnectionGuard&) = delete;
    
    QSqlDatabase& getDatabase();
    bool isValid() const;

private:
    SimpleConnectionPool* m_pool;
    QSqlDatabase m_database;
    bool m_isValid;
};

// 简化的连接池类
class SimpleConnectionPool : public QObject {
    Q_OBJECT

public:
    static SimpleConnectionPool& getInstance();
    
    // 初始化连接池
    bool initialize(const DbType& dbType, const DbInfo& dbInfo, 
                   int maxConnections = 20, int minConnections = 2);
    
    // 获取连接（阻塞式）
    QSqlDatabase getConnection(int timeoutMs = 5000);
    
    // 释放连接
    void releaseConnection(const QSqlDatabase& connection);
    
    // 获取连接保护器
    std::unique_ptr<ConnectionGuard> getConnectionGuard();
    
    // 获取统计信息
    int getTotalConnections() const;
    int getActiveConnections() const;
    int getAvailableConnections() const;
    
    // 关闭所有连接
    void closeAll();

signals:
    void error(const QString& message);
    void debug(const QString& message);

private slots:
    void performMaintenance();

private:
    explicit SimpleConnectionPool(QObject* parent = nullptr);
    ~SimpleConnectionPool();
    
    // 创建新连接
    QSqlDatabase createNewConnection();
    
    // 验证连接有效性
    bool validateConnection(const QSqlDatabase& db);
    
    // 清理过期连接
    void cleanupExpiredConnections();
    
    // 确保最小连接数
    void ensureMinimumConnections();

private:
    // 连接池配置
    DbType m_dbType;
    DbInfo m_dbInfo;
    int m_maxConnections;
    int m_minConnections;
    int m_connectionTimeoutSeconds;
    
    // 连接管理
    QQueue<ConnectionInfo> m_availableConnections;  // 可用连接队列
    QList<ConnectionInfo> m_activeConnections;      // 活动连接列表
    
    // 线程同步
    mutable QMutex m_mutex;
    QWaitCondition m_condition;
    
    // 维护定时器
    QTimer* m_maintenanceTimer;
    
    // 计数器
    std::atomic<int> m_connectionCounter{0};
    
    // 初始化标志
    std::atomic<bool> m_isInitialized{false};
    
    friend class ConnectionGuard;
};

// 全局异常类
class DatabaseException : public std::exception {
public:
    explicit DatabaseException(const QString& message) 
        : m_message(message.toStdString()) {}
    
    const char* what() const noexcept override {
        return m_message.c_str();
    }
    
private:
    std::string m_message;
};

#endif // DBCONNECTIONPOOL_H
