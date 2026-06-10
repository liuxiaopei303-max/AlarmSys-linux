#include "NewTrackStructConvert.hpp"

#include <cmath>
#include <cstring>

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

/** 自报位：sources[].radar_source.target_profile.fusionSources 中 dataSourceId==zibaowei 的 trackId */
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

bool parseUint(const std::string& s, uint32_t& out)
{
    if (s.empty()) {
        return false;
    }
    try {
        size_t pos = 0;
        const unsigned long v = std::stoul(s, &pos, 10);
        if (pos != s.size()) {
            return false;
        }
        out = static_cast<uint32_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

/** 稳定唯一号阈值：小于此值的 radar.unique_id 多为自报位/fusion id，不能当 uniqueID */
constexpr uint32_t kStableUniqueIdThreshold = 100000U;

bool isLikelyFusionOrDisplayId(uint32_t id, uint32_t zibaoweiId)
{
    if (id == 0U) {
        return true;
    }
    if (zibaoweiId != 0U && id == zibaoweiId) {
        return true;
    }
    return id < kStableUniqueIdThreshold;
}

uint32_t resolveSecondaryUniqueId(const TargetFull::TargetObject& t,
                                  const TargetFull::RadarObservedTargetProfile* r)
{
    uint32_t fromTargetId = 0U;
    const bool hasTargetId = parseUint(t.target_id(), fromTargetId) && fromTargetId != 0U;
    const uint32_t zibaoweiId = zibaoweiTrackIdFromFusionSources(t);

    if (hasTargetId && fromTargetId >= kStableUniqueIdThreshold) {
        return fromTargetId;
    }

    if (r != nullptr && r->unique_id() != 0UL) {
        const uint32_t radarUnique = static_cast<uint32_t>(r->unique_id());
        if (!isLikelyFusionOrDisplayId(radarUnique, zibaoweiId)) {
            return radarUnique;
        }
    }

    // 小号 target_id 多为自报位/fusion id，不能作为 uniqueID
    if (hasTargetId && !isLikelyFusionOrDisplayId(fromTargetId, zibaoweiId)) {
        return fromTargetId;
    }

    return 0U;
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

uint32_t preserveStableUniqueId(uint32_t previous, uint32_t incoming)
{
    constexpr uint32_t threshold = 100000U;
    if (previous >= threshold && incoming < threshold) {
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

int resolveFuseMapKey(const TargetFull::TargetObject& t, int alarmLogicMode, bool birdTopic)
{
    uint32_t targetId = 0U;
    const bool hasTargetId = parseUint(t.target_id(), targetId);

    if (alarmLogicMode == 1) {
        return hasTargetId ? static_cast<int>(targetId) : -1;
    }

    const TargetFull::RadarObservedTargetProfile* r = findRadarProfile(t);
    const uint32_t radarId = (r != nullptr) ? static_cast<uint32_t>(r->track_id()) : 0U;

    uint32_t extId = 0U;
    const bool hasExtId = parseUint(t.external_target_id(), extId);
    if (birdTopic) {
        // 鸟情：与 Mode0 相同优先级，但不限制 <10000（支持界面显示号如 43420）
        if (hasExtId && extId != 0U) {
            return static_cast<int>(extId);
        }
        if (radarId != 0U) {
            return static_cast<int>(radarId);
        }
        if (hasTargetId && targetId != 0U) {
            return static_cast<int>(targetId);
        }
        return -1;
    }

    // fuse topic：external_id 不限制 <10000（对海大号线如 41414）
    if (hasExtId && extId != 0U) {
        return static_cast<int>(extId);
    }
    if (radarId != 0U && radarId < 10000U) {
        return static_cast<int>(radarId);
    }
    if (hasTargetId && targetId < 10000U) {
        return static_cast<int>(targetId);
    }
    return -1;
}

bool targetToSpxExtended(
    const TargetFull::TargetObject& t,
    int alarmLogicMode,
    SPxPacketTrackExtended& out,
    bool birdTopic)
{
    const int mapKey = resolveFuseMapKey(t, alarmLogicMode, birdTopic);
    if (mapKey < 0) {
        return false;
    }

    std::memset(&out, 0, sizeof(out));

    const TargetFull::RadarObservedTargetProfile* r = findRadarProfile(t);
    const auto& kin = t.target_kinematics();
    const auto& pos = kin.position();

    // 位置：① kinematics.position ② radar.lat/long
    out.latDegs = static_cast<float>(pickFirstNonZero(pos.latitude(), r ? r->latDegs() : 0.0));
    out.longDegs = static_cast<float>(pickFirstNonZero(pos.longitude(), r ? r->longDegs() : 0.0));
    out.altitudeMetres = static_cast<float>(pickFirstNonZero(pos.altitude(), r ? r->altitudeMetres() : 0.0));

    // 距离/方位：① fused_* ② radar
    out.norm.min.rangeMetres = static_cast<float>(pickFirstNonZero(
        t.fused_range_m(), r ? r->range_m() : 0.0));
    out.norm.min.azimuthDegrees = static_cast<float>(pickFirstNonZero(
        t.fused_azimuth_deg(), r ? r->azimuth_deg() : 0.0));

    out.norm.min.speedMps = static_cast<float>(kin.speed());

    // 航向：① target_orientation.yaw ② atan2(velocity_enu)
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

    const uint32_t resolvedUniqueId = resolveSecondaryUniqueId(t, r);
    out.secondary.uniqueID = resolvedUniqueId;

    out.norm.reserved3 = static_cast<uint32_t>(threatScoreFromPriority(t));

    out.norm.min.id = static_cast<uint32_t>(mapKey);

    // fusion.trackID：sources[].radar.fusionSources；trackID[0] 优先 zibaowei（自报位）
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
