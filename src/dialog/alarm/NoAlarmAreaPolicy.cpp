#include "dialog/alarm/NoAlarmAreaPolicy.h"

namespace NoAlarmAreaPolicy {

QString areaKey(int groupId, int areaId)
{
    return QStringLiteral("%1/%2").arg(groupId).arg(areaId);
}

int domainMaskForTrackType(int trackType)
{
    if (trackType == 0) return SurfaceDomain;
    if (trackType == 3) return AirDomain;
    return NoDomain;
}

bool appliesTo(const AreaDomainMap& configuredAreas,
               int groupId,
               int areaId,
               int trackType)
{
    if (groupId < 0 || areaId < 0) return false;
    const int targetDomain = domainMaskForTrackType(trackType);
    if (targetDomain == NoDomain) return false;
    return (configuredAreas.value(areaKey(groupId, areaId), NoDomain)
            & targetDomain) != 0;
}

bool shouldSuppressNewEvent(bool insideConfiguredArea,
                            bool hasPublishedAlarm)
{
    return insideConfiguredArea && !hasPublishedAlarm;
}

} // namespace NoAlarmAreaPolicy
