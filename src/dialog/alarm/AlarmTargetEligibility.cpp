#include "dialog/alarm/AlarmTargetEligibility.h"

namespace AlarmTargetEligibility {
namespace {

constexpr uint32_t kSelfReportMarker = 0x53525054U; // SRPT
constexpr uint32_t kVirtualMarker = 0x5652544cU;    // VRTL

bool isAllowedSelfReportId(uint32_t trackId)
{
    return trackId == 4005U || trackId == 4006U || trackId == 4007U;
}

} // namespace

void markExplicitSelfReport(SPxPacketTrackExtended& track)
{
    track.fusion.reserved[0] = kSelfReportMarker;
}

void markExplicitVirtual(SPxPacketTrackExtended& track)
{
    track.fusion.reserved[1] = kVirtualMarker;
}

bool isExplicitSelfReport(const SPxPacketTrackExtended& track)
{
    return track.fusion.reserved[0] == kSelfReportMarker;
}

bool isExplicitVirtual(const SPxPacketTrackExtended& track)
{
    return track.fusion.reserved[1] == kVirtualMarker;
}

Decision decide(const SPxPacketTrackExtended& track, bool selfReportVirtualOnly)
{
    Decision decision;
    if (!selfReportVirtualOnly) {
        decision.reason = QStringLiteral("scheme_filter_disabled");
        return decision;
    }

    if (isExplicitVirtual(track)) {
        decision.reason = QStringLiteral("explicit_virtual");
        return decision;
    }

    if (track.norm.min.reserved1 == 3 && isExplicitSelfReport(track)) {
        // Both unified converters place the explicit zibaowei source in slot 0.
        // Other slots may contain unrelated fused sources with overlapping IDs.
        const uint32_t sourceTrackId = track.fusion.trackID[0];
        if (isAllowedSelfReportId(sourceTrackId)) {
            decision.matchedSelfReportTrackId = static_cast<int>(sourceTrackId);
            decision.reason = QStringLiteral("allowed_self_report_drone");
            return decision;
        }
    }

    decision.eligible = false;
    decision.reason = QStringLiteral("not_allowed_by_scheme_target_filter");
    return decision;
}

} // namespace AlarmTargetEligibility
