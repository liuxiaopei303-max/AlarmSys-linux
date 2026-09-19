#include "grpc_target_threat/TargetThreatQueryAssessment.hpp"
#include "dialog/alarm/ThreatAssessmentCalculator.h"
#include "dialog/alarm/AreaEscalationProtectionResolver.h"

#include <cmath>
#include <iostream>

namespace {
namespace proto = ::alarmsys::grpc::target_threat::v1;
namespace alarm = ::trackmanager::grpc::alarm;
using namespace alarmsys::grpc_target_threat;
int failures = 0;
#define CHECK(name, expr) do { if (!(expr)) { \
    std::cerr << "FAIL " << name << " line " << __LINE__ << '\n'; ++failures; \
} } while (false)

bool near(double a, double b) { return std::abs(a - b) < 1e-6; }

AreaInfo rectangle(int group, int id, int level)
{
    AreaInfo area{};
    area.groupID = group;
    area.areaID = id;
    area.areaName = QStringLiteral("query-area-%1-%2").arg(group).arg(id);
    area.areaType = 1;
    area.areaRect = QStringLiteral("37,122,38,123");
    area.waringType = level;
    return area;
}

proto::GetTargetThreatResponse assess(
    const SPxPacketTrackExtended& track, const QString& type,
    const TargetThreatQueryAssessmentConfig& config,
    alarm::EnvironmentType environment = alarm::AIR, bool withAlarm = false)
{
    proto::GetTargetThreatResponse response;
    response.set_target_id("1234567890123");
    response.set_response_time(123.5);
    auto* target = response.mutable_result();
    target->set_environment(environment);
    target->mutable_position()->set_longitude(track.longDegs);
    target->mutable_position()->set_latitude(track.latDegs);
    target->set_speed_mps(track.norm.min.speedMps);
    target->set_course_deg(track.norm.min.courseDegrees);
    if (withAlarm) {
        target->mutable_current_alarm()->set_alarm_id("existing-alarm");
        target->mutable_current_alarm()->set_level(alarm::HIGH);
        target->mutable_current_alarm()->set_content("unchanged-content");
    }
    fillTargetThreatQueryAssessment(track, type, config, 123.5, &response);
    CHECK("找到目标返回QUERY_OK及assessment", response.status() == proto::QUERY_OK
        && response.result().status() == proto::QUERY_OK && response.result().has_assessment());
    CHECK("目标元数据保留", response.target_id() == "1234567890123"
        && response.result().environment() == environment && near(response.response_time(), 123.5));
    CHECK("仅附带原有告警", response.result().has_current_alarm() == withAlarm);
    if (withAlarm) CHECK("原有告警内容不变", response.result().current_alarm().alarm_id() == "existing-alarm"
        && response.result().current_alarm().content() == "unchanged-content"
        && response.result().current_alarm().level() == alarm::HIGH);
    return response;
}
} // namespace

int main()
{
    SPxPacketTrackExtended track{};
    track.latDegs = 37.5;
    track.longDegs = 122.5;
    track.norm.min.speedMps = 17.5F;
    track.norm.min.courseDegrees = 90.0F;
    track.norm.min.rangeMetres = 25.0F;
    TargetThreatQueryAssessmentConfig config;

    const auto outside = assess(track, QStringLiteral("uav"), config, alarm::AIR, true);
    const auto& out = outside.result().assessment();
    CHECK("区外默认权重且未归一化", near(out.type_score().weight(), 0.42)
        && near(out.speed_score().weight(), 0.16) && near(out.total_score(), 50.0));
    CHECK("区外方向距离不计分", near(out.direction_score().raw_score(), 0.0)
        && near(out.direction_score().weight(), 0.0) && near(out.direction_score().weighted_score(), 0.0)
        && near(out.distance_score().raw_score(), 0.0) && near(out.distance_score().weight(), 0.0)
        && near(out.distance_score().weighted_score(), 0.0));
    CHECK("区外不伪造区域和参考点", out.group_id() == 0 && out.area_id() == 0
        && out.area_level() == 0 && out.area_name().empty() && out.rule_id().empty()
        && out.direction_basis() == proto::DIRECTION_BASIS_UNSPECIFIED
        && out.distance_basis() == proto::DISTANCE_BASIS_UNSPECIFIED
        && !out.has_protection_reference_position() && out.protection_reference_source().empty()
        && near(out.distance_m(), 0.0) && near(out.direction_angle_deg(), 0.0));
    const float speeds[] = {0, 4, 5, 17.5F, 30, 31};
    const double scores[] = {42, 42, 42, 50, 58, 58};
    for (int i = 0; i < 6; ++i) {
        track.norm.min.speedMps = speeds[i];
        CHECK("区外速度边界及最高58分", near(assess(track, QStringLiteral("uav"), config)
            .result().assessment().total_score(), scores[i]));
    }
    track.norm.min.speedMps = 0;
    const auto zero = assess(track, QStringLiteral("buoy"), config, alarm::SURFACE);
    CHECK("零分也有评分结果", near(zero.result().assessment().total_score(), 0)
        && zero.result().assessment().grade() == proto::NO_THREAT);
    CHECK("海面类型默认分", near(assess(track, QStringLiteral("ship"), config, alarm::SURFACE)
        .result().assessment().type_score().weighted_score(), 16.8));
    track.norm.min.speedMps = 0.1F;
    const auto slowShip = assess(track, QStringLiteral("fishing"), config, alarm::SURFACE);
    CHECK("200250454低速船样例仍有类型分", near(slowShip.result().assessment().total_score(), 21)
        && near(slowShip.result().assessment().speed_score().weighted_score(), 0));
    track.norm.min.speedMps = 0;
    track.norm.min.reserved1 = 3;
    const auto fallback = assess(track, QString(), config);
    CHECK("沿用可信航迹类型兜底", fallback.result().target_type() == "drone"
        && near(fallback.result().assessment().total_score(), 42));
    track.norm.min.reserved1 = 0;

    track.norm.min.speedMps = 17.5F;
    const auto warning = rectangle(3, 17, 2);
    config.schemeAreas = {warning};
    config.areas = {warning};
    auto inside = assess(track, QStringLiteral("uav"), config);
    CHECK("区内没有威胁规则仍算四项", near(inside.result().assessment().total_score(), 71)
        && inside.result().assessment().area_id() == 17
        && inside.result().assessment().rule_id().empty()
        && inside.result().assessment().direction_basis() == proto::COURSE
        && inside.result().assessment().distance_basis() == proto::TRACK_RANGE);
    // 研判规则只提供来源 ID；识别门槛、航向范围、照片等条件不能阻止按需评分。
    AlarmRule rule{};
    rule.alarmstate = 1;
    rule.group_id = 3;
    rule.area_id = 17;
    rule.alarm_level = 2;
    rule.track_type = 3;
    rule.condition_id = QStringLiteral("z-rule");
    rule.min_track_age_seconds = 1000;
    rule.require_optic_photo = true;
    rule.heading_min = 200;
    rule.heading_max = 210;
    config.rules.insert(rule.condition_id, rule);
    rule.condition_id = QStringLiteral("a-rule");
    config.rules.insert(rule.condition_id, rule);
    const auto withRule = assess(track, QStringLiteral("uav"), config, alarm::AIR, true);
    CHECK("原有评分不因规则门槛改变", near(withRule.result().assessment().total_score(), 71)
        && withRule.result().assessment().rule_id() == "a-rule");
    CHECK("不依赖该环境存在威胁规则", assess(track, QStringLiteral("ship"), config, alarm::SURFACE)
        .result().assessment().area_level() == 2);
    config.rules.clear();

    AreaInfo protect{};
    protect.groupID = 3;
    protect.areaID = 99;
    protect.areaType = 2;
    protect.stratPoint = QStringLiteral("37.51,122.5");
    protect.endPoint = QStringLiteral("37.511,122.5");
    config.areas.append(protect);
    config.schemeAreas[0].protectGroupID = 3;
    config.schemeAreas[0].protectAreaID = 99;
    ThreatAssessmentParams params(3, 17);
    params.maxEffectiveDistance = 5000;
    config.threatParams = {params};
    ThreatAssessmentContext context;
    context.hasProtectArea = true;
    context.protectCenter = QPointF(37.51, 122.5);
    context.directionAngleDeg = AreaEscalationProtectionResolver::attackAngleDegrees(
        track.norm.min.courseDegrees, AreaEscalationProtectionResolver::bearingDegrees(
            QPointF(track.latDegs, track.longDegs), context.protectCenter));
    const auto shared = calculateTargetThreatAssessment(track, QStringLiteral("uav"), params, context);
    const auto protectedResult = assess(track, QStringLiteral("uav"), config);
    CHECK("区内保护区沿用共享几何计算", near(protectedResult.result().assessment().total_score(),
        shared.assessment.totalThreatLevel)
        && protectedResult.result().assessment().direction_basis() == proto::ATTACK_ANGLE
        && protectedResult.result().assessment().distance_basis() == proto::PROTECTION_AREA
        && protectedResult.result().assessment().has_protection_reference_position());

    config.schemeAreas = {warning, rectangle(4, 2, 3), rectangle(3, 15, 3), rectangle(2, 18, 3)};
    config.threatParams.clear();
    const auto tied = assess(track, QStringLiteral("uav"), config);
    CHECK("同分优先级别高及组编号小", tied.result().assessment().group_id() == 2
        && tied.result().assessment().area_id() == 18);
    config.schemeAreas.removeLast();
    CHECK("同组同分优先区域编号小", assess(track, QStringLiteral("uav"), config)
        .result().assessment().area_id() == 15);
    params = ThreatAssessmentParams(3, 17);
    params.typeWeight = 1;
    params.speedWeight = params.angleWeight = params.distanceWeight = 0;
    config.threatParams = {params};
    CHECK("多个区域取最高分", assess(track, QStringLiteral("uav"), config)
        .result().assessment().area_id() == 17);
    params.typeWeight = 0;
    config.schemeAreas = {warning};
    config.threatParams = {params};
    CHECK("区内零权重保留", near(assess(track, QStringLiteral("uav"), config)
        .result().assessment().total_score(), 0));

    track.latDegs = 39;
    config.useBasePoint = true;
    config.basePoint = QPointF(39, track.longDegs);
    track.norm.min.courseDegrees = 0;
    track.norm.min.rangeMetres = 1;
    auto result = assess(track, QStringLiteral("uav"), config);
    CHECK("区外不使用方案参数或参考点", near(result.result().assessment().total_score(), 50)
        && !result.result().assessment().has_protection_reference_position());
    track.norm.min.courseDegrees = 315;
    track.norm.min.rangeMetres = 100000;
    CHECK("区外分数与航向距离无关", near(assess(track, QStringLiteral("uav"), config)
        .result().assessment().total_score(), 50));
    config.schemeAreas = {rectangle(3, 17, 1), rectangle(3, 15, 4), rectangle(3, 99, 0)};
    track.latDegs = 37.5;
    CHECK("保护区免告警区未设置区域不是评分监控区", assess(track, QStringLiteral("uav"), config)
        .result().assessment().area_level() == 0);
    CHECK("输入配置未被修改", near(config.threatParams.first().typeWeight, 0)
        && config.rules.isEmpty());

    // 全局区域包含目标，但不在启用方案里，不能被当作区内评分来源。
    config.schemeAreas.clear();
    config.areas = {warning};
    CHECK("不采用其他方案或全局区域", assess(track, QStringLiteral("uav"), config)
        .result().assessment().area_level() == 0);
    AreaInfo circle = protect;
    circle.waringType = 2;
    track.latDegs = 37.51;
    config.schemeAreas = {circle};
    CHECK("圆形区域无规则也算分", assess(track, QStringLiteral("uav"), config)
        .result().assessment().area_id() == 99);
    AreaInfo polygon = warning;
    polygon.areaType = 3;
    polygon.areaPoints = QStringLiteral("4,37,122,38,122,38,123,37,123");
    config.schemeAreas = {polygon};
    CHECK("多边形区域无规则也算分", assess(track, QStringLiteral("uav"), config)
        .result().assessment().area_id() == 17);
    std::cout << "TargetThreatQueryAssessment tests completed, failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
