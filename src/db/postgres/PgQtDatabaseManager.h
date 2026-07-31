#pragma once

/**
 * Qt 风格封装：接口形态对齐现有 DatabaseManager
 * （executeQuery / executeNonQuery / executeScalar），底层走 libpq。
 * 当前仅供独立测试与后续切换，不替换 DataAccessLayer / DatabaseManager。
 */

#include "PgDatabaseManager.h"

#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

class PgQtDatabaseManager
{
public:
    static PgQtDatabaseManager& getInstance();

    /** 从 bin/db.ini（或显式参数）初始化，返回是否登录成功 */
    bool initializeFromDbIni(const QString& dbIniPath = QString(), int maxConnections = 5);
    bool initialize(const PgDbInfo& dbInfo, int maxConnections = 5);
    void cleanup();
    bool isInitialized() const { return m_initialized; }

    /** 与 Qt DatabaseManager 一致：SQL 用 `?` 占位，内部转为 `$1,$2,...` */
    QVector<QVariantMap> executeQuery(const QString& query,
                                      const QVariantList& params = QVariantList(),
                                      int timeoutMs = 0);
    bool executeNonQuery(const QString& query,
                         const QVariantList& params = QVariantList(),
                         int timeoutMs = 0);
    QVariant executeScalar(const QString& query,
                           const QVariantList& params = QVariantList(),
                           int timeoutMs = 0);

    /** 线程本地事务（与 DatabaseManager::beginTransaction 对齐） */
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();
    bool hasActiveTransaction() const;

    QString lastError() const { return m_lastError; }

    PgDatabaseManager& rawManager() { return m_pg; }

private:
    PgQtDatabaseManager();
    ~PgQtDatabaseManager() = default;
    PgQtDatabaseManager(const PgQtDatabaseManager&) = delete;
    PgQtDatabaseManager& operator=(const PgQtDatabaseManager&) = delete;

    static QString convertQtPlaceholders(const QString& sql);
    static std::vector<PgParam> toPgParams(const QVariantList& params);
    static QVector<QVariantMap> toQtRows(const std::vector<PgResultRow>& rows);

    PgDatabaseManager& m_pg;
    bool m_initialized = false;
    QString m_lastError;
};
