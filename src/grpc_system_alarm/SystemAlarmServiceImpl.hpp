#pragma once

#include <grpcpp/grpcpp.h>

#include "system_alarm.grpc.pb.h"

namespace alarmsys {
namespace grpc_system_alarm {

class SystemAlarmServiceImpl final : public ::alarmsys::grpc::system_alarm::SystemAlarmService::Service {
public:
    ::grpc::Status ReportAlarm(::grpc::ServerContext* context,
                               const ::alarmsys::grpc::system_alarm::ReportAlarmRequest* request,
                               ::alarmsys::grpc::system_alarm::CommonAck* response) override;

    ::grpc::Status CancelAlarm(::grpc::ServerContext* context,
                               const ::alarmsys::grpc::system_alarm::CancelAlarmRequest* request,
                               ::alarmsys::grpc::system_alarm::CommonAck* response) override;

    ::grpc::Status GetActiveAlarms(::grpc::ServerContext* context,
                                   const ::alarmsys::grpc::system_alarm::GetActiveAlarmsRequest* request,
                                   ::alarmsys::grpc::system_alarm::GetActiveAlarmsResponse* response) override;
};

}  // namespace grpc_system_alarm
}  // namespace alarmsys
