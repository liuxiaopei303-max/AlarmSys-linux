#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include <grpcpp/grpcpp.h>

#include "SystemAlarmServiceImpl.hpp"

namespace alarmsys {
namespace grpc_system_alarm {

/** 系统告警 gRPC Server：独立线程 Wait，进程退出时 Shutdown。 */
class SystemAlarmGrpcServer {
public:
    SystemAlarmGrpcServer();
    ~SystemAlarmGrpcServer();

    SystemAlarmGrpcServer(const SystemAlarmGrpcServer&) = delete;
    SystemAlarmGrpcServer& operator=(const SystemAlarmGrpcServer&) = delete;

    bool start(const std::string& listen_host, int port);
    void stop();
    bool isRunning() const { return m_running.load(); }

private:
    void runWait();

    std::unique_ptr<::grpc::Server> m_server;
    std::unique_ptr<SystemAlarmServiceImpl> m_service;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::string m_address;
};

}  // namespace grpc_system_alarm
}  // namespace alarmsys
