#include "dialog/alarm/AlarmAreaGeometryParser.h"

#include <QStringList>

AlarmAreaPointList parseAlarmAreaPointList(const QString& areaPoints)
{
    AlarmAreaPointList result;
    const QStringList parts = areaPoints.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (parts.isEmpty()) {
        result.error = QStringLiteral("area_points为空");
        return result;
    }

    bool okCount = false;
    result.declaredPointCount = parts.at(0).trimmed().toInt(&okCount);
    if (!okCount || result.declaredPointCount < 2) {
        result.error = QStringLiteral("点数无效");
        return result;
    }

    const int required = 1 + result.declaredPointCount * 2;
    if (parts.size() < required) {
        result.error = QStringLiteral("坐标数量不足");
        return result;
    }

    for (int index = 0; index < result.declaredPointCount; ++index) {
        bool okLatitude = false;
        bool okLongitude = false;
        const double latitude = parts.at(index * 2 + 1).trimmed().toDouble(&okLatitude);
        const double longitude = parts.at(index * 2 + 2).trimmed().toDouble(&okLongitude);
        if (!okLatitude || !okLongitude) {
            result.error = QStringLiteral("第%1个坐标无效").arg(index + 1);
            result.points.clear();
            return result;
        }
        result.points.append(QPointF(latitude, longitude));
    }
    result.valid = result.points.size() >= 2;
    return result;
}
