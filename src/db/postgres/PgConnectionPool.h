#ifndef PGCONNECTIONPOOL_H
#define PGCONNECTIONPOOL_H

#include "PgDbInfo.h"
#include <vector>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <functional>
#include <thread>
#include <atomic>

// PostgreSQL连接池类
class PgConnectionPool {
public:
    // 获取单例实例
    static PgConnectionPool& getInstance();
    
    // 初始化连接池
    bool initializePool(const PgDbType& dbType, const PgDbInfo& dbInfo, 
                       int maxConnections = 50, int minConnections = 1);
    
    // 获取连接
    void* getConnection();
    
    // 释放连接
    void releaseConnection(void* connection);
    
    // 关闭所有连接
    void closeAllConnections();
    
    // 连接有效性检查
    bool isConnectionValid(void* connection);
    
    // 重新打开连接
    bool reopenConnection(void* connection);
    
    // 确保最小连接数
    void ensureMinConnections();
    
    // 设置错误回调
    void setErrorCallback(std::function<void(const std::string&)> callback);
    
    // 设置调试回调
    void setDebugCallback(std::function<void(const std::string&)> callback);
    
    // 获取连接池统计信息
    struct PoolStats {
        int totalConnections;
        int activeConnections;
        int idleConnections;
        int maxConnections;
        int minConnections;
    };
    PoolStats getStats() const;
    
private:
    PgConnectionPool();
    ~PgConnectionPool();
    
    // 禁用拷贝和赋值
    PgConnectionPool(const PgConnectionPool&) = delete;
    PgConnectionPool& operator=(const PgConnectionPool&) = delete;
    
    // 创建新连接
    PgPooledConnection createConnection(int connectionNumber);
    
    // 打开连接
    bool openConnection(void* connection);
    
    // 关闭连接
    void closeConnection(void* connection);
    
    // 移除过期连接
    void removeStaleConnections();
    
    // 连接检查线程
    void checkConnectionsThread();
    
    // 启动检查线程
    void startCheckThread();
    
    // 停止检查线程
    void stopCheckThread();
    
    // 数据成员
    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    
    PgDbType m_dbType;
    PgDbInfo m_dbInfo;
    int m_maxConnections;
    int m_minConnections;
    
    std::vector<PgPooledConnection> m_connections;
    
    int m_connectionTimeout;    // 连接超时时间（秒）
    int m_idleTimeout;          // 空闲连接超时时间（秒）
    
    std::atomic<bool> m_running;
    std::condition_variable m_stopCondition;
    std::mutex m_stopMutex;
    std::thread m_checkThread;
    
    std::function<void(const std::string&)> m_errorCallback;
    std::function<void(const std::string&)> m_debugCallback;
    
    // 内部工具函数
    void logError2(const std::string& msg);
    void logDebug(const std::string& msg);
};

#endif // PGCONNECTIONPOOL_H

