#include "dialog/alarm/AlarmContentBuilder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    AreaEscalationEvaluator::Result result;
    result.targetId = 413312550;
    result.domain = AreaEscalationEvaluator::TargetDomain::Surface;
    result.stage = AreaEscalationEvaluator::Stage::Alarm;
    result.disposition = AreaEscalationEvaluator::Disposition::Unassigned;
    result.reason = QStringLiteral("direct_entry");
    result.conditionId = QStringLiteral("rule-1");
    result.qualificationArea = {3, 16};
    result.eventArea = {3, 15};
    result.score = 79;
    result.speedMps = 14.9;
    result.courseDeg = 52.0;
    result.hardConditions = QStringLiteral("all_passed");

    const QString readableRuleContent = QStringLiteral(
        R"({"v":1,"summary":"海上目标未知目标（ID 413312550）速度 14.9 m/s，进攻方向 52.0 度，触发告警规则：速度大于 3 m/s，进攻方向在 0~45 度，威胁度≥20，威胁分：总分 79.0（类型 42.0，速度 6.3，进攻方向 12.8，距离 17.9）","speed_mps":14.9,"course_deg":52.0,"threat_total":79.0})");
    const QString merged = buildAreaEscalationContent(result, readableRuleContent);
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(merged.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qCritical() << "merged alarm content is not valid JSON" << error.errorString();
        return 1;
    }
    const QJsonObject object = document.object();
    const QString summary = object.value(QStringLiteral("summary")).toString();
    if (!summary.contains(QStringLiteral("海上目标未知目标"))
        || !summary.contains(QStringLiteral("速度 14.9 m/s"))
        || !summary.contains(QStringLiteral("触发告警规则"))
        || !summary.contains(QStringLiteral("威胁分：总分 79.0"))
        || !summary.contains(QStringLiteral("持预警资格进入告警区"))) {
        qCritical().noquote() << "readable rule summary was lost:" << summary;
        return 1;
    }
    if (object.value(QStringLiteral("stage")).toString() != QStringLiteral("HIGH")
        || object.value(QStringLiteral("upgrade_reason")).toString()
            != QStringLiteral("direct_entry")) {
        qCritical() << "escalation evidence was not retained" << object;
        return 1;
    }

    AreaEscalationEvaluator::Result archiveResult;
    archiveResult.targetId = 413312551;
    archiveResult.domain = AreaEscalationEvaluator::TargetDomain::Surface;
    archiveResult.stage = AreaEscalationEvaluator::Stage::Alarm;
    archiveResult.disposition = AreaEscalationEvaluator::Disposition::VerifySuccess;
    archiveResult.reason = QStringLiteral("archive_visit");
    archiveResult.conditionId = QStringLiteral("rule-B");
    archiveResult.eventArea = {3, 15};
    archiveResult.score = 90;
    archiveResult.archiveTargetLabel = QStringLiteral("知识库威胁船A");
    archiveResult.currentPosition = QPointF(37.55, 122.10);

    const QJsonDocument archiveDocument = QJsonDocument::fromJson(
        buildAreaEscalationContent(archiveResult).toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !archiveDocument.isObject()) {
        qCritical() << "archive alarm content is not valid JSON" << error.errorString();
        return 1;
    }
    const QJsonObject archiveObject = archiveDocument.object();
    const QString archiveSummary = archiveObject.value(QStringLiteral("summary")).toString();
    if (archiveSummary
            != QStringLiteral("目标 413312551（知识库威胁船A）为知识库中的威胁目标，直接告警")
        || archiveObject.value(QStringLiteral("upgrade_reason")).toString()
            != QStringLiteral("archive_visit")
        || archiveObject.value(QStringLiteral("archive_status")).toBool() != true
        || archiveObject.value(QStringLiteral("archive_target_type")).toString()
            != QStringLiteral("知识库威胁船A")
        || archiveObject.value(QStringLiteral("disposition")).toString()
            != QStringLiteral("VERIFY_SUCCESS")) {
        qCritical().noquote() << "archive evidence contract failed:" << archiveObject;
        return 1;
    }

    archiveResult.archiveTargetLabel.clear();
    const QJsonObject unnamedArchiveObject = QJsonDocument::fromJson(
        buildAreaEscalationContent(archiveResult).toUtf8()).object();
    if (unnamedArchiveObject.value(QStringLiteral("summary")).toString()
        != QStringLiteral("目标 413312551 为知识库中的威胁目标，直接告警")) {
        qCritical() << "archive fallback summary failed" << unnamedArchiveObject;
        return 1;
    }

    qInfo() << "Alarm content builder test passed";
    return 0;
}
