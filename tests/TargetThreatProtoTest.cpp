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

    // 已接入服务无需重新生成客户端：响应字段及 RPC 调用契约不能增加或删除。
    if (proto::GetTargetThreatResponse::descriptor()->field_count() != 5
        || proto::TargetThreatResult::descriptor()->field_count() != 10
        || proto::ThreatAssessment::descriptor()->field_count() != 20
        || proto::ThreatScoreComponent::descriptor()->field_count() != 3) {
        std::cerr << "Target threat response structure changed unexpectedly\n";
        return 1;
    }
    const auto* service = proto::GetTargetThreatRequest::descriptor()->file()
        ->FindServiceByName("TargetThreatQueryService");
    if (service == nullptr || service->method_count() != 1
        || service->full_name() != "alarmsys.grpc.target_threat.v1.TargetThreatQueryService"
        || service->method(0)->name() != "GetTargetThreat"
        || service->method(0)->input_type() != requestDescriptor
        || service->method(0)->output_type() != proto::GetTargetThreatResponse::descriptor()) {
        std::cerr << "Target threat RPC invocation contract changed unexpectedly\n";
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

    // 区外评分仍用原有四个分项消息，零值按标准 protobuf 规则传输。
    proto::GetTargetThreatResponse outside;
    outside.set_status(proto::QUERY_OK);
    auto* assessment = outside.mutable_result()->mutable_assessment();
    assessment->set_total_score(50.0);
    assessment->mutable_type_score()->set_weight(0.42);
    assessment->mutable_type_score()->set_weighted_score(42.0);
    assessment->mutable_speed_score()->set_weight(0.16);
    assessment->mutable_speed_score()->set_weighted_score(8.0);
    assessment->mutable_direction_score();
    assessment->mutable_distance_score();
    if (!outside.SerializeToString(&bytes) || !decoded.ParseFromString(bytes)
        || !decoded.result().assessment().has_type_score()
        || !decoded.result().assessment().has_speed_score()
        || !decoded.result().assessment().has_direction_score()
        || !decoded.result().assessment().has_distance_score()
        || decoded.result().assessment().direction_score().weighted_score() != 0.0
        || decoded.result().assessment().distance_score().weighted_score() != 0.0
        || decoded.result().assessment().total_score() != 50.0
        || decoded.result().has_current_alarm()) {
        std::cerr << "Outside-area assessment structure round trip failed\n";
        return 1;
    }
    return 0;
}
