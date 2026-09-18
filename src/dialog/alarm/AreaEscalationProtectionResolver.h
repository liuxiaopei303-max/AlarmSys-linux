#pragma once

#include <QPointF>
#include <QString>

/**
 * 将当前方案绑定的数据库圆形保护区收敛为一次计算可复用的保护上下文。
 *
 * 调用方不得绕过 Context 分别计算进入角、距离或到达时间，以免同一事件采用不同基准。
 */
class AreaEscalationProtectionResolver
{
public:
    enum class Source {
        Unavailable,
        DatabaseProtectArea
    };

    struct Request {
        bool databaseCircleAvailable = false;
        QPointF databaseCenter; // x=latitude, y=longitude
        double databaseRadiusMeters = 0.0;
    };

    struct Context {
        bool available = false;
        QPointF center; // x=latitude, y=longitude
        double radiusMeters = 0.0;
        Source source = Source::Unavailable;
        QPointF databaseCenter;

        QString sourceName() const;
    };

    static Context resolve(const Request& request);
    static double bearingDegrees(const QPointF& from, const QPointF& to);
    static double attackAngleDegrees(double courseDegrees, double bearingDegrees);
    static double distanceMeters(const QPointF& from, const QPointF& to);
};
