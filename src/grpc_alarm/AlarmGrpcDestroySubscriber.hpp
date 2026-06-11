#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/channel.h>

#include "destroy.grpc.pb.h"

class CustomConfig;

/** 订阅 NexusUI destroy gRPC，收到灭告警事件后写入 m_listAlarmFilter（等同 HTTP /api/alarm_filter） */
class AlarmGrpcDestroySubscriber
{
public:
    AlarmGrpcDestroySubscriber();
    ~AlarmGrpcDestroySubscriber();

    void configure(const std::string& host, int port, bool enabled);
    bool isEnabled() const { return m_enabled; }

    void start(CustomConfig* cfg);
    void stop();

private:
    void runLoop();
    bool subscribeOnce(CustomConfig* cfg);
    void handleDestroyEvent(const destroy::DestroyEvent& event, CustomConfig* cfg);

    bool m_enabled = false;
    std::string m_host = "192.168.18.141";
    int m_port = 50061;
    CustomConfig* m_cfg = nullptr;
    std::atomic<bool> m_stop{false};
    std::thread m_thread;
    std::shared_ptr<grpc::Channel> m_channel;
};
