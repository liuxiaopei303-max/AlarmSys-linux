#include "PgHelper.h"
#include <libpq-fe.h>
#include <algorithm>
#include <ctime>
#include <random>
#include <iomanip>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#endif

std::string PgHelper::getDbTypeString(const PgDbType& dbType) {
    std::string type = "OTHER";
    if (dbType == PgDbType_PostgreSQL) {
        type = "POSTGRESQL";
    } else if (dbType == PgDbType_KingBase) {
        type = "KINGBASE";
    }
    return type;
}

PgDbType PgHelper::getDbType(const std::string& dbType) {
    std::string flag = dbType;
    std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
    
    PgDbType type = PgDbType_Other;
    if (flag == "POSTGRESQL") {
        type = PgDbType_PostgreSQL;
    } else if (flag == "KINGBASE") {
        type = PgDbType_KingBase;
    }
    
    return type;
}

void PgHelper::getDbDefaultInfo(const std::string& dbType,
                                int& hostPort,
                                std::string& userName,
                                std::string& userPwd) {
    std::string flag = dbType;
    std::transform(flag.begin(), flag.end(), flag.begin(), ::toupper);
    
    if (flag == "POSTGRESQL") {
        hostPort = 5432;
        userName = "postgres";
        userPwd = "123456";
    } else if (flag == "KINGBASE") {
        hostPort = 54321;
        userName = "SYSTEM";
        userPwd = "123456";
    }
}

std::string PgHelper::getSelect1Sql() {
    return "SELECT 1";
}

std::string PgHelper::getSelectCountSql(const std::string& table,
                                       const std::string& column,
                                       const std::string& where,
                                       const std::string& order,
                                       int count) {
    std::ostringstream oss;
    
    oss << "SELECT " << column << " FROM " << table;
    
    if (!where.empty()) {
        oss << " WHERE " << where;
    }
    
    if (!order.empty()) {
        oss << " ORDER BY " << order;
    }
    
    if (count > 0) {
        oss << " LIMIT " << count;
    }
    
    return oss.str();
}

bool PgHelper::ipLive(const std::string& ip, int port, int timeout) {
    if (timeout <= 0) {
        return true;
    }
    
#ifdef _WIN32
    // Windows平台初始化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }
#endif
    
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
#ifdef _WIN32
        WSACleanup();
#endif
        return false;
    }
    
    // 设置非阻塞模式
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
#else
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);
#endif
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);
    
    connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    
    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(sock, &fdset);
    
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
    
    bool result = false;
    if (select(sock + 1, nullptr, &fdset, nullptr, &tv) > 0) {
        int error = 0;
        socklen_t len = sizeof(error);
        getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&error, &len);
        result = (error == 0);
    }
    
#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    
    return result;
}

bool PgHelper::checkDatabase(const PgDbInfo& dbInfo) {
    // 检查网络连接
    if (!ipLive(dbInfo.hostName, dbInfo.hostPort, dbInfo.timeout)) {
        return false;
    }
    
    return true;
}

std::vector<std::string> PgHelper::getTables(void* connection) {
    std::vector<std::string> tables;
    
    if (connection == nullptr) {
        return tables;
    }
    
    PGconn* conn = static_cast<PGconn*>(connection);
    
    const char* sql = "SELECT tablename FROM pg_tables WHERE schemaname = 'public' ORDER BY tablename";
    PGresult* res = PQexec(conn, sql);
    
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int numRows = PQntuples(res);
        for (int i = 0; i < numRows; ++i) {
            tables.push_back(PQgetvalue(res, i, 0));
        }
    }
    
    PQclear(res);
    return tables;
}

std::vector<std::string> PgHelper::getColumns(void* connection, const std::string& tableName) {
    std::vector<std::string> columns;
    
    if (connection == nullptr) {
        return columns;
    }
    
    PGconn* conn = static_cast<PGconn*>(connection);
    
    std::string sql = "SELECT column_name FROM information_schema.columns "
                     "WHERE table_name = '" + escapeSqlString(tableName) + "' "
                     "AND table_schema = 'public' ORDER BY ordinal_position";
    
    PGresult* res = PQexec(conn, sql.c_str());
    
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int numRows = PQntuples(res);
        for (int i = 0; i < numRows; ++i) {
            columns.push_back(PQgetvalue(res, i, 0));
        }
    }
    
    PQclear(res);
    return columns;
}

bool PgHelper::clearTable(void* connection, const std::string& tableName) {
    if (connection == nullptr) {
        return false;
    }
    
    PGconn* conn = static_cast<PGconn*>(connection);
    
    // 使用TRUNCATE清空表并重置自增序列
    std::string sql = "TRUNCATE TABLE " + tableName + " RESTART IDENTITY CASCADE";
    PGresult* res = PQexec(conn, sql.c_str());
    
    bool success = (PQresultStatus(res) == PGRES_COMMAND_OK);
    PQclear(res);
    
    return success;
}

int PgHelper::getMaxID(void* connection, const std::string& tableName, 
                       const std::string& columnName) {
    if (connection == nullptr) {
        return 0;
    }
    
    PGconn* conn = static_cast<PGconn*>(connection);
    
    std::string sql = "SELECT MAX(" + columnName + ") FROM " + tableName;
    PGresult* res = PQexec(conn, sql.c_str());
    
    int maxID = 0;
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        const char* value = PQgetvalue(res, 0, 0);
        if (value && strlen(value) > 0) {
            maxID = std::atoi(value);
        }
    }
    
    PQclear(res);
    return maxID;
}

void PgHelper::getBetweenDate(std::string& sql,
                              const std::string& columnName,
                              const std::string& dateTimeStart,
                              const std::string& dateTimeEnd,
                              bool varchar) {
    if (varchar) {
        // 字段类型是字符串，直接比较
        sql += " AND " + columnName + " >= '" + dateTimeStart + "'";
        sql += " AND " + columnName + " <= '" + dateTimeEnd + "'";
    } else {
        // 字段类型是时间戳，需要转换
        sql += " AND " + columnName + " >= '" + dateTimeStart + "'::timestamp";
        sql += " AND " + columnName + " <= '" + dateTimeEnd + "'::timestamp";
    }
}

std::string PgHelper::escapeSqlString(const std::string& str) {
    std::string result;
    result.reserve(str.length() * 2);
    
    for (char c : str) {
        if (c == '\'') {
            result += "''";  // 单引号转义为两个单引号
        } else if (c == '\\') {
            result += "\\\\"; // 反斜杠转义
        } else {
            result += c;
        }
    }
    
    return result;
}

std::string PgHelper::buildInsertSql(const std::string& tableName,
                                     const std::vector<std::string>& columns) {
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
    
    return oss.str();
}

std::string PgHelper::buildUpdateSql(const std::string& tableName,
                                     const std::vector<std::string>& columns,
                                     const std::string& whereClause) {
    std::ostringstream oss;
    
    oss << "UPDATE " << tableName << " SET ";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << columns[i] << " = $" << (i + 1);
    }
    
    if (!whereClause.empty()) {
        oss << " WHERE " << whereClause;
    }
    
    return oss.str();
}

std::string PgHelper::buildDeleteSql(const std::string& tableName,
                                     const std::string& whereClause) {
    std::ostringstream oss;
    
    oss << "DELETE FROM " << tableName;
    
    if (!whereClause.empty()) {
        oss << " WHERE " << whereClause;
    }
    
    return oss.str();
}

std::string PgHelper::buildSelectSql(const std::string& tableName,
                                     const std::vector<std::string>& columns,
                                     const std::string& whereClause,
                                     const std::string& orderBy,
                                     int limit) {
    std::ostringstream oss;
    
    oss << "SELECT ";
    
    if (columns.empty()) {
        oss << "*";
    } else {
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << columns[i];
        }
    }
    
    oss << " FROM " << tableName;
    
    if (!whereClause.empty()) {
        oss << " WHERE " << whereClause;
    }
    
    if (!orderBy.empty()) {
        oss << " ORDER BY " << orderBy;
    }
    
    if (limit > 0) {
        oss << " LIMIT " << limit;
    }
    
    return oss.str();
}

std::string PgHelper::getServerVersion(void* connection) {
    if (connection == nullptr) {
        return "";
    }
    
    PGconn* conn = static_cast<PGconn*>(connection);
    return PQparameterStatus(conn, "server_version");
}

int PgHelper::getServerVersionNum(void* connection) {
    if (connection == nullptr) {
        return 0;
    }
    
    PGconn* conn = static_cast<PGconn*>(connection);
    return PQserverVersion(conn);
}

std::string PgHelper::boolToString(bool value) {
    return value ? "true" : "false";
}

bool PgHelper::stringToBool(const std::string& str) {
    return str == "t" || str == "true" || str == "1";
}

std::string PgHelper::timestampToString(long long timestamp) {
    std::time_t time = timestamp;
    std::tm* tm = std::localtime(&time);
    
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

long long PgHelper::stringToTimestamp(const std::string& str) {
    std::tm tm = {};
    std::istringstream iss(str);
    iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::mktime(&tm);
}

bool PgHelper::supportsJson(void* connection) {
    if (connection == nullptr) {
        return false;
    }
    
    // PostgreSQL 9.2+ 支持JSON
    return getServerVersionNum(connection) >= 90200;
}

std::string PgHelper::generateUUID() {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<unsigned long long> dis;
    
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    
    // 生成UUID格式: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
    oss << std::setw(8) << (dis(gen) & 0xFFFFFFFF) << "-";
    oss << std::setw(4) << (dis(gen) & 0xFFFF) << "-";
    oss << std::setw(4) << ((dis(gen) & 0x0FFF) | 0x4000) << "-";
    oss << std::setw(4) << ((dis(gen) & 0x3FFF) | 0x8000) << "-";
    oss << std::setw(12) << (dis(gen) & 0xFFFFFFFFFFFF);
    
    return oss.str();
}

std::string PgHelper::getIsolationLevelString(IsolationLevel level) {
    switch (level) {
        case READ_UNCOMMITTED:
            return "READ UNCOMMITTED";
        case READ_COMMITTED:
            return "READ COMMITTED";
        case REPEATABLE_READ:
            return "REPEATABLE READ";
        case SERIALIZABLE:
            return "SERIALIZABLE";
        default:
            return "READ COMMITTED";
    }
}

std::string PgHelper::formatError(const std::string& operation,
                                  const std::string& errorMessage) {
    std::ostringstream oss;
    oss << "操作失败 [" << operation << "]: " << errorMessage;
    return oss.str();
}

std::string PgHelper::formatSql(const std::string& sql,
                               const std::vector<std::string>& params) {
    std::ostringstream oss;
    oss << "SQL: " << sql;
    
    if (!params.empty()) {
        oss << "\n参数: [";
        for (size_t i = 0; i < params.size(); ++i) {
            if (i > 0) oss << ", ";
            oss << params[i];
        }
        oss << "]";
    }
    
    return oss.str();
}

