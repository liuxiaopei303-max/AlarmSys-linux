#include "dialog/alarm/NoAlarmAreaPolicy.h"

#include <QRegularExpression>

namespace NoAlarmAreaPolicy {

namespace {

QStringList splitConfiguredValues(const QVariant& configuredValue)
{
    QStringList configuredParts;
    if (configuredValue.canConvert<QStringList>())
        configuredParts = configuredValue.toStringList();
    if (configuredParts.isEmpty()) {
        const QString scalar = configuredValue.toString().trimmed();
        if (!scalar.isEmpty())
            configuredParts.append(scalar);
    }

    QStringList tokens;
    for (const QString& part : configuredParts) {
        const QStringList split = part.split(
            QRegularExpression(QStringLiteral("[,;]")), QString::SkipEmptyParts);
        for (const QString& token : split) {
            const QString trimmed = token.trimmed();
            if (!trimmed.isEmpty())
                tokens.append(trimmed);
        }
    }
    return tokens;
}

} // namespace

QString areaKey(int groupId, int areaId)
{
    return QStringLiteral("%1/%2").arg(groupId).arg(areaId);
}

QSet<QString> parseAreaKeys(const QVariant& configuredValue, QStringList* rejectedValues)
{
    if (rejectedValues != nullptr)
        rejectedValues->clear();

    const QStringList tokens = splitConfiguredValues(configuredValue);

    static const QRegularExpression keyPattern(
        QStringLiteral("^([0-9]+)\\s*[-/_:]\\s*([0-9]+)$"));
    QSet<QString> keys;
    for (const QString& token : tokens) {
        const QRegularExpressionMatch match = keyPattern.match(token);
        bool groupOk = false;
        bool areaOk = false;
        const int groupId = match.hasMatch() ? match.captured(1).toInt(&groupOk) : -1;
        const int areaId = match.hasMatch() ? match.captured(2).toInt(&areaOk) : -1;
        if (!match.hasMatch() || !groupOk || !areaOk) {
            if (rejectedValues != nullptr)
                rejectedValues->append(token);
            continue;
        }
        keys.insert(areaKey(groupId, areaId));
    }
    return keys;
}

QSet<QString> parseSchemeIds(const QVariant& configuredValue)
{
    QSet<QString> schemeIds;
    for (const QString& token : splitConfiguredValues(configuredValue))
        schemeIds.insert(token);
    return schemeIds;
}

bool isEnabledForScheme(const QSet<QString>& configuredSchemeIds,
                        const QString& activeSchemeId)
{
    return configuredSchemeIds.isEmpty()
        || configuredSchemeIds.contains(activeSchemeId.trimmed());
}

bool isConfigured(const QSet<QString>& configuredKeys, int groupId, int areaId)
{
    return groupId >= 0 && areaId >= 0
        && configuredKeys.contains(areaKey(groupId, areaId));
}

bool shouldSuppressNewEvent(bool isSurfaceTarget,
                            bool insideConfiguredArea,
                            bool hasPublishedAlarm)
{
    return isSurfaceTarget && insideConfiguredArea && !hasPublishedAlarm;
}

} // namespace NoAlarmAreaPolicy
