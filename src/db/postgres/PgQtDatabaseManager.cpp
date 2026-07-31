#include "PgQtDatabaseManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QSettings>
#include <QThreadStorage>
#include <QUuid>

namespace {

struct PgQtTxnSlot {
    PgDatabaseManager::Transaction txn;
    bool active = false;
};

QThreadStorage<PgQtTxnSlot*>& pgQtTxnStorage()
{
    static QThreadStorage<PgQtTxnSlot*> storage;
    return storage;
}

PgQtTxnSlot& pgQtTxnSlot()
{
    auto& storage = pgQtTxnStorage();
    if (!storage.hasLocalData()) {
        storage.setLocalData(new PgQtTxnSlot());
    }
    return *storage.localData();
}

} // namespace

PgQtDatabaseManager& PgQtDatabaseManager::getInstance()
{
    static PgQtDatabaseManager instance;
    return instance;
}

PgQtDatabaseManager::PgQtDatabaseManager()
    : m_pg(PgDatabaseManager::getInstance())
{
}

bool PgQtDatabaseManager::initializeFromDbIni(const QString& dbIniPath, int maxConnections)
{
    QString path = dbIniPath;
    if (path.isEmpty()) {
        path = QCoreApplication::applicationDirPath() + QStringLiteral("/db.ini");
        if (!QFileInfo::exists(path)) {
            path = QStringLiteral("db.ini");
        }
    }

    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("Database"));

    PgDbInfo info;
    info.connMode = 0;
    info.connName = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    info.dbName = settings.value(QStringLiteral("dbName"), QStringLiteral("watchsystem")).toString().toStdString();
    info.hostName = settings.value(QStringLiteral("hostName"), QStringLiteral("127.0.0.1")).toString().toStdString();
    info.hostPort = settings.value(QStringLiteral("hostPort"), 5432).toInt();
    info.userName = settings.value(QStringLiteral("userName"), QStringLiteral("postgres")).toString().toStdString();
    info.userPwd = settings.value(QStringLiteral("userPwd"), QStringLiteral("123456")).toString().toStdString();
    info.applicationName = "AlarmSys-linux-pgqt";
    info.sslMode = "disable";
    settings.endGroup();

    return initialize(info, maxConnections);
}

bool PgQtDatabaseManager::initialize(const PgDbInfo& dbInfo, int maxConnections)
{
    m_lastError.clear();
    m_initialized = m_pg.initialize(PgDbType_PostgreSQL, dbInfo, maxConnections);
    if (!m_initialized) {
        m_lastError = QStringLiteral("PgQtDatabaseManager initialize failed");
    }
    return m_initialized;
}

void PgQtDatabaseManager::cleanup()
{
    m_pg.cleanup();
    m_initialized = false;
}

QString PgQtDatabaseManager::convertQtPlaceholders(const QString& sql)
{
    QString out;
    out.reserve(sql.size() + 16);
    int idx = 0;
    bool inSingle = false;
    for (int i = 0; i < sql.size(); ++i) {
        const QChar c = sql.at(i);
        if (c == QLatin1Char('\'')) {
            // 简单单引号切换（不处理 '' 转义的复杂情况，足以覆盖现有业务 SQL）
            if (inSingle && i + 1 < sql.size() && sql.at(i + 1) == QLatin1Char('\'')) {
                out.append(c);
                out.append(sql.at(++i));
                continue;
            }
            inSingle = !inSingle;
            out.append(c);
            continue;
        }
        if (!inSingle && c == QLatin1Char('?')) {
            out.append(QLatin1Char('$'));
            out.append(QString::number(++idx));
            continue;
        }
        out.append(c);
    }
    return out;
}

std::vector<PgParam> PgQtDatabaseManager::toPgParams(const QVariantList& params)
{
    std::vector<PgParam> out;
    out.reserve(static_cast<size_t>(params.size()));
    for (const QVariant& v : params) {
        if (!v.isValid() || v.isNull()) {
            out.push_back(PgParam::Null());
            continue;
        }
        switch (v.type()) {
        case QVariant::Bool:
            out.push_back(PgParam::Bool(v.toBool()));
            break;
        case QVariant::Int:
        case QVariant::UInt:
            out.push_back(PgParam::Int(v.toInt()));
            break;
        case QVariant::LongLong:
        case QVariant::ULongLong:
            out.push_back(PgParam::Int64(v.toLongLong()));
            break;
        case QVariant::Double:
            out.push_back(PgParam::Double(v.toDouble()));
            break;
        case QVariant::DateTime:
            out.push_back(PgParam::String(
                v.toDateTime().toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"))
                    .toStdString()));
            break;
        case QVariant::ByteArray: {
            const QByteArray ba = v.toByteArray();
            out.push_back(PgParam::Bytea(
                std::vector<unsigned char>(ba.begin(), ba.end())));
            break;
        }
        default:
            out.push_back(PgParam::String(v.toString().toStdString()));
            break;
        }
    }
    return out;
}

QVector<QVariantMap> PgQtDatabaseManager::toQtRows(const std::vector<PgResultRow>& rows)
{
    QVector<QVariantMap> out;
    out.reserve(static_cast<int>(rows.size()));
    for (const PgResultRow& row : rows) {
        QVariantMap m;
        for (size_t i = 0; i < row.columns.size(); ++i) {
            const QString key = QString::fromStdString(row.columns[i]);
            const QString val = (i < row.valuesByIndex.size())
                ? QString::fromStdString(row.valuesByIndex[i])
                : QString();
            m.insert(QStringLiteral("_c%1").arg(static_cast<int>(i)), val);
            // 同名列以最后一次写入为准；按序访问用 _cN
            m.insert(key, val);
        }
        out.push_back(m);
    }
    return out;
}

QVector<QVariantMap> PgQtDatabaseManager::executeQuery(const QString& query,
                                                       const QVariantList& params,
                                                       int timeoutMs)
{
    m_lastError.clear();
    if (!m_initialized) {
        m_lastError = QStringLiteral("not initialized");
        return {};
    }
    const QString sql = convertQtPlaceholders(query);
    const auto pgParams = toPgParams(params);
    PgQtTxnSlot& slot = pgQtTxnSlot();
    std::vector<PgResultRow> rows;
    if (slot.active) {
        Q_UNUSED(timeoutMs);
        rows = m_pg.executeQueryInTransaction(slot.txn, sql.toStdString(), pgParams);
    } else {
        rows = m_pg.executeQuery(sql.toStdString(), pgParams, timeoutMs);
    }
    return toQtRows(rows);
}

bool PgQtDatabaseManager::executeNonQuery(const QString& query,
                                          const QVariantList& params,
                                          int timeoutMs)
{
    m_lastError.clear();
    if (!m_initialized) {
        m_lastError = QStringLiteral("not initialized");
        return false;
    }
    const QString sql = convertQtPlaceholders(query);
    const auto pgParams = toPgParams(params);
    PgQtTxnSlot& slot = pgQtTxnSlot();
    bool ok = false;
    if (slot.active) {
        Q_UNUSED(timeoutMs);
        ok = m_pg.executeNonQueryInTransaction(slot.txn, sql.toStdString(), pgParams);
    } else {
        ok = m_pg.executeNonQuery(sql.toStdString(), pgParams, timeoutMs);
    }
    if (!ok) {
        m_lastError = QStringLiteral("executeNonQuery failed");
    }
    return ok;
}

QVariant PgQtDatabaseManager::executeScalar(const QString& query,
                                            const QVariantList& params,
                                            int timeoutMs)
{
    m_lastError.clear();
    if (!m_initialized) {
        m_lastError = QStringLiteral("not initialized");
        return {};
    }
    const QString sql = convertQtPlaceholders(query);
    const auto pgParams = toPgParams(params);
    PgQtTxnSlot& slot = pgQtTxnSlot();
    if (slot.active) {
        Q_UNUSED(timeoutMs);
        const auto rows = m_pg.executeQueryInTransaction(slot.txn, sql.toStdString(), pgParams);
        if (rows.empty() || rows.front().valuesByIndex.empty()) {
            return {};
        }
        return QString::fromStdString(rows.front().valuesByIndex.front());
    }
    const std::string scalar = m_pg.executeScalar(sql.toStdString(), pgParams, timeoutMs);
    return QString::fromStdString(scalar);
}

bool PgQtDatabaseManager::beginTransaction()
{
    m_lastError.clear();
    if (!m_initialized) {
        m_lastError = QStringLiteral("not initialized");
        return false;
    }
    PgQtTxnSlot& slot = pgQtTxnSlot();
    if (slot.active) {
        m_lastError = QStringLiteral("transaction already active on this thread");
        return false;
    }
    slot.txn = m_pg.beginTransaction();
    slot.active = slot.txn.active;
    if (!slot.active) {
        m_lastError = QStringLiteral("beginTransaction failed");
    }
    return slot.active;
}

bool PgQtDatabaseManager::commitTransaction()
{
    m_lastError.clear();
    PgQtTxnSlot& slot = pgQtTxnSlot();
    if (!slot.active) {
        m_lastError = QStringLiteral("no active transaction");
        return false;
    }
    const bool ok = m_pg.commitTransaction(slot.txn);
    slot.active = false;
    if (!ok) {
        m_lastError = QStringLiteral("commitTransaction failed");
    }
    return ok;
}

bool PgQtDatabaseManager::rollbackTransaction()
{
    m_lastError.clear();
    PgQtTxnSlot& slot = pgQtTxnSlot();
    if (!slot.active) {
        m_lastError = QStringLiteral("no active transaction");
        return false;
    }
    const bool ok = m_pg.rollbackTransaction(slot.txn);
    slot.active = false;
    if (!ok) {
        m_lastError = QStringLiteral("rollbackTransaction failed");
    }
    return ok;
}

bool PgQtDatabaseManager::hasActiveTransaction() const
{
    auto& storage = pgQtTxnStorage();
    if (!storage.hasLocalData()) {
        return false;
    }
    return storage.localData()->active;
}
