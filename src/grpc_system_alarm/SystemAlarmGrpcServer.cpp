#include "SystemAlarmGrpcServer.hpp"

#include "grpc_target_threat/TargetThreatQueryServiceImpl.hpp"

#include <QDebug>

namespace alarmsys {
namespace grpc_system_alarm {

SystemAlarmGrpcServer::SystemAlarmGrpcServer() = default;

SystemAlarmGrpcServer::~SystemAlarmGrpcServer()
{
    stop();
}

bool SystemAlarmGrpcServer::start(
    const std::string& listen_host, int port, CustomConfig* config)
{
    if (m_running.load()) {
        qWarning() << "SystemAlarmGrpcServer 已在运行:" << QString::fromStdString(m_address);
        return true;
    }
    if (port <= 0 || port > 65535) {
        qWarning() << "SystemAlarmGrpcServer 端口非法:" << port;
        return false;
    }

    const std::string host = listen_host.empty() ? std::string("192.168.18.141") : listen_host;
    m_address = host + ":" + std::to_string(port);

    m_service = std::make_unique<SystemAlarmServiceImpl>();
    m_targetThreatService =
        std::make_unique<grpc_target_threat::TargetThreatQueryServiceImpl>(config);
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(m_address, ::grpc::InsecureServerCredentials());
    builder.RegisterService(m_service.get());
    builder.RegisterService(m_targetThreatService.get());
    m_server = builder.BuildAndStart();
    if (!m_server) {
        qWarning() << "SystemAlarmGrpcServer 启动失败:" << QString::fromStdString(m_address);
        m_targetThreatService.reset();
        m_service.reset();
        return false;
    }

    m_running.store(true);
    m_thread = std::thread([this]() { runWait(); });
    qInfo() << "SystemAlarmGrpcServer 已监听" << QString::fromStdString(m_address)
            << "(ReportAlarm/CancelAlarm/GetActiveAlarms/GetTargetThreat)";
    return true;
}

void SystemAlarmGrpcServer::runWait()
{
    if (m_server) {
        m_server->Wait();
    }
    m_running.store(false);
}

void SystemAlarmGrpcServer::stop()
{
    if (m_server) {
        m_server->Shutdown();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_server.reset();
    m_targetThreatService.reset();
    m_service.reset();
    m_running.store(false);
}

}  // namespace grpc_system_alarm
}  // namespace alarmsys
