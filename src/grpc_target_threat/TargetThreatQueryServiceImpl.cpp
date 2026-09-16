#include "TargetThreatQueryServiceImpl.hpp"

#include <QDebug>
#include <QString>

#include <exception>

namespace alarmsys {
namespace grpc_target_threat {

namespace proto = ::alarmsys::grpc::target_threat::v1;

TargetThreatQueryServiceImpl::TargetThreatQueryServiceImpl(CustomConfig* config)
    : m_engine(config)
{
}

::grpc::Status TargetThreatQueryServiceImpl::GetTargetThreat(
    ::grpc::ServerContext* context,
    const proto::GetTargetThreatRequest* request,
    proto::GetTargetThreatResponse* response)
{
    Q_UNUSED(context);
    if (request == nullptr || response == nullptr) {
        return ::grpc::Status(
            ::grpc::StatusCode::INTERNAL, "invalid server request context");
    }

    const QString targetIdText = QString::fromStdString(request->target_id());
    bool decimalDigitsOnly = !targetIdText.isEmpty() && targetIdText.at(0) != QLatin1Char('0');
    for (const QChar ch : targetIdText) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9')) {
            decimalDigitsOnly = false;
            break;
        }
    }
    bool ok = false;
    const qint64 targetId = targetIdText.toLongLong(&ok, 10);
    if (!decimalDigitsOnly || !ok || targetId <= 0) {
        return ::grpc::Status(
            ::grpc::StatusCode::INVALID_ARGUMENT,
            "target_id must be a positive decimal int64 string");
    }

    try {
        m_engine.query(targetId, targetIdText, response);
        qInfo().noquote() << QStringLiteral(
            "TargetThreatQuery target_id=%1 status=%2 env=%3 assessment=%4 current_alarm=%5")
            .arg(targetIdText)
            .arg(static_cast<int>(response->status()))
            .arg(response->has_result()
                    ? static_cast<int>(response->result().environment()) : 0)
            .arg(response->has_result() && response->result().has_assessment() ? 1 : 0)
            .arg(response->has_result() && response->result().has_current_alarm() ? 1 : 0);
        return ::grpc::Status::OK;
    } catch (const std::exception& ex) {
        qCritical() << "TargetThreatQuery 内部异常 target_id=" << targetIdText
                    << "error=" << ex.what();
        return ::grpc::Status(
            ::grpc::StatusCode::INTERNAL, "failed to evaluate target threat");
    } catch (...) {
        qCritical() << "TargetThreatQuery 未知内部异常 target_id=" << targetIdText;
        return ::grpc::Status(
            ::grpc::StatusCode::INTERNAL, "failed to evaluate target threat");
    }
}

} // namespace grpc_target_threat
} // namespace alarmsys
