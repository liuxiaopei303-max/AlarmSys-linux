#include "dialog/alarm/AreaEscalationProtectionResolver.h"

#include <QCoreApplication>
#include <QDebug>

#include <cmath>
#include <cstdlib>

using Resolver = AreaEscalationProtectionResolver;

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

bool closeTo(double actual, double expected, double tolerance)
{
    return std::abs(actual - expected) <= tolerance;
}

Resolver::Request baseRequest()
{
    Resolver::Request request;
    request.databaseCircleAvailable = true;
    request.databaseCenter = QPointF(37.54904, 122.09802);
    request.databaseRadiusMeters = 600.0;
    return request;
}

void resolutionTests()
{
    Resolver::Request request = baseRequest();
    Resolver::Context context = Resolver::resolve(request);
    CHECK("预警告警方案使用绑定保护区圆心",
          context.available
              && context.source == Resolver::Source::DatabaseProtectArea
              && context.center == request.databaseCenter
              && context.sourceName() == QStringLiteral("protect_area"));
    CHECK("保护区半径来自数据库", closeTo(context.radiusMeters, 600.0, 0.001));

    request.databaseCenter = QPointF(37.55, 122.1);
    request.databaseRadiusMeters = 450.0;
    context = Resolver::resolve(request);
    CHECK("其他方案绑定不同保护区时使用自身圆心和半径",
          context.source == Resolver::Source::DatabaseProtectArea
              && context.center == request.databaseCenter
              && closeTo(context.radiusMeters, 450.0, 0.001));

    request = baseRequest();
    request.databaseCenter = QPointF(100.0, 122.09802);
    context = Resolver::resolve(request);
    CHECK("无效保护区圆心不提供保护上下文", !context.available);

    request = baseRequest();
    request.databaseCircleAvailable = false;
    context = Resolver::resolve(request);
    CHECK("缺少圆形保护区时不伪造半径", !context.available);

    request = baseRequest();
    request.databaseRadiusMeters = 0.0;
    context = Resolver::resolve(request);
    CHECK("无效保护区半径不提供保护上下文", !context.available);
}

void geometryTests()
{
    const QPointF protectCenter = baseRequest().databaseCenter;
    const QPointF south(protectCenter.x() - 0.01, protectCenter.y());
    const double bearing = Resolver::bearingDegrees(south, protectCenter);
    CHECK("正对保护区圆心航向的进入角为零",
          closeTo(bearing, 0.0, 0.1)
              && closeTo(Resolver::attackAngleDegrees(0.0, bearing), 0.0, 0.1));
    CHECK("航向角跨零度差值正确",
          closeTo(Resolver::attackAngleDegrees(350.0, 10.0), 20.0, 0.001));

    const double centerDistance = Resolver::distanceMeters(south, protectCenter);
    CHECK("距保护区圆心的距离为有效米数",
          centerDistance > baseRequest().databaseRadiusMeters
              && centerDistance < 1200.0);
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    resolutionTests();
    geometryTests();
    qInfo() << "AreaEscalationProtectionResolver tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
