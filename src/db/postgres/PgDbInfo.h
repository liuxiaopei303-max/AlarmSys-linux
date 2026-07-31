#ifndef PGDBINFO_H
#define PGDBINFO_H

#include <string>
#include <cstring>
#include <sstream>
#include <chrono>

#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif

// PostgreSQL数据库类型枚举
enum PgDbType {
    PgDbType_PostgreSQL = 3,  // PostgreSQL数据库
    PgDbType_KingBase = 6,    // 人大金仓数据库（基于PostgreSQL）
    PgDbType_Other = 255      // 其他兼容PostgreSQL的数据库
};

// PostgreSQL数据库连接信息结构体
struct PgDbInfo {
    int connMode;           // 连接方式 0-直连数据库 1-使用连接字符串
    int timeout;            // 超时时间（毫秒） 0-不处理
    std::string connName;   // 连接名称
    std::string dbName;     // 数据库名
    
    std::string hostName;   // 主机地址
    int hostPort;           // 通信端口
    std::string userName;   // 用户名称
    std::string userPwd;    // 用户密码
    
    // 高级连接参数
    std::string sslMode;    // SSL模式: disable, allow, prefer, require, verify-ca, verify-full
    std::string sslCert;    // SSL证书路径
    std::string sslKey;     // SSL密钥路径
    std::string sslRootCert;// SSL根证书路径
    int connectTimeout;     // 连接超时（秒）
    std::string applicationName; // 应用名称
    
    // 默认构造函数
    PgDbInfo() {
        connMode = 0;
        timeout = 10000;
        connName = "pg_default_connection";
        dbName = "watchsystem";
        
        hostName = "192.168.18.141";
        hostPort = 5432;
        userName = "postgres";
        userPwd = "123456";
        
        sslMode = "disable";
        connectTimeout = 30;
        applicationName = "WatchSysApp";
    }
    
    // 生成PostgreSQL连接字符串
    std::string getConnectionString() const {
        std::ostringstream oss;
        
        if (connMode == 0) {
            // 直连模式
            oss << "host=" << hostName << " ";
            oss << "port=" << hostPort << " ";
            oss << "dbname=" << dbName << " ";
            oss << "user=" << userName << " ";
            if (!userPwd.empty()) {
                oss << "password=" << userPwd << " ";
            }
            
            // SSL配置
            if (!sslMode.empty()) {
                oss << "sslmode=" << sslMode << " ";
            }
            if (!sslCert.empty()) {
                oss << "sslcert=" << sslCert << " ";
            }
            if (!sslKey.empty()) {
                oss << "sslkey=" << sslKey << " ";
            }
            if (!sslRootCert.empty()) {
                oss << "sslrootcert=" << sslRootCert << " ";
            }
            
            // 连接超时
            if (connectTimeout > 0) {
                oss << "connect_timeout=" << connectTimeout << " ";
            }
            
            // 应用名称
            if (!applicationName.empty()) {
                oss << "application_name=" << applicationName << " ";
            }
            
            // 客户端编码
            oss << "client_encoding=UTF8";
        } else {
            // 使用自定义连接字符串（connName作为连接字符串）
            return connName;
        }
        
        return oss.str();
    }
    
    // 打印输出格式
    std::string toString() const {
        std::ostringstream oss;
        oss << "连接方式: " << (connMode == 0 ? "直连数据库" : "自定义连接字符串") << "\n";
        oss << "超时时间: " << timeout << "毫秒\n";
        oss << "连接名称: " << connName << "\n";
        oss << "数据库名: " << dbName << "\n";
        oss << "主机地址: " << hostName << "\n";
        oss << "通信端口: " << hostPort << "\n";
        oss << "用户名称: " << userName << "\n";
        oss << "SSL模式: " << sslMode << "\n";
        oss << "应用名称: " << applicationName;
        return oss.str();
    }
};

// 连接池中的连接信息
struct PgPooledConnection {
    void* connection;           // PGconn* 指针
    std::chrono::system_clock::time_point lastUsed;  // 最后使用时间
    bool inUse;                 // 是否正在使用
    std::string connectionName; // 连接名称
    
    PgPooledConnection() : connection(nullptr), inUse(false) {
        lastUsed = std::chrono::system_clock::now();
    }
};

// PostgreSQL查询结果封装
struct PgQueryResult {
    void* result;               // PGresult* 指针
    int rowCount;               // 行数
    int columnCount;            // 列数
    bool success;               // 是否成功
    std::string errorMessage;   // 错误消息
    
    PgQueryResult() : result(nullptr), rowCount(0), columnCount(0), success(false) {}
};

#endif // PGDBINFO_H

