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
    request.config.enabled = 1;
    request.config.protectionReferenceEnabled = 1;
    request.config.protectionReferenceSchemeId = QStringLiteral("SCHEME_CB9C875E");
    request.config.protectionReferenceDomains = QStringLiteral("SURFACE");
    request.config.protectionReferenceLatitude = 37.5492;
    request.config.protectionReferenceLongitude = 122.1016;
    request.activeSchemeId = QStringLiteral("SCHEME_CB9C875E");
    request.targetDomain = QStringLiteral("SURFACE");
    request.databaseCircleAvailable = true;
    request.databaseCenter = QPointF(37.54904, 122.09802);
    request.databaseRadiusMeters = 600.0;
    return request;
}

void resolutionTests()
{
    Resolver::Request request = baseRequest();
    Resolver::Context context = Resolver::resolve(request);
    CHECK("演示方案 SURFACE 使用固定参考点",
          context.available
              && context.source == Resolver::Source::ConfiguredReference
              && context.center == QPointF(37.5492, 122.1016));
    CHECK("固定参考点仍保留数据库保护区半径", closeTo(context.radiusMeters, 600.0, 0.001));

    request.targetDomain = QStringLiteral("AIR");
    context = Resolver::resolve(request);
    CHECK("演示方案 AIR 保持数据库圆心",
          context.source == Resolver::Source::DatabaseProtectArea
              && context.center == request.databaseCenter);

    request = baseRequest();
    request.activeSchemeId = QStringLiteral("SCHEME_A753403E");
    context = Resolver::resolve(request);
    CHECK("日常方案 SURFACE 保持数据库圆心",
          context.source == Resolver::Source::DatabaseProtectArea
              && context.center == request.databaseCenter);

    request = baseRequest();
    request.config.protectionReferenceEnabled = 0;
    context = Resolver::resolve(request);
    CHECK("关闭覆盖后保持数据库圆心",
          context.source == Resolver::Source::DatabaseProtectArea);

    request = baseRequest();
    request.config.protectionReferenceLatitude = 100.0;
    context = Resolver::resolve(request);
    CHECK("无效固定坐标安全回退数据库圆心",
          context.source == Resolver::Source::DatabaseProtectArea);

    request = baseRequest();
    request.databaseCircleAvailable = false;
    context = Resolver::resolve(request);
    CHECK("缺少数据库圆形保护区时不伪造半径", !context.available);
}

void geometryTests()
{
    const QPointF reference(37.5492, 122.1016);
    const QPointF south(37.5392, 122.1016);
    const double bearing = Resolver::bearingDegrees(south, reference);
    CHECK("正对固定点航向的进入角为零",
          closeTo(bearing, 0.0, 0.1)
              && closeTo(Resolver::attackAngleDegrees(0.0, bearing), 0.0, 0.1));
    CHECK("航向角跨零度差值正确",
          closeTo(Resolver::attackAngleDegrees(350.0, 10.0), 20.0, 0.001));

    const QPointF databaseCenter(37.54904, 122.09802);
    const QPointF target(37.5595, 122.1004);
    const double fixedDistance = Resolver::distanceMeters(target, reference);
    const double databaseDistance = Resolver::distanceMeters(target, databaseCenter);
    CHECK("固定圆心会改变距离计算且结果为有效米数",
          fixedDistance > 0.0 && databaseDistance > 0.0
              && std::abs(fixedDistance - databaseDistance) > 1.0);
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
