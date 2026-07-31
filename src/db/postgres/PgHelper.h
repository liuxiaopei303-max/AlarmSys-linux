#ifndef PGHELPER_H
#define PGHELPER_H

#include "PgDbInfo.h"
#include <string>
#include <vector>
#include <sstream>

// PostgreSQL辅助工具类
class PgHelper {
public:
    // 数据库类型转换
    static std::string getDbTypeString(const PgDbType& dbType);
    static PgDbType getDbType(const std::string& dbType);
    
    // 获取默认连接信息
    static void getDbDefaultInfo(const std::string& dbType, 
                                 int& hostPort,
                                 std::string& userName, 
                                 std::string& userPwd);
    
    // 获取测试连接语句
    static std::string getSelect1Sql();
    
    // 获取查询记录数语句
    static std::string getSelectCountSql(const std::string& table,
                                        const std::string& column,
                                        const std::string& where,
                                        const std::string& order,
                                        int count);
    
    // IP地址存活检测
    static bool ipLive(const std::string& ip, int port, int timeout = 1000);
    
    // 检查数据库连接前置条件
    static bool checkDatabase(const PgDbInfo& dbInfo);
    
    // 获取所有用户表
    static std::vector<std::string> getTables(void* connection);
    
    // 获取表的所有列
    static std::vector<std::string> getColumns(void* connection, const std::string& tableName);
    
    // 清空表数据并重置自增ID
    static bool clearTable(void* connection, const std::string& tableName);
    
    // 获取最大ID
    static int getMaxID(void* connection, const std::string& tableName, 
                       const std::string& columnName);
    
    // 构建日期范围SQL
    static void getBetweenDate(std::string& sql,
                              const std::string& columnName,
                              const std::string& dateTimeStart,
                              const std::string& dateTimeEnd,
                              bool varchar = true);
    
    // 转义SQL字符串
    static std::string escapeSqlString(const std::string& str);
    
    // 构建INSERT语句
    static std::string buildInsertSql(const std::string& tableName,
                                     const std::vector<std::string>& columns);
    
    // 构建UPDATE语句
    static std::string buildUpdateSql(const std::string& tableName,
                                     const std::vector<std::string>& columns,
                                     const std::string& whereClause);
    
    // 构建DELETE语句
    static std::string buildDeleteSql(const std::string& tableName,
                                     const std::string& whereClause);
    
    // 构建SELECT语句
    static std::string buildSelectSql(const std::string& tableName,
                                     const std::vector<std::string>& columns,
                                     const std::string& whereClause = "",
                                     const std::string& orderBy = "",
                                     int limit = 0);
    
    // PostgreSQL版本检测
    static std::string getServerVersion(void* connection);
    static int getServerVersionNum(void* connection);
    
    // 数据类型转换辅助
    static std::string boolToString(bool value);
    static bool stringToBool(const std::string& str);
    
    // 时间戳转换
    static std::string timestampToString(long long timestamp);
    static long long stringToTimestamp(const std::string& str);
    
    // JSON支持检测
    static bool supportsJson(void* connection);
    
    // UUID生成
    static std::string generateUUID();
    
    // 事务隔离级别
    enum IsolationLevel {
        READ_UNCOMMITTED,
        READ_COMMITTED,
        REPEATABLE_READ,
        SERIALIZABLE
    };
    
    static std::string getIsolationLevelString(IsolationLevel level);
    
    // 错误信息格式化
    static std::string formatError(const std::string& operation, 
                                   const std::string& errorMessage);
    
    // SQL语句格式化（用于日志）
    static std::string formatSql(const std::string& sql, 
                                 const std::vector<std::string>& params);
};

#endif // PGHELPER_H

