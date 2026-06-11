#include "AlarmGrpcSnapshotClient.hpp"

#include "customconfig.h"
#include "dialog/alarm/AlarmFileLogger.h"

#include <QDateTime>
#include <QDebug>
#include <QMutexLocker>
#include <QReadLocker>
#include <grpcpp/grpcpp.h>

#include "alarm_service.grpc.pb.h"
#include "alarm_service.pb.h"

namespace {

using trackmanager::grpc::alarm::AlarmCategory;
using trackmanager::grpc::alarm::AlarmDispositionStatus;
using trackmanager::grpc::alarm::AlarmItem;
using trackmanager::grpc::alarm::AlarmLifecycleStatus;
using trackmanager::grpc::alarm::AlarmLocationType;
using trackmanager::grpc::alarm::AlarmSnapshotRequest;
using trackmanager::grpc::alarm::AlarmSpatialInfo;
using trackmanager::grpc::alarm::EnvironmentType;
using trackmanager::grpc::alarm::GeoPosition;
using trackmanager::grpc::alarm::TargetAlarmRecord;
using trackmanager::grpc::alarm::ThreatLevel;

double parseAlarmTimeSec(const QString& timeStr)
{
    const QDateTime dt = QDateTime::fromString(timeStr, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    if (dt.isValid()) {
        return static_cast<double>(dt.toSecsSinceEpoch())
            + static_cast<double>(dt.time().msec()) / 1000.0;
    }
    return static_cast<double>(QDateTime::currentDateTime().toSecsSinceEpoch());
}

ThreatLevel threatLevelFromScore(int score)
{
    if (score >= 70) {
        return ThreatLevel::HIGH;
    }
    if (score >= 30) {
        return ThreatLevel::MEDIUM;
    }
    return ThreatLevel::LOW;
}

AlarmLifecycleStatus lifecycleFromStatus(int alarmStatus)
{
    switch (alarmStatus) {
    case 1:
        return AlarmLifecycleStatus::FOCUS;
    case 2:
        return AlarmLifecycleStatus::DELETED;
    default:
        return AlarmLifecycleStatus::ACTIVE;
    }
}

AlarmDispositionStatus dispositionFromTaskStatus(int taskStatus)
{
    switch (taskStatus) {
    case 1:
        return AlarmDispositionStatus::ASSIGNED;
    case 2:
        return AlarmDispositionStatus::VERIFY_FAILED;
    case 3:
        return AlarmDispositionStatus::VERIFY_SUCCESS;
    default:
        return AlarmDispositionStatus::UNASSIGNED;
    }
}

EnvironmentType environmentFromRuleTrackType(int ruleTrackType)
{
    return (ruleTrackType > 0) ? EnvironmentType::AIR : EnvironmentType::SURFACE;
}

void fillGeoPosition(GeoPosition* pos, double lon, double lat, double alt = 0.0)
{
    pos->set_longitude(lon);
    pos->set_latitude(lat);
    pos->set_altitude(alt);
    pos->set_is_2d(alt == 0.0);
    pos->set_altitude_ref(trackmanager::grpc::alarm::HEIGHT_ABOVE_WGS84);
}

void fillPointSpatialInfo(
    AlarmSpatialInfo* spatial, double lon, double lat, double alt = 0.0, double bearingDeg = 0.0)
{
    spatial->set_location_type(AlarmLocationType::POINT);
    spatial->set_point_position_set(true);
    fillGeoPosition(spatial->mutable_point_position(), lon, lat, alt);
    spatial->set_bearing_deg(bearingDeg);
    spatial->set_spatial_confidence(1.0);
}

AlarmItem buildAlarmItem(const AlarmData& alarmData, CustomConfig* cfg, const QString& areaName)
{
    AlarmItem item;
    item.set_alarm_id(alarmData.alarm_id.toStdString());
    item.add_categories(AlarmCategory::TRACK_AREA);
    item.set_status(lifecycleFromStatus(alarmData.alarm_status));
    item.set_disposition(dispositionFromTaskStatus(alarmData.task_status));
    item.set_content(
        QStringLiteral("告警规则:%1 航迹:%2")
            .arg(alarmData.condition_id)
            .arg(alarmData.unique_id)
            .toStdString());
    item.set_level(threatLevelFromScore(alarmData.threatScore));
    item.mutable_area()->set_area_id(
        QStringLiteral("%1_%2").arg(alarmData.group_id).arg(alarmData.area_id).toStdString());
    item.mutable_area()->set_area_name(areaName.toStdString());
    item.set_class_id(alarmData.targettype);
    item.set_behavior_id(alarmData.targetbehavior);
    fillGeoPosition(item.mutable_position(), alarmData.targetlon, alarmData.targetlat);
    fillPointSpatialInfo(
        item.mutable_spatial_info(), alarmData.targetlon, alarmData.targetlat, 0.0, alarmData.targetdir);

    const double raisedSec = parseAlarmTimeSec(alarmData.origintime.isEmpty() ? alarmData.time : alarmData.origintime);
    const double updatedSec = parseAlarmTimeSec(alarmData.time);
    item.set_raised_time(raisedSec);
    item.set_updated_time(updatedSec);
    item.set_resolved_time(0.0);
    item.add_rule_ids(alarmData.condition_id.toStdString());
    item.set_detection_box_set(false);
    Q_UNUSED(cfg);
    return item;
}

bool passesTrackFilter(const AlarmData& alarmData, CustomConfig* cfg)
{
    if (!cfg->m_mapAlarmRule.contains(alarmData.condition_id)) {
        return true;
    }
    const int ruleTrackType = cfg->m_mapAlarmRule.value(alarmData.condition_id).track_type;
    const int apiType = (ruleTrackType > 0) ? 1 : 0;
    QMutexLocker filterLocker(&cfg->m_alarmFilterMutex);
    return !cfg->m_listAlarmFilter.contains(
        qMakePair(apiType, static_cast<qint64>(alarmData.unique_id)));
}

qint64 resolveGrpcTargetUniqueId(const AlarmData& alarmData, CustomConfig* /*cfg*/, int /*ruleTrackType*/)
{
    return static_cast<qint64>(alarmData.unique_id);
}

struct SnapshotKey
{
    int environment = 0;
    QString targetId;

    bool operator<(const SnapshotKey& other) const
    {
        if (environment != other.environment) {
            return environment < other.environment;
        }
        return targetId < other.targetId;
    }
};

QString environmentName(EnvironmentType env)
{
    switch (env) {
    case EnvironmentType::AIR:
        return QStringLiteral("AIR");
    case EnvironmentType::SURFACE:
        return QStringLiteral("SURFACE");
    case EnvironmentType::MARITIME:
        return QStringLiteral("MARITIME");
    case EnvironmentType::LAND:
        return QStringLiteral("LAND");
    case EnvironmentType::SUBSURFACE:
        return QStringLiteral("SUBSURFACE");
    case EnvironmentType::SPACE:
        return QStringLiteral("SPACE");
    default:
        return QStringLiteral("UNKNOWN");
    }
}

QString formatGrpcSnapshotItem(const TargetAlarmRecord& record)
{
    const AlarmItem& alarm = record.alarm();
    QString ruleId;
    if (alarm.rule_ids_size() > 0) {
        ruleId = QString::fromStdString(alarm.rule_ids(0));
    }
    return QStringLiteral(
               "env=%1 target_id=%2 producer=%3 update_time=%4 "
               "alarm_id=%5 level=%6 status=%7 disposition=%8 "
               "class_id=%9 behavior_id=%10 lon=%11 lat=%12 bearing_deg=%13 rule=%14 content=%15")
        .arg(environmentName(record.environment()))
        .arg(QString::fromStdString(record.target_id()))
        .arg(QString::fromStdString(record.producer_id()))
        .arg(record.update_time(), 0, 'f', 3)
        .arg(QString::fromStdString(alarm.alarm_id()))
        .arg(static_cast<int>(alarm.level()))
        .arg(static_cast<int>(alarm.status()))
        .arg(static_cast<int>(alarm.disposition()))
        .arg(alarm.class_id())
        .arg(alarm.behavior_id())
        .arg(alarm.position().longitude(), 0, 'f', 6)
        .arg(alarm.position().latitude(), 0, 'f', 6)
        .arg(alarm.spatial_info().bearing_deg(), 0, 'f', 2)
        .arg(ruleId)
        .arg(QString::fromStdString(alarm.content()));
}

void logGrpcSnapshotRequest(const AlarmSnapshotRequest& request)
{
    AlarmFileLogger::logGrpcAlarmSnapshot(
        QStringLiteral("GrpcSnapshot push------items=%1").arg(request.items_size()));
    for (int i = 0; i < request.items_size(); ++i) {
        AlarmFileLogger::logGrpcAlarmSnapshot(
            QStringLiteral("GrpcSnapshot item[%1]------%2")
                .arg(i)
                .arg(formatGrpcSnapshotItem(request.items(i))));
    }
}

} // namespace

AlarmGrpcSnapshotClient::AlarmGrpcSnapshotClient() = default;

AlarmGrpcSnapshotClient::~AlarmGrpcSnapshotClient() = default;

void AlarmGrpcSnapshotClient::configure(
    const std::string& host,
    int port,
    bool enabled,
    const std::string& producerId)
{
    m_host = host;
    m_port = port;
    m_enabled = enabled;
    m_producerId = producerId;
    m_channel.reset();
    m_stub.reset();
}

bool AlarmGrpcSnapshotClient::ensureChannel()
{
    if (!m_enabled) {
        return false;
    }
    if (m_stub) {
        return true;
    }

    const std::string address = m_host + ":" + std::to_string(m_port);
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    if (!channel) {
        qWarning() << "AlarmGrpcSnapshotClient: 创建 gRPC channel 失败" << QString::fromStdString(address);
        return false;
    }

    m_channel = channel;
    m_stub = trackmanager::grpc::alarm::AlarmResultService::NewStub(channel);
    qInfo() << "AlarmGrpcSnapshotClient: 已连接" << QString::fromStdString(address);
    return true;
}

bool AlarmGrpcSnapshotClient::pushSnapshot(CustomConfig* cfg)
{
    if (!m_enabled || cfg == nullptr) {
        return false;
    }
    if (!ensureChannel()) {
        return false;
    }

    AlarmSnapshotRequest request;
    const double nowSec = static_cast<double>(QDateTime::currentDateTime().toSecsSinceEpoch());
    QMap<SnapshotKey, TargetAlarmRecord> dedup;

    {
        QMutexLocker locker(&cfg->m_alarmDataMutex);
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

        for (auto it = cfg->m_mapAlarmData.constBegin(); it != cfg->m_mapAlarmData.constEnd(); ++it) {
            const AlarmData& alarmData = it.value();
            const qint64 timeAlarmMs =
                QDateTime::fromString(alarmData.time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")).toMSecsSinceEpoch();
            if (nowMs - timeAlarmMs > 600 * 1000) {
                continue;
            }
            if (!passesTrackFilter(alarmData, cfg)) {
                continue;
            }

            const int ruleTrackType = cfg->m_mapAlarmRule.value(alarmData.condition_id).track_type;
            const qint64 grpcTargetId = resolveGrpcTargetUniqueId(alarmData, cfg, ruleTrackType);
            if (grpcTargetId <= 0) {
                AlarmFileLogger::logGrpcAlarmSnapshot(
                    QStringLiteral("GrpcSnapshot skip------trackId:%1 alarmId:%2 unique_id:%3")
                        .arg(alarmData.track_id)
                        .arg(alarmData.alarm_id)
                        .arg(alarmData.unique_id));
                continue;
            }
            if (grpcTargetId != alarmData.unique_id) {
                AlarmFileLogger::logGrpcAlarmSnapshot(
                    QStringLiteral("GrpcSnapshot unique_id repair------trackId:%1 alarmUnique:%2 grpcTarget:%3")
                        .arg(alarmData.track_id)
                        .arg(alarmData.unique_id)
                        .arg(grpcTargetId));
            }

            QString areaName;
            for (const AlarmRule& rule : cfg->m_mapAlarmRule) {
                if (rule.area_id == alarmData.area_id && rule.group_id == alarmData.group_id) {
                    areaName = rule.area_name;
                    break;
                }
            }

            SnapshotKey key;
            key.environment = static_cast<int>(environmentFromRuleTrackType(ruleTrackType));
            key.targetId = QString::number(grpcTargetId);

            TargetAlarmRecord record;
            record.set_environment(static_cast<EnvironmentType>(key.environment));
            record.set_target_id(key.targetId.toStdString());
            record.set_producer_id(m_producerId);
            record.set_update_time(nowSec);

            AlarmItem item = buildAlarmItem(alarmData, cfg, areaName);
            const AlarmEffectiveDisposition effDisp = cfg->resolveRuleAlarmDisposition(alarmData);
            item.set_disposition(dispositionFromTaskStatus(effDisp.task_status));
            if (!effDisp.resolved_time_str.isEmpty()) {
                item.set_resolved_time(parseAlarmTimeSec(effDisp.resolved_time_str));
            }
            *record.mutable_alarm() = item;

            if (!dedup.contains(key) || dedup[key].alarm().level() < record.alarm().level()) {
                dedup.insert(key, record);
            }
        }

        for (auto it = cfg->m_mapManualAlarmData.constBegin(); it != cfg->m_mapManualAlarmData.constEnd(); ++it) {
            const AlarmData& alarmData = it.value();
            const bool isAirTrack = cfg->m_mapManualAlarmAirTrack.value(
                static_cast<qint64>(alarmData.unique_id), false);
            const int apiType = isAirTrack ? 1 : 0;
            {
                QMutexLocker filterLocker(&cfg->m_alarmFilterMutex);
                if (cfg->m_listAlarmFilter.contains(
                        qMakePair(apiType, static_cast<qint64>(alarmData.unique_id)))) {
                    continue;
                }
            }

            const qint64 timeAlarmMs =
                QDateTime::fromString(alarmData.time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")).toMSecsSinceEpoch();
            if (nowMs - timeAlarmMs > 600 * 1000) {
                continue;
            }
            if (alarmData.unique_id <= 0) {
                continue;
            }

            SnapshotKey key;
            key.environment = static_cast<int>(isAirTrack ? EnvironmentType::AIR : EnvironmentType::SURFACE);
            key.targetId = QString::number(alarmData.unique_id);

            TargetAlarmRecord record;
            record.set_environment(static_cast<EnvironmentType>(key.environment));
            record.set_target_id(key.targetId.toStdString());
            record.set_producer_id(m_producerId);
            record.set_update_time(nowSec);

            AlarmItem item = buildAlarmItem(alarmData, cfg, QString());
            item.set_disposition(AlarmDispositionStatus::VERIFY_SUCCESS);
            *record.mutable_alarm() = item;

            if (!dedup.contains(key) || dedup[key].alarm().level() < record.alarm().level()) {
                dedup.insert(key, record);
            }
        }
    }

    for (const TargetAlarmRecord& record : dedup) {
        *request.add_items() = record;
    }

    logGrpcSnapshotRequest(request);

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));
    trackmanager::grpc::alarm::UpdateAck response;
    const grpc::Status status = m_stub->UpdateAlarmSnapshot(&context, request, &response);
    if (!status.ok()) {
        const QString err = QStringLiteral("GrpcSnapshot fail------code=%1 msg=%2 items=%3")
                                .arg(static_cast<int>(status.error_code()))
                                .arg(QString::fromStdString(status.error_message()))
                                .arg(request.items_size());
        qWarning() << "AlarmGrpcSnapshotClient: UpdateAlarmSnapshot 失败"
                   << status.error_code() << QString::fromStdString(status.error_message());
        AlarmFileLogger::logGrpcAlarmSnapshot(err);
        m_stub.reset();
        m_channel.reset();
        return false;
    }

    if (!response.accepted()) {
        const QString err = QStringLiteral("GrpcSnapshot rejected------reason=%1 items=%2")
                                .arg(QString::fromStdString(response.reason()))
                                .arg(request.items_size());
        qWarning() << "AlarmGrpcSnapshotClient: 服务端拒绝快照 reason="
                   << QString::fromStdString(response.reason());
        AlarmFileLogger::logGrpcAlarmSnapshot(err);
        return false;
    }

    AlarmFileLogger::logGrpcAlarmSnapshot(
        QStringLiteral("GrpcSnapshot ack------accepted=1 reason=%1 server_time=%2 items=%3")
            .arg(QString::fromStdString(response.reason()))
            .arg(response.server_time(), 0, 'f', 3)
            .arg(request.items_size()));
    qInfo() << "AlarmGrpcSnapshotClient: 已推送告警快照 items=" << request.items_size();
    return true;
}
