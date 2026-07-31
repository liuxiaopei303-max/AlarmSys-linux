#pragma once

#include <cstdint>

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

bool parseTargetId(const trackmanager::grpc::new_track_struct::TargetObject& t, qint64* outTargetId);

bool targetToSpxExtended(
    const trackmanager::grpc::new_track_struct::TargetObject& t,
    SPxPacketTrackExtended& out,
    bool birdTopic);

uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming);

} // namespace NewTrackStructGrpcConvert
