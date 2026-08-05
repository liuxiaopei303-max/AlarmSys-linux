#pragma once

#include "datastruct/commonStruct.h"
#include "db/DataAccessLayer.h"
#include "dialog/alarm/AreaEscalationEvaluator.h"

class CustomConfig;

/** 告警 content 组装入参（在 TrackAlarmThread 研判通过时填充；数值须为触发时刻） */
struct AlarmContentBuildInput {
    const AlarmRule* rule = nullptr;
    CustomConfig* cfg = nullptr;
    qint64 uniqueId = 0;
    double speedMps = 0.0;
    double courseDeg = 0.0;
    double attackAngleDeg = 0.0;
    double heightM = 0.0;
    int threatScore = 0;
    bool hasProtectArea = false;
    bool isAirTrack = false;
    /** 威胁度单项分（calculateThreatAssessment 产出）；hasThreatBreakdown=false 时不写入证据链 */
    bool hasThreatBreakdown = false;
    ThreatAssessmentResult threatBreakdown;
    /** blacklist / threat_direct / rule_match（sustained 不重建 content） */
    QString triggerPath;
    DataAccessLayer::DetectionTypeResult detection;
    bool hasDetection = false;
};

/** 规则触发的告警原因（写入 AlarmItem.content；JSON：summary + 分值 + 图片元数据） */
QString buildRuleAlarmContent(const AlarmContentBuildInput& input);

/** 用户手动确认告警原因（JSON，含 summary） */
QString buildManualAlarmContent(
    CustomConfig* cfg, qint64 uniqueId, bool isAirTrack, double speedMps, double courseDeg);

/**
 * 三态升级证据链（仅内存/gRPC；不要求数据库新增字段）。
 * readableRuleContent 有效时保留原规则告警的中文 summary 和评分字段，再追加升级证据。
 */
QString buildAreaEscalationContent(
    const AreaEscalationEvaluator::Result& result,
    const QString& readableRuleContent = QString());
