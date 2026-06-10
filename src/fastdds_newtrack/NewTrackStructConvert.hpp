#pragma once

#include <cstdint>

#include "NewTrackRealTimeStatus.hpp"
#include "SPxLibData/SPxPackets.h"

namespace NewTrackStructConvert {

/** 从 Config.ini [AlarmLogic]/Mode 解析融合航迹 map 主键（与 TrackClassSubscriber 一致） */
int resolveFuseMapKey(
    const TargetFull::TargetObject& t,
    int alarmLogicMode,
    bool birdTopic = false);

/** 将 TargetObject 转为 SPxPacketTrackExtended（按实测优先级选字段源） */
bool targetToSpxExtended(
    const TargetFull::TargetObject& t,
    int alarmLogicMode,
    SPxPacketTrackExtended& out,
    bool birdTopic = false);

/** 对空目标：classified_type==DRONE -> reserved1=3，其余=1 */
void applyAirTargetReserved1(const TargetFull::TargetObject& t, SPxPacketTrackExtended& track);

/** 鸟情融合：fusion.trackID[0]（reserved1 同 applyAirTargetReserved1） */
void applyBirdFuseExtras(const TargetFull::TargetObject& t, SPxPacketTrackExtended& track);

/** map 更新时：已有稳定 uniqueID 不被 0 / 自报位小号覆盖 */
uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming);

} // namespace NewTrackStructConvert
