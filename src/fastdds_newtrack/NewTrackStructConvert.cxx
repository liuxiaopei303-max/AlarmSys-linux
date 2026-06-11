#include "NewTrackStructConvert.hpp"

#include <cmath>
#include <cstring>
#include <limits>

#include <QDateTime>

namespace NewTrackStructConvert {
namespace {

constexpr double kEps = 1e-6;

const TargetFull::RadarObservedTargetProfile* findRadarProfile(const TargetFull::TargetObject& t)
{
    for (const TargetFull::TargetSourceItem& src : t.sources()) {
        if (!src.source_profile().has_radar_source()) {
            continue;
        }
        return &src.source_profile().radar_source().target_profile();
    }
    return nullptr;
}

uint32_t zibaoweiTrackIdFromFusionSources(const TargetFull::TargetObject& t)
{
    for (const TargetFull::TargetSourceItem& src : t.sources()) {
        if (!src.source_profile().has_radar_source()) {
            continue;
        }
        const auto& fusionSources =
            src.source_profile().radar_source().target_profile().fusionSources();
        for (const TargetFull::FusionSourceItem& fs : fusionSources) {
            if (fs.dataSourceId() == "zibaowei") {
                return static_cast<uint32_t>(fs.trackId());
            }
        }
    }
    return 0U;
}

void fillFusionTrackIds(const TargetFull::TargetObject& t, SPxPacketTrackExtended& out)
{
    int idx = 0;
    for (const TargetFull::TargetSourceItem& src : t.sources()) {
        if (!src.source_profile().has_radar_source()) {
            continue;
        }
        for (const TargetFull::FusionSourceItem& fs :
             src.source_profile().radar_source().target_profile().fusionSources()) {
            if (fs.dataSourceId() == "tan_niao") {
                continue;
            }
            if (idx < 8) {
                out.fusion.trackID[idx++] = static_cast<uint32_t>(fs.trackId());
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

uint32_t threatScoreFromPriority(const TargetFull::TargetObject& t)
{
    const auto& th = t.priority().threat();
    switch (th.threat_level()) {
    case TargetFull::ThreatLevel::HIGH:
        return 75U;
    case TargetFull::ThreatLevel::MEDIUM:
        return 50U;
    case TargetFull::ThreatLevel::LOW:
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

bool parseTargetId(const TargetFull::TargetObject& t, qint64* outTargetId)
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

void applyAirTargetReserved1(const TargetFull::TargetObject& t, SPxPacketTrackExtended& track)
{
    if (t.classified_type() == TargetFull::UnitType::DRONE) {
        track.norm.min.reserved1 = 3;
    } else {
        track.norm.min.reserved1 = 1;
    }
}

int resolveFuseMapKey(const TargetFull::TargetObject& t, int /*alarmLogicMode*/, bool /*birdTopic*/)
{
    qint64 targetId = 0;
    if (!parseTargetId(t, &targetId)) {
        return -1;
    }
    if (targetId > static_cast<qint64>(std::numeric_limits<int>::max())) {
        return -1;
    }
    return static_cast<int>(targetId);
}

bool targetToSpxExtended(
    const TargetFull::TargetObject& t,
    int /*alarmLogicMode*/,
    SPxPacketTrackExtended& out,
    bool /*birdTopic*/)
{
    qint64 targetId = 0;
    if (!parseTargetId(t, &targetId)) {
        return false;
    }

    std::memset(&out, 0, sizeof(out));

    const TargetFull::RadarObservedTargetProfile* r = findRadarProfile(t);
    const auto& kin = t.target_kinematics();
    const auto& pos = kin.position();

    out.latDegs = static_cast<float>(pickFirstNonZero(pos.latitude(), r ? r->latDegs() : 0.0));
    out.longDegs = static_cast<float>(pickFirstNonZero(pos.longitude(), r ? r->longDegs() : 0.0));
    out.altitudeMetres = static_cast<float>(pickFirstNonZero(pos.altitude(), r ? r->altitudeMetres() : 0.0));

    out.norm.min.rangeMetres = static_cast<float>(pickFirstNonZero(
        t.fused_range_m(), r ? r->range_m() : 0.0));
    out.norm.min.azimuthDegrees = static_cast<float>(pickFirstNonZero(
        t.fused_azimuth_deg(), r ? r->azimuth_deg() : 0.0));

    out.norm.min.speedMps = static_cast<float>(kin.speed());

    const double yaw = kin.target_orientation().yaw();
    out.norm.min.courseDegrees = static_cast<float>(pickFirstNonZero(
        yaw, courseFromEnu(kin.velocity_enu().e(), kin.velocity_enu().n())));

    if (r != nullptr) {
        const auto& plot = r->plot_profile();
        out.norm.min.sizeMetres = static_cast<float>(plot.end_range_m() - plot.start_range_m());
        out.norm.min.sizeDegrees = static_cast<float>(plot.end_azimuth_deg() - plot.start_azimuth_deg());
    }

    out.msgTimeSecs = static_cast<uint32_t>(QDateTime::currentDateTime().toSecsSinceEpoch());
    out.msgTimeUsecs = 0;

    out.norm.min.id = 0;
    if (targetId <= static_cast<qint64>(std::numeric_limits<uint32_t>::max())) {
        out.secondary.uniqueID = static_cast<uint32_t>(targetId);
    } else {
        out.secondary.uniqueID = static_cast<uint32_t>(targetId & 0xFFFFFFFFULL);
    }

    out.norm.reserved3 = static_cast<uint32_t>(threatScoreFromPriority(t));

    fillFusionTrackIds(t, out);
    applyAirTargetReserved1(t, out);

    return true;
}

void applyBirdFuseExtras(const TargetFull::TargetObject& t, SPxPacketTrackExtended& track)
{
    applyAirTargetReserved1(t, track);
    fillFusionTrackIds(t, track);
}

} // namespace NewTrackStructConvert
