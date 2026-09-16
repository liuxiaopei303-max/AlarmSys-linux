#pragma once

#include "alarm_service.pb.h"

#include <QString>

#include <functional>
#include <memory>

namespace alarmsys::http_alarm {

/**
 * 独立的告警 HTTP 全量快照发布模块。
 *
 * 模块启动后立即 POST 一次，随后按 Config.ini 周期发布；网络故障不会向调用方抛出。
 */
class AlarmHttpSnapshotPublisher final
{
public:
    using SnapshotFactory =
        std::function<trackmanager::grpc::alarm::AlarmSnapshotRequest()>;

    explicit AlarmHttpSnapshotPublisher(SnapshotFactory snapshotFactory);
    ~AlarmHttpSnapshotPublisher();

    AlarmHttpSnapshotPublisher(const AlarmHttpSnapshotPublisher&) = delete;
    AlarmHttpSnapshotPublisher& operator=(const AlarmHttpSnapshotPublisher&) = delete;

    void start(const QString& configFile = QStringLiteral("Config.ini"));
    void stop();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace alarmsys::http_alarm
