#pragma once

#include <cstdint>

#include <QString>
#include <QStringList>
#include <QtGlobal>

#include "SPxLibData/SPxPackets.h"

namespace trackmanager {
namespace grpc {
namespace new_track_struct {
class TargetObject;
} // namespace new_track_struct
} // namespace grpc
} // namespace trackmanager

namespace NewTrackStructGrpcConvert {

enum class TargetRouteDomain {
    Unsupported,
    Surface,
    Air,
};

struct TargetRouteDecision {
    TargetRouteDomain domain = TargetRouteDomain::Unsupported;
    QString sourceId;
    QString reason;

    bool accepted() const { return domain != TargetRouteDomain::Unsupported; }
};

enum class TargetLifecycleAction {
    Ingest,
    Remove,
};

struct TargetLifecycleDecision {
    TargetLifecycleAction action = TargetLifecycleAction::Ingest;
    QString sourceId;
    QString reason;

    bool shouldRemove() const { return action == TargetLifecycleAction::Remove; }
    bool usedLegacyConfirmedCompatibility() const
    {
        return reason == QStringLiteral("legacy_confirmed_state_2");
    }
};

/**
 * 将统一 NewTrackStruct 目标路由到 AlarmSys 的海/空航迹表。
 *
 * 虚兵必须来自显式白名单；环境域必须由上游明确给出，禁止把 UNKNOWN
 * 猜成对海。调用方只消费本决策，不再自行解释 protobuf 枚举。
 */
TargetRouteDecision routeTarget(
    const trackmanager::grpc::new_track_struct::TargetObject& target,
    const QStringList& allowedVirtualSourceIds);

/**
 * 解释统一 NewTrackStruct 目标的生命周期状态。
 *
 * 部分真实融合来源沿用旧航迹状态值（1=暂定、2=确认、3=删除），
 * 其他来源遵循 protobuf TargetState 枚举。
 */
TargetLifecycleDecision decideTargetLifecycle(
    const trackmanager::grpc::new_track_struct::TargetObject& target);

bool parseTargetId(const trackmanager::grpc::new_track_struct::TargetObject& t, qint64* outTargetId);

bool targetToSpxExtended(
    const trackmanager::grpc::new_track_struct::TargetObject& t,
    SPxPacketTrackExtended& out,
    bool birdTopic);

/** 仅识别统一航迹明确标记的虚拟、对海、船只；旧航迹不做猜测。 */
bool isVirtualSurfaceShip(const SPxPacketTrackExtended& track);

/**
 * 解析告警评分使用的目标类型。
 *
 * 认知结果始终优先；仅在认知结果为空时，才使用统一航迹转换时保留的
 * 明确分类。返回空字符串表示没有可靠的评分类型。
 */
QString resolveTargetTypeForScoring(
    const QString& cognitiveTargetType,
    const SPxPacketTrackExtended& track);

uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming);

} // namespace NewTrackStructGrpcConvert
