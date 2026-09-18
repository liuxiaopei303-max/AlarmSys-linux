#include "dialog/alarm/AreaEscalationProtectionResolver.h"

#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {

constexpr double kEarthRadiusMeters = 6378137.0;
constexpr double kPi = 3.14159265358979323846;

double radians(double degrees)
{
    return degrees * kPi / 180.0;
}

double normalizedDegrees(double degrees)
{
    double value = std::fmod(degrees, 360.0);
    if (value < 0.0)
        value += 360.0;
    return value;
}

bool validCoordinates(const QPointF& point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y())
        && point.x() >= -90.0 && point.x() <= 90.0
        && point.y() >= -180.0 && point.y() <= 180.0;
}

} // namespace

QString AreaEscalationProtectionResolver::Context::sourceName() const
{
    switch (source) {
    case Source::DatabaseProtectArea:
        return QStringLiteral("protect_area");
    default:
        return QStringLiteral("unavailable");
    }
}

AreaEscalationProtectionResolver::Context
AreaEscalationProtectionResolver::resolve(const Request& request)
{
    Context context;
    if (!request.databaseCircleAvailable
        || !validCoordinates(request.databaseCenter)
        || !std::isfinite(request.databaseRadiusMeters)
        || request.databaseRadiusMeters <= 0.0) {
        return context;
    }

    context.available = true;
    context.center = request.databaseCenter;
    context.databaseCenter = request.databaseCenter;
    context.radiusMeters = request.databaseRadiusMeters;
    context.source = Source::DatabaseProtectArea;
    return context;
}

double AreaEscalationProtectionResolver::bearingDegrees(
    const QPointF& from,
    const QPointF& to)
{
    const double lat1 = radians(from.x());
    const double lon1 = radians(from.y());
    const double lat2 = radians(to.x());
    const double lon2 = radians(to.y());
    const double deltaLon = lon2 - lon1;
    const double y = std::sin(deltaLon) * std::cos(lat2);
    const double x = std::cos(lat1) * std::sin(lat2)
        - std::sin(lat1) * std::cos(lat2) * std::cos(deltaLon);
    return normalizedDegrees(std::atan2(y, x) * 180.0 / kPi);
}

double AreaEscalationProtectionResolver::attackAngleDegrees(
    double courseDegrees,
    double bearing)
{
    double difference = std::abs(normalizedDegrees(courseDegrees) - normalizedDegrees(bearing));
    if (difference > 180.0)
        difference = 360.0 - difference;
    return difference;
}

double AreaEscalationProtectionResolver::distanceMeters(
    const QPointF& from,
    const QPointF& to)
{
    const double lat1 = radians(from.x());
    const double lat2 = radians(to.x());
    const double deltaLat = lat1 - lat2;
    const double deltaLon = radians(from.y()) - radians(to.y());
    const double haversine = std::pow(std::sin(deltaLat / 2.0), 2.0)
        + std::cos(lat1) * std::cos(lat2)
            * std::pow(std::sin(deltaLon / 2.0), 2.0);
    const double bounded = std::min(1.0, std::max(0.0, haversine));
    return 2.0 * std::asin(std::sqrt(bounded)) * kEarthRadiusMeters;
}
