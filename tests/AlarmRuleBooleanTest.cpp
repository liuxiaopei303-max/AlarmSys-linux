#include "db/SqlBooleanValue.h"

#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>

#include <iostream>

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"));
    db.setDatabaseName(QStringLiteral(":memory:"));
    if (!db.open()) return 2;

    QSqlQuery query(db);
    if (!query.exec(QStringLiteral("CREATE TABLE alarm_setting (require_optic_photo TEXT, ignore_threat_score TEXT)"))
        || !query.exec(QStringLiteral("INSERT INTO alarm_setting VALUES ('f', 'f'), ('t', 't')"))
        || !query.exec(QStringLiteral("SELECT * FROM alarm_setting"))) return 2;

    if (!query.next()) return 2;
    const bool surfacePhoto = readSqlBooleanValue(query.value("require_optic_photo"));
    const bool surfaceSkipScore = readSqlBooleanValue(query.value("ignore_threat_score"));
    if (!query.next()) return 2;
    const bool airPhoto = readSqlBooleanValue(query.value("require_optic_photo"));
    const bool airSkipScore = readSqlBooleanValue(query.value("ignore_threat_score"));
    if (surfacePhoto || surfaceSkipScore || !airPhoto || !airSkipScore
        || !readSqlBooleanValue(QVariant(true))
        || readSqlBooleanValue(QVariant(false))
        || readSqlBooleanValue(QVariant())
        || !readSqlBooleanValue(QVariant(QStringLiteral("T")))
        || readSqlBooleanValue(QVariant(QStringLiteral("F")))
        || readSqlBooleanValue(QVariant(QStringLiteral("false")))
        || readSqlBooleanValue(QVariant(QStringLiteral("0")))) {
        std::cerr << "FAIL: libpq f/t boolean fields are not read as configured\n";
        return 1;
    }
    std::cout << "PASS: demo surface/air boolean rule fields\n";
    return 0;
}
