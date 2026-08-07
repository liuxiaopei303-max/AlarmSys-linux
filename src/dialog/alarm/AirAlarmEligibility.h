#pragma once

#include "SPxLibData/SPxPackets.h"
#include "datastruct/commonStruct.h"

#include <QString>

namespace AirAlarmEligibility {

struct Decision
{
    bool skip = false;
    int matchedFusionTrackId = 0;
    QString reason;
};

/**
 * 对空三态告警入口决策。
 *
 * BirdSkipTrackIds 存的是自报位源航迹 ID（例如无人机 1 对应
 * 4001），不是前端显示的融合 target_id。上游可把多个源写入
 * fusion.trackID[0..7]，因此所有槽位都必须检查。
 */
Decision decide(const SPxPacketTrackExtended& track, const AlarmLogicConfig& config);

} // namespace AirAlarmEligibility
