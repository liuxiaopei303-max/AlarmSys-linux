#include "TargetThreatQueryEngine.hpp"

#include "customconfig.h"
#include "dialog/alarm/AlarmAreaGeometryParser.h"
#include "dialog/alarm/AreaEscalationProtectionResolver.h"
#include "dialog/alarm/ThreatAssessmentCalculator.h"
#include "grpc_alarm/AlarmGrpcSnapshotClient.hpp"
#include "grpc_alarm/AlarmGrpcSnapshotMapping.h"
#include "grpc_track/new_track_struct_grpc_convert.h"

#include <QDateTime>
#include <QMutexLocker>
#include <QReadLocker>
#include <QRectF>

#include <algorithm>
#include <cmath>
#include <limits>

namespace alarmsys {
namespace grpc_target_threat {
namespace {

namespace proto = ::alarmsys::grpc::target_threat::v1;
namespace alarm_proto = ::trackmanager::grpc::alarm;

struct TrackHit
{
    bool found = false;
    alarm_proto::EnvironmentType environment = alarm_proto::ENVIRONMENT_TYPE_UNSPECIFIED;
    SPxPacketTrackExtended track{};
};

struct RuntimeArea
{
    AreaInfo source;
    AlarmArea geometry;
};

struct QueryConfigSnapshot
{
    QMap<QString, AlarmRule> rules;
    QList<AreaInfo> areas;
    QList<ThreatAssessmentParams> threatParams;
    QMap<QString, QPair<int, int>> protectAreas;
    AreaEscalationConfig areaEscalation;
    QString activeSchemeId;
    bool useBasePoint = false;
    QPointF basePoint;
};

struct Candidate
{
    AlarmRule rule;
    QString areaName;
    AreaEscalationProtectionResolver::Context protection;
    ThreatAssessmentParams params;
    ThreatAssessmentCalculation calculation;
};

bool parseCoordPair(const QString& csv, QPointF* out)
{
    if (out == nullptr)
        return false;
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

bool parseAreaRect(const QString& csv, QRectF* out)
{
    if (out == nullptr)
        return false;
    const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (parts.size() < 4)
        return false;
    bool ok[4] = {false, false, false, false};
    const double x1 = parts.at(0).trimmed().toDouble(&ok[0]);
    const double y1 = parts.at(1).trimmed().toDouble(&ok[1]);
    const double x2 = parts.at(2).trimmed().toDouble(&ok[2]);
    const double y2 = parts.at(3).trimmed().toDouble(&ok[3]);
    if (!ok[0] || !ok[1] || !ok[2] || !ok[3])
        return false;
    *out = QRectF(QPointF(x1, y1), QPointF(x2, y2)).normalized();
    return true;
}

bool buildRuntimeArea(const AreaInfo& source, RuntimeArea* out)
{
    if (out == nullptr)
        return false;
    RuntimeArea area;
    area.source = source;
    area.geometry.groupID = source.groupID;
    area.geometry.areaID = source.areaID;
    area.geometry.groupName = source.groupName;
    area.geometry.areaName = source.areaName;
    area.geometry.m_alertAreaType = source.areaType;
    area.geometry.checkState = source.checkState;
    area.geometry.waringType = source.waringType;
    area.geometry.waringTime = source.waringTime;

    switch (source.areaType) {
    case 1:
        if (!parseAreaRect(source.areaRect, &area.geometry.m_alertAreaRect))
            return false;
        area.geometry.m_alertAreaPolygon = QPolygonF(area.geometry.m_alertAreaRect);
        break;
    case 2:
        if (!parseCoordPair(source.stratPoint, &area.geometry.m_startP)
            || !parseCoordPair(source.endPoint, &area.geometry.m_endP)) {
            return false;
        }
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
        const double distance = AreaEscalationProtectionResolver::distanceMeters(
            area.m_startP, point);
        return radius > 0.0 && distance <= radius;
    }
    case 3:
        return area.m_alertAreaPolygon.containsPoint(point, Qt::OddEvenFill);
    default:
        return false;
    }
}

template<typename Key>
bool findTrackInMap(
    const QMap<Key, SPxPacketTrackExtended>& tracks,
    qint64 targetId,
    SPxPacketTrackExtended* out)
{
    if (out == nullptr)
        return false;
    if (targetId >= static_cast<qint64>(std::numeric_limits<Key>::min())
        && targetId <= static_cast<qint64>(std::numeric_limits<Key>::max())) {
        const auto it = tracks.constFind(static_cast<Key>(targetId));
        if (it != tracks.constEnd()) {
            *out = it.value();
            return true;
        }
    }
    for (auto it = tracks.constBegin(); it != tracks.constEnd(); ++it) {
        if (static_cast<qint64>(it.value().secondary.uniqueID) == targetId) {
            *out = it.value();
            return true;
        }
    }
    return false;
}

TrackHit findTrack(CustomConfig* config, qint64 targetId)
{
    TrackHit hit;
    if (config == nullptr || targetId <= 0)
        return hit;

    QReadLocker locker(&config->m_trackDataLock);
    if (findTrackInMap(config->m_mapBirdFuseTrack, targetId, &hit.track)) {
        hit.found = true;
        hit.environment = alarm_proto::AIR;
        return hit;
    }
    if (findTrackInMap(config->m_mapFuseTrack, targetId, &hit.track)) {
        hit.found = true;
        hit.environment = alarm_proto::SURFACE;
        return hit;
    }

    // 兼容旧雷达航迹。现有目标告警映射中 track_type=1 属于 AIR。
    if (findTrackInMap(config->m_mapRadarTrack, targetId, &hit.track)) {
        hit.found = true;
        hit.environment = alarm_proto::AIR;
    }
    return hit;
}

QueryConfigSnapshot copyQueryConfig(CustomConfig* config)
{
    QueryConfigSnapshot snapshot;
    if (config == nullptr)
        return snapshot;
    QReadLocker locker(&config->m_alarmConfigLock);
    snapshot.rules = config->m_mapAlarmRule;
    snapshot.areas = config->m_alarmArea;
    snapshot.threatParams = config->m_listThreatAssessmentParams;
    snapshot.protectAreas = config->m_mapSchemeProtectAreas;
    snapshot.areaEscalation = config->m_areaEscalation;
    snapshot.activeSchemeId = config->m_activeAlarmSchemeId;
    snapshot.useBasePoint = config->m_struBasicConfig.m_nUseBasePoint == 1;
    snapshot.basePoint = QPointF(
        config->m_struBasicConfig.m_dBasePointLat,
        config->m_struBasicConfig.m_dBasePointLon);
    return snapshot;
}

QMap<QString, RuntimeArea> buildAreaMap(const QList<AreaInfo>& sourceAreas)
{
    QMap<QString, RuntimeArea> areas;
    for (const AreaInfo& source : sourceAreas) {
        RuntimeArea runtime;
        if (!buildRuntimeArea(source, &runtime))
            continue;
        const QString key = QStringLiteral("%1_%2").arg(source.groupID).arg(source.areaID);
        areas.insert(key, runtime);
    }
    return areas;
}

ThreatAssessmentParams threatParamsFor(
    const QueryConfigSnapshot& config, int groupId, int areaId)
{
    for (const ThreatAssessmentParams& params : config.threatParams) {
        if (params.groupId == groupId && params.areaId == areaId)
            return params;
    }
    return ThreatAssessmentParams(groupId, areaId);
}

AreaEscalationProtectionResolver::Context resolveProtection(
    const QueryConfigSnapshot& config,
    const QMap<QString, RuntimeArea>& areas,
    const AlarmRule& rule,
    alarm_proto::EnvironmentType environment)
{
    AreaEscalationProtectionResolver::Request request;
    request.config = config.areaEscalation;
    request.activeSchemeId = config.activeSchemeId;
    request.targetDomain = environment == alarm_proto::AIR
        ? QStringLiteral("AIR") : QStringLiteral("SURFACE");

    const QString areaKey = QStringLiteral("%1_%2").arg(rule.group_id).arg(rule.area_id);
    const auto protectIt = config.protectAreas.constFind(areaKey);
    if (protectIt != config.protectAreas.constEnd()) {
        const QString protectKey = QStringLiteral("%1_%2")
            .arg(protectIt.value().first).arg(protectIt.value().second);
        const auto runtimeIt = areas.constFind(protectKey);
        if (runtimeIt != areas.constEnd()
            && runtimeIt.value().geometry.m_alertAreaType == 2) {
            const AlarmArea& area = runtimeIt.value().geometry;
            request.databaseCircleAvailable = true;
            request.databaseCenter = area.m_startP;
            request.databaseRadiusMeters =
                AreaEscalationProtectionResolver::distanceMeters(
                    area.m_startP, area.m_endP);
        }
    }
    return AreaEscalationProtectionResolver::resolve(request);
}

bool betterCandidate(const Candidate& left, const Candidate& right)
{
    const double leftScore = left.calculation.assessment.totalThreatLevel;
    const double rightScore = right.calculation.assessment.totalThreatLevel;
    if (std::abs(leftScore - rightScore) > 1e-9)
        return leftScore > rightScore;
    if (left.rule.alarm_level != right.rule.alarm_level)
        return left.rule.alarm_level > right.rule.alarm_level;
    if (left.rule.group_id != right.rule.group_id)
        return left.rule.group_id < right.rule.group_id;
    if (left.rule.area_id != right.rule.area_id)
        return left.rule.area_id < right.rule.area_id;
    return left.rule.condition_id < right.rule.condition_id;
}

void fillPosition(alarm_proto::GeoPosition* position, const SPxPacketTrackExtended& track)
{
    if (position == nullptr)
        return;
    position->set_longitude(track.longDegs);
    position->set_latitude(track.latDegs);
    position->set_altitude(track.altitudeMetres);
    position->set_is_2d(track.altitudeMetres == 0.0f);
    position->set_altitude_ref(alarm_proto::HEIGHT_ABOVE_WGS84);
}

proto::ThreatGrade gradeFor(double score)
{
    if (score >= 70.0)
        return proto::HIGH_THREAT;
    if (score >= 30.0)
        return proto::MEDIUM_THREAT;
    if (score > 0.0)
        return proto::LOW_THREAT;
    return proto::NO_THREAT;
}

void fillComponent(
    proto::ThreatScoreComponent* out,
    double rawScore,
    double weight,
    double weightedScore)
{
    out->set_raw_score(rawScore);
    out->set_weight(weight);
    out->set_weighted_score(weightedScore);
}

void fillAssessment(
    proto::ThreatAssessment* out,
    const Candidate& candidate,
    double evaluatedTime)
{
    const ThreatAssessmentResult& result = candidate.calculation.assessment;
    out->set_group_id(candidate.rule.group_id);
    out->set_area_id(candidate.rule.area_id);
    out->set_area_name(candidate.areaName.toStdString());
    out->set_rule_id(candidate.rule.condition_id.toStdString());
    out->set_area_level(candidate.rule.alarm_level);
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

    out->set_direction_basis(candidate.calculation.usesAttackAngle
        ? proto::ATTACK_ANGLE : proto::COURSE);
    out->set_direction_angle_deg(candidate.calculation.directionAngleDeg);
    switch (candidate.calculation.distanceBasis) {
    case ThreatDistanceBasis::ProtectionArea:
        out->set_distance_basis(proto::PROTECTION_AREA);
        break;
    case ThreatDistanceBasis::BasePoint:
        out->set_distance_basis(proto::BASE_POINT);
        break;
    case ThreatDistanceBasis::TrackRange:
        out->set_distance_basis(proto::TRACK_RANGE);
        break;
    default:
        out->set_distance_basis(proto::DISTANCE_BASIS_UNSPECIFIED);
        break;
    }
    out->set_distance_m(candidate.calculation.distanceMeters);

    if (candidate.protection.available) {
        out->set_protection_reference_source(
            candidate.protection.sourceName().toStdString());
        alarm_proto::GeoPosition* ref = out->mutable_protection_reference_position();
        ref->set_latitude(candidate.protection.center.x());
        ref->set_longitude(candidate.protection.center.y());
        ref->set_is_2d(true);
        ref->set_altitude_ref(alarm_proto::HEIGHT_ABOVE_WGS84);
        out->set_protection_radius_m(candidate.protection.radiusMeters);
    }
    out->set_evaluated_time(evaluatedTime);
}

const alarm_proto::AlarmItem* findCurrentAlarm(
    const alarm_proto::AlarmSnapshotRequest& snapshot,
    const QString& targetId,
    alarm_proto::EnvironmentType environment)
{
    for (const alarm_proto::TargetAlarmRecord& record : snapshot.items()) {
        if (record.environment() == environment
            && QString::fromStdString(record.target_id()) == targetId) {
            return &record.alarm();
        }
    }
    return nullptr;
}

} // namespace

TargetThreatQueryEngine::TargetThreatQueryEngine(CustomConfig* config)
    : m_config(config)
{
}

void TargetThreatQueryEngine::query(
    qint64 targetId,
    const QString& targetIdText,
    proto::GetTargetThreatResponse* response) const
{
    const double nowSec = static_cast<double>(QDateTime::currentMSecsSinceEpoch()) / 1000.0;
    response->Clear();
    response->set_target_id(targetIdText.toStdString());
    response->set_response_time(nowSec);

    const TrackHit hit = findTrack(m_config, targetId);
    if (!hit.found) {
        response->set_status(proto::TARGET_NOT_FOUND);
        response->set_message("目标不存在或已离开实时航迹缓存");
        return;
    }

    proto::TargetThreatResult* target = response->mutable_result();
    target->set_environment(hit.environment);
    fillPosition(target->mutable_position(), hit.track);
    target->set_speed_mps(hit.track.norm.min.speedMps);
    target->set_course_deg(hit.track.norm.min.courseDegrees);
    target->set_target_update_time(
        static_cast<double>(hit.track.msgTimeSecs)
        + static_cast<double>(hit.track.msgTimeUsecs) / 1000000.0);

    const DataAccessLayer::DetectionTypeResult detection =
        m_config->dbHelper.getDetectionTypesByReId(targetId);
    target->set_target_type(
        NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
            detection.finalTargetType, hit.track).toStdString());

    if (m_config->m_alarmGrpcClient) {
        const alarm_proto::AlarmSnapshotRequest alarms =
            m_config->m_alarmGrpcClient->buildCurrentSnapshot(m_config);
        if (const alarm_proto::AlarmItem* alarm =
                findCurrentAlarm(alarms, targetIdText, hit.environment)) {
            *target->mutable_current_alarm() = *alarm;
        }
    }

    const QueryConfigSnapshot config = copyQueryConfig(m_config);
    const QMap<QString, RuntimeArea> areas = buildAreaMap(config.areas);
    const QPointF targetPoint(hit.track.latDegs, hit.track.longDegs);
    QList<Candidate> candidates;

    for (const AlarmRule& rule : config.rules) {
        if (!rule.alarmstate || (rule.alarm_level != 2 && rule.alarm_level != 3))
            continue;
        const AlarmTargetEnvironment mapped = alarmEnvironmentFromTrackType(rule.track_type);
        if (static_cast<int>(mapped) != static_cast<int>(hit.environment))
            continue;

        const QString areaKey = QStringLiteral("%1_%2").arg(rule.group_id).arg(rule.area_id);
        const auto areaIt = areas.constFind(areaKey);
        if (areaIt == areas.constEnd()
            || !containsPoint(areaIt.value().geometry, targetPoint)) {
            continue;
        }

        Candidate candidate;
        candidate.rule = rule;
        candidate.areaName = areaIt.value().source.areaName;
        candidate.params = threatParamsFor(config, rule.group_id, rule.area_id);
        candidate.protection = resolveProtection(config, areas, rule, hit.environment);

        ThreatAssessmentContext context;
        context.hasProtectArea = candidate.protection.available;
        context.protectCenter = candidate.protection.center;
        context.protectRadiusMeters = candidate.protection.radiusMeters;
        context.protectionReferenceSource = candidate.protection.sourceName();
        context.useBasePoint = config.useBasePoint;
        context.basePoint = config.basePoint;
        context.directionAngleDeg = hit.track.norm.min.courseDegrees;
        if (candidate.protection.available) {
            const double bearing = AreaEscalationProtectionResolver::bearingDegrees(
                targetPoint, candidate.protection.center);
            context.directionAngleDeg =
                AreaEscalationProtectionResolver::attackAngleDegrees(
                    hit.track.norm.min.courseDegrees, bearing);
        }
        candidate.calculation = calculateTargetThreatAssessment(
            hit.track, detection.finalTargetType, candidate.params, context);
        candidates.append(candidate);
    }

    if (candidates.isEmpty()) {
        target->set_status(proto::OUTSIDE_MONITORED_AREA);
        target->set_message("目标不在警戒区，无威胁分");
        response->set_status(proto::OUTSIDE_MONITORED_AREA);
        response->set_message("目标不在警戒区，无威胁分");
        return;
    }

    std::sort(candidates.begin(), candidates.end(), betterCandidate);
    const Candidate& best = candidates.first();
    fillAssessment(target->mutable_assessment(), best, nowSec);
    target->set_target_type(best.calculation.targetType.toStdString());
    target->set_status(proto::QUERY_OK);
    target->set_message("ok");
    response->set_status(proto::QUERY_OK);
    response->set_message("ok");
}

} // namespace grpc_target_threat
} // namespace alarmsys
