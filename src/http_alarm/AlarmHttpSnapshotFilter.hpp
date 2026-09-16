#pragma once

#include "alarm_service.pb.h"

namespace alarmsys::http_alarm {

/** HTTP 只发布正式 HIGH 告警；不改变上游 gRPC 快照。 */
trackmanager::grpc::alarm::AlarmSnapshotRequest keepHighAlarms(
    const trackmanager::grpc::alarm::AlarmSnapshotRequest& source);

} // namespace alarmsys::http_alarm
