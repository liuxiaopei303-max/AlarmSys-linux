#pragma once

#include "TargetThreatQueryEngine.hpp"
#include "target_threat_query.grpc.pb.h"

class CustomConfig;

namespace alarmsys {
namespace grpc_target_threat {

/** gRPC 薄适配层：校验 target_id，并把业务查询委托给 QueryEngine。 */
class TargetThreatQueryServiceImpl final
    : public ::alarmsys::grpc::target_threat::v1::TargetThreatQueryService::Service
{
public:
    explicit TargetThreatQueryServiceImpl(CustomConfig* config);

    ::grpc::Status GetTargetThreat(
        ::grpc::ServerContext* context,
        const ::alarmsys::grpc::target_threat::v1::GetTargetThreatRequest* request,
        ::alarmsys::grpc::target_threat::v1::GetTargetThreatResponse* response) override;

private:
    TargetThreatQueryEngine m_engine;
};

} // namespace grpc_target_threat
} // namespace alarmsys
