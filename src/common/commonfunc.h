#ifndef COMMONFUNC_H
#define COMMONFUNC_H

#include <QString>
#include <QVector2D>

#include "datastruct/commonStruct.h"

class CommonFunc {
public:
    static double rad(double d);
    static double GetDistance(double lon1, double lat1, double lon2, double lat2);
    static double GetDistance2(double lat1, double lng1, double lat2, double lng2);
    static double getAngle2(double lat1, double lng1, double lat2, double lng2);
    static double pointToSegmentDistance(const QVector2D& point,
        const QVector2D& segmentStart,
        const QVector2D& segmentEnd);
    static QString GenerateAlarmRuleMsg(AlarmRule rule);
};

#endif
