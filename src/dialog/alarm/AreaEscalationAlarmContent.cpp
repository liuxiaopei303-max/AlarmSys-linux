#include "AlarmContentBuilder.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString readableEscalationSuffix(const AreaEscalationEvaluator::Result& result)
{
    if (result.reason == QLatin1String("direct_entry"))
        return QStringLiteral("，升级为正式告警：持预警资格进入告警区");
    if (result.reason == QLatin1String("optic"))
        return QStringLiteral("，升级为正式告警：告警区内已有光电取证");
    if (result.reason == QLatin1String("alarm_area_dwell"))
        return QStringLiteral("，升级为正式告警：在告警区连续停留达到阈值");
    if (result.reason == QLatin1String("score")) {
        if (result.stage == AreaEscalationEvaluator::Stage::Prewarning)
            return QStringLiteral("，当前阶段：预警（评分达到预警阈值）");
        if (result.stage == AreaEscalationEvaluator::Stage::Threat)
            return QStringLiteral("，当前阶段：威胁（评分达到威胁阈值）");
    }
    return QString();
}

} // namespace

QString buildAreaEscalationContent(
    const AreaEscalationEvaluator::Result& result,
    const QString& readableRuleContent)
{
    const auto timeText = [](qint64 ms) {
        return ms > 0
            ? QDateTime::fromMSecsSinceEpoch(ms).toString(
                  QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
            : QString();
    };
    const auto pointJson = [](const QPointF& point) {
        QJsonObject object;
        object.insert(QStringLiteral("lat"), point.x());
        object.insert(QStringLiteral("lon"), point.y());
        return object;
    };

    QJsonObject object;
    if (!readableRuleContent.trimmed().isEmpty()) {
        QJsonParseError error;
        const QJsonDocument readableDocument =
            QJsonDocument::fromJson(readableRuleContent.toUtf8(), &error);
        if (error.error == QJsonParseError::NoError && readableDocument.isObject())
            object = readableDocument.object();
    }
    if (object.value(QStringLiteral("summary")).toString().trimmed().isEmpty()) {
        object.insert(
            QStringLiteral("summary"),
            QStringLiteral("目标 %1 当前阶段 %2，升级原因 %3")
                .arg(result.targetId)
                .arg(AreaEscalationEvaluator::stageName(result.stage), result.reason));
    }
    const QString readableSuffix = readableEscalationSuffix(result);
    if (!readableSuffix.isEmpty()) {
        QString summary = object.value(QStringLiteral("summary")).toString();
        if (!summary.contains(readableSuffix)) {
            summary += readableSuffix;
            object.insert(QStringLiteral("summary"), summary);
        }
    }

    object.insert(QStringLiteral("v"), 2);
    object.insert(QStringLiteral("unique_id"), result.targetId);
    object.insert(
        QStringLiteral("domain"),
        result.domain == AreaEscalationEvaluator::TargetDomain::Air
            ? QStringLiteral("air") : QStringLiteral("sea"));
    object.insert(QStringLiteral("lane_id"), result.laneId);
    object.insert(QStringLiteral("track_type"), result.trackType);
    object.insert(
        QStringLiteral("stage"), AreaEscalationEvaluator::stageName(result.stage));
    object.insert(
        QStringLiteral("level"), AreaEscalationEvaluator::stageName(result.stage));
    object.insert(QStringLiteral("upgrade_reason"), result.reason);
    object.insert(
        QStringLiteral("disposition"),
        AreaEscalationEvaluator::dispositionName(result.disposition));
    object.insert(QStringLiteral("score"), result.score);
    object.insert(QStringLiteral("threat_threshold"), result.threatThreshold);
    object.insert(QStringLiteral("prewarning_threshold"), result.prewarningThreshold);
    object.insert(QStringLiteral("condition_id"), result.conditionId);
    object.insert(QStringLiteral("qualification_group_id"), result.qualificationArea.groupId);
    object.insert(QStringLiteral("qualification_area_id"), result.qualificationArea.areaId);
    object.insert(QStringLiteral("event_group_id"), result.eventArea.groupId);
    object.insert(QStringLiteral("event_area_id"), result.eventArea.areaId);
    object.insert(QStringLiteral("inside_warning"), result.insideWarning);
    object.insert(QStringLiteral("inside_alarm"), result.insideAlarm);
    object.insert(QStringLiteral("qualified"), result.qualified);
    object.insert(QStringLiteral("qualification_time"), timeText(result.qualificationTimeMs));
    object.insert(QStringLiteral("alarm_entry_time"), timeText(result.alarmEntryTimeMs));
    object.insert(QStringLiteral("alarm_dwell_ms"), result.alarmDwellMs);
    object.insert(QStringLiteral("track_duration"), result.alarmDwellMs / 1000.0);
    object.insert(QStringLiteral("warning_to_alarm_ms"), result.warningToAlarmMs);
    object.insert(QStringLiteral("course_deg"), result.courseDeg);
    object.insert(QStringLiteral("speed_mps"), result.speedMps);
    object.insert(QStringLiteral("hard_conditions"), result.hardConditions);
    object.insert(QStringLiteral("entry_geometry"), result.entryGeometry);
    object.insert(QStringLiteral("current_position"), pointJson(result.currentPosition));
    if (result.hasPreviousPosition)
        object.insert(QStringLiteral("previous_position"), pointJson(result.previousPosition));
    else
        object.insert(QStringLiteral("previous_position"), QJsonValue::Null);

    return QString::fromUtf8(QJsonDocument(object).toJson(QJsonDocument::Compact));
}
