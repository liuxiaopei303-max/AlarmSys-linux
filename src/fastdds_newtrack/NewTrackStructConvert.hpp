#pragma once

#include <cstdint>

#include <QtGlobal>

#include "NewTrackRealTimeStatus.hpp"
#include "SPxLibData/SPxPackets.h"

namespace NewTrackStructConvert {

/** 解析 NewTrack DDS TargetObject.target_id（全局唯一目标 ID） */
bool parseTargetId(const TargetFull::TargetObject& t, qint64* outTargetId);

/** @deprecated 仅兼容旧调用；新代码请用 parseTargetId */
int resolveFuseMapKey(
    const TargetFull::TargetObject& t,
    int alarmLogicMode,
    bool birdTopic = false);

/** 将 TargetObject 转为 SPxPacketTrackExtended；map 主键为 target_id（由 parseTargetId 解析） */
bool targetToSpxExtended(
    const TargetFull::TargetObject& t,
    int alarmLogicMode,
    SPxPacketTrackExtended& out,
    bool birdTopic = false);

/** 对空目标：classified_type==DRONE -> reserved1=3，其余=1 */
void applyAirTargetReserved1(const TargetFull::TargetObject& t, SPxPacketTrackExtended& track);

/** 鸟情融合：fusion.trackID[0]（reserved1 同 applyAirTargetReserved1） */
void applyBirdFuseExtras(const TargetFull::TargetObject& t, SPxPacketTrackExtended& track);

/** map 更新时保留已有 target_id（incoming 无效时不覆盖） */
uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming);

} // namespace NewTrackStructConvert
