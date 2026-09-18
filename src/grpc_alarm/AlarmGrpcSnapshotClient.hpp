#pragma once

#include <memory>
#include <string>

#include <QHash>
#include <QMutex>
#include <grpcpp/channel.h>

#include "alarm_service.grpc.pb.h"

class CustomConfig;

/** 调用 TrackManager gRPC UpdateAlarmSnapshot，推送当前全量目标告警快照 */
class AlarmGrpcSnapshotClient
{
public:
    AlarmGrpcSnapshotClient();
    ~AlarmGrpcSnapshotClient();

    void configure(const std::string& host, int port, bool enabled, const std::string& producerId);
    bool isEnabled() const { return m_enabled; }

    /** 构造与 UpdateAlarmSnapshot 完全相同的当前全量快照，不执行网络调用。 */
    trackmanager::grpc::alarm::AlarmSnapshotRequest buildCurrentSnapshot(CustomConfig* cfg) const;

    /** 从 CustomConfig 组装快照并推送；无有效告警时发送空 items */
    bool pushSnapshot(CustomConfig* cfg);

private:
    bool ensureChannel();

    bool m_enabled = false;
    std::string m_host = "192.168.18.141";
    int m_port = 25051;
    std::string m_producerId = "AlarmSys-linux";
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<trackmanager::grpc::alarm::AlarmResultService::Stub> m_stub;
    // 全量快照会保留最近十分钟的告警；源航迹短暂消失后也不能重发已排除目标。
    mutable QMutex m_suppressedAirMutex;
    mutable QHash<qint64, qint64> m_suppressedAirUntilMs;
};
