#include "TargetTypeGrpcClient.hpp"

#include <QDebug>
#include <QString>

#include <chrono>
#include <grpcpp/grpcpp.h>

namespace {

static const char* kAllowedTargetTypes[] = {
    "ship", "yacht", "cargo", "fishing", "buoy", "uav", "bird", "other",
};

bool isAllowedTargetType(const QString& type)
{
    for (const char* allowed : kAllowedTargetTypes) {
        if (type.compare(QString::fromLatin1(allowed), Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

QString canonicalTargetType(const QString& type)
{
    const QString normalized = type.trimmed().toLower();
    for (const char* allowed : kAllowedTargetTypes) {
        if (normalized == QLatin1String(allowed)) {
            return QString::fromLatin1(allowed);
        }
    }
    return normalized;
}

} // namespace

TargetTypeGrpcClient::TargetTypeGrpcClient() = default;

TargetTypeGrpcClient::~TargetTypeGrpcClient() = default;

void TargetTypeGrpcClient::configure(const std::string& host, int port, bool enabled)
{
    m_host = host;
    m_port = port;
    m_enabled = enabled && !host.empty() && port > 0;
    m_channel.reset();
    m_stub.reset();
}

bool TargetTypeGrpcClient::ensureChannel()
{
    if (m_stub && m_channel) {
        return true;
    }
    if (!m_enabled) {
        return false;
    }

    const std::string address = m_host + ":" + std::to_string(m_port);
    m_channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    if (!m_channel) {
        qWarning() << "TargetTypeGrpcClient: 创建 gRPC channel 失败"
                   << QString::fromStdString(address);
        return false;
    }

    m_stub = targetTypePkg::TargetTypeService::NewStub(m_channel);
    qInfo() << "TargetTypeGrpcClient: 已连接" << QString::fromStdString(address);
    return true;
}

bool TargetTypeGrpcClient::updateTargetType(qint64 targetId, const QString& targetType)
{
    if (!m_enabled || targetId <= 0) {
        return false;
    }

    const QString canonicalType = canonicalTargetType(targetType);
    if (!isAllowedTargetType(canonicalType)) {
        qWarning() << "TargetTypeGrpcClient: 跳过非法 targetType"
                   << targetType << "target_id" << targetId;
        return false;
    }

    if (!ensureChannel()) {
        return false;
    }

    targetTypePkg::TargetTypeUpdate request;
    request.set_targetid(QString::number(targetId).toStdString());
    request.set_targettype(canonicalType.toStdString());

    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));
    targetTypePkg::TargetTypeUpdateAck response;
    const grpc::Status status = m_stub->UpdateTargetType(&context, request, &response);
    if (!status.ok()) {
        qWarning() << "TargetTypeGrpcClient: UpdateTargetType 失败"
                   << "target_id" << targetId
                   << "type" << canonicalType
                   << "code" << status.error_code()
                   << "msg" << QString::fromStdString(status.error_message());
        m_channel.reset();
        m_stub.reset();
        return false;
    }

    if (!response.accepted()) {
        qWarning() << "TargetTypeGrpcClient: 服务端拒绝"
                   << "target_id" << targetId
                   << "type" << canonicalType
                   << "reason" << QString::fromStdString(response.reason());
        return false;
    }

    qInfo() << "TargetTypeGrpcClient: 已推送目标类型"
            << "target_id" << targetId
            << "type" << canonicalType
            << "reason" << QString::fromStdString(response.reason());
    return true;
}
