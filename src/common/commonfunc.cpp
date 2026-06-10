#include "commonfunc.h"

#include <QtMath>
#include <cmath>

static const double EARTH_RADIUS = 6378137;
#define EARTH_RADIUS2 6371.393

double CommonFunc::rad(double d)
{
    return d * M_PI / 180.0;
}

double CommonFunc::GetDistance(double lon1, double lat1, double lon2, double lat2)
{
    const double radLat1 = rad(lat1);
    const double radLat2 = rad(lat2);
    const double a = radLat1 - radLat2;
    const double b = rad(lon1) - rad(lon2);
    double s = 2 * qAsin(qSqrt(qPow(qSin(a / 2), 2) + qCos(radLat1) * qCos(radLat2) * qPow(qSin(b / 2), 2)));
    s = s * EARTH_RADIUS;
    return s;
}

double CommonFunc::GetDistance2(double lat1, double lng1, double lat2, double lng2)
{
    const double radLat1 = lat1 * M_PI / 180.0;
    const double radLat2 = lat2 * M_PI / 180.0;
    const double a = radLat1 - radLat2;
    const double b = lng1 * M_PI / 180.0 - lng2 * M_PI / 180.0;
    double dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2))));
    dst = dst * EARTH_RADIUS2;
    dst = round(dst * 10000) / 10000;
    dst *= 1000;
    return dst;
}

double CommonFunc::getAngle2(double lat1, double lng1, double lat2, double lng2)
{
    double dRotateAngle = atan2(fabs(lng1 - lng2), fabs(lat1 - lat2));
    if (lng2 >= lng1) {
        if (lat2 < lat1) {
            dRotateAngle = M_PI - dRotateAngle;
        }
    } else {
        if (lat2 >= lat1) {
            dRotateAngle = 2 * M_PI - dRotateAngle;
        } else {
            dRotateAngle = M_PI + dRotateAngle;
        }
    }
    dRotateAngle = dRotateAngle * 180 / M_PI;
    return dRotateAngle;
}

double CommonFunc::pointToSegmentDistance(const QVector2D& point, const QVector2D& segmentStart, const QVector2D& segmentEnd)
{
    const QVector2D segmentDirection = segmentEnd - segmentStart;
    const QVector2D vecFromStartToPoint = point - segmentStart;
    const qreal denom = QVector2D::dotProduct(segmentDirection, segmentDirection);
    if (qFuzzyIsNull(denom)) {
        return QVector2D(point - segmentStart).length();
    }
    qreal t = QVector2D::dotProduct(vecFromStartToPoint, segmentDirection) / denom;
    if (t < 0.0) {
        return QVector2D(point - segmentStart).length();
    }
    if (t > 1.0) {
        return QVector2D(point - segmentEnd).length();
    }
    const QVector2D projection = segmentStart + t * segmentDirection;
    return QVector2D(point - projection).length();
}

QString CommonFunc::GenerateAlarmRuleMsg(AlarmRule rule)
{
    QString msg;
    QString levelMsg;
    if (rule.alarm_level == 1) {
        levelMsg = QStringLiteral("  告警等级：低；");
    } else if (rule.alarm_level == 2) {
        levelMsg = QStringLiteral("  告警等级：中；");
    } else if (rule.alarm_level == 3) {
        levelMsg = QStringLiteral("  告警等级：高；");
    }
    QString areadisMsg;
    if (rule.area_judge > 0) {
        QString areaName = rule.area_name + QLatin1Char(' ');
        if (rule.area_name.isEmpty()) {
            areaName = QStringLiteral("全海域；");
        }
        if (rule.area_judge == 1) {
            areadisMsg = QStringLiteral(" 停留 ") + areaName;
        } else if (rule.area_judge == 2) {
            areadisMsg = QStringLiteral(" 离开 ") + areaName;
        } else if (rule.area_judge == 3) {
            areadisMsg = QStringLiteral(" 靠近 ") + areaName;
        } else if (rule.area_judge == 4) {
            areadisMsg = QStringLiteral(" 进入 ") + areaName;
        } else if (rule.area_judge == 5) {
            areadisMsg = QStringLiteral(" 避让 ") + areaName;
        }
    }
    msg = levelMsg + areadisMsg;
    return msg;
}
