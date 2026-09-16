#include "AlarmHttpSnapshotJson.hpp"

#include "alarm_service.pb.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace alarmsys::http_alarm {
namespace {

using namespace trackmanager::grpc::alarm;

QString fromStdString(const std::string& value)
{
    return QString::fromStdString(value);
}

QJsonObject geoPositionToJson(const GeoPosition& position)
{
    QJsonObject out;
    out.insert(QStringLiteral("longitude"), position.longitude());
    out.insert(QStringLiteral("latitude"), position.latitude());
    out.insert(QStringLiteral("altitude"), position.altitude());
    out.insert(QStringLiteral("altitude_agl_m"), position.altitude_agl_m());
    out.insert(QStringLiteral("altitude_afs_m"), position.altitude_afs_m());
    out.insert(QStringLiteral("pressure_depth_m"), position.pressure_depth_m());
    out.insert(QStringLiteral("is_2d"), position.is_2d());
    out.insert(
        QStringLiteral("altitude_ref"),
        fromStdString(AltitudeReference_Name(position.altitude_ref())));
    return out;
}

QJsonObject measurementToJson(const Measurement& measurement)
{
    QJsonObject out;
    out.insert(QStringLiteral("value"), measurement.value());
    out.insert(QStringLiteral("sigma"), measurement.sigma());
    return out;
}

QJsonObject entityRefToJson(const EntityRef& entity)
{
    QJsonObject out;
    out.insert(QStringLiteral("entity_type"), fromStdString(entity.entity_type()));
    out.insert(QStringLiteral("entity_id"), fromStdString(entity.entity_id()));
    out.insert(QStringLiteral("entity_name"), fromStdString(entity.entity_name()));
    return out;
}

QJsonObject areaToJson(const AlarmAreaRef& area)
{
    QJsonObject out;
    out.insert(QStringLiteral("area_id"), fromStdString(area.area_id()));
    out.insert(QStringLiteral("area_name"), fromStdString(area.area_name()));
    return out;
}

QJsonObject detectionBoxToJson(const EODetectionBox& box)
{
    QJsonObject out;
    out.insert(QStringLiteral("camera_id"), fromStdString(box.camera_id()));
    // Qt 5 的 JSON 数字使用 double。可能超过 2^53 的无符号 ID 以字符串输出，避免精度丢失。
    out.insert(QStringLiteral("sync_header"), QString::number(box.sync_header()));
    out.insert(QStringLiteral("box_id"), QString::number(box.box_id()));
    out.insert(QStringLiteral("track_id"), QString::number(box.track_id()));
    out.insert(QStringLiteral("class_id"), static_cast<double>(box.class_id()));
    out.insert(QStringLiteral("behavior_id"), static_cast<double>(box.behavior_id()));
    out.insert(QStringLiteral("x"), box.x());
    out.insert(QStringLiteral("y"), box.y());
    out.insert(QStringLiteral("width"), box.width());
    out.insert(QStringLiteral("height"), box.height());
    out.insert(QStringLiteral("center_x"), box.center_x());
    out.insert(QStringLiteral("center_y"), box.center_y());
    out.insert(QStringLiteral("confidence"), box.confidence());
    return out;
}

QJsonObject spatialInfoToJson(const AlarmSpatialInfo& spatial)
{
    QJsonObject out;
    out.insert(
        QStringLiteral("location_type"),
        fromStdString(AlarmLocationType_Name(spatial.location_type())));
    out.insert(QStringLiteral("point_position_set"), spatial.point_position_set());
    if (spatial.point_position_set() && spatial.has_point_position()) {
        out.insert(QStringLiteral("point_position"), geoPositionToJson(spatial.point_position()));
    }
    out.insert(QStringLiteral("reference_position_set"), spatial.reference_position_set());
    if (spatial.reference_position_set() && spatial.has_reference_position()) {
        out.insert(
            QStringLiteral("reference_position"),
            geoPositionToJson(spatial.reference_position()));
    }
    out.insert(QStringLiteral("bearing_deg"), spatial.bearing_deg());
    out.insert(QStringLiteral("bearing_sigma_deg"), spatial.bearing_sigma_deg());
    out.insert(QStringLiteral("range_estimate_set"), spatial.range_estimate_set());
    if (spatial.range_estimate_set() && spatial.has_range_estimate_m()) {
        out.insert(
            QStringLiteral("range_estimate_m"),
            measurementToJson(spatial.range_estimate_m()));
    }
    out.insert(QStringLiteral("range_min_set"), spatial.range_min_set());
    if (spatial.range_min_set() && spatial.has_range_min_m()) {
        out.insert(QStringLiteral("range_min_m"), measurementToJson(spatial.range_min_m()));
    }
    out.insert(QStringLiteral("range_max_set"), spatial.range_max_set());
    if (spatial.range_max_set() && spatial.has_range_max_m()) {
        out.insert(QStringLiteral("range_max_m"), measurementToJson(spatial.range_max_m()));
    }
    out.insert(QStringLiteral("sector_start_deg"), spatial.sector_start_deg());
    out.insert(QStringLiteral("sector_end_deg"), spatial.sector_end_deg());
    out.insert(QStringLiteral("spatial_confidence"), spatial.spatial_confidence());
    if (spatial.has_source_sensor()) {
        out.insert(QStringLiteral("source_sensor"), entityRefToJson(spatial.source_sensor()));
    }
    return out;
}

QJsonObject contentToJson(const std::string& rawContent)
{
    const QByteArray raw = QByteArray::fromStdString(rawContent);
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(raw, &error);
    if (error.error == QJsonParseError::NoError && document.isObject()) {
        return document.object();
    }

    const QString text = QString::fromUtf8(raw);
    QJsonObject out;
    out.insert(QStringLiteral("summary"), text);
    out.insert(QStringLiteral("raw"), text);
    return out;
}

QJsonObject alarmToJson(const AlarmItem& alarm)
{
    QJsonObject out;
    out.insert(QStringLiteral("alarm_id"), fromStdString(alarm.alarm_id()));

    QJsonArray categories;
    for (int i = 0; i < alarm.categories_size(); ++i) {
        categories.append(fromStdString(AlarmCategory_Name(alarm.categories(i))));
    }
    out.insert(QStringLiteral("categories"), categories);
    out.insert(
        QStringLiteral("status"),
        fromStdString(AlarmLifecycleStatus_Name(alarm.status())));
    out.insert(
        QStringLiteral("disposition"),
        fromStdString(AlarmDispositionStatus_Name(alarm.disposition())));
    out.insert(QStringLiteral("content"), contentToJson(alarm.content()));
    out.insert(QStringLiteral("level"), fromStdString(ThreatLevel_Name(alarm.level())));

    if (alarm.has_area()) {
        out.insert(QStringLiteral("area"), areaToJson(alarm.area()));
    }
    out.insert(QStringLiteral("class_id"), static_cast<double>(alarm.class_id()));
    out.insert(QStringLiteral("behavior_id"), static_cast<double>(alarm.behavior_id()));
    if (alarm.has_position()) {
        out.insert(QStringLiteral("position"), geoPositionToJson(alarm.position()));
    }
    if (alarm.has_spatial_info()) {
        out.insert(QStringLiteral("spatial_info"), spatialInfoToJson(alarm.spatial_info()));
    }

    out.insert(QStringLiteral("raised_time"), alarm.raised_time());
    out.insert(QStringLiteral("updated_time"), alarm.updated_time());
    out.insert(QStringLiteral("resolved_time"), alarm.resolved_time());
    out.insert(QStringLiteral("resolved_by"), fromStdString(alarm.resolved_by()));
    out.insert(
        QStringLiteral("resolution_details"),
        fromStdString(alarm.resolution_details()));

    QJsonArray ruleIds;
    for (int i = 0; i < alarm.rule_ids_size(); ++i) {
        ruleIds.append(fromStdString(alarm.rule_ids(i)));
    }
    out.insert(QStringLiteral("rule_ids"), ruleIds);
    out.insert(QStringLiteral("detection_box_set"), alarm.detection_box_set());
    if (alarm.detection_box_set() && alarm.has_detection_box()) {
        out.insert(QStringLiteral("detection_box"), detectionBoxToJson(alarm.detection_box()));
    }
    return out;
}

QJsonObject recordToJson(const TargetAlarmRecord& record)
{
    QJsonObject out;
    out.insert(
        QStringLiteral("environment"),
        fromStdString(EnvironmentType_Name(record.environment())));
    out.insert(QStringLiteral("target_id"), fromStdString(record.target_id()));
    out.insert(QStringLiteral("producer_id"), fromStdString(record.producer_id()));
    out.insert(QStringLiteral("update_time"), record.update_time());
    if (record.has_alarm()) {
        out.insert(QStringLiteral("alarm"), alarmToJson(record.alarm()));
    }
    return out;
}

} // namespace

QByteArray serializeAlarmSnapshot(
    const AlarmSnapshotRequest& snapshot,
    double snapshotTimeSec,
    const QString& fallbackProducerId)
{
    QString producerId = fallbackProducerId;
    if (snapshot.items_size() > 0 && !snapshot.items(0).producer_id().empty()) {
        producerId = fromStdString(snapshot.items(0).producer_id());
    }

    QJsonObject root;
    root.insert(QStringLiteral("schema_version"), 1);
    root.insert(QStringLiteral("snapshot_time"), snapshotTimeSec);
    root.insert(QStringLiteral("producer_id"), producerId);

    QJsonArray items;
    for (int i = 0; i < snapshot.items_size(); ++i) {
        items.append(recordToJson(snapshot.items(i)));
    }
    root.insert(QStringLiteral("items"), items);

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

} // namespace alarmsys::http_alarm
