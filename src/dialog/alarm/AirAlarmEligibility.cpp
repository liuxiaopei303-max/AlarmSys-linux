#include "dialog/alarm/AirAlarmEligibility.h"
#include "dialog/alarm/AlarmTargetEligibility.h"

namespace AirAlarmEligibility {

Decision decide(
    const SPxPacketTrackExtended& track,
    const AlarmLogicConfig& config,
    bool selfReportVirtualOnly)
{
    Decision decision;

    if (selfReportVirtualOnly) {
        const AlarmTargetEligibility::Decision targetDecision =
            AlarmTargetEligibility::decide(track, true);
        if (!targetDecision.eligible) {
            decision.skip = true;
            decision.matchedFusionTrackId = targetDecision.matchedSelfReportTrackId;
            decision.reason = targetDecision.reason;
        }
        return decision;
    }

    // Mode=0 的对空规则只接受分类为 DRONE 的融合航迹。
    if (config.mode == 0 && track.norm.min.reserved1 != 3) {
        decision.skip = true;
        decision.reason = QStringLiteral("not_drone");
        return decision;
    }

    for (int i = 0; i < 8; ++i) {
        const int sourceTrackId = static_cast<int>(track.fusion.trackID[i]);
        if (sourceTrackId > 0 && config.birdSkipTrackIds.contains(sourceTrackId)) {
            decision.skip = true;
            decision.matchedFusionTrackId = sourceTrackId;
            decision.reason = QStringLiteral("bird_skip_fusion_id");
            return decision;
        }
    }

    return decision;
}

} // namespace AirAlarmEligibility
