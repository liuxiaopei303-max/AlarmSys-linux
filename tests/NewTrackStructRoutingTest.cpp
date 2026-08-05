#include "grpc_track/new_track_struct_grpc_convert.h"
#include "target_stream.pb.h"

#include <QCoreApplication>
#include <QDebug>

namespace proto = trackmanager::grpc::new_track_struct;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    proto::TargetObject target;
    target.set_target_id("400");
    target.set_reality_type(proto::RealityType_VIRTUAL);
    target.set_environment(proto::EnvironmentType_SURFACE);
    target.set_classified_type(proto::UnitType_SURFACE_SHIP);
    target.mutable_target_board()->set_entity_id("virtual_new_track_struct_grpc_client");

    const auto decision = NewTrackStructGrpcConvert::routeTarget(
        target, {QStringLiteral("virtual_new_track_struct_grpc_client")});
    if (!decision.accepted()
        || decision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Surface) {
        qCritical() << "normal virtual surface ship was not routed to SURFACE"
                    << decision.reason;
        return 1;
    }

    proto::TargetObject buoyTarget;
    buoyTarget.set_target_id("404");
    buoyTarget.set_reality_type(proto::RealityType_VIRTUAL);
    buoyTarget.set_environment(proto::EnvironmentType_SURFACE);
    buoyTarget.set_classified_type(proto::UnitType_BUOY);
    buoyTarget.mutable_target_board()->set_entity_id("virtual_new_track_struct_grpc_client");
    const auto buoyDecision = NewTrackStructGrpcConvert::routeTarget(
        buoyTarget, {QStringLiteral("virtual_new_track_struct_grpc_client")});
    if (!buoyDecision.accepted()
        || buoyDecision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Surface) {
        qCritical() << "normal virtual surface buoy was not routed to SURFACE"
                    << buoyDecision.reason;
        return 1;
    }

    proto::TargetObject realSurfaceTarget;
    realSurfaceTarget.set_target_id("732341");
    realSurfaceTarget.set_reality_type(proto::RealityType_REAL);
    realSurfaceTarget.set_environment(proto::EnvironmentType_SURFACE);
    realSurfaceTarget.set_classified_type(proto::UnitType_SURFACE_SHIP);
    realSurfaceTarget.mutable_target_board()->set_entity_id("real_track_source");
    const auto realDecision = NewTrackStructGrpcConvert::routeTarget(
        realSurfaceTarget, {QStringLiteral("virtual_new_track_struct_grpc_client")});
    if (!realDecision.accepted()
        || realDecision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Surface) {
        qCritical() << "real surface target was unexpectedly filtered by virtual allow-list"
                    << realDecision.reason;
        return 1;
    }

    proto::TargetObject airTarget;
    airTarget.set_target_id("397");
    airTarget.set_reality_type(proto::RealityType_VIRTUAL);
    airTarget.set_environment(proto::EnvironmentType_AIR);
    airTarget.set_classified_type(proto::UnitType_DRONE);
    airTarget.mutable_target_board()->set_entity_id("virtual_new_track_struct_grpc_client");
    const auto airDecision = NewTrackStructGrpcConvert::routeTarget(
        airTarget, {QStringLiteral("virtual_new_track_struct_grpc_client")});
    if (!airDecision.accepted()
        || airDecision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Air) {
        qCritical() << "normal virtual air target was not routed to AIR"
                    << airDecision.reason;
        return 1;
    }

    proto::TargetObject legacyDdsTarget;
    legacyDdsTarget.set_target_id("655002");
    legacyDdsTarget.set_reality_type(proto::RealityType_VIRTUAL);
    legacyDdsTarget.set_environment(proto::EnvironmentType_AIR);
    legacyDdsTarget.set_classified_type(proto::UnitType_DRONE);
    legacyDdsTarget.mutable_target_board()->set_entity_id("dds_xu_bing_dui_kong_rong_he");
    const auto legacyDecision = NewTrackStructGrpcConvert::routeTarget(
        legacyDdsTarget, {QStringLiteral("virtual_new_track_struct_grpc_client")});
    if (legacyDecision.accepted()
        || legacyDecision.reason != QStringLiteral("virtual_source_not_allowed")) {
        qCritical() << "legacy DDS virtual source was not rejected"
                    << legacyDecision.reason;
        return 1;
    }

    proto::TargetObject unknownTarget;
    unknownTarget.set_target_id("655003");
    unknownTarget.set_reality_type(proto::RealityType_VIRTUAL);
    unknownTarget.set_environment(proto::EnvironmentType_UNKNOWN);
    unknownTarget.set_classified_type(proto::UnitType_UNKNOWN);
    unknownTarget.mutable_target_board()->set_entity_id("virtual_new_track_struct_grpc_client");
    const auto unknownDecision = NewTrackStructGrpcConvert::routeTarget(
        unknownTarget, {QStringLiteral("virtual_new_track_struct_grpc_client")});
    if (unknownDecision.accepted()
        || unknownDecision.reason != QStringLiteral("unknown_environment")) {
        qCritical() << "UNKNOWN environment was not rejected explicitly"
                    << unknownDecision.reason;
        return 1;
    }

    qInfo() << "NewTrackStruct routing test passed";
    return 0;
}
