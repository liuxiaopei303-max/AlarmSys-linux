#pragma once

#include "datastruct/commonStruct.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QStringList>

#include <cmath>

// Demo-only upgrade seam: OR between linked rules, AND between enabled criteria
// in each rule. Unknown/invalid conditions fail closed, never create a HIGH event.
inline QString matchingDemoRecognitionRule(
    const QStringList& linkedIds,
    const QMap<QString, QList<AlarmIdentificationRuleSub>>& rules,
    int recognitionType, double trackAgeSeconds, bool opticSeen)
{
    for (const QString& id : linkedIds) {
        const auto criteria = rules.value(id);
        bool matched = false;
        bool valid = true;
        for (const auto& row : criteria) {
            if (!row.enabled) continue;
            if (row.rule_type != recognitionType) { valid = false; break; }
            const auto json = QJsonDocument::fromJson(row.detection_rules_json.toUtf8());
            if (!json.isObject()) { valid = false; break; }
            const QJsonObject condition = json.object();
            const bool hasPhoto = condition.value(QStringLiteral("optic")).toBool(false);
            const bool hasAge = condition.contains(QStringLiteral("track_duration"));
            const QJsonValue ageValue = condition.value(QStringLiteral("track_duration"));
            if ((!hasPhoto && !hasAge)
                || (hasAge && (!ageValue.isDouble() || ageValue.toDouble() < 0.0
                               || !std::isfinite(ageValue.toDouble())))) {
                valid = false;
                break;
            }
            matched = true;
            if ((hasPhoto && !opticSeen)
                || (hasAge && !(trackAgeSeconds > ageValue.toDouble()))) {
                valid = false;
                break;
            }
        }
        if (matched && valid) return id;
    }
    return {};
}
