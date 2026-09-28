#pragma once

#include "SPxLibData/SPxPackets.h"

#include <QString>

namespace AlarmTargetEligibility {

struct Decision
{
    bool eligible = true;
    int matchedSelfReportTrackId = 0;
    QString reason;
};

/** Preserve explicit upstream provenance in SPx fusion reserved fields. */
void markExplicitSelfReport(SPxPacketTrackExtended& track);
void markExplicitVirtual(SPxPacketTrackExtended& track);

bool isExplicitSelfReport(const SPxPacketTrackExtended& track);
bool isExplicitVirtual(const SPxPacketTrackExtended& track);

/**
 * Scheme-level admission gate. When enabled, only explicit virtual targets and
 * explicit self-report drones 5/6/7 (source IDs 4005/4006/4007) are eligible.
 */
Decision decide(const SPxPacketTrackExtended& track, bool selfReportVirtualOnly);

} // namespace AlarmTargetEligibility
