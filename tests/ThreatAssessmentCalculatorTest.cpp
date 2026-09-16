#include "dialog/alarm/ThreatAssessmentCalculator.h"

#include <cmath>
#include <iostream>

namespace {

bool near(double actual, double expected, double epsilon = 1e-6)
{
    if (std::abs(actual - expected) <= epsilon)
        return true;
    std::cerr << "expected " << expected << ", got " << actual << '\n';
    return false;
}

} // namespace

int main()
{
    SPxPacketTrackExtended track{};
    track.norm.min.speedMps = 17.5F;
    track.norm.min.rangeMetres = 25.0F;
    track.norm.min.courseDegrees = 90.0F;

    QString targetType = QStringLiteral("uav");

    const ThreatAssessmentParams params;
    ThreatAssessmentContext context;
    context.directionAngleDeg = track.norm.min.courseDegrees;

    const ThreatAssessmentCalculation calculation =
        calculateTargetThreatAssessment(track, targetType, params, context);
    const ThreatAssessmentResult& result = calculation.assessment;

    if (calculation.targetType != QStringLiteral("uav")
        || calculation.usesAttackAngle
        || calculation.distanceBasis != ThreatDistanceBasis::TrackRange
        || !near(calculation.distanceMeters, 25.0)
        || !near(result.targetTypeScore, 10.0)
        || !near(result.speedScore, 5.0)
        || !near(result.capabilityScore, 5.0)
        || !near(result.distanceScore, 5.0)
        || !near(result.weightedTypeScore, 42.0)
        || !near(result.weightedSpeedScore, 8.0)
        || !near(result.weightedAngleScore, 9.0)
        || !near(result.weightedDistanceScore, 12.0)
        || !near(result.totalThreatLevel, 71.0)
        || result.threatDescription != QStringLiteral("高威胁")) {
        return 1;
    }

    targetType = QStringLiteral("buoy");
    const ThreatAssessmentCalculation buoy =
        calculateTargetThreatAssessment(track, targetType, params, context);
    if (!near(buoy.assessment.targetTypeScore, 0.0)
        || !near(buoy.assessment.capabilityScore, 0.0)) {
        return 1;
    }

    return 0;
}
