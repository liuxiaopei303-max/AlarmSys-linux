#pragma once

#include <QVariant>

// libpq 查询经 SQLite TEXT 中转后，PostgreSQL boolean 的 f/t 是字符串。
inline bool readSqlBooleanValue(const QVariant& value)
{
    const QString text = value.toString().trimmed().toLower();
    return text == QStringLiteral("t")
        || text == QStringLiteral("true")
        || text == QStringLiteral("1");
}
