#pragma once

#include "datastruct/commonStruct.h"
#include "SPxLibData/SPxPackets.h"
#include "target_threat_query.pb.h"

#include <QPointF>
#include <QMap>

namespace alarmsys::grpc_target_threat {

// 查询专用快照；不修改告警线程的规则、参数或状态。
struct TargetThreatQueryAssessmentConfig
{
    QString activeSchemeId;
    QList<AreaInfo> schemeAreas; // 方案区域角色与保护区，不依赖威胁规则绑定
    QList<AreaInfo> areas;       // 包括保护区的几何信息
    QMap<QString, AlarmRule> rules;
    QList<ThreatAssessmentParams> threatParams;
    bool useBasePoint = false;
    QPointF basePoint;
};

// 仅填充评分、类型和业务状态，保留目标信息及 current_alarm。
void fillTargetThreatQueryAssessment(
    const SPxPacketTrackExtended& track,
    const QString& cognitiveTargetType,
    const TargetThreatQueryAssessmentConfig& config,
    double evaluatedTime,
    ::alarmsys::grpc::target_threat::v1::GetTargetThreatResponse* response);

} // namespace alarmsys::grpc_target_threat
