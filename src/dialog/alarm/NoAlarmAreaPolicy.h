#pragma once

#include <QMap>
#include <QString>

/**
 * 精确免告警区策略。
 *
 * 区域键统一为 `group_id/area_id`，值是由区域威胁规则 track_type 生成的目标域掩码。
 * 空规则由数据库适配器映射为 AllDomains；已有发布告警继续处理。
 */
namespace NoAlarmAreaPolicy {

enum DomainMask {
    NoDomain = 0,
    SurfaceDomain = 1,
    AirDomain = 2,
    AllDomains = SurfaceDomain | AirDomain
};

using AreaDomainMap = QMap<QString, int>;

QString areaKey(int groupId, int areaId);

int domainMaskForTrackType(int trackType);

bool appliesTo(const AreaDomainMap& configuredAreas,
               int groupId,
               int areaId,
               int trackType);

bool shouldSuppressNewEvent(bool insideConfiguredArea,
                            bool hasPublishedAlarm);

} // namespace NoAlarmAreaPolicy
