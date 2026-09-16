#pragma once

#include "datastruct/commonStruct.h"
#include "SPxLibData/SPxPackets.h"

#include <QPointF>
#include <QString>

enum class ThreatDistanceBasis {
    Unspecified,
    ProtectionArea,
    BasePoint,
    TrackRange,
};

struct ThreatAssessmentContext {
    bool hasProtectArea = false;
    QPointF protectCenter; // x=latitude, y=longitude
    double protectRadiusMeters = 0.0;
    QString protectionReferenceSource;

    // hasProtectArea=true 时为进攻角，否则为航向角。
    double directionAngleDeg = 0.0;

    bool useBasePoint = false;
    QPointF basePoint; // x=latitude, y=longitude
};

struct ThreatAssessmentCalculation {
    ThreatAssessmentResult assessment;
    QString targetType;
    bool usesAttackAngle = false;
    double directionAngleDeg = 0.0;
    ThreatDistanceBasis distanceBasis = ThreatDistanceBasis::Unspecified;
    double distanceMeters = 0.0;
};

/**
 * 告警线程与按目标查询共用的威胁度计算模块。
 * 输入包含目标快照、认知结果、区域参数与空间参考；函数无写操作。
 */
ThreatAssessmentCalculation calculateTargetThreatAssessment(
    const SPxPacketTrackExtended& track,
    const QString& cognitiveTargetType,
    const ThreatAssessmentParams& threatParams,
    const ThreatAssessmentContext& context);
