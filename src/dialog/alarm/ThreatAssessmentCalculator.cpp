#include "dialog/alarm/ThreatAssessmentCalculator.h"

#include "dialog/alarm/AreaEscalationProtectionResolver.h"
#include "grpc_track/new_track_struct_grpc_convert.h"

#include <algorithm>
#include <cmath>

ThreatAssessmentCalculation calculateTargetThreatAssessment(
    const SPxPacketTrackExtended& track,
    const QString& cognitiveTargetType,
    const ThreatAssessmentParams& threatParams,
    const ThreatAssessmentContext& context)
{
    ThreatAssessmentCalculation calculation;
    ThreatAssessmentResult& result = calculation.assessment;
    calculation.targetType = NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
        cognitiveTargetType, track);
    const QString& finalTargetType = calculation.targetType;

    if (!finalTargetType.isEmpty()) {
        if (finalTargetType == QLatin1String("speedboat")
            || finalTargetType == QLatin1String("yacht")) {
            result.targetTypeScore = threatParams.seaSpeedboatScore;
        } else if (finalTargetType == QLatin1String("warship")) {
            result.targetTypeScore = threatParams.seaWarshipScore;
        } else if (finalTargetType == QLatin1String("motorboat")) {
            result.targetTypeScore = threatParams.seaMotorboatScore;
        } else if (finalTargetType == QLatin1String("fishingboat")) {
            result.targetTypeScore = threatParams.seaFishingBoatScore;
        } else if (finalTargetType == QLatin1String("ship")) {
            result.targetTypeScore = threatParams.seaShipScore;
        } else if (finalTargetType == QLatin1String("cargoship")) {
            result.targetTypeScore = threatParams.seaCargoShipScore;
        } else if (finalTargetType == QLatin1String("buoy")) {
            result.targetTypeScore = threatParams.seaBuoyScore;
        } else if (finalTargetType == QLatin1String("uav")
                   || finalTargetType == QLatin1String("drone")) {
            result.targetTypeScore = threatParams.airDroneScore;
        } else if (finalTargetType == QLatin1String("drone_swarm")) {
            result.targetTypeScore = threatParams.airDroneSwarmScore;
        } else if (finalTargetType == QLatin1String("compound_wing")) {
            result.targetTypeScore = threatParams.airCompoundWingScore;
        } else if (finalTargetType == QLatin1String("rotorcraft")) {
            result.targetTypeScore = threatParams.airRotorcraftScore;
        } else if (finalTargetType == QLatin1String("aircraft")) {
            result.targetTypeScore = threatParams.airAircraftScore;
        } else if (finalTargetType == QLatin1String("bird")) {
            result.targetTypeScore = threatParams.airBirdScore;
        } else if (finalTargetType == QLatin1String("bird_flock")) {
            result.targetTypeScore = threatParams.airBirdFlockScore;
        } else {
            result.targetTypeScore = threatParams.seaUnknownScore;
        }
    }

    const double speed = track.norm.min.speedMps;
    if (speed < threatParams.speedThresholdLow) {
        result.speedScore = 0.0;
    } else if (threatParams.speedThresholdHigh > threatParams.speedThresholdLow
               && speed <= threatParams.speedThresholdHigh) {
        result.speedScore = (speed - threatParams.speedThresholdLow)
            / (threatParams.speedThresholdHigh - threatParams.speedThresholdLow) * 10.0;
    } else {
        result.speedScore = 10.0;
    }

    calculation.usesAttackAngle = context.hasProtectArea;
    calculation.directionAngleDeg = context.directionAngleDeg;
    if (context.hasProtectArea) {
        calculation.distanceBasis = ThreatDistanceBasis::ProtectionArea;
        calculation.distanceMeters = AreaEscalationProtectionResolver::distanceMeters(
            QPointF(track.latDegs, track.longDegs), context.protectCenter);
    } else if (context.useBasePoint) {
        calculation.distanceBasis = ThreatDistanceBasis::BasePoint;
        calculation.distanceMeters = AreaEscalationProtectionResolver::distanceMeters(
            QPointF(track.latDegs, track.longDegs), context.basePoint);
    } else {
        calculation.distanceBasis = ThreatDistanceBasis::TrackRange;
        calculation.distanceMeters = track.norm.min.rangeMetres;
    }
    if (calculation.distanceMeters > 0.0 && threatParams.maxEffectiveDistance > 0.0) {
        result.distanceScore = std::max(
            0.0,
            (threatParams.maxEffectiveDistance - calculation.distanceMeters)
                / threatParams.maxEffectiveDistance * 10.0);
    }

    if (finalTargetType == QLatin1String("buoy")) {
        result.capabilityScore = 0.0;
    } else {
        result.capabilityScore = std::abs(
            (180.0 - context.directionAngleDeg) / 180.0 * 10.0);
    }

    result.weightedTypeScore =
        result.targetTypeScore * threatParams.typeWeight / 10.0 * 100.0;
    result.weightedAngleScore =
        result.capabilityScore * threatParams.angleWeight / 10.0 * 100.0;
    result.weightedSpeedScore =
        result.speedScore * threatParams.speedWeight / 10.0 * 100.0;
    result.weightedDistanceScore =
        result.distanceScore * threatParams.distanceWeight / 10.0 * 100.0;

    result.totalThreatLevel = std::clamp(
        result.weightedTypeScore + result.weightedAngleScore
            + result.weightedSpeedScore + result.weightedDistanceScore,
        0.0,
        100.0);

    if (result.totalThreatLevel >= 70.0) {
        result.threatDescription = QStringLiteral("高威胁");
    } else if (result.totalThreatLevel >= 30.0) {
        result.threatDescription = QStringLiteral("中威胁");
    } else if (result.totalThreatLevel > 0.0) {
        result.threatDescription = QStringLiteral("低威胁");
    } else {
        result.threatDescription = QStringLiteral("无威胁");
    }

    return calculation;
}
