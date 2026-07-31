/**
 * Linux libpq 数据接口冒烟测试（不替换现有 Qt SQL 路径）。
 *
 * 用法（在 bin/ 目录，或传 db.ini）:
 *   ./pg_iface_test [db.ini路径]
 *
 * 验证：登录 / 读取 / 插入（测试表，结束后 DROP）
 */
#include "db/postgres/PgQtDatabaseManager.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QUuid>

#include <cstdio>

namespace {

int fail(const char* step, const QString& detail = QString())
{
    qCritical().noquote() << "[FAIL]" << step << detail;
    return 1;
}

} // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QDir::setCurrent(QCoreApplication::applicationDirPath());

    const QString dbIni = (argc > 1)
        ? QString::fromLocal8Bit(argv[1])
        : (QCoreApplication::applicationDirPath() + QStringLiteral("/db.ini"));

    qInfo().noquote() << "pg_iface_test start"
                      << "cwd=" << QDir::currentPath()
                      << "db.ini=" << dbIni;

    PgQtDatabaseManager& db = PgQtDatabaseManager::getInstance();

    // 关掉默认 DEBUG 刷屏
    db.rawManager().setDebugCallback([](const std::string&) {});
    db.rawManager().setErrorCallback([](const std::string& msg) {
        qWarning().noquote() << "[PgError]" << QString::fromStdString(msg);
    });

    // 1) 登录
    if (!db.initializeFromDbIni(dbIni, 3)) {
        return fail("login/initialize", db.lastError());
    }
    const QVariant ping = db.executeScalar(QStringLiteral("SELECT 1"));
    if (ping.toString() != QLatin1String("1")) {
        db.cleanup();
        return fail("login SELECT 1", QStringLiteral("got='%1'").arg(ping.toString()));
    }
    qInfo() << "[OK] login";

    // 2) 读取（业务表）
    const auto cog = db.executeQuery(
        QStringLiteral(
            "SELECT unique_id, COALESCE(cam_target_type,'') AS cam, "
            "COALESCE(llm_target_type,'') AS llm "
            "FROM cognitive_results_comprehensive "
            "ORDER BY observed_at DESC NULLS LAST LIMIT 3"));
    if (cog.isEmpty()) {
        // 表空不一定失败，再验证 COUNT 能读
        const QVariant cnt = db.executeScalar(
            QStringLiteral("SELECT COUNT(*) FROM cognitive_results_comprehensive"));
        if (!cnt.isValid() || cnt.toString().isEmpty()) {
            db.cleanup();
            return fail("read cognitive_results_comprehensive", db.lastError());
        }
        qInfo().noquote() << "[OK] read cognitive COUNT=" << cnt.toString();
    } else {
        qInfo().noquote() << "[OK] read cognitive sample rows=" << cog.size()
                          << "first unique_id=" << cog.first().value(QStringLiteral("unique_id")).toString()
                          << "cam=" << cog.first().value(QStringLiteral("cam")).toString()
                          << "llm=" << cog.first().value(QStringLiteral("llm")).toString();
    }

    // 占位符路径（? -> $1）
    const auto byId = db.executeQuery(
        QStringLiteral(
            "SELECT unique_id FROM cognitive_results_comprehensive WHERE unique_id = ? LIMIT 1"),
        {QVariant::fromValue(qint64(-1))});
    qInfo() << "[OK] read with ? placeholder" << "rows" << byId.size();

    // 3) 插入（临时测试表，验证完 DROP）
    const QString table = QStringLiteral("alarmsys_pg_iface_smoke_%1")
        .arg(QDateTime::currentMSecsSinceEpoch());

    if (!db.executeNonQuery(
            QStringLiteral("CREATE TABLE %1 ("
                           "id BIGSERIAL PRIMARY KEY, "
                           "note TEXT NOT NULL, "
                           "created_at TIMESTAMPTZ NOT NULL DEFAULT NOW()"
                           ")").arg(table))) {
        db.cleanup();
        return fail("create smoke table", db.lastError());
    }

    const QString note = QStringLiteral("pg_iface_test_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    if (!db.executeNonQuery(
            QStringLiteral("INSERT INTO %1 (note) VALUES (?)").arg(table),
            {note})) {
        db.executeNonQuery(QStringLiteral("DROP TABLE IF EXISTS %1").arg(table));
        db.cleanup();
        return fail("insert", db.lastError());
    }

    const auto inserted = db.executeQuery(
        QStringLiteral("SELECT id, note FROM %1 WHERE note = ?").arg(table),
        {note});
    if (inserted.size() != 1 || inserted.first().value(QStringLiteral("note")).toString() != note) {
        db.executeNonQuery(QStringLiteral("DROP TABLE IF EXISTS %1").arg(table));
        db.cleanup();
        return fail("insert verify", QStringLiteral("rows=%1").arg(inserted.size()));
    }
    qInfo().noquote() << "[OK] insert id=" << inserted.first().value(QStringLiteral("id")).toString()
                      << "note=" << note;

    if (!db.executeNonQuery(QStringLiteral("DROP TABLE IF EXISTS %1").arg(table))) {
        db.cleanup();
        return fail("drop smoke table", db.lastError());
    }
    qInfo() << "[OK] cleanup smoke table";

    db.cleanup();
    qInfo() << "[PASS] login/read/insert all ok (libpq path)";
    return 0;
}
