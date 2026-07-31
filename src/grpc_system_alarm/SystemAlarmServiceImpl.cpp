#include "SystemAlarmServiceImpl.hpp"

#include "SystemAlarmStore.hpp"

#include <QDebug>

#include <chrono>

namespace alarmsys {
namespace grpc_system_alarm {

namespace {

int64_t nowMs()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

}  // namespace

::grpc::Status SystemAlarmServiceImpl::ReportAlarm(
    ::grpc::ServerContext* /*context*/,
    const ::alarmsys::grpc::system_alarm::ReportAlarmRequest* request,
    ::alarmsys::grpc::system_alarm::CommonAck* response)
{
    response->set_server_time_ms(nowMs());
    if (!request || !request->has_alarm()) {
        response->set_ok(false);
        response->set_message("invalid_param: alarm missing");
        return ::grpc::Status::OK;
    }

    std::string msg;
    const bool ok = SystemAlarmStore::instance().report(request->alarm(), &msg);
    response->set_ok(ok);
    response->set_message(msg);

    const auto& a = request->alarm();
    qInfo() << "SystemAlarm ReportAlarm"
            << "ok=" << ok
            << "id=" << QString::fromStdString(a.alarm_id())
            << "sys=" << QString::fromStdString(a.system_id())
            << "kind=" << static_cast<int>(a.alarm_kind())
            << "active=" << SystemAlarmStore::instance().size()
            << "msg=" << QString::fromStdString(msg);
    return ::grpc::Status::OK;
}

::grpc::Status SystemAlarmServiceImpl::CancelAlarm(
    ::grpc::ServerContext* /*context*/,
    const ::alarmsys::grpc::system_alarm::CancelAlarmRequest* request,
    ::alarmsys::grpc::system_alarm::CommonAck* response)
{
    response->set_server_time_ms(nowMs());
    if (!request) {
        response->set_ok(false);
        response->set_message("invalid_param");
        return ::grpc::Status::OK;
    }

    std::string msg;
    const bool ok = SystemAlarmStore::instance().cancel(
        request->alarm_id(), request->system_id(), request->canceled_time_ms(), &msg);
    response->set_ok(ok);
    response->set_message(msg);

    qInfo() << "SystemAlarm CancelAlarm"
            << "ok=" << ok
            << "id=" << QString::fromStdString(request->alarm_id())
            << "sys=" << QString::fromStdString(request->system_id())
            << "canceled_ms=" << request->canceled_time_ms()
            << "reason=" << QString::fromStdString(request->reason())
            << "active=" << SystemAlarmStore::instance().size()
            << "msg=" << QString::fromStdString(msg);
    return ::grpc::Status::OK;
}

::grpc::Status SystemAlarmServiceImpl::GetActiveAlarms(
    ::grpc::ServerContext* /*context*/,
    const ::alarmsys::grpc::system_alarm::GetActiveAlarmsRequest* request,
    ::alarmsys::grpc::system_alarm::GetActiveAlarmsResponse* response)
{
    response->set_server_time_ms(nowMs());
    const std::string system_id = request ? request->system_id() : std::string();
    const auto kind = request ? request->alarm_kind()
                              : ::alarmsys::grpc::system_alarm::ALARM_KIND_UNSPECIFIED;
    const bool level_filter_set = request && request->level_filter_set();
    const auto level = request ? request->level() : ::alarmsys::grpc::system_alarm::LOW;

    const auto list = SystemAlarmStore::instance().listActive(
        system_id, kind, level_filter_set, level);
    for (const auto& item : list) {
        *response->add_alarms() = item;
    }

    qInfo() << "SystemAlarm GetActiveAlarms"
            << "filter_sys=" << QString::fromStdString(system_id)
            << "kind=" << static_cast<int>(kind)
            << "count=" << response->alarms_size();
    return ::grpc::Status::OK;
}

}  // namespace grpc_system_alarm
}  // namespace alarmsys
