#include "AlarmHttpSnapshotFilter.hpp"

namespace alarmsys::http_alarm {

trackmanager::grpc::alarm::AlarmSnapshotRequest keepHighAlarms(
    const trackmanager::grpc::alarm::AlarmSnapshotRequest& source)
{
    trackmanager::grpc::alarm::AlarmSnapshotRequest filtered;
    for (const auto& record : source.items()) {
        if (!record.has_alarm()
            || record.alarm().level() != trackmanager::grpc::alarm::ThreatLevel::HIGH) {
            continue;
        }
        *filtered.add_items() = record;
    }
    return filtered;
}

} // namespace alarmsys::http_alarm
