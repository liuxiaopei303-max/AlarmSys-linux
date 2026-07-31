#include "PgDatabaseManager.h"
#include <libpq-fe.h>
#include <iostream>
#include <sstream>
#include <chrono>
#include <cstring>

PgDatabaseManager& PgDatabaseManager::getInstance() {
    static PgDatabaseManager instance;
    return instance;
}

PgDatabaseManager::PgDatabaseManager()
    : m_connectionPool(PgConnectionPool::getInstance())
    , m_queryTimeoutMs(10000)
    , m_shuttingDown(false) {
}

PgDatabaseManager::~PgDatabaseManager() {
    cancelAsyncOperations();
    cleanup();
}

bool PgDatabaseManager::initialize(const PgDbType& dbType, const PgDbInfo& dbInfo, int maxConnections) {
    logDebug("Initializing PostgreSQL database manager...");
    
    // 设置回调函数传递给连接池
    m_connectionPool.setErrorCallback([this](const std::string& msg) {
        this->logError3(msg);
    });
    
    m_connectionPool.setDebugCallback([this](const std::string& msg) {
        this->logDebug(msg);
    });
    
    bool result = m_connectionPool.initializePool(dbType, dbInfo, maxConnections);
    
    if (result) {
        logDebug("PostgreSQL database manager initialized successfully");
    } else {
        logError3("PostgreSQL database manager initialization failed");
    }
    
    return result;
}

void PgDatabaseManager::cleanup() {
    logDebug("Cleaning up PostgreSQL database manager...");
    m_connectionPool.closeAllConnections();
}

void PgDatabaseManager::setQueryTimeout(int milliseconds) {
    m_queryTimeoutMs = milliseconds;
}

int PgDatabaseManager::queryTimeout() const {
    return m_queryTimeoutMs;
}

std::vector<PgResultRow> PgDatabaseManager::executeQuery(const std::string& query,
                                                         const std::vector<PgParam>& params,
                                                         int timeoutMs) {
    if (m_shuttingDown) {
        logError3("系统正在关闭，拒绝执行查询");
        return std::vector<PgResultRow>();
    }
    
    // 获取连接
    void* conn = m_connectionPool.getConnection();
    if (conn == nullptr) {
        logError3("无法获取数据库连接");
        return std::vector<PgResultRow>();
    }
    
    try {
        // 执行查询
        PgQueryResult result = executeInternal(conn, query, params, timeoutMs);
        
        // 释放连接
        m_connectionPool.releaseConnection(conn);
        
        if (result.success) {
            return convertResult(result.result);
        } else {
            logError3("查询执行失败: " + result.errorMessage);
            return std::vector<PgResultRow>();
        }
    } catch (const std::exception& e) {
        logError3(std::string("查询执行异常: ") + e.what());
        m_connectionPool.releaseConnection(conn);
        return std::vector<PgResultRow>();
    }
}

bool PgDatabaseManager::executeNonQuery(const std::string& query,
                                       const std::vector<PgParam>& params,
                                       int timeoutMs) {
    if (m_shuttingDown) {
        logError3("系统正在关闭，拒绝执行非查询操作");
        return false;
    }
    
    // 获取连接
    void* conn = m_connectionPool.getConnection();
    if (conn == nullptr) {
        logError3("无法获取数据库连接");
        return false;
    }
    
    try {
        // 执行查询
        PgQueryResult result = executeInternal(conn, query, params, timeoutMs);
        
        // 释放连接
        m_connectionPool.releaseConnection(conn);
        
        if (!result.success) {
            logError3("非查询执行失败: " + result.errorMessage);
        }
        
        return result.success;
    } catch (const std::exception& e) {
        logError3(std::string("非查询执行异常: ") + e.what());
        m_connectionPool.releaseConnection(conn);
        return false;
    }
}

std::string PgDatabaseManager::executeScalar(const std::string& query,
                                            const std::vector<PgParam>& params,
                                            int timeoutMs) {
    std::vector<PgResultRow> results = executeQuery(query, params, timeoutMs);
    
    if (!results.empty() && !results[0].values.empty()) {
        // 返回第一行第一列的值
        return results[0].values.begin()->second;
    }
    
    return "";
}

std::future<std::vector<PgResultRow>> PgDatabaseManager::executeQueryAsync(
    const std::string& query,
    const std::vector<PgParam>& params) {
    
    if (m_shuttingDown) {
        logError3("系统正在关闭，拒绝异步查询");
        return std::async(std::launch::async, []() {
            return std::vector<PgResultRow>();
        });
    }
    
    return std::async(std::launch::async, [this, query, params]() {
        return this->executeQuery(query, params, 0);
    });
}

std::future<bool> PgDatabaseManager::executeNonQueryAsync(
    const std::string& query,
    const std::vector<PgParam>& params) {
    
    if (m_shuttingDown) {
        logError3("系统正在关闭，拒绝异步非查询");
        return std::async(std::launch::async, []() {
            return false;
        });
    }
    
    return std::async(std::launch::async, [this, query, params]() {
        return this->executeNonQuery(query, params, 0);
    });
}

std::future<std::string> PgDatabaseManager::executeScalarAsync(
    const std::string& query,
    const std::vector<PgParam>& params) {
    
    if (m_shuttingDown) {
        logError3("系统正在关闭，拒绝异步标量查询");
        return std::async(std::launch::async, []() {
            return std::string();
        });
    }
    
    return std::async(std::launch::async, [this, query, params]() {
        return this->executeScalar(query, params, 0);
    });
}

std::future<int> PgDatabaseManager::executeBatchInsertAsync(
    const std::string& tableName,
    const std::vector<std::map<std::string, PgParam>>& dataList) {
    
    if (m_shuttingDown) {
        logError3("系统正在关闭，拒绝批量插入");
        return std::async(std::launch::async, []() {
            return 0;
        });
    }
    
    return std::async(std::launch::async, [this, tableName, dataList]() {
        if (dataList.empty()) {
            return 0;
        }
        
        // 开始事务
        Transaction trans = beginTransaction();
        if (!trans.active) {
            logError3("无法开始批量插入事务");
            return 0;
        }
        
        int successCount = 0;
        
        try {
            // 获取列名（从第一条数据）
            std::vector<std::string> columns;
            for (const auto& kv : dataList[0]) {
                columns.push_back(kv.first);
            }
            
            // 构建INSERT语句
            std::ostringstream oss;
            oss << "INSERT INTO " << tableName << " (";
            for (size_t i = 0; i < columns.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << columns[i];
            }
            oss << ") VALUES (";
            for (size_t i = 0; i < columns.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << "$" << (i + 1);
            }
            oss << ")";
            
            std::string insertSql = oss.str();
            
            // 批量插入
            for (const auto& data : dataList) {
                if (m_shuttingDown) {
                    logError3("批量插入被中断");
                    rollbackTransaction(trans);
                    return successCount;
                }
                
                // 准备参数
                std::vector<PgParam> params;
                for (const auto& col : columns) {
                    auto it = data.find(col);
                    if (it != data.end()) {
                        params.push_back(it->second);
                    } else {
                        params.push_back(PgParam::Null());
                    }
                }
                
                // 执行插入
                if (executeNonQueryInTransaction(trans, insertSql, params)) {
                    successCount++;
                } else {
                    logError3("批量插入失败，回滚事务");
                    rollbackTransaction(trans);
                    return successCount;
                }
            }
            
            // 提交事务
            if (commitTransaction(trans)) {
                logDebug("批量插入成功: " + std::to_string(successCount) + " 条记录");
            } else {
                logError3("批量插入提交失败");
                successCount = 0;
            }
            
        } catch (const std::exception& e) {
            logError3(std::string("批量插入异常: ") + e.what());
            rollbackTransaction(trans);
            return successCount;
        }
        
        return successCount;
    });
}

PgDatabaseManager::Transaction PgDatabaseManager::beginTransaction() {
    Transaction trans;
    
    // 获取连接
    void* conn = m_connectionPool.getConnection();
    if (conn == nullptr) {
        logError3("无法获取数据库连接以开始事务");
        return trans;
    }
    
    PGconn* pgConn = static_cast<PGconn*>(conn);
    
    // 开始事务
    PGresult* res = PQexec(pgConn, "BEGIN");
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        logError3(std::string("开始事务失败: ") + PQerrorMessage(pgConn));
        PQclear(res);
        m_connectionPool.releaseConnection(conn);
        return trans;
    }
    
    PQclear(res);
    
    trans.connection = conn;
    trans.active = true;
    
    logDebug("事务已开始");
    return trans;
}

bool PgDatabaseManager::commitTransaction(Transaction& trans) {
    if (!trans.active || trans.connection == nullptr) {
        logError3("无效的事务状态");
        return false;
    }
    
    PGconn* pgConn = static_cast<PGconn*>(trans.connection);
    
    // 提交事务
    PGresult* res = PQexec(pgConn, "COMMIT");
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        logError3(std::string("提交事务失败: ") + PQerrorMessage(pgConn));
    } else {
        logDebug("事务已提交");
    }
    
    PQclear(res);
    
    // 释放连接
    m_connectionPool.releaseConnection(trans.connection);
    trans.connection = nullptr;
    trans.active = false;
    
    return success;
}

bool PgDatabaseManager::rollbackTransaction(Transaction& trans) {
    if (!trans.active || trans.connection == nullptr) {
        logError3("无效的事务状态");
        return false;
    }
    
    PGconn* pgConn = static_cast<PGconn*>(trans.connection);
    
    // 回滚事务
    PGresult* res = PQexec(pgConn, "ROLLBACK");
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    
    if (!success) {
        logError3(std::string("回滚事务失败: ") + PQerrorMessage(pgConn));
    } else {
        logDebug("事务已回滚");
    }
    
    PQclear(res);
    
    // 释放连接
    m_connectionPool.releaseConnection(trans.connection);
    trans.connection = nullptr;
    trans.active = false;
    
    return success;
}

std::vector<PgResultRow> PgDatabaseManager::executeQueryInTransaction(
    Transaction& trans,
    const std::string& query,
    const std::vector<PgParam>& params) {
    
    if (!trans.active || trans.connection == nullptr) {
        logError3("无效的事务状态");
        return std::vector<PgResultRow>();
    }
    
    try {
        PgQueryResult result = executeInternal(trans.connection, query, params, 0);
        
        if (result.success) {
            return convertResult(result.result);
        } else {
            logError3("事务中查询执行失败: " + result.errorMessage);
            return std::vector<PgResultRow>();
        }
    } catch (const std::exception& e) {
        logError3(std::string("事务中查询执行异常: ") + e.what());
        return std::vector<PgResultRow>();
    }
}

bool PgDatabaseManager::executeNonQueryInTransaction(
    Transaction& trans,
    const std::string& query,
    const std::vector<PgParam>& params) {
    
    if (!trans.active || trans.connection == nullptr) {
        logError3("无效的事务状态");
        return false;
    }
    
    try {
        PgQueryResult result = executeInternal(trans.connection, query, params, 0);
        
        if (!result.success) {
            logError3("事务中非查询执行失败: " + result.errorMessage);
        }
        
        return result.success;
    } catch (const std::exception& e) {
        logError3(std::string("事务中非查询执行异常: ") + e.what());
        return false;
    }
}

void PgDatabaseManager::cancelAsyncOperations() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_shuttingDown = true;
    logDebug("异步操作已取消");
}

void PgDatabaseManager::setErrorCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = callback;
}

void PgDatabaseManager::setDebugCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_debugCallback = callback;
}

void PgDatabaseManager::setQueryTimeoutCallback(std::function<void(const std::string&)> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_queryTimeoutCallback = callback;
}

PgQueryResult PgDatabaseManager::executeInternal(void* connection,
                                                 const std::string& query,
                                                 const std::vector<PgParam>& params,
                                                 int timeoutMs) {
    PgQueryResult queryResult;
    
    if (connection == nullptr) {
        queryResult.errorMessage = "连接为空";
        return queryResult;
    }
    
    PGconn* pgConn = static_cast<PGconn*>(connection);
    
    // 准备参数
    std::vector<const char*> paramValues;
    std::vector<int> paramLengths;
    std::vector<int> paramFormats;
    std::vector<std::string> stringStorage;
    
    prepareParams(params, paramValues, paramLengths, paramFormats, stringStorage);
    
    // 执行查询
    PGresult* res = nullptr;
    
    if (params.empty()) {
        res = PQexec(pgConn, query.c_str());
    } else {
        res = PQexecParams(pgConn,
                          query.c_str(),
                          params.size(),
                          nullptr,
                          paramValues.data(),
                          paramLengths.data(),
                          paramFormats.data(),
                          0);
    }
    
    if (res == nullptr) {
        queryResult.errorMessage = std::string("查询执行失败: ") + PQerrorMessage(pgConn);
        return queryResult;
    }
    
    ExecStatusType status = PQresultStatus(res);
    
    if (status == PGRES_TUPLES_OK || status == PGRES_COMMAND_OK) {
        queryResult.success = true;
        queryResult.result = res;
        queryResult.rowCount = PQntuples(res);
        queryResult.columnCount = PQnfields(res);
    } else {
        queryResult.success = false;
        queryResult.errorMessage = PQresultErrorMessage(res);
        PQclear(res);
    }
    
    return queryResult;
}

void PgDatabaseManager::prepareParams(const std::vector<PgParam>& params,
                                     std::vector<const char*>& paramValues,
                                     std::vector<int>& paramLengths,
                                     std::vector<int>& paramFormats,
                                     std::vector<std::string>& stringStorage) {
    paramValues.clear();
    paramLengths.clear();
    paramFormats.clear();
    stringStorage.clear();
    
    stringStorage.reserve(params.size());
    
    for (const auto& param : params) {
        switch (param.type) {
            case PG_PARAM_NULL:
                paramValues.push_back(nullptr);
                paramLengths.push_back(0);
                paramFormats.push_back(0);
                break;
                
            case PG_PARAM_INT: {
                std::string str = std::to_string(param.intValue);
                stringStorage.push_back(str);
                paramValues.push_back(stringStorage.back().c_str());
                paramLengths.push_back(stringStorage.back().length());
                paramFormats.push_back(0);
                break;
            }
                
            case PG_PARAM_INT64: {
                std::string str = std::to_string(param.int64Value);
                stringStorage.push_back(str);
                paramValues.push_back(stringStorage.back().c_str());
                paramLengths.push_back(stringStorage.back().length());
                paramFormats.push_back(0);
                break;
            }
                
            case PG_PARAM_DOUBLE: {
                std::ostringstream oss;
                oss.precision(15);
                oss << param.doubleValue;
                stringStorage.push_back(oss.str());
                paramValues.push_back(stringStorage.back().c_str());
                paramLengths.push_back(stringStorage.back().length());
                paramFormats.push_back(0);
                break;
            }
                
            case PG_PARAM_STRING:
                stringStorage.push_back(param.stringValue);
                paramValues.push_back(stringStorage.back().c_str());
                paramLengths.push_back(stringStorage.back().length());
                paramFormats.push_back(0);
                break;
                
            case PG_PARAM_BOOL: {
                std::string str = param.boolValue ? "true" : "false";
                stringStorage.push_back(str);
                paramValues.push_back(stringStorage.back().c_str());
                paramLengths.push_back(stringStorage.back().length());
                paramFormats.push_back(0);
                break;
            }
                
            case PG_PARAM_BYTEA:
                if (!param.byteaValue.empty()) {
                    paramValues.push_back(reinterpret_cast<const char*>(param.byteaValue.data()));
                    paramLengths.push_back(param.byteaValue.size());
                    paramFormats.push_back(1); // 二进制格式
                } else {
                    paramValues.push_back(nullptr);
                    paramLengths.push_back(0);
                    paramFormats.push_back(0);
                }
                break;
        }
    }
}

std::vector<PgResultRow> PgDatabaseManager::convertResult(void* result) {
    std::vector<PgResultRow> rows;
    
    if (result == nullptr) {
        return rows;
    }
    
    PGresult* res = static_cast<PGresult*>(result);
    
    int numRows = PQntuples(res);
    int numCols = PQnfields(res);
    
    // 获取列名
    std::vector<std::string> columnNames;
    for (int i = 0; i < numCols; ++i) {
        columnNames.push_back(PQfname(res, i));
    }
    
    // 转换每一行
    for (int row = 0; row < numRows; ++row) {
        PgResultRow resultRow;
        resultRow.columns = columnNames;
        resultRow.valuesByIndex.reserve(static_cast<size_t>(numCols));
        
        for (int col = 0; col < numCols; ++col) {
            std::string columnName = columnNames[col];
            std::string cell;
            if (PQgetisnull(res, row, col)) {
                cell.clear();
            } else {
                cell = PQgetvalue(res, row, col);
            }
            resultRow.valuesByIndex.push_back(cell);
            // 同名列以最后一次为准；按序访问请用 valuesByIndex
            resultRow.values[columnName] = cell;
        }
        
        rows.push_back(resultRow);
    }
    
    // 清理结果
    PQclear(res);
    
    return rows;
}

void PgDatabaseManager::logError3(const std::string& msg) {
    if (m_errorCallback) {
        m_errorCallback(msg);
    } else {
        std::cerr << "[PgDatabaseManager ERROR] " << msg << std::endl;
    }
}

void PgDatabaseManager::logDebug(const std::string& msg) {
    if (m_debugCallback) {
        m_debugCallback(msg);
    } else {
        std::cout << "[PgDatabaseManager DEBUG] " << msg << std::endl;
    }
}

void PgDatabaseManager::logQueryTimeout(const std::string& query) {
    if (m_queryTimeoutCallback) {
        m_queryTimeoutCallback(query);
    } else {
        std::cerr << "[PgDatabaseManager TIMEOUT] " << query << std::endl;
    }
}

