#include "PgConnectionPool.h"
#include <libpq-fe.h>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

PgConnectionPool& PgConnectionPool::getInstance() {
    static PgConnectionPool instance;
    static bool atexitRegistered = false;
    if (!atexitRegistered) {
        atexitRegistered = true;
        std::atexit([]() {
            PgConnectionPool::getInstance().closeAllConnections();
        });
    }
    return instance;
}

PgConnectionPool::PgConnectionPool() 
    : m_maxConnections(50)
    , m_minConnections(1)
    , m_connectionTimeout(300)
    , m_idleTimeout(3000)
    , m_running(false) {
}

PgConnectionPool::~PgConnectionPool() {
    stopCheckThread();
    closeAllConnections();
}

bool PgConnectionPool::initializePool(const PgDbType& dbType, const PgDbInfo& dbInfo,
                                     int maxConnections, int minConnections) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_dbType = dbType;
    m_dbInfo = dbInfo;
    m_maxConnections = maxConnections;
    m_minConnections = minConnections;
    
    // 初始化时创建一个连接以测试连接参数
    PgPooledConnection conn = createConnection(0);
    if (conn.connection != nullptr) {
        m_connections.push_back(conn);
        
        // 启动连接检查线程
        startCheckThread();
        
        logDebug("PostgreSQL连接池初始化成功");
        return true;
    }
    
    logError2("PostgreSQL连接池初始化失败");
    return false;
}

void* PgConnectionPool::getConnection() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    logDebug("请求数据库连接，当前连接池大小: " + std::to_string(m_connections.size()));
    
    // 查找可用连接
    for (auto& conn : m_connections) {
        if (!conn.inUse) {
            // 检查连接是否有效
            if (!isConnectionValid(conn.connection)) {
                // 尝试重新打开
                if (!reopenConnection(conn.connection)) {
                    continue;
                }
            }
            
            conn.inUse = true;
            conn.lastUsed = std::chrono::system_clock::now();
            logDebug("复用现有连接: " + conn.connectionName);
            return conn.connection;
        }
    }
    
    // 如果没有可用连接且未达到最大连接数，创建新连接
    if (static_cast<int>(m_connections.size()) < m_maxConnections) {
        PgPooledConnection newConn = createConnection(m_connections.size());
        if (newConn.connection != nullptr) {
            newConn.inUse = true;
            newConn.lastUsed = std::chrono::system_clock::now();
            m_connections.push_back(newConn);
            logDebug("创建新连接: " + newConn.connectionName);
            return newConn.connection;
        }
    }
    
    // 等待可用连接（最多等待30秒）
    logDebug("连接池已满，等待可用连接...");
    if (m_condition.wait_for(lock, std::chrono::seconds(30), [this] {
        return std::any_of(m_connections.begin(), m_connections.end(),
                          [](const PgPooledConnection& c) { return !c.inUse; });
    })) {
        // 再次尝试获取连接
        for (auto& conn : m_connections) {
            if (!conn.inUse) {
                if (isConnectionValid(conn.connection) || reopenConnection(conn.connection)) {
                    conn.inUse = true;
                    conn.lastUsed = std::chrono::system_clock::now();
                    logDebug("等待后获取连接: " + conn.connectionName);
                    return conn.connection;
                }
            }
        }
    }
    
    logError2("无法获取数据库连接：连接池已满且等待超时");
    return nullptr;
}

void PgConnectionPool::releaseConnection(void* connection) {
    if (connection == nullptr) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& conn : m_connections) {
        if (conn.connection == connection) {
            conn.inUse = false;
            conn.lastUsed = std::chrono::system_clock::now();
            logDebug("释放连接: " + conn.connectionName);
            
            // 通知等待的线程
            m_condition.notify_one();
            break;
        }
    }
}

void PgConnectionPool::closeAllConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    for (auto& conn : m_connections) {
        if (conn.connection != nullptr) {
            PGconn* pgConn = static_cast<PGconn*>(conn.connection);
            PQfinish(pgConn);
            conn.connection = nullptr;
        }
    }
    
    m_connections.clear();
    logDebug("所有数据库连接已关闭");
}

bool PgConnectionPool::isConnectionValid(void* connection) {
    if (connection == nullptr) {
        return false;
    }
    
    PGconn* pgConn = static_cast<PGconn*>(connection);
    
    // 检查连接状态
    if (PQstatus(pgConn) != CONNECTION_OK) {
        return false;
    }
    
    // 执行简单查询测试连接
    PGresult* res = PQexec(pgConn, "SELECT 1");
    if (res == nullptr || PQresultStatus(res) != PGRES_TUPLES_OK) {
        if (res != nullptr) {
            PQclear(res);
        }
        return false;
    }
    
    PQclear(res);
    return true;
}

bool PgConnectionPool::reopenConnection(void* connection) {
    if (connection == nullptr) {
        return false;
    }
    
    PGconn* pgConn = static_cast<PGconn*>(connection);
    
    // 重置连接
    PQreset(pgConn);
    
    // 检查重置后的状态
    if (PQstatus(pgConn) == CONNECTION_OK) {
        logDebug("连接重置成功");
        return true;
    }
    
    logError2(std::string("连接重置失败: ") + PQerrorMessage(pgConn));
    return false;
}

void PgConnectionPool::ensureMinConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    int currentConnections = m_connections.size();
    int connectionsToAdd = std::max(0, m_minConnections - currentConnections);
    
    for (int i = 0; i < connectionsToAdd; ++i) {
        PgPooledConnection newConn = createConnection(currentConnections + i);
        if (newConn.connection != nullptr) {
            m_connections.push_back(newConn);
            logDebug("添加最小连接: " + newConn.connectionName);
        } else {
            logError2("无法创建最小连接数");
            break;
        }
    }
}

PgPooledConnection PgConnectionPool::createConnection(int connectionNumber) {
    PgPooledConnection pooledConn;
    
    // 生成唯一连接名称
    std::ostringstream oss;
    oss << "PgConn_" << connectionNumber << "_" 
        << std::chrono::system_clock::now().time_since_epoch().count();
    pooledConn.connectionName = oss.str();
    
    // 获取连接字符串
    std::string connStr = m_dbInfo.getConnectionString();
    
    // 创建PostgreSQL连接
    PGconn* pgConn = PQconnectdb(connStr.c_str());
    
    if (PQstatus(pgConn) != CONNECTION_OK) {
        logError2(std::string("创建连接失败: ") + PQerrorMessage(pgConn));
        PQfinish(pgConn);
        return pooledConn;
    }
    
    // 设置客户端编码为UTF8
    PGresult* res = PQexec(pgConn, "SET client_encoding = 'UTF8'");
    if (res != nullptr) {
        PQclear(res);
    }
    
    pooledConn.connection = pgConn;
    pooledConn.inUse = false;
    pooledConn.lastUsed = std::chrono::system_clock::now();
    
    logDebug("创建连接成功: " + pooledConn.connectionName);
    return pooledConn;
}

void PgConnectionPool::removeStaleConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto now = std::chrono::system_clock::now();
    
    auto it = m_connections.begin();
    while (it != m_connections.end()) {
        if (!it->inUse) {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                now - it->lastUsed).count();
            
            if (elapsed > m_idleTimeout) {
                logDebug("移除过期空闲连接: " + it->connectionName);
                closeConnection(it->connection);
                it = m_connections.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void PgConnectionPool::checkConnectionsThread() {
    logDebug("连接检查线程启动");
    
    while (m_running) {
        // 可被 stopCheckThread 立即唤醒，避免退出时卡 30s
        {
            std::unique_lock<std::mutex> stopLock(m_stopMutex);
            m_stopCondition.wait_for(stopLock, std::chrono::seconds(30), [this] {
                return !m_running.load();
            });
        }
        
        if (!m_running) {
            break;
        }
        
        // 移除过期连接
        removeStaleConnections();
        
        // 确保最小连接数
        ensureMinConnections();
        
        // 检查连接健康状态
        std::lock_guard<std::mutex> lock(m_mutex);
        auto now = std::chrono::system_clock::now();
        
        for (auto& conn : m_connections) {
            if (!conn.inUse) {
                // 检查空闲连接是否有效
                if (!isConnectionValid(conn.connection)) {
                    logDebug("检测到无效连接，尝试重新打开: " + conn.connectionName);
                    reopenConnection(conn.connection);
                }
            } else {
                // 检查使用中的连接是否超时
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - conn.lastUsed).count();
                
                if (elapsed > m_connectionTimeout) {
                    logError2("连接使用超时: " + conn.connectionName);
                }
            }
        }
    }
    
    logDebug("连接检查线程停止");
}

void PgConnectionPool::startCheckThread() {
    if (!m_running) {
        m_running = true;
        m_checkThread = std::thread(&PgConnectionPool::checkConnectionsThread, this);
    }
}

void PgConnectionPool::stopCheckThread() {
    if (m_running) {
        m_running = false;
        m_stopCondition.notify_all();
        if (m_checkThread.joinable()) {
            m_checkThread.join();
        }
    }
}

void PgConnectionPool::closeConnection(void* connection) {
    if (connection != nullptr) {
        PGconn* pgConn = static_cast<PGconn*>(connection);
        PQfinish(pgConn);
    }
}

void PgConnectionPool::setErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = callback;
}

void PgConnectionPool::setDebugCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_debugCallback = callback;
}

PgConnectionPool::PoolStats PgConnectionPool::getStats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    PoolStats stats;
    stats.totalConnections = m_connections.size();
    stats.maxConnections = m_maxConnections;
    stats.minConnections = m_minConnections;
    stats.activeConnections = 0;
    stats.idleConnections = 0;
    
    for (const auto& conn : m_connections) {
        if (conn.inUse) {
            stats.activeConnections++;
        } else {
            stats.idleConnections++;
        }
    }
    
    return stats;
}

void PgConnectionPool::logError2(const std::string& msg) {
    if (m_errorCallback) {
        m_errorCallback(msg);
    } else {
        std::cerr << "[PgConnectionPool ERROR] " << msg << std::endl;
    }
}

void PgConnectionPool::logDebug(const std::string& msg) {
    if (m_debugCallback) {
        m_debugCallback(msg);
    } else {
        std::cout << "[PgConnectionPool DEBUG] " << msg << std::endl;
    }
}

