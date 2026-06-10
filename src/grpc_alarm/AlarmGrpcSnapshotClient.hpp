#pragma once

#include <memory>
#include <string>

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
};
