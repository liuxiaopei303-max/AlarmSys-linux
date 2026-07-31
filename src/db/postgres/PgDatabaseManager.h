#ifndef PGDATABASEMANAGER_H
#define PGDATABASEMANAGER_H

#include "PgConnectionPool.h"
#include "PgDbInfo.h"
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <future>
#include <thread>
#include <atomic>

// 参数类型（支持多种数据类型）
enum PgParamType {
    PG_PARAM_NULL,
    PG_PARAM_INT,
    PG_PARAM_INT64,
    PG_PARAM_DOUBLE,
    PG_PARAM_STRING,
    PG_PARAM_BOOL,
    PG_PARAM_BYTEA
};

// SQL参数封装
struct PgParam {
    PgParamType type;
    union {
        int intValue;
        long long int64Value;
        double doubleValue;
        bool boolValue;
    };
    std::string stringValue;
    std::vector<unsigned char> byteaValue;
    
    PgParam() : type(PG_PARAM_NULL), intValue(0) {}
    
    static PgParam Null() {
        PgParam p;
        p.type = PG_PARAM_NULL;
        return p;
    }
    
    static PgParam Int(int value) {
        PgParam p;
        p.type = PG_PARAM_INT;
        p.intValue = value;
        return p;
    }
    
    static PgParam Int64(long long value) {
        PgParam p;
        p.type = PG_PARAM_INT64;
        p.int64Value = value;
        return p;
    }
    
    static PgParam Double(double value) {
        PgParam p;
        p.type = PG_PARAM_DOUBLE;
        p.doubleValue = value;
        return p;
    }
    
    static PgParam String(const std::string& value) {
        PgParam p;
        p.type = PG_PARAM_STRING;
        p.stringValue = value;
        return p;
    }
    
    static PgParam Bool(bool value) {
        PgParam p;
        p.type = PG_PARAM_BOOL;
        p.boolValue = value;
        return p;
    }
    
    static PgParam Bytea(const std::vector<unsigned char>& value) {
        PgParam p;
        p.type = PG_PARAM_BYTEA;
        p.byteaValue = value;
        return p;
    }
};

// 查询结果行
struct PgResultRow {
    std::vector<std::string> columns;
    std::map<std::string, std::string> values;
    /** 按列序取值（避免 coalesce 等同名列互相覆盖） */
    std::vector<std::string> valuesByIndex;
    
    std::string getValue(const std::string& columnName, const std::string& defaultValue = "") const {
        auto it = values.find(columnName);
        return (it != values.end()) ? it->second : defaultValue;
    }

    std::string getValueAt(size_t index, const std::string& defaultValue = "") const {
        if (index < valuesByIndex.size()) {
            return valuesByIndex[index];
        }
        if (index < columns.size()) {
            return getValue(columns[index], defaultValue);
        }
        return defaultValue;
    }
    
    int getInt(const std::string& columnName, int defaultValue = 0) const {
        auto it = values.find(columnName);
        if (it != values.end() && !it->second.empty()) {
            try {
                return std::stoi(it->second);
            } catch (...) {}
        }
        return defaultValue;
    }
    
    long long getInt64(const std::string& columnName, long long defaultValue = 0) const {
        auto it = values.find(columnName);
        if (it != values.end() && !it->second.empty()) {
            try {
                return std::stoll(it->second);
            } catch (...) {}
        }
        return defaultValue;
    }
    
    double getDouble(const std::string& columnName, double defaultValue = 0.0) const {
        auto it = values.find(columnName);
        if (it != values.end() && !it->second.empty()) {
            try {
                return std::stod(it->second);
            } catch (...) {}
        }
        return defaultValue;
    }
    
    bool getBool(const std::string& columnName, bool defaultValue = false) const {
        auto it = values.find(columnName);
        if (it != values.end()) {
            return it->second == "t" || it->second == "true" || it->second == "1";
        }
        return defaultValue;
    }
};

// PostgreSQL数据库管理器
class PgDatabaseManager {
public:
    // 获取单例实例
    static PgDatabaseManager& getInstance();
    
    // 初始化数据库管理器
    bool initialize(const PgDbType& dbType, const PgDbInfo& dbInfo, int maxConnections = 50);
    
    // 清理资源
    void cleanup();
    
    // 超时设置
    void setQueryTimeout(int milliseconds);
    int queryTimeout() const;
    
    // 同步查询方法
    std::vector<PgResultRow> executeQuery(const std::string& query, 
                                          const std::vector<PgParam>& params = std::vector<PgParam>(),
                                          int timeoutMs = 0);
    
    // 同步非查询方法（INSERT, UPDATE, DELETE等）
    bool executeNonQuery(const std::string& query,
                        const std::vector<PgParam>& params = std::vector<PgParam>(),
                        int timeoutMs = 0);
    
    // 同步标量查询（返回单个值）
    std::string executeScalar(const std::string& query,
                             const std::vector<PgParam>& params = std::vector<PgParam>(),
                             int timeoutMs = 0);
    
    // 异步查询方法
    std::future<std::vector<PgResultRow>> executeQueryAsync(const std::string& query,
                                                           const std::vector<PgParam>& params = std::vector<PgParam>());
    
    std::future<bool> executeNonQueryAsync(const std::string& query,
                                          const std::vector<PgParam>& params = std::vector<PgParam>());
    
    std::future<std::string> executeScalarAsync(const std::string& query,
                                               const std::vector<PgParam>& params = std::vector<PgParam>());
    
    // 批量插入
    std::future<int> executeBatchInsertAsync(const std::string& tableName,
                                            const std::vector<std::map<std::string, PgParam>>& dataList);
    
    // 事务管理
    struct Transaction {
        void* connection;
        bool active;
        Transaction() : connection(nullptr), active(false) {}
    };
    
    Transaction beginTransaction();
    bool commitTransaction(Transaction& trans);
    bool rollbackTransaction(Transaction& trans);
    
    // 在事务中执行查询
    std::vector<PgResultRow> executeQueryInTransaction(Transaction& trans, const std::string& query,
                                                      const std::vector<PgParam>& params = std::vector<PgParam>());
    
    bool executeNonQueryInTransaction(Transaction& trans, const std::string& query,
                                     const std::vector<PgParam>& params = std::vector<PgParam>());
    
    // 取消异步操作
    void cancelAsyncOperations();
    
    // 设置回调函数
    void setErrorCallback(std::function<void(const std::string&)> callback);
    void setDebugCallback(std::function<void(const std::string&)> callback);
    void setQueryTimeoutCallback(std::function<void(const std::string&)> callback);
    
    // 获取连接池引用
    PgConnectionPool& getConnectionPool() { return m_connectionPool; }
    
private:
    PgDatabaseManager();
    ~PgDatabaseManager();
    
    // 禁用拷贝和赋值
    PgDatabaseManager(const PgDatabaseManager&) = delete;
    PgDatabaseManager& operator=(const PgDatabaseManager&) = delete;
    
    // 内部执行函数
    PgQueryResult executeInternal(void* connection, const std::string& query,
                                  const std::vector<PgParam>& params, int timeoutMs);
    
    // 参数准备
    void prepareParams(const std::vector<PgParam>& params,
                      std::vector<const char*>& paramValues,
                      std::vector<int>& paramLengths,
                      std::vector<int>& paramFormats,
                      std::vector<std::string>& stringStorage);
    
    // 结果转换
    std::vector<PgResultRow> convertResult(void* result);
    
    // 日志函数
    void logError3(const std::string& msg);
    void logDebug(const std::string& msg);
    void logQueryTimeout(const std::string& query);
    
    // 数据成员
    PgConnectionPool& m_connectionPool;
    std::atomic<int> m_queryTimeoutMs;
    std::atomic<bool> m_shuttingDown;
    
    mutable std::mutex m_mutex;
    
    std::function<void(const std::string&)> m_errorCallback;
    std::function<void(const std::string&)> m_debugCallback;
    std::function<void(const std::string&)> m_queryTimeoutCallback;
};

#endif // PGDATABASEMANAGER_H

