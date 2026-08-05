#pragma once

#include <QPolygonF>
#include <QString>

/** 数据库 area_points 的解析结果；调用方不再自行猜测首尾点约定。 */
struct AlarmAreaPointList
{
    QPolygonF points;
    int declaredPointCount = 0;
    bool valid = false;
    QString error;
};

/** 解析 `点数,纬度,经度,...`，供区域成员判断统一复用。 */
AlarmAreaPointList parseAlarmAreaPointList(const QString& areaPoints);
