#pragma once

#include <QByteArray>
#include <QString>

namespace trackmanager::grpc::alarm {
class AlarmSnapshotRequest;
}

namespace alarmsys::http_alarm {

/** 将航迹告警 protobuf 全量快照转换为对外 HTTP JSON。 */
QByteArray serializeAlarmSnapshot(
    const trackmanager::grpc::alarm::AlarmSnapshotRequest& snapshot,
    double snapshotTimeSec,
    const QString& fallbackProducerId);

} // namespace alarmsys::http_alarm
