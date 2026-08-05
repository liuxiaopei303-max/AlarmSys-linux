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

/**
 * 将统一 NewTrackStruct 目标路由到 AlarmSys 的海/空航迹表。
 *
 * 虚兵必须来自显式白名单；环境域必须由上游明确给出，禁止把 UNKNOWN
 * 猜成对海。调用方只消费本决策，不再自行解释 protobuf 枚举。
 */
TargetRouteDecision routeTarget(
    const trackmanager::grpc::new_track_struct::TargetObject& target,
    const QStringList& allowedVirtualSourceIds);

bool parseTargetId(const trackmanager::grpc::new_track_struct::TargetObject& t, qint64* outTargetId);

bool targetToSpxExtended(
    const trackmanager::grpc::new_track_struct::TargetObject& t,
    SPxPacketTrackExtended& out,
    bool birdTopic);

uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming);

} // namespace NewTrackStructGrpcConvert
