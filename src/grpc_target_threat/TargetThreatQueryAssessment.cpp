#include "TargetThreatQueryAssessment.hpp"

#include "dialog/alarm/AlarmAreaGeometryParser.h"
#include "dialog/alarm/AreaEscalationProtectionResolver.h"
#include "dialog/alarm/ThreatAssessmentCalculator.h"
#include "grpc_alarm/AlarmGrpcSnapshotMapping.h"

#include <QRectF>

#include <algorithm>
#include <cmath>

namespace alarmsys::grpc_target_threat {
namespace {

namespace proto = ::alarmsys::grpc::target_threat::v1;
namespace alarm_proto = ::trackmanager::grpc::alarm;

struct RuntimeArea
{
    AreaInfo source{};
    AlarmArea geometry{};
};

struct Candidate
{
    AreaInfo area{};
    QString ruleId;
    AreaEscalationProtectionResolver::Context protection;
    ThreatAssessmentParams params;
    ThreatAssessmentCalculation calculation;
};

bool parseCoordPair(const QString& csv, QPointF* out)
{
    const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (parts.size() < 2)
        return false;
    bool okLat = false;
    bool okLon = false;
    const double latitude = parts.at(0).trimmed().toDouble(&okLat);
    const double longitude = parts.at(1).trimmed().toDouble(&okLon);
    if (!okLat || !okLon)
        return false;
    *out = QPointF(latitude, longitude);
    return true;
}

bool buildRuntimeArea(const AreaInfo& source, RuntimeArea* out)
{
    RuntimeArea area;
    area.source = source;
    area.geometry.m_alertAreaType = source.areaType;
    switch (source.areaType) {
    case 1: {
        const QStringList parts = source.areaRect.split(QLatin1Char(','), QString::SkipEmptyParts);
        if (parts.size() < 4)
            return false;
        bool ok[4] = {false, false, false, false};
        const double x1 = parts.at(0).trimmed().toDouble(&ok[0]);
        const double y1 = parts.at(1).trimmed().toDouble(&ok[1]);
        const double x2 = parts.at(2).trimmed().toDouble(&ok[2]);
        const double y2 = parts.at(3).trimmed().toDouble(&ok[3]);
        if (!ok[0] || !ok[1] || !ok[2] || !ok[3])
            return false;
        area.geometry.m_alertAreaRect = QRectF(QPointF(x1, y1), QPointF(x2, y2)).normalized();
        break;
    }
    case 2:
        if (!parseCoordPair(source.stratPoint, &area.geometry.m_startP)
            || !parseCoordPair(source.endPoint, &area.geometry.m_endP))
            return false;
        break;
    case 3: {
        const AlarmAreaPointList parsed = parseAlarmAreaPointList(source.areaPoints);
        if (!parsed.valid || parsed.points.size() < 3)
            return false;
        area.geometry.m_alertAreaPolygon = parsed.points;
        break;
    }
    default:
        return false;
    }
    *out = area;
    return true;
}

bool containsPoint(const AlarmArea& area, const QPointF& point)
{
    switch (area.m_alertAreaType) {
    case 1:
        return area.m_alertAreaRect.normalized().contains(point);
    case 2: {
        const double radius = AreaEscalationProtectionResolver::distanceMeters(
            area.m_startP, area.m_endP);
        return radius > 0.0 && AreaEscalationProtectionResolver::distanceMeters(
            area.m_startP, point) <= radius;
    }
    case 3:
        return area.m_alertAreaPolygon.containsPoint(point, Qt::OddEvenFill);
    default:
        return false;
    }
}

QString areaKey(int groupId, int areaId)
{
    return QStringLiteral("%1_%2").arg(groupId).arg(areaId);
}

ThreatAssessmentParams threatParamsFor(
    const TargetThreatQueryAssessmentConfig& config, const AreaInfo& area)
{
    for (const ThreatAssessmentParams& params : config.threatParams) {
        if (params.groupId == area.groupID && params.areaId == area.areaID)
            return params;
    }
    return ThreatAssessmentParams(area.groupID, area.areaID);
}

QString ruleIdFor(
    const TargetThreatQueryAssessmentConfig& config,
    const AreaInfo& area,
    alarm_proto::EnvironmentType environment)
{
    QString best;
    for (const AlarmRule& rule : config.rules) {
        if (!rule.alarmstate || rule.group_id != area.groupID || rule.area_id != area.areaID
            || rule.alarm_level != area.waringType
            || static_cast<int>(alarmEnvironmentFromTrackType(rule.track_type))
                != static_cast<int>(environment))
            continue;
        if (best.isEmpty() || rule.condition_id < best)
            best = rule.condition_id;
    }
    return best; // 仅作为评分来源说明；没有匹配规则也照常算分。
}

bool betterCandidate(const Candidate& left, const Candidate& right)
{
    const double leftScore = left.calculation.assessment.totalThreatLevel;
    const double rightScore = right.calculation.assessment.totalThreatLevel;
    if (std::abs(leftScore - rightScore) > 1e-9)
        return leftScore > rightScore;
    if (left.area.waringType != right.area.waringType)
        return left.area.waringType > right.area.waringType;
    if (left.area.groupID != right.area.groupID)
        return left.area.groupID < right.area.groupID;
    if (left.area.areaID != right.area.areaID)
        return left.area.areaID < right.area.areaID;
    return left.ruleId < right.ruleId;
}

proto::ThreatGrade gradeFor(double score)
{
    if (score >= 70.0) return proto::HIGH_THREAT;
    if (score >= 30.0) return proto::MEDIUM_THREAT;
    if (score > 0.0) return proto::LOW_THREAT;
    return proto::NO_THREAT;
}

void fillComponent(proto::ThreatScoreComponent* out, double raw, double weight, double weighted)
{
    out->set_raw_score(raw);
    out->set_weight(weight);
    out->set_weighted_score(weighted);
}

void fillAssessment(proto::ThreatAssessment* out, const Candidate& candidate, double now)
{
    out->Clear();
    const auto& result = candidate.calculation.assessment;
    out->set_group_id(candidate.area.groupID);
    out->set_area_id(candidate.area.areaID);
    out->set_area_name(candidate.area.areaName.toStdString());
    out->set_rule_id(candidate.ruleId.toStdString());
    out->set_area_level(candidate.area.waringType);
    out->set_total_score(result.totalThreatLevel);
    out->set_grade(gradeFor(result.totalThreatLevel));
    out->set_description(result.threatDescription.toStdString());
    fillComponent(out->mutable_type_score(),
        result.targetTypeScore, candidate.params.typeWeight, result.weightedTypeScore);
    fillComponent(out->mutable_speed_score(),
        result.speedScore, candidate.params.speedWeight, result.weightedSpeedScore);
    fillComponent(out->mutable_direction_score(),
        result.capabilityScore, candidate.params.angleWeight, result.weightedAngleScore);
    fillComponent(out->mutable_distance_score(),
        result.distanceScore, candidate.params.distanceWeight, result.weightedDistanceScore);

    // 区外仅有类型/速度，不输出未使用的方向、距离或保护参考点。
    if (candidate.area.waringType != 0) {
        out->set_direction_basis(candidate.calculation.usesAttackAngle
            ? proto::ATTACK_ANGLE : proto::COURSE);
        out->set_direction_angle_deg(candidate.calculation.directionAngleDeg);
        switch (candidate.calculation.distanceBasis) {
        case ThreatDistanceBasis::ProtectionArea: out->set_distance_basis(proto::PROTECTION_AREA); break;
        case ThreatDistanceBasis::BasePoint: out->set_distance_basis(proto::BASE_POINT); break;
        case ThreatDistanceBasis::TrackRange: out->set_distance_basis(proto::TRACK_RANGE); break;
        default: break;
        }
        out->set_distance_m(candidate.calculation.distanceMeters);
        if (candidate.protection.available) {
            out->set_protection_reference_source(candidate.protection.sourceName().toStdString());
            auto* ref = out->mutable_protection_reference_position();
            ref->set_latitude(candidate.protection.center.x());
            ref->set_longitude(candidate.protection.center.y());
            ref->set_is_2d(true);
            ref->set_altitude_ref(alarm_proto::HEIGHT_ABOVE_WGS84);
            out->set_protection_radius_m(candidate.protection.radiusMeters);
        }
    }
    out->set_evaluated_time(now);
}

} // namespace

void fillTargetThreatQueryAssessment(
    const SPxPacketTrackExtended& track,
    const QString& cognitiveTargetType,
    const TargetThreatQueryAssessmentConfig& config,
    double evaluatedTime,
    proto::GetTargetThreatResponse* response)
{
    auto* target = response->mutable_result();
    QMap<QString, RuntimeArea> referenceAreas;
    for (const AreaInfo& source : config.areas) {
        RuntimeArea area;
        if (buildRuntimeArea(source, &area))
            referenceAreas.insert(areaKey(source.groupID, source.areaID), area);
    }

    const QPointF point(track.latDegs, track.longDegs);
    QList<Candidate> candidates;
    for (const AreaInfo& source : config.schemeAreas) {
        if (source.waringType != 2 && source.waringType != 3)
            continue;
        RuntimeArea area;
        if (!buildRuntimeArea(source, &area) || !containsPoint(area.geometry, point))
            continue;

        Candidate candidate;
        candidate.area = source;
        candidate.ruleId = ruleIdFor(config, source, target->environment());
        candidate.params = threatParamsFor(config, source);
        AreaEscalationProtectionResolver::Request request;
        const auto protect = referenceAreas.constFind(areaKey(source.protectGroupID, source.protectAreaID));
        if (protect != referenceAreas.constEnd() && protect->geometry.m_alertAreaType == 2) {
            request.databaseCircleAvailable = true;
            request.databaseCenter = protect->geometry.m_startP;
            request.databaseRadiusMeters = AreaEscalationProtectionResolver::distanceMeters(
                protect->geometry.m_startP, protect->geometry.m_endP);
        }
        candidate.protection = AreaEscalationProtectionResolver::resolve(request);

        ThreatAssessmentContext context;
        context.hasProtectArea = candidate.protection.available;
        context.protectCenter = candidate.protection.center;
        context.protectRadiusMeters = candidate.protection.radiusMeters;
        context.protectionReferenceSource = candidate.protection.sourceName();
        context.useBasePoint = config.useBasePoint;
        context.basePoint = config.basePoint;
        context.directionAngleDeg = track.norm.min.courseDegrees;
        if (context.hasProtectArea) {
            context.directionAngleDeg = AreaEscalationProtectionResolver::attackAngleDegrees(
                track.norm.min.courseDegrees,
                AreaEscalationProtectionResolver::bearingDegrees(point, context.protectCenter));
        }
        candidate.calculation = calculateTargetThreatAssessment(
            track, cognitiveTargetType, candidate.params, context);
        candidates.append(candidate);
    }

    const bool outside = candidates.isEmpty();
    Candidate best;
    if (outside) {
        // 只修改查询侧的参数副本；默认类型/速度权重不归一化。
        best.params.angleWeight = 0.0;
        best.params.distanceWeight = 0.0;
        best.params.maxEffectiveDistance = 0.0;
        ThreatAssessmentContext context;
        context.directionAngleDeg = 180.0; // 未使用的方向原始分也返回 0
        best.calculation = calculateTargetThreatAssessment(
            track, cognitiveTargetType, best.params, context);
    } else {
        std::sort(candidates.begin(), candidates.end(), betterCandidate);
        best = candidates.first();
    }

    fillAssessment(target->mutable_assessment(), best, evaluatedTime);
    target->set_target_type(best.calculation.targetType.toStdString());
    const std::string message = outside
        ? "目标不在启用预警/告警区域内，仅按默认类型和速度参数计算威胁分" : "ok";
    target->set_status(proto::QUERY_OK);
    target->set_message(message);
    response->set_status(proto::QUERY_OK);
    response->set_message(message);
}

} // namespace alarmsys::grpc_target_threat
