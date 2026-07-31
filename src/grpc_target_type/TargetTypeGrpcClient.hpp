#pragma once

#include <memory>
#include <string>

#include <QtGlobal>
#include <QString>
#include <grpcpp/channel.h>

#include "target_type.grpc.pb.h"

/** 调用 TrackManager TargetTypeService.UpdateTargetType，推送研判后的目标类型 */
class TargetTypeGrpcClient
{
public:
    TargetTypeGrpcClient();
    ~TargetTypeGrpcClient();

    void configure(const std::string& host, int port, bool enabled);
    bool isEnabled() const { return m_enabled; }

    /** targetId 通常为 target_id（unique_id）；targetType 为小写英文类型 */
    bool updateTargetType(qint64 targetId, const QString& targetType);

private:
    bool ensureChannel();

    bool m_enabled = false;
    std::string m_host = "192.168.18.141";
    int m_port = 60054;
    std::shared_ptr<grpc::Channel> m_channel;
    std::unique_ptr<targetTypePkg::TargetTypeService::Stub> m_stub;
};
