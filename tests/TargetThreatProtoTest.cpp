#include "target_threat_query.pb.h"

#include <iostream>
#include <string>

int main()
{
    namespace proto = ::alarmsys::grpc::target_threat::v1;

    const auto* requestDescriptor = proto::GetTargetThreatRequest::descriptor();
    if (requestDescriptor == nullptr
        || requestDescriptor->field_count() != 1
        || requestDescriptor->field(0)->name() != "target_id"
        || requestDescriptor->field(0)->number() != 1) {
        std::cerr << "GetTargetThreatRequest contract changed unexpectedly\n";
        return 1;
    }

    proto::GetTargetThreatResponse source;
    source.set_status(proto::QUERY_OK);
    source.set_target_id("413000001");
    source.set_response_time(123.5);
    source.mutable_result()->set_environment(::trackmanager::grpc::alarm::AIR);
    source.mutable_result()->set_status(proto::QUERY_OK);
    source.mutable_result()->mutable_assessment()->set_total_score(71.0);

    std::string bytes;
    if (!source.SerializeToString(&bytes))
        return 1;

    proto::GetTargetThreatResponse decoded;
    if (!decoded.ParseFromString(bytes)
        || decoded.status() != proto::QUERY_OK
        || decoded.target_id() != "413000001"
        || !decoded.has_result()
        || decoded.result().environment() != ::trackmanager::grpc::alarm::AIR
        || !decoded.result().has_assessment()
        || decoded.result().assessment().total_score() != 71.0
        || decoded.result().has_current_alarm()) {
        std::cerr << "GetTargetThreatResponse round trip failed\n";
        return 1;
    }

    proto::GetTargetThreatResponse notFound;
    notFound.set_status(proto::TARGET_NOT_FOUND);
    if (notFound.has_result()) {
        std::cerr << "not-found response must not contain result\n";
        return 1;
    }
    return 0;
}
