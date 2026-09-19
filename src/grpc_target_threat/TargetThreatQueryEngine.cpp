#include "TargetThreatQueryEngine.hpp"
#include "TargetThreatQueryAssessment.hpp"

#include "customconfig.h"
#include "grpc_alarm/AlarmGrpcSnapshotClient.hpp"
#include "grpc_track/new_track_struct_grpc_convert.h"

#include <QDateTime>
#include <QReadLocker>

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

TargetThreatQueryAssessmentConfig copyQueryConfig(CustomConfig* config)
{
    TargetThreatQueryAssessmentConfig snapshot;
    if (config == nullptr)
        return snapshot;
    QReadLocker locker(&config->m_alarmConfigLock);
    snapshot.rules = config->m_mapAlarmRule;
    snapshot.areas = config->m_alarmArea;
    snapshot.threatParams = config->m_listThreatAssessmentParams;
    snapshot.activeSchemeId = config->m_activeAlarmSchemeId;
    snapshot.useBasePoint = config->m_struBasicConfig.m_nUseBasePoint == 1;
    snapshot.basePoint = QPointF(
        config->m_struBasicConfig.m_dBasePointLat,
        config->m_struBasicConfig.m_dBasePointLon);
    return snapshot;
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

    TargetThreatQueryAssessmentConfig config = copyQueryConfig(m_config);
    config.schemeAreas = m_config->dbHelper.getTargetThreatQueryAreas(config.activeSchemeId);
    fillTargetThreatQueryAssessment(
        hit.track, detection.finalTargetType, config, nowSec, response);
}

} // namespace grpc_target_threat
} // namespace alarmsys
