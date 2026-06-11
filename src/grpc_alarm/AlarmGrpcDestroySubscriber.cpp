#include "AlarmGrpcDestroySubscriber.hpp"

#include "customconfig.h"

#include <QDebug>
#include <QThread>

#include <chrono>
#include <grpcpp/grpcpp.h>

namespace {

constexpr char kFilterSpecType[] = "type.casia.tasks.v1.filterTargetOrForce";

bool parseFilterTargetId(const std::string& idStr, qint64* outTargetId)
{
    if (outTargetId == nullptr || idStr.empty()) {
        return false;
    }
    bool ok = false;
    const qint64 targetId = QString::fromStdString(idStr).trimmed().toLongLong(&ok);
    if (!ok || targetId <= 0) {
        return false;
    }
    *outTargetId = targetId;
    return true;
}

} // namespace

AlarmGrpcDestroySubscriber::AlarmGrpcDestroySubscriber() = default;

AlarmGrpcDestroySubscriber::~AlarmGrpcDestroySubscriber()
{
    stop();
}

void AlarmGrpcDestroySubscriber::configure(const std::string& host, int port, bool enabled)
{
    const bool wasRunning = m_thread.joinable();
    if (wasRunning) {
        stop();
    }

    m_host = host;
    m_port = port;
    m_enabled = enabled;
    m_channel.reset();

    if (wasRunning && m_enabled && m_cfg != nullptr) {
        start(m_cfg);
    }
}

void AlarmGrpcDestroySubscriber::start(CustomConfig* cfg)
{
    if (!m_enabled || cfg == nullptr) {
        return;
    }
    if (m_thread.joinable()) {
        return;
    }

    m_cfg = cfg;
    m_stop = false;
    m_thread = std::thread([this]() { runLoop(); });
}

void AlarmGrpcDestroySubscriber::stop()
{
    m_stop = true;
    m_channel.reset();
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_channel.reset();
}

void AlarmGrpcDestroySubscriber::runLoop()
{
    while (!m_stop.load()) {
        CustomConfig* cfg = m_cfg;
        if (cfg == nullptr || !m_enabled) {
            break;
        }

        if (!subscribeOnce(cfg)) {
            for (int i = 0; i < 30 && !m_stop.load(); ++i) {
                QThread::msleep(100);
            }
        }
    }
}

bool AlarmGrpcDestroySubscriber::subscribeOnce(CustomConfig* cfg)
{
    if (m_stop.load() || cfg == nullptr || !m_enabled) {
        return true;
    }

    const std::string address = m_host + ":" + std::to_string(m_port);
    m_channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    if (!m_channel) {
        qWarning() << "AlarmGrpcDestroySubscriber: 创建 gRPC channel 失败" << QString::fromStdString(address);
        return false;
    }

    auto stub = destroy::DestroyEventService::NewStub(m_channel);
    destroy::SubscribeDestroyEventsRequest request;
    grpc::ClientContext context;

    qInfo() << "AlarmGrpcDestroySubscriber: 订阅 destroy 事件" << QString::fromStdString(address);

    std::unique_ptr<grpc::ClientReader<destroy::DestroyEvent>> reader(
        stub->SubscribeDestroyEvents(&context, request));
    if (!reader) {
        qWarning() << "AlarmGrpcDestroySubscriber: SubscribeDestroyEvents 失败"
                   << QString::fromStdString(address);
        m_channel.reset();
        return false;
    }

    destroy::DestroyEvent event;
    while (!m_stop.load() && reader->Read(&event)) {
        handleDestroyEvent(event, cfg);
    }

    const grpc::Status status = reader->Finish();
    m_channel.reset();

    if (m_stop.load()) {
        return true;
    }

    if (!status.ok()) {
        qWarning() << "AlarmGrpcDestroySubscriber: 订阅流结束"
                   << status.error_code()
                   << QString::fromStdString(status.error_message());
        return false;
    }

    qInfo() << "AlarmGrpcDestroySubscriber: 订阅流正常结束，准备重连";
    return false;
}

void AlarmGrpcDestroySubscriber::handleDestroyEvent(
    const destroy::DestroyEvent& event, CustomConfig* cfg)
{
    if (cfg == nullptr) {
        return;
    }

    const destroy::DestroySpecification& spec = event.specification();
    const QString specAtType = QString::fromStdString(spec.at_type());
    if (specAtType != QLatin1String(kFilterSpecType)) {
        qInfo() << "AlarmGrpcDestroySubscriber: 忽略非 filter 事件 task_id="
                << QString::fromStdString(event.task_id())
                << "at_type=" << specAtType;
        return;
    }

    const int type = spec.type();
    if (type < 0 || type > 1) {
        qWarning() << "AlarmGrpcDestroySubscriber: 无效 type=" << type
                   << "task_id=" << QString::fromStdString(event.task_id());
        return;
    }

    qint64 targetId = 0;
    if (!parseFilterTargetId(spec.id(), &targetId)) {
        qWarning() << "AlarmGrpcDestroySubscriber: 无效 target_id="
                   << QString::fromStdString(spec.id())
                   << "task_id=" << QString::fromStdString(event.task_id());
        return;
    }

    cfg->addAlarmFilter(type, targetId);
    qInfo() << "destroy_grpc filter type=" << type << "target_id=" << targetId
            << "task_id=" << QString::fromStdString(event.task_id());
}
