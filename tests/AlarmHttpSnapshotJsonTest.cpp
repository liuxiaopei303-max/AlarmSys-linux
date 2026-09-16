#include "http_alarm/AlarmHttpSnapshotFilter.hpp"
#include "http_alarm/AlarmHttpSnapshotJson.hpp"

#include "alarm_service.pb.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

using namespace trackmanager::grpc::alarm;

AlarmSnapshotRequest buildSnapshot()
{
    AlarmSnapshotRequest snapshot;
    TargetAlarmRecord* record = snapshot.add_items();
    record->set_environment(EnvironmentType::SURFACE);
    record->set_target_id("413000001");
    record->set_producer_id("AlarmSys-linux");
    record->set_update_time(1780000000.125);

    AlarmItem* alarm = record->mutable_alarm();
    alarm->set_alarm_id("alarm-1");
    alarm->add_categories(AlarmCategory::TRACK_AREA);
    alarm->set_status(AlarmLifecycleStatus::ACTIVE);
    alarm->set_disposition(AlarmDispositionStatus::VERIFY_SUCCESS);
    alarm->set_content(
        R"({"v":1,"summary":"目标触发告警","threat_total":82.5})");
    alarm->set_level(ThreatLevel::HIGH);
    alarm->mutable_area()->set_area_id("1_2");
    alarm->mutable_area()->set_area_name("核心区");
    alarm->set_class_id(7);
    alarm->set_behavior_id(3);
    alarm->mutable_position()->set_longitude(121.25);
    alarm->mutable_position()->set_latitude(31.5);
    alarm->mutable_position()->set_is_2d(true);
    alarm->mutable_position()->set_altitude_ref(AltitudeReference::HEIGHT_ABOVE_WGS84);
    AlarmSpatialInfo* spatial = alarm->mutable_spatial_info();
    spatial->set_location_type(AlarmLocationType::POINT);
    spatial->set_point_position_set(true);
    spatial->mutable_point_position()->CopyFrom(alarm->position());
    spatial->set_bearing_deg(90.5);
    spatial->set_spatial_confidence(1.0);
    alarm->set_raised_time(1779999999.0);
    alarm->set_updated_time(1780000000.0);
    alarm->set_resolved_time(1780000001.0);
    alarm->add_rule_ids("rule-a");
    // 正式告警即使同时带有可疑目标标记，也必须继续发送。
    alarm->add_rule_ids("suspicious_target");
    alarm->set_detection_box_set(false);

    TargetAlarmRecord* suspicious = snapshot.add_items();
    suspicious->set_environment(EnvironmentType::AIR);
    suspicious->set_target_id("44553");
    suspicious->set_producer_id("AlarmSys-linux");
    suspicious->set_update_time(1780000000.125);
    suspicious->mutable_alarm()->set_alarm_id("suspicious_44553");
    suspicious->mutable_alarm()->add_categories(AlarmCategory::CUSTOM1);
    suspicious->mutable_alarm()->set_content("SuspiciousTarget");
    suspicious->mutable_alarm()->set_level(ThreatLevel::MEDIUM);
    suspicious->mutable_alarm()->add_rule_ids("suspicious_target");
    return snapshot;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    const AlarmSnapshotRequest source = buildSnapshot();
    const AlarmSnapshotRequest filtered = alarmsys::http_alarm::keepHighAlarms(source);
    CHECK("HTTP只保留HIGH正式告警", filtered.items_size() == 1);
    CHECK("带可疑标记的HIGH正式告警仍保留",
          filtered.items(0).alarm().rule_ids_size() == 2);

    const QByteArray json = alarmsys::http_alarm::serializeAlarmSnapshot(
        filtered, 1780000000.5, QStringLiteral("fallback-producer"));
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(json, &error);
    CHECK("输出是合法 JSON 对象", error.error == QJsonParseError::NoError && document.isObject());

    const QJsonObject root = document.object();
    CHECK("顶层版本固定为1", root.value("schema_version").toInt() == 1);
    CHECK("顶层时间保留毫秒小数", root.value("snapshot_time").toDouble() == 1780000000.5);
    CHECK("优先使用快照中的生产者", root.value("producer_id").toString() == "AlarmSys-linux");
    const QJsonArray items = root.value("items").toArray();
    CHECK("预警和可疑目标不输出", items.size() == 1);

    const QJsonObject first = items.at(0).toObject();
    CHECK("环境枚举输出名称", first.value("environment").toString() == "SURFACE");
    CHECK("目标ID保持字符串", first.value("target_id").toString() == "413000001");
    const QJsonObject alarm = first.value("alarm").toObject();
    CHECK("告警级别输出名称", alarm.value("level").toString() == "HIGH");
    CHECK("处置状态输出名称", alarm.value("disposition").toString() == "VERIFY_SUCCESS");
    CHECK("content JSON字符串转换为对象",
          alarm.value("content").toObject().value("threat_total").toDouble() == 82.5);
    CHECK("区域信息输出", alarm.value("area").toObject().value("area_name").toString() == "核心区");
    CHECK("点位和方位信息输出",
          alarm.value("spatial_info").toObject().value("bearing_deg").toDouble() == 90.5);
    CHECK("规则ID输出",
          alarm.value("rule_ids").toArray().at(0).toString() == "rule-a");

    AlarmSnapshotRequest mediumOnly;
    *mediumOnly.add_items() = source.items(1);
    CHECK("只有MEDIUM预警时过滤为空",
          alarmsys::http_alarm::keepHighAlarms(mediumOnly).items_size() == 0);

    AlarmSnapshotRequest rawContentSource;
    TargetAlarmRecord* rawRecord = rawContentSource.add_items();
    rawRecord->set_environment(EnvironmentType::AIR);
    rawRecord->set_target_id("9001");
    rawRecord->set_producer_id("AlarmSys-linux");
    rawRecord->mutable_alarm()->set_level(ThreatLevel::HIGH);
    rawRecord->mutable_alarm()->set_content("plain alarm content");
    const QJsonObject rawAlarm = QJsonDocument::fromJson(
        alarmsys::http_alarm::serializeAlarmSnapshot(
            alarmsys::http_alarm::keepHighAlarms(rawContentSource),
            1780000000.75,
            QStringLiteral("fallback-producer")))
                                     .object()
                                     .value("items")
                                     .toArray()
                                     .at(0)
                                     .toObject()
                                     .value("alarm")
                                     .toObject();
    CHECK("HIGH告警普通字符串content包装为对象",
          rawAlarm.value("content").toObject().value("raw").toString()
              == "plain alarm content");
    CHECK("未设置的复合字段不伪造", !rawAlarm.contains("position"));

    AlarmSnapshotRequest empty;
    const QJsonObject emptyRoot = QJsonDocument::fromJson(
        alarmsys::http_alarm::serializeAlarmSnapshot(
            empty, 1780000001.0, QStringLiteral("configured-producer")))
                                      .object();
    CHECK("空快照仍保留配置生产者",
          emptyRoot.value("producer_id").toString() == "configured-producer");
    CHECK("空快照输出空items", emptyRoot.value("items").toArray().isEmpty());

    qInfo() << "AlarmHttpSnapshotJson tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
