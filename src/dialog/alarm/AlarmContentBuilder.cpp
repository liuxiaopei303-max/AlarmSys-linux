#include "AlarmContentBuilder.h"

#include "customconfig.h"

#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>

namespace {

QString targetTypeToChinese(const QString& raw)
{
    if (raw.isEmpty()) {
        return QString();
    }
    static const QHash<QString, QString> kMap = {
        {QStringLiteral("ship"), QStringLiteral("船舶")},
        {QStringLiteral("speedboat"), QStringLiteral("快艇")},
        {QStringLiteral("yacht"), QStringLiteral("游艇")},
        {QStringLiteral("buoy"), QStringLiteral("浮标")},
        {QStringLiteral("cargoship"), QStringLiteral("货轮")},
        {QStringLiteral("cargo"), QStringLiteral("货轮")},
        {QStringLiteral("fishingboat"), QStringLiteral("渔船")},
        {QStringLiteral("fishing"), QStringLiteral("渔船")},
        {QStringLiteral("warship"), QStringLiteral("军舰")},
        {QStringLiteral("motorboat"), QStringLiteral("摩托艇")},
        {QStringLiteral("uav"), QStringLiteral("无人机")},
        {QStringLiteral("drone"), QStringLiteral("无人机")},
        {QStringLiteral("missile"), QStringLiteral("巡飞弹")},
        {QStringLiteral("missle"), QStringLiteral("巡飞弹")},
        {QStringLiteral("bird"), QStringLiteral("鸟类")},
        {QStringLiteral("aircraft"), QStringLiteral("飞机")},
        {QStringLiteral("other"), QStringLiteral("其他目标")},
        {QStringLiteral("unknown"), QStringLiteral("未知目标")},
    };
    const QString key = raw.trimmed().toLower();
    const auto it = kMap.constFind(key);
    if (it != kMap.constEnd()) {
        return it.value();
    }
    return raw;
}

QString resolveTargetTypeRaw(
    CustomConfig* cfg, qint64 uniqueId, const DataAccessLayer::DetectionTypeResult* detection)
{
    if (detection && detection->found) {
        if (!detection->finalTargetType.isEmpty()) {
            return detection->finalTargetType.trimmed().toLower();
        }
        if (!detection->camTargetType.isEmpty()) {
            return detection->camTargetType.trimmed().toLower();
        }
        if (!detection->llmTargetType.isEmpty()) {
            return detection->llmTargetType.trimmed().toLower();
        }
    }

    if (cfg != nullptr) {
        const DataAccessLayer::DetectionTypeResult dbDet =
            cfg->dbHelper.getDetectionTypesByReId(uniqueId);
        if (dbDet.found) {
            if (!dbDet.finalTargetType.isEmpty()) {
                return dbDet.finalTargetType.trimmed().toLower();
            }
            if (!dbDet.camTargetType.isEmpty()) {
                return dbDet.camTargetType.trimmed().toLower();
            }
            if (!dbDet.llmTargetType.isEmpty()) {
                return dbDet.llmTargetType.trimmed().toLower();
            }
        }
    }

    return QStringLiteral("unknown");
}

QString resolveTargetTypeLabel(
    CustomConfig* cfg, qint64 uniqueId, const DataAccessLayer::DetectionTypeResult* detection)
{
    const QString raw = resolveTargetTypeRaw(cfg, uniqueId, detection);
    const QString label = targetTypeToChinese(raw);
    if (!label.isEmpty()) {
        return label;
    }

    if (cfg != nullptr) {
        const int typeId = cfg->m_mapTargetType.value(QString::number(uniqueId), 0);
        if (typeId > 0) {
            const QStringList dict = cfg->dbHelper.getDictTargetTypes();
            if (typeId > 0 && typeId <= dict.size()) {
                return dict.at(typeId - 1);
            }
        }
    }

    return QStringLiteral("未知类型");
}

bool speedSatisfiesRule(double speedMps, int speedCondition, int speedThreshold)
{
    if (speedCondition <= 0) {
        return false;
    }
    if (speedCondition == 1) {
        return speedMps < speedThreshold;
    }
    if (speedCondition == 2) {
        return speedMps > speedThreshold;
    }
    return false;
}

QString formatSpeedRuleText(int speedCondition, int speedThreshold)
{
    switch (speedCondition) {
    case 1:
        return QStringLiteral("速度小于 %1 m/s").arg(speedThreshold);
    case 2:
        return QStringLiteral("速度大于 %1 m/s").arg(speedThreshold);
    default:
        return QString();
    }
}

bool courseEnabled(const AlarmRule& rule)
{
    if (rule.course_min == rule.course_max) {
        return false;
    }
    if (rule.course_min == 0 && rule.course_max == 360) {
        return false;
    }
    return true;
}

bool courseSatisfiesRule(double angleDeg, const AlarmRule& rule)
{
    if (!courseEnabled(rule)) {
        return false;
    }
    if (rule.course_min <= rule.course_max) {
        return angleDeg >= rule.course_min && angleDeg <= rule.course_max;
    }
    return angleDeg >= rule.course_min || angleDeg <= rule.course_max;
}

QString formatCourseRuleText(const AlarmRule& rule, bool hasProtectArea)
{
    if (!courseEnabled(rule)) {
        return QString();
    }
    if (hasProtectArea) {
        return QStringLiteral("进攻方向在 %1~%2 度").arg(rule.course_min).arg(rule.course_max);
    }
    return QStringLiteral("航向在 %1~%2 度").arg(rule.course_min).arg(rule.course_max);
}

bool heightEnabled(const AlarmRule& rule)
{
    if (rule.height_min == rule.height_max) {
        return false;
    }
    if (rule.height_min == 0 && (rule.height_max == 100 || rule.height_max == 1000)) {
        return false;
    }
    return true;
}

bool heightSatisfiesRule(double heightM, const AlarmRule& rule)
{
    if (!heightEnabled(rule)) {
        return false;
    }
    return heightM >= rule.height_min && heightM <= rule.height_max;
}

QString formatHeightRuleText(const AlarmRule& rule)
{
    if (!heightEnabled(rule)) {
        return QString();
    }
    return QStringLiteral("高度 %1~%2 m").arg(rule.height_min).arg(rule.height_max);
}

QString formatRuleTriggerSummary(const AlarmContentBuildInput& input)
{
    if (input.triggerPath == QLatin1String("blacklist")) {
        return QStringLiteral("黑名单目标");
    }
    if (input.rule == nullptr) {
        return QStringLiteral("区域/规则条件满足");
    }

    const AlarmRule& rule = *input.rule;
    QStringList parts;
    const double angleValue = input.hasProtectArea ? input.attackAngleDeg : input.courseDeg;

    if (speedSatisfiesRule(input.speedMps, rule.speed_condition, rule.speed)) {
        parts << formatSpeedRuleText(rule.speed_condition, rule.speed);
    }
    if (courseSatisfiesRule(angleValue, rule)) {
        parts << formatCourseRuleText(rule, input.hasProtectArea);
    }
    if (heightSatisfiesRule(input.heightM, rule)) {
        parts << formatHeightRuleText(rule);
    }
    if (rule.camera_detect_type > 0 && input.hasDetection
        && !input.detection.camTargetType.isEmpty()) {
        parts << QStringLiteral("光电检测目标类型匹配");
    }
    if (rule.llm_detect_type > 0 && input.hasDetection
        && !input.detection.llmTargetType.isEmpty()) {
        parts << QStringLiteral("大模型研判目标类型匹配");
    }
    if (rule.uav_detect_type > 0 && input.hasDetection
        && !input.detection.uavTargetType.isEmpty()) {
        parts << QStringLiteral("无人机检测目标类型匹配");
    }
    if (rule.targetattr_type > 0) {
        parts << QStringLiteral("目标属性类型匹配");
    }
    if (rule.dist_to_protect_area >= 0) {
        parts << QStringLiteral("距保护区小于 %1 m").arg(rule.dist_to_protect_area);
    }
    if (rule.threat_level1 > 0 && input.threatScore >= rule.threat_level1) {
        parts << QStringLiteral("威胁度≥%1").arg(rule.threat_level1);
    }

    if (parts.isEmpty() && !rule.name.isEmpty()) {
        parts << rule.name;
    }
    if (parts.isEmpty()) {
        return QStringLiteral("区域/规则条件满足");
    }
    return parts.join(QStringLiteral("，"));
}

QString formatThreatBreakdownSuffix(const AlarmContentBuildInput& input)
{
    if (!input.hasThreatBreakdown) {
        return QString();
    }
    const ThreatAssessmentResult& t = input.threatBreakdown;
    const QString angleName =
        input.hasProtectArea ? QStringLiteral("进攻方向") : QStringLiteral("航向");
    return QStringLiteral("，威胁分：总分 %1（类型 %2，速度 %3，%4 %5，距离 %6）")
        .arg(t.totalThreatLevel, 0, 'f', 1)
        .arg(t.weightedTypeScore, 0, 'f', 1)
        .arg(t.weightedSpeedScore, 0, 'f', 1)
        .arg(angleName)
        .arg(t.weightedAngleScore, 0, 'f', 1)
        .arg(t.weightedDistanceScore, 0, 'f', 1);
}

QString formatEoVerificationSuffix(
    CustomConfig* cfg, qint64 uniqueId, const DataAccessLayer::DetectionTypeResult* detection)
{
    DataAccessLayer::DetectionTypeResult det;
    if (detection && detection->found) {
        det = *detection;
    } else if (cfg != nullptr) {
        det = cfg->dbHelper.getDetectionTypesByReId(uniqueId);
    }

    if (!det.camTargetType.isEmpty()) {
        return QStringLiteral("，光电取证确认为「%1」")
            .arg(targetTypeToChinese(det.camTargetType));
    }
    if (!det.llmTargetType.isEmpty()) {
        return QStringLiteral("，大模型研判为「%1」")
            .arg(targetTypeToChinese(det.llmTargetType));
    }
    if (cfg != nullptr
        && cfg->dbHelper.hasMinioMultiMetadataForUniqueId(uniqueId, QStringLiteral("%"))) {
        return QStringLiteral("，并经过光电取证");
    }
    return QString();
}

void resolveScoreMaxes(
    const AlarmContentBuildInput& input, double* typeMax, double* speedMax, double* angleMax,
    double* distanceMax)
{
    // 默认权重：0.42 / 0.16 / 0.18 / 0.24 → 满分贡献 42/16/18/24
    *typeMax = 42.0;
    *speedMax = 16.0;
    *angleMax = 18.0;
    *distanceMax = 24.0;

    if (input.cfg == nullptr || input.rule == nullptr) {
        return;
    }
    for (const ThreatAssessmentParams& params : input.cfg->m_listThreatAssessmentParams) {
        if (params.groupId == input.rule->group_id && params.areaId == input.rule->area_id) {
            *typeMax = params.typeWeight * 100.0;
            *speedMax = params.speedWeight * 100.0;
            *angleMax = params.angleWeight * 100.0;
            *distanceMax = params.distanceWeight * 100.0;
            return;
        }
    }
}

void appendImageFields(QJsonObject& obj, CustomConfig* cfg, qint64 uniqueId)
{
    if (cfg == nullptr || uniqueId <= 0) {
        return;
    }
    const DataAccessLayer::MinioMetadataResult meta =
        cfg->dbHelper.getLatestMinioMetadataByUniqueId(uniqueId);
    if (!meta.found) {
        return;
    }
    obj.insert(QStringLiteral("image_bucket"), meta.minioBucket);
    obj.insert(QStringLiteral("image_object_key"), meta.minioObjectKey);
    if (!meta.downloadUrl.isEmpty()) {
        obj.insert(QStringLiteral("image_download_url"), meta.downloadUrl);
    }
    if (!meta.cameraIndex.isEmpty()) {
        obj.insert(QStringLiteral("image_camera_index"), meta.cameraIndex);
    }
    if (!meta.uploadedAt.isEmpty()) {
        obj.insert(QStringLiteral("image_uploaded_at"), meta.uploadedAt);
    }
}

QString toCompactJson(const QJsonObject& obj)
{
    return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

} // namespace

QString buildRuleAlarmContent(const AlarmContentBuildInput& input)
{
    const DataAccessLayer::DetectionTypeResult* detPtr =
        input.hasDetection ? &input.detection : nullptr;
    const QString domain = input.isAirTrack ? QStringLiteral("空中") : QStringLiteral("海上");
    const QString domainKey = input.isAirTrack ? QStringLiteral("air") : QStringLiteral("sea");
    const QString typeRaw = resolveTargetTypeRaw(input.cfg, input.uniqueId, detPtr);
    const QString typeLabel = resolveTargetTypeLabel(input.cfg, input.uniqueId, detPtr);
    const QString angleLabel =
        input.hasProtectArea ? QStringLiteral("进攻方向") : QStringLiteral("航向");
    const double angleValue = input.hasProtectArea ? input.attackAngleDeg : input.courseDeg;

    QString summary = QStringLiteral("%1目标%2（ID %3）速度 %4 m/s，%5 %6 度")
                          .arg(domain)
                          .arg(typeLabel)
                          .arg(input.uniqueId)
                          .arg(input.speedMps, 0, 'f', 1)
                          .arg(angleLabel)
                          .arg(angleValue, 0, 'f', 1);
    summary += QStringLiteral("，触发告警规则：") + formatRuleTriggerSummary(input);
    summary += formatThreatBreakdownSuffix(input);
    summary += formatEoVerificationSuffix(input.cfg, input.uniqueId, detPtr);

    QJsonObject obj;
    obj.insert(QStringLiteral("v"), 1);
    obj.insert(QStringLiteral("summary"), summary);
    obj.insert(QStringLiteral("unique_id"), static_cast<qint64>(input.uniqueId));
    obj.insert(QStringLiteral("domain"), domainKey);
    obj.insert(QStringLiteral("target_type"), typeRaw);
    obj.insert(QStringLiteral("target_type_label"), typeLabel);
    obj.insert(QStringLiteral("angle_label"), angleLabel);

    if (input.hasThreatBreakdown) {
        const ThreatAssessmentResult& t = input.threatBreakdown;
        double typeMax = 42.0;
        double speedMax = 16.0;
        double angleMax = 18.0;
        double distanceMax = 24.0;
        resolveScoreMaxes(input, &typeMax, &speedMax, &angleMax, &distanceMax);

        obj.insert(QStringLiteral("threat_total"), t.totalThreatLevel);
        obj.insert(QStringLiteral("score_type"), t.weightedTypeScore);
        obj.insert(QStringLiteral("score_speed"), t.weightedSpeedScore);
        obj.insert(QStringLiteral("score_angle"), t.weightedAngleScore);
        obj.insert(QStringLiteral("score_distance"), t.weightedDistanceScore);
        obj.insert(QStringLiteral("score_max_type"), typeMax);
        obj.insert(QStringLiteral("score_max_speed"), speedMax);
        obj.insert(QStringLiteral("score_max_angle"), angleMax);
        obj.insert(QStringLiteral("score_max_distance"), distanceMax);
    } else {
        obj.insert(QStringLiteral("threat_total"), QJsonValue::Null);
        obj.insert(QStringLiteral("score_type"), QJsonValue::Null);
        obj.insert(QStringLiteral("score_speed"), QJsonValue::Null);
        obj.insert(QStringLiteral("score_angle"), QJsonValue::Null);
        obj.insert(QStringLiteral("score_distance"), QJsonValue::Null);
    }

    appendImageFields(obj, input.cfg, input.uniqueId);
    return toCompactJson(obj);
}

QString buildManualAlarmContent(
    CustomConfig* cfg, qint64 uniqueId, bool isAirTrack, double speedMps, double courseDeg)
{
    Q_UNUSED(speedMps);
    Q_UNUSED(courseDeg);
    const QString domain = isAirTrack ? QStringLiteral("空中") : QStringLiteral("海上");
    const QString domainKey = isAirTrack ? QStringLiteral("air") : QStringLiteral("sea");
    const QString typeRaw = resolveTargetTypeRaw(cfg, uniqueId, nullptr);
    const QString typeLabel = resolveTargetTypeLabel(cfg, uniqueId, nullptr);
    const QString summary = QStringLiteral("%1目标%2（ID %3）由用户判定为威胁目标")
                                .arg(domain)
                                .arg(typeLabel)
                                .arg(uniqueId);

    QJsonObject obj;
    obj.insert(QStringLiteral("v"), 1);
    obj.insert(QStringLiteral("summary"), summary);
    obj.insert(QStringLiteral("unique_id"), static_cast<qint64>(uniqueId));
    obj.insert(QStringLiteral("domain"), domainKey);
    obj.insert(QStringLiteral("target_type"), typeRaw);
    obj.insert(QStringLiteral("target_type_label"), typeLabel);
    obj.insert(QStringLiteral("threat_total"), QJsonValue::Null);
    obj.insert(QStringLiteral("score_type"), QJsonValue::Null);
    obj.insert(QStringLiteral("score_speed"), QJsonValue::Null);
    obj.insert(QStringLiteral("score_angle"), QJsonValue::Null);
    obj.insert(QStringLiteral("score_distance"), QJsonValue::Null);
    appendImageFields(obj, cfg, uniqueId);
    return toCompactJson(obj);
}
