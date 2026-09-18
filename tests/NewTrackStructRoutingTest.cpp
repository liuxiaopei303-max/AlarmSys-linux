#include "grpc_track/new_track_struct_grpc_convert.h"
#include "target_stream.pb.h"
#include "datastruct/commonStruct.h"
#include "dialog/alarm/AirAlarmEligibility.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>

namespace proto = trackmanager::grpc::new_track_struct;

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    const BasicConfig defaultConfig{};
    const QStringList& allowedVirtualSources = defaultConfig.m_virtualTrackSourceAllowList;

    proto::TargetObject target;
    target.set_target_id("400");
    target.set_reality_type(proto::RealityType_VIRTUAL);
    target.set_environment(proto::EnvironmentType_SURFACE);
    target.set_classified_type(proto::UnitType_SURFACE_SHIP);
    target.mutable_target_board()->set_entity_id("virtual_new_track_struct_grpc_client");

    const auto decision = NewTrackStructGrpcConvert::routeTarget(
        target, allowedVirtualSources);
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
        buoyTarget, allowedVirtualSources);
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
        realSurfaceTarget, allowedVirtualSources);
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
        airTarget, allowedVirtualSources);
    if (!airDecision.accepted()
        || airDecision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Air) {
        qCritical() << "normal virtual air target was not routed to AIR"
                    << airDecision.reason;
        return 1;
    }

    proto::TargetObject selfReportedDrone = airTarget;
    selfReportedDrone.set_target_id("2900001");
    const auto originalBirthSeconds = QDateTime::currentSecsSinceEpoch() - 12;
    selfReportedDrone.set_created_time(originalBirthSeconds);
    auto* selfReportSource = selfReportedDrone.add_sources();
    selfReportSource->mutable_source_profile()->set_radar_source_present(true);
    auto* selfReportFusion = selfReportSource->mutable_source_profile()
        ->mutable_radar_source()->mutable_target_profile()->add_fusionsources();
    selfReportFusion->set_datasourceid("zibaowei");
    selfReportFusion->set_trackid(4001);
    SPxPacketTrackExtended convertedSelfReportedDrone;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(
            selfReportedDrone, convertedSelfReportedDrone, true)
        || convertedSelfReportedDrone.fusion.trackID[0] != 4001U) {
        qCritical() << "self-report source track id was not preserved for air allow-list"
                    << convertedSelfReportedDrone.fusion.trackID[0];
        return 1;
    }
    if (NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
            QString(), convertedSelfReportedDrone) != QStringLiteral("drone")) {
        qCritical() << "existing drone scoring fallback changed unexpectedly";
        return 1;
    }
    if (convertedSelfReportedDrone.norm.min.reserved5 != 0x444d4f41U
        || convertedSelfReportedDrone.norm.min.reserved4
            != static_cast<uint32_t>(originalBirthSeconds)) {
        qCritical() << "track origin timestamp was not preserved independently of liveness";
        return 1;
    }

    AlarmLogicConfig skipConfig;
    skipConfig.mode = 0;
    skipConfig.birdSkipTrackIds = {4001, 4002, 4008, 4021};
    for (int excludedId : {4001, 4002, 4008, 4021}) {
        proto::TargetObject realFusion = airTarget;
        realFusion.set_target_id("227993434");
        auto* source = realFusion.add_sources();
        source->set_entity_id("zi_bao_wei");
        source->set_source_track_id(std::to_string(excludedId));
        auto* strike = realFusion.add_sources();
        strike->set_entity_id("udp_strike_uav_track");
        strike->set_source_track_id("589");
        SPxPacketTrackExtended converted;
        if (!NewTrackStructGrpcConvert::targetToSpxExtended(realFusion, converted, true)
            || !AirAlarmEligibility::decide(converted, skipConfig).skip) {
            qCritical() << "top-level self-report source was not excluded"
                        << excludedId << converted.fusion.trackID[0];
            return 1;
        }
    }
    proto::TargetObject ordinaryFusion = airTarget;
    ordinaryFusion.set_target_id("227993435");
    auto* ordinarySource = ordinaryFusion.add_sources();
    ordinarySource->set_entity_id("zi_bao_wei");
    ordinarySource->set_source_track_id("4005");
    SPxPacketTrackExtended convertedOrdinary;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(ordinaryFusion, convertedOrdinary, true)
        || AirAlarmEligibility::decide(convertedOrdinary, skipConfig).skip) {
        qCritical() << "ordinary self-report drone was excluded";
        return 1;
    }

    proto::TargetObject unifiedAirTarget;
    unifiedAirTarget.set_target_id("1545999");
    unifiedAirTarget.set_reality_type(proto::RealityType_VIRTUAL);
    unifiedAirTarget.set_environment(proto::EnvironmentType_AIR);
    unifiedAirTarget.set_classified_type(proto::UnitType_DRONE);
    unifiedAirTarget.mutable_target_board()->set_entity_id("dui_kong_rong_he");
    const auto unifiedAirDecision = NewTrackStructGrpcConvert::routeTarget(
        unifiedAirTarget, allowedVirtualSources);
    if (!unifiedAirDecision.accepted()
        || unifiedAirDecision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Air) {
        qCritical() << "unified virtual air target was not routed to AIR"
                    << unifiedAirDecision.reason;
        return 1;
    }

    proto::TargetObject unifiedSurfaceTarget;
    unifiedSurfaceTarget.set_target_id("1546000");
    unifiedSurfaceTarget.set_reality_type(proto::RealityType_VIRTUAL);
    unifiedSurfaceTarget.set_environment(proto::EnvironmentType_SURFACE);
    unifiedSurfaceTarget.set_classified_type(proto::UnitType_SURFACE_SHIP);
    unifiedSurfaceTarget.mutable_target_board()->set_entity_id("dui_kong_rong_he");
    const auto unifiedSurfaceDecision = NewTrackStructGrpcConvert::routeTarget(
        unifiedSurfaceTarget, allowedVirtualSources);
    if (!unifiedSurfaceDecision.accepted()
        || unifiedSurfaceDecision.domain != NewTrackStructGrpcConvert::TargetRouteDomain::Surface) {
        qCritical() << "unified virtual surface target was not routed to SURFACE"
                    << unifiedSurfaceDecision.reason;
        return 1;
    }

    SPxPacketTrackExtended convertedUnifiedSurface;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(
            unifiedSurfaceTarget, convertedUnifiedSurface, false)) {
        qCritical() << "unified virtual surface ship was not converted";
        return 1;
    }
    if (NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
            QString(), convertedUnifiedSurface) != QStringLiteral("ship")) {
        qCritical() << "explicit SURFACE_SHIP was not retained for alarm scoring"
                    << convertedUnifiedSurface.norm.min.reserved2;
        return 1;
    }
    if (!NewTrackStructGrpcConvert::isVirtualSurfaceShip(convertedUnifiedSurface)) {
        qCritical() << "virtual surface ship metadata was lost during conversion";
        return 1;
    }
    if (NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
            QStringLiteral("fishingboat"), convertedUnifiedSurface)
        != QStringLiteral("fishingboat")) {
        qCritical() << "cognitive target type no longer overrides track fallback";
        return 1;
    }

    SPxPacketTrackExtended convertedBuoy;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(
            buoyTarget, convertedBuoy, false)
        || !NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
                QString(), convertedBuoy).isEmpty()) {
        qCritical() << "surface buoy was incorrectly treated as ship";
        return 1;
    }
    if (NewTrackStructGrpcConvert::isVirtualSurfaceShip(convertedBuoy)) {
        qCritical() << "virtual buoy incorrectly classified as ship";
        return 1;
    }
    SPxPacketTrackExtended convertedRealShip;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(
            realSurfaceTarget, convertedRealShip, false)
        || NewTrackStructGrpcConvert::isVirtualSurfaceShip(convertedRealShip)) {
        qCritical() << "real ship incorrectly classified as virtual";
        return 1;
    }
    proto::TargetObject virtualAirShip = unifiedSurfaceTarget;
    virtualAirShip.set_environment(proto::EnvironmentType_AIR);
    SPxPacketTrackExtended convertedAirShip;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(
            virtualAirShip, convertedAirShip, true)
        || NewTrackStructGrpcConvert::isVirtualSurfaceShip(convertedAirShip)) {
        qCritical() << "air target incorrectly matched virtual surface ship";
        return 1;
    }
    proto::TargetObject unknownRealityShip = unifiedSurfaceTarget;
    unknownRealityShip.set_reality_type(proto::RealityType_UNKNOWN_REALITY);
    SPxPacketTrackExtended convertedUnknownReality;
    if (!NewTrackStructGrpcConvert::targetToSpxExtended(
            unknownRealityShip, convertedUnknownReality, false)
        || NewTrackStructGrpcConvert::isVirtualSurfaceShip(convertedUnknownReality)) {
        qCritical() << "unknown reality incorrectly treated as virtual";
        return 1;
    }

    SPxPacketTrackExtended legacySurfaceTrack{};
    legacySurfaceTrack.norm.min.reserved1 = 1;
    legacySurfaceTrack.norm.min.reserved2 = proto::UnitType_SURFACE_SHIP;
    if (!NewTrackStructGrpcConvert::resolveTargetTypeForScoring(
            QString(), legacySurfaceTrack).isEmpty()) {
        qCritical() << "unsigned legacy reserved2 value was incorrectly trusted";
        return 1;
    }

    proto::TargetObject legacyDdsTarget;
    legacyDdsTarget.set_target_id("655002");
    legacyDdsTarget.set_reality_type(proto::RealityType_VIRTUAL);
    legacyDdsTarget.set_environment(proto::EnvironmentType_AIR);
    legacyDdsTarget.set_classified_type(proto::UnitType_DRONE);
    legacyDdsTarget.mutable_target_board()->set_entity_id("dds_xu_bing_dui_kong_rong_he");
    const auto legacyDecision = NewTrackStructGrpcConvert::routeTarget(
        legacyDdsTarget, allowedVirtualSources);
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
        unknownTarget, allowedVirtualSources);
    if (unknownDecision.accepted()
        || unknownDecision.reason != QStringLiteral("unknown_environment")) {
        qCritical() << "UNKNOWN environment was not rejected explicitly"
                    << unknownDecision.reason;
        return 1;
    }

    proto::TargetObject realSurfaceConfirmed;
    realSurfaceConfirmed.set_reality_type(proto::RealityType_REAL);
    realSurfaceConfirmed.set_state(proto::TargetState_LOST);
    realSurfaceConfirmed.mutable_target_board()->set_entity_id("dui_hai_rong_he");
    const auto realSurfaceLifecycle =
        NewTrackStructGrpcConvert::decideTargetLifecycle(realSurfaceConfirmed);
    if (realSurfaceLifecycle.shouldRemove()
        || !realSurfaceLifecycle.usedLegacyConfirmedCompatibility()) {
        qCritical() << "real unified surface state 2 was not treated as legacy CONFIRMED"
                    << realSurfaceLifecycle.sourceId << realSurfaceLifecycle.reason;
        return 1;
    }

    proto::TargetObject realAirConfirmed;
    realAirConfirmed.set_reality_type(proto::RealityType_REAL);
    realAirConfirmed.set_state(proto::TargetState_LOST);
    realAirConfirmed.mutable_target_board()->set_entity_id("dui_kong_rong_he");
    const auto realAirLifecycle =
        NewTrackStructGrpcConvert::decideTargetLifecycle(realAirConfirmed);
    if (realAirLifecycle.shouldRemove()
        || !realAirLifecycle.usedLegacyConfirmedCompatibility()) {
        qCritical() << "real unified air state 2 was not treated as legacy CONFIRMED"
                    << realAirLifecycle.sourceId << realAirLifecycle.reason;
        return 1;
    }

    proto::TargetObject realSurfaceDeleted = realSurfaceConfirmed;
    realSurfaceDeleted.set_state(proto::TargetState_MERGED);
    const auto realSurfaceDeletedLifecycle =
        NewTrackStructGrpcConvert::decideTargetLifecycle(realSurfaceDeleted);
    if (!realSurfaceDeletedLifecycle.shouldRemove()
        || realSurfaceDeletedLifecycle.reason != QStringLiteral("state_merged")) {
        qCritical() << "real unified surface legacy state 3 was not removed"
                    << realSurfaceDeletedLifecycle.sourceId
                    << realSurfaceDeletedLifecycle.reason;
        return 1;
    }

    proto::TargetObject virtualLost = realAirConfirmed;
    virtualLost.set_reality_type(proto::RealityType_VIRTUAL);
    const auto virtualLostLifecycle =
        NewTrackStructGrpcConvert::decideTargetLifecycle(virtualLost);
    if (!virtualLostLifecycle.shouldRemove()
        || virtualLostLifecycle.reason != QStringLiteral("state_lost")) {
        qCritical() << "virtual state 2 no longer followed protobuf LOST semantics"
                    << virtualLostLifecycle.sourceId << virtualLostLifecycle.reason;
        return 1;
    }

    proto::TargetObject standardRealLost = realSurfaceConfirmed;
    standardRealLost.mutable_target_board()->set_entity_id("real_track_source");
    const auto standardRealLostLifecycle =
        NewTrackStructGrpcConvert::decideTargetLifecycle(standardRealLost);
    if (!standardRealLostLifecycle.shouldRemove()
        || standardRealLostLifecycle.reason != QStringLiteral("state_lost")) {
        qCritical() << "non-unified real source no longer followed protobuf LOST semantics"
                    << standardRealLostLifecycle.sourceId
                    << standardRealLostLifecycle.reason;
        return 1;
    }

    proto::TargetObject stableReal = realSurfaceConfirmed;
    stableReal.set_state(proto::TargetState_STABLE);
    const auto stableRealLifecycle =
        NewTrackStructGrpcConvert::decideTargetLifecycle(stableReal);
    if (stableRealLifecycle.shouldRemove()
        || stableRealLifecycle.reason != QStringLiteral("active_state")) {
        qCritical() << "stable real target was removed unexpectedly"
                    << stableRealLifecycle.sourceId << stableRealLifecycle.reason;
        return 1;
    }

    qInfo() << "NewTrackStruct routing test passed";
    return 0;
}
