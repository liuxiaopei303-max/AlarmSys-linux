#include "new_track_struct_grpc_convert.h"

#ifdef signals
#pragma push_macro("signals")
#undef signals
#define WATCHSYS_RESTORE_QT_SIGNALS_MACRO
#endif
#include "target_stream.pb.h"
#ifdef WATCHSYS_RESTORE_QT_SIGNALS_MACRO
#pragma pop_macro("signals")
#undef WATCHSYS_RESTORE_QT_SIGNALS_MACRO
#endif

#include <QDateTime>
#include <QString>

#include <cmath>
#include <cstring>
#include <limits>

namespace NewTrackStructGrpcConvert {
namespace {

using trackmanager::grpc::new_track_struct::FusionSourceItem;
using trackmanager::grpc::new_track_struct::EnvironmentType_AIR;
using trackmanager::grpc::new_track_struct::EnvironmentType_MARITIME;
using trackmanager::grpc::new_track_struct::EnvironmentType_SUBSURFACE;
using trackmanager::grpc::new_track_struct::EnvironmentType_SURFACE;
using trackmanager::grpc::new_track_struct::EnvironmentType_UNKNOWN;
using trackmanager::grpc::new_track_struct::RadarObservedTargetProfile;
using trackmanager::grpc::new_track_struct::RealityType_VIRTUAL;
using trackmanager::grpc::new_track_struct::TargetObject;
using trackmanager::grpc::new_track_struct::TargetSourceItem;
using trackmanager::grpc::new_track_struct::ThreatLevel_HIGH;
using trackmanager::grpc::new_track_struct::ThreatLevel_LOW;
using trackmanager::grpc::new_track_struct::ThreatLevel_MEDIUM;
using trackmanager::grpc::new_track_struct::UnitType_DRONE;

constexpr double kEps = 1e-6;

const RadarObservedTargetProfile* findRadarProfile(const TargetObject& t)
{
    for (int i = 0; i < t.sources_size(); ++i) {
        const TargetSourceItem& src = t.sources(i);
        if (!src.source_profile().radar_source_present()) {
            continue;
        }
        return &src.source_profile().radar_source().target_profile();
    }
    return nullptr;
}

uint32_t zibaoweiTrackIdFromFusionSources(const TargetObject& t)
{
    for (int i = 0; i < t.sources_size(); ++i) {
        const TargetSourceItem& src = t.sources(i);
        if (!src.source_profile().radar_source_present()) {
            continue;
        }
        const auto& fusionSources =
            src.source_profile().radar_source().target_profile().fusionsources();
        for (int j = 0; j < fusionSources.size(); ++j) {
            if (fusionSources.Get(j).datasourceid() == "zibaowei") {
                return fusionSources.Get(j).trackid();
            }
        }
    }
    return 0U;
}

void fillFusionTrackIds(const TargetObject& t, SPxPacketTrackExtended& out)
{
    int idx = 0;
    for (int i = 0; i < t.sources_size(); ++i) {
        const TargetSourceItem& src = t.sources(i);
        if (!src.source_profile().radar_source_present()) {
            continue;
        }
        const auto& fusionSources =
            src.source_profile().radar_source().target_profile().fusionsources();
        for (int j = 0; j < fusionSources.size(); ++j) {
            const FusionSourceItem& fs = fusionSources.Get(j);
            if (fs.datasourceid() == "tan_niao") {
                continue;
            }
            if (idx < 8) {
                out.fusion.trackID[idx++] = fs.trackid();
            }
        }
    }
    if (idx > 0) {
        out.fusion.sensors = (1U << idx) - 1U;
    }
    const uint32_t zibaoweiId = zibaoweiTrackIdFromFusionSources(t);
    if (zibaoweiId != 0U) {
        out.fusion.trackID[0] = zibaoweiId;
    }
}

double pickFirstNonZero(double a, double b, double c = 0.0)
{
    if (std::fabs(a) > kEps) {
        return a;
    }
    if (std::fabs(b) > kEps) {
        return b;
    }
    return c;
}

double courseFromEnu(double e, double n)
{
    if (std::fabs(e) < kEps && std::fabs(n) < kEps) {
        return 0.0;
    }
    double deg = std::atan2(e, n) * 180.0 / M_PI;
    if (deg < 0) {
        deg += 360.0;
    }
    return deg;
}

uint32_t threatScoreFromPriority(const TargetObject& t)
{
    const auto& th = t.priority().threat();
    switch (th.threat_level()) {
    case ThreatLevel_HIGH:
        return 75U;
    case ThreatLevel_MEDIUM:
        return 50U;
    case ThreatLevel_LOW:
        return 25U;
    default:
        break;
    }
    if (th.confidence() > kEps) {
        return static_cast<uint32_t>(th.confidence() * 100.0);
    }
    return 0U;
}

} // namespace

TargetRouteDecision routeTarget(
    const TargetObject& target,
    const QStringList& allowedVirtualSourceIds)
{
    TargetRouteDecision decision;
    decision.sourceId = QString::fromStdString(target.target_board().entity_id()).trimmed();

    if (target.reality_type() == RealityType_VIRTUAL
        && !allowedVirtualSourceIds.contains(decision.sourceId)) {
        decision.reason = QStringLiteral("virtual_source_not_allowed");
        return decision;
    }

    if (target.environment() == EnvironmentType_SURFACE
        || target.environment() == EnvironmentType_MARITIME
        || target.environment() == EnvironmentType_SUBSURFACE) {
        decision.domain = TargetRouteDomain::Surface;
        return decision;
    }
    if (target.environment() == EnvironmentType_AIR) {
        decision.domain = TargetRouteDomain::Air;
        return decision;
    }
    if (target.environment() == EnvironmentType_UNKNOWN) {
        decision.reason = QStringLiteral("unknown_environment");
        return decision;
    }

    decision.reason = QStringLiteral("unsupported_environment");
    return decision;
}

bool parseTargetId(const TargetObject& t, qint64* outTargetId)
{
    if (outTargetId == nullptr) {
        return false;
    }
    const QString s = QString::fromStdString(t.target_id()).trimmed();
    if (s.isEmpty()) {
        return false;
    }
    bool ok = false;
    const qint64 v = s.toLongLong(&ok);
    if (!ok || v <= 0) {
        return false;
    }
    *outTargetId = v;
    return true;
}

uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming)
{
    if (previous != 0U && incoming == 0U) {
        return previous;
    }
    return incoming;
}

bool targetToSpxExtended(const TargetObject& t, SPxPacketTrackExtended& out, bool /*birdTopic*/)
{
    qint64 targetId = 0;
    if (!parseTargetId(t, &targetId)) {
        return false;
    }

    std::memset(&out, 0, sizeof(out));
    const RadarObservedTargetProfile* r = findRadarProfile(t);
    const auto& kin = t.target_kinematics();
    const auto& pos = kin.position();

    out.latDegs = static_cast<float>(pickFirstNonZero(pos.latitude(), r ? r->latdegs() : 0.0));
    out.longDegs = static_cast<float>(pickFirstNonZero(pos.longitude(), r ? r->longdegs() : 0.0));
    out.altitudeMetres =
        static_cast<float>(pickFirstNonZero(pos.altitude(), r ? r->altitudemetres() : 0.0));
    out.norm.min.rangeMetres = static_cast<float>(
        pickFirstNonZero(t.fused_range_m(), r ? r->range_m() : 0.0));
    out.norm.min.azimuthDegrees = static_cast<float>(
        pickFirstNonZero(t.fused_azimuth_deg(), r ? r->azimuth_deg() : 0.0));
    out.norm.min.speedMps = static_cast<float>(kin.speed());

    const double yaw = kin.target_orientation().yaw();
    out.norm.min.courseDegrees = static_cast<float>(pickFirstNonZero(
        yaw, courseFromEnu(kin.velocity_enu().e(), kin.velocity_enu().n())));

    if (r != nullptr) {
        const auto& plot = r->plot_profile();
        out.norm.min.sizeMetres = static_cast<float>(plot.end_range_m() - plot.start_range_m());
        out.norm.min.sizeDegrees =
            static_cast<float>(plot.end_azimuth_deg() - plot.start_azimuth_deg());
    }

    // 用「本机收到该目标这一帧」的时间做存活时钟。
    // 勿用 last_update_time：上游常不刷新该字段，会导致 >4s/8s prune 把仍在推送的目标误删，告警中断。
    out.msgTimeSecs = static_cast<uint32_t>(QDateTime::currentDateTime().toSecsSinceEpoch());
    out.msgTimeUsecs = 0;
    out.norm.min.id = 0;
    if (targetId <= static_cast<qint64>(std::numeric_limits<uint32_t>::max())) {
        out.secondary.uniqueID = static_cast<uint32_t>(targetId);
    } else {
        out.secondary.uniqueID = static_cast<uint32_t>(targetId & 0xFFFFFFFFULL);
    }
    out.norm.reserved3 = threatScoreFromPriority(t);
    fillFusionTrackIds(t, out);

    if (t.classified_type() == UnitType_DRONE) {
        out.norm.min.reserved1 = 3;
    } else {
        out.norm.min.reserved1 = 1;
    }
    return true;
}

} // namespace NewTrackStructGrpcConvert
