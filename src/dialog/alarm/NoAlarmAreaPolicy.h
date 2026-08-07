#pragma once

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariant>

/**
 * 精确免告警区策略。
 *
 * 区域键统一为 `group_id/area_id`，配置兼容 `4-6`、`4/6`、`4_6`。
 * 精确区域只用于对海新事件抑制；已有告警和对空目标不受影响。
 */
namespace NoAlarmAreaPolicy {

QString areaKey(int groupId, int areaId);

QSet<QString> parseAreaKeys(const QVariant& configuredValue,
                            QStringList* rejectedValues = nullptr);

QSet<QString> parseSchemeIds(const QVariant& configuredValue);

bool isEnabledForScheme(const QSet<QString>& configuredSchemeIds,
                        const QString& activeSchemeId);

bool isConfigured(const QSet<QString>& configuredKeys, int groupId, int areaId);

bool shouldSuppressNewEvent(bool isSurfaceTarget,
                            bool insideConfiguredArea,
                            bool hasPublishedAlarm);

} // namespace NoAlarmAreaPolicy
