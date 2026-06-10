/**
 * 同时订阅老 TrackDataClass 融合 topic 与新 TargetOutputSet 融合 topic，
 * 按多种 ID 键匹配同一条航迹，打印字段对照与差值，用于确定映射取值来源。
 *
 * 用法（在 bin 目录，与 Config.ini / XML 同级）:
 *   ./track_dds_compare_test [秒数，默认25]
 */
#include "TrackClassPubSubTypes.hpp"
#include "NewTrackRealTimeStatusPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#include "TrackClass.hpp"
#include "NewTrackRealTimeStatus.hpp"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace eprosima::fastdds::dds;

namespace {

constexpr int kDomainId = 141;
constexpr char kOldXml[] = "trackclass_recv_multi.xml";
constexpr char kOldProfile[] = "participant_trackclass_recv_multi";
constexpr char kOldTopic[] = "TrackDataClassTopic_FuseTrack";
constexpr char kNewXml[] = "newtrack_sub_recv.xml";
constexpr char kNewProfile[] = "participant_newtrack_sub";
constexpr char kNewTopicFuse[] = "TrackTopic_NewStruct_FuseTrack";
constexpr char kNewTopicBird[] = "TrackTopic_NewStruct_FuseBirdTrack";
constexpr char kOldTopicBird[] = "TrackDataClassTopic_FuseBirdRadarTrack";

struct OldSnap {
    TrackDataClass sample;
    std::chrono::steady_clock::time_point t;
};

struct NewFieldBundle {
    std::string target_id;
    std::string external_target_id;
    uint32_t radar_track_id = 0;
    uint32_t radar_unique_id = 0;
    double kin_lat = 0, kin_lon = 0, kin_alt = 0;
    double fused_lat = 0, fused_lon = 0, fused_alt = 0;
    double radar_lat = 0, radar_lon = 0, radar_alt = 0;
    double fused_range = 0, fused_az = 0;
    double radar_range = 0, radar_az = 0;
    double kin_speed = 0, kin_course = 0, kin_yaw = 0;
    double vel_e = 0, vel_n = 0, vel_u = 0;
    double plot_size_m = 0, plot_size_deg = 0;
    double threat_conf = 0;
    int threat_level = -1;
    uint32_t fusion_track_ids[8] = {};
    int fusion_src_count = 0;
    uint32_t sensors_mask = 0; // 尝试从 sources 推断，仅展示
    bool has_radar = false;
};

struct NewSnap {
    TargetFull::TargetObject target;
    NewFieldBundle fields;
    std::chrono::steady_clock::time_point t;
};

std::mutex g_mu;
std::map<uint32_t, OldSnap> g_old_by_track_id;
std::map<uint32_t, OldSnap> g_old_by_reserved4;
std::map<uint32_t, NewSnap> g_new_by_radar_track_id;
std::map<uint32_t, NewSnap> g_new_by_target_id_int;
std::atomic<uint64_t> g_old_samples{0};
std::atomic<uint64_t> g_new_samples{0};
std::atomic<bool> g_old_matched{false};
std::atomic<bool> g_new_matched{false};

double courseFromEnu(double e, double n)
{
    if (std::fabs(e) < 1e-9 && std::fabs(n) < 1e-9) {
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
        unsigned long v = std::stoul(s, &pos, 10);
        if (pos != s.size()) {
            return false;
        }
        out = static_cast<uint32_t>(v);
        return true;
    } catch (...) {
        return false;
    }
}

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

NewFieldBundle extractNewFields(const TargetFull::TargetObject& t)
{
    NewFieldBundle f;
    f.target_id = t.target_id();
    f.external_target_id = t.external_target_id();
    const auto& kin = t.target_kinematics();
    const auto& pos = kin.position();
    f.kin_lat = pos.latitude();
    f.kin_lon = pos.longitude();
    f.kin_alt = pos.altitude();
    f.kin_speed = kin.speed();
    f.vel_e = kin.velocity_enu().e();
    f.vel_n = kin.velocity_enu().n();
    f.vel_u = kin.velocity_enu().u();
    f.kin_course = courseFromEnu(f.vel_e, f.vel_n);
    f.kin_yaw = kin.target_orientation().yaw();
    f.fused_range = t.fused_range_m();
    f.fused_az = t.fused_azimuth_deg();
    f.threat_conf = t.priority().threat().confidence();
    f.threat_level = static_cast<int>(t.priority().threat().threat_level());

    if (const auto* r = findRadarProfile(t)) {
        f.has_radar = true;
        f.radar_track_id = static_cast<uint32_t>(r->track_id());
        f.radar_unique_id = static_cast<uint32_t>(r->unique_id());
        f.radar_lat = r->latDegs();
        f.radar_lon = r->longDegs();
        f.radar_alt = r->altitudeMetres();
        f.radar_range = r->range_m();
        f.radar_az = r->azimuth_deg();
        int idx = 0;
        for (const TargetFull::FusionSourceItem& fs : r->fusionSources()) {
            if (fs.dataSourceId() == "tan_niao") {
                continue;
            }
            if (idx < 8) {
                f.fusion_track_ids[idx++] = static_cast<uint32_t>(fs.trackId());
            }
        }
        f.fusion_src_count = idx;
        const auto& plot = r->plot_profile();
        f.plot_size_m = plot.end_range_m() - plot.start_range_m();
        f.plot_size_deg = plot.end_azimuth_deg() - plot.start_azimuth_deg();
    }
    return f;
}

void storeOld(const TrackDataClass& st)
{
    const uint32_t tid = static_cast<uint32_t>(st.trackId());
    const uint32_t r4 = static_cast<uint32_t>(st.reserved4());
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lk(g_mu);
    g_old_by_track_id[tid] = OldSnap{st, now};
    if (r4 != 0) {
        g_old_by_reserved4[r4] = OldSnap{st, now};
    }
}

void storeNew(const TargetFull::TargetObject& t)
{
    NewFieldBundle f = extractNewFields(t);
    const auto now = std::chrono::steady_clock::now();
    NewSnap snap{t, f, now};
    std::lock_guard<std::mutex> lk(g_mu);
    if (f.has_radar && f.radar_track_id != 0) {
        g_new_by_radar_track_id[f.radar_track_id] = snap;
    }
    uint32_t tid = 0;
    if (parseUint(f.target_id, tid) && tid != 0) {
        g_new_by_target_id_int[tid] = snap;
    }
}

class OldListener : public DataReaderListener
{
public:
    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
    {
        if (info.current_count_change == 1) {
            std::cout << "[old] matched publishers=" << info.current_count << std::endl;
            g_old_matched.store(true);
        }
    }

    void on_data_available(DataReader* reader) override
    {
        TrackDataClass st;
        SampleInfo info;
        while (RETCODE_OK == reader->take_next_sample(&st, &info)) {
            if (!info.valid_data) {
                continue;
            }
            ++g_old_samples;
            storeOld(st);
        }
    }
};

class NewListener : public DataReaderListener
{
public:
    explicit NewListener(std::string label) : label_(std::move(label)) {}

    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
    {
        if (info.current_count_change == 1) {
            std::cout << "[new:" << label_ << "] matched publishers=" << info.current_count << std::endl;
            g_new_matched.store(true);
        }
    }

    void on_data_available(DataReader* reader) override
    {
        TargetFull::TargetOutputSet set;
        SampleInfo info;
        while (RETCODE_OK == reader->take_next_sample(&set, &info)) {
            if (!info.valid_data) {
                continue;
            }
            ++g_new_samples;
            for (const TargetFull::TargetObject& t : set.targets()) {
                storeNew(t);
            }
        }
    }

private:
    std::string label_;
};

std::string diffLine(const char* name, double oldV, double newV, double thr = 1e-4)
{
    std::ostringstream os;
    const double d = newV - oldV;
    os << "  " << std::setw(22) << std::left << name << " old=" << std::setw(14) << oldV
       << " new=" << std::setw(14) << newV << " delta=" << d;
    if (std::fabs(d) <= thr) {
        os << "  [OK]";
    } else {
        os << "  [DIFF]";
    }
    return os.str();
}

void printPair(uint32_t matchId, const char* matchKey, const OldSnap& o, const NewSnap& n)
{
    const TrackDataClass& st = o.sample;
    const NewFieldBundle& f = n.fields;
    std::cout << "\n========== match id=" << matchId << " key=" << matchKey << " ==========\n";
    std::cout << "old.trackId=" << st.trackId() << " reserved4=" << st.reserved4()
              << " uniqueId=" << st.uniqueId() << "\n";
    std::cout << "new.target_id=" << f.target_id << " external=" << f.external_target_id
              << " radar.track_id=" << f.radar_track_id << " radar.unique_id=" << f.radar_unique_id
              << "\n";

    std::cout << diffLine("lat(kin)", st.latitude(), f.kin_lat) << "\n";
    std::cout << diffLine("lat(radar)", st.latitude(), f.radar_lat) << "\n";
    std::cout << diffLine("lon(kin)", st.longitude(), f.kin_lon) << "\n";
    std::cout << diffLine("lon(radar)", st.longitude(), f.radar_lon) << "\n";
    std::cout << diffLine("height(kin_alt)", st.height(), f.kin_alt) << "\n";
    std::cout << diffLine("height(radar_alt)", st.height(), f.radar_alt) << "\n";
    std::cout << diffLine("distance(fused_range)", st.distance(), f.fused_range) << "\n";
    std::cout << diffLine("distance(radar_range)", st.distance(), f.radar_range) << "\n";
    std::cout << diffLine("azimuth(fused)", st.azimuth(), f.fused_az) << "\n";
    std::cout << diffLine("azimuth(radar)", st.azimuth(), f.radar_az) << "\n";
    std::cout << diffLine("speed(kin)", st.speed(), f.kin_speed) << "\n";
    std::cout << diffLine("course(kin_atan2)", st.course(), f.kin_course) << "\n";
    std::cout << diffLine("course(kin_yaw)", st.course(), f.kin_yaw) << "\n";
    std::cout << diffLine("sizeMetres(plot)", st.sizeMetres(), f.plot_size_m) << "\n";
    std::cout << diffLine("sizeDegrees(plot)", st.sizeDegrees(), f.plot_size_deg) << "\n";
    std::cout << "  threatScore(old)=" << st.threatScore()
              << " new.threat_conf=" << f.threat_conf << " new.threat_level=" << f.threat_level << "\n";
    std::cout << "  sensors(old)=" << st.sensors() << " fusion_src_count=" << f.fusion_src_count << "\n";
    std::cout << "  old.trackID[0..3]=" << st.trackID().at(0) << "," << st.trackID().at(1) << ","
              << st.trackID().at(2) << "," << st.trackID().at(3) << "\n";
    std::cout << "  new.fusion_trackID[0..3]=" << f.fusion_track_ids[0] << "," << f.fusion_track_ids[1]
              << "," << f.fusion_track_ids[2] << "," << f.fusion_track_ids[3] << "\n";
}

struct MatchResult {
    uint32_t id = 0;
    std::string key;
    double score = 0; // 位置+速度综合差，越小越好
};

std::optional<MatchResult> bestNewForOld(uint32_t oldId, const OldSnap& o)
{
    std::vector<std::pair<std::string, NewSnap>> candidates;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        auto it = g_new_by_radar_track_id.find(oldId);
        if (it != g_new_by_radar_track_id.end()) {
            candidates.emplace_back("radar.track_id", it->second);
        }
        it = g_new_by_target_id_int.find(oldId);
        if (it != g_new_by_target_id_int.end()) {
            candidates.emplace_back("target_id", it->second);
        }
    }
    if (candidates.empty()) {
        return std::nullopt;
    }
    const TrackDataClass& st = o.sample;
    MatchResult best;
    bool has = false;
    for (const auto& c : candidates) {
        const NewFieldBundle& f = c.second.fields;
        auto scoreOf = [&](double lat, double lon, double spd) {
            return std::fabs(lat - st.latitude()) + std::fabs(lon - st.longitude()) + std::fabs(spd - st.speed());
        };
        const double sKin = scoreOf(f.kin_lat, f.kin_lon, f.kin_speed);
        const double sRadar = scoreOf(f.radar_lat, f.radar_lon, f.kin_speed);
        const double s = std::min(sKin, sRadar);
        if (!has || s < best.score) {
            has = true;
            best.id = oldId;
            best.key = c.first + (sKin <= sRadar ? "+kin" : "+radar");
            best.score = s;
        }
    }
    if (!has) {
        return std::nullopt;
    }
    return best;
}

} // namespace

int main(int argc, char* argv[])
{
    int seconds = 25;
    if (argc >= 2) {
        seconds = std::atoi(argv[1]);
        if (seconds < 5) {
            seconds = 5;
        }
        if (seconds > 120) {
            seconds = 120;
        }
    }

    std::cout << "track_dds_compare_test domain=" << kDomainId << " duration=" << seconds << "s\n";
    std::cout << "old fuse: " << kOldTopic << "  old bird: " << kOldTopicBird << "\n";
    std::cout << "new fuse: " << kNewTopicFuse << "  new bird: " << kNewTopicBird << "\n";

    DomainParticipantFactory::get_instance()->load_XML_profiles_file(kOldXml);

    // 单 participant 订阅全部 topic，避免双 participant 发现异常
    DomainParticipant* part = DomainParticipantFactory::get_instance()->create_participant_with_profile(
        kDomainId, kOldProfile);
    if (!part) {
        std::cerr << "create participant failed\n";
        return 1;
    }

    TypeSupport oldTypeSupport(new TrackDataClassPubSubType());
    TypeSupport newTypeSupport(new TargetFull::TargetOutputSetPubSubType());
    oldTypeSupport.register_type(part);
    newTypeSupport.register_type(part);

    Subscriber* sub = part->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    Topic* oldTopicFuse = part->create_topic(kOldTopic, oldTypeSupport.get_type_name(), TOPIC_QOS_DEFAULT);
    Topic* oldTopicBird = part->create_topic(kOldTopicBird, oldTypeSupport.get_type_name(), TOPIC_QOS_DEFAULT);
    Topic* newTopicFuse = part->create_topic(kNewTopicFuse, newTypeSupport.get_type_name(), TOPIC_QOS_DEFAULT);
    Topic* newTopicBird = part->create_topic(kNewTopicBird, newTypeSupport.get_type_name(), TOPIC_QOS_DEFAULT);

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 1;

    OldListener oldLisFuse;
    OldListener oldLisBird;
    NewListener newLisFuse("fuse");
    NewListener newLisBird("bird");
    DataReader* oldReaderFuse = sub->create_datareader(oldTopicFuse, rqos, &oldLisFuse, StatusMask::all());
    DataReader* oldReaderBird = sub->create_datareader(oldTopicBird, rqos, &oldLisBird, StatusMask::all());
    DataReader* newReaderFuse = sub->create_datareader(newTopicFuse, rqos, &newLisFuse, StatusMask::all());
    DataReader* newReaderBird = sub->create_datareader(newTopicBird, rqos, &newLisBird, StatusMask::all());
    if (!oldReaderFuse || !newReaderFuse) {
        std::cerr << "create datareader failed\n";
        return 1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(seconds));

    std::cout << "\n========== 统计 ==========\n";
    std::cout << "old_samples=" << g_old_samples.load() << " new_samples=" << g_new_samples.load()
              << " old_pub_matched=" << g_old_matched.load() << " new_pub_matched=" << g_new_matched.load()
              << "\n";

    size_t oldIds = 0, newRadarIds = 0, newTargetIds = 0, matched = 0;
    std::vector<uint32_t> matchedIds;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        oldIds = g_old_by_track_id.size();
        newRadarIds = g_new_by_radar_track_id.size();
        newTargetIds = g_new_by_target_id_int.size();
        for (const auto& p : g_old_by_track_id) {
            if (g_new_by_radar_track_id.count(p.first) || g_new_by_target_id_int.count(p.first)) {
                ++matched;
                matchedIds.push_back(p.first);
            }
        }
    }
    std::cout << "old_unique_trackId=" << oldIds << " new_unique_radar_track_id=" << newRadarIds
              << " new_unique_target_id=" << newTargetIds << " id_intersection=" << matched << "\n";

    int printed = 0;
    for (uint32_t id : matchedIds) {
        if (printed >= 5) {
            break;
        }
        OldSnap o;
        NewSnap n;
        std::string key;
        {
            std::lock_guard<std::mutex> lk(g_mu);
            o = g_old_by_track_id.at(id);
            if (g_new_by_radar_track_id.count(id)) {
                n = g_new_by_radar_track_id.at(id);
                key = "trackId==radar.track_id";
            } else {
                n = g_new_by_target_id_int.at(id);
                key = "trackId==target_id";
            }
        }
        printPair(id, key.c_str(), o, n);
        ++printed;
    }

    // reserved4 与 target_id 交叉（Mode=1 场景）
    int r4printed = 0;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        for (const auto& p : g_old_by_reserved4) {
            if (r4printed >= 3 || p.first == 0) {
                continue;
            }
            auto it = g_new_by_target_id_int.find(p.first);
            if (it == g_new_by_target_id_int.end()) {
                continue;
            }
            printPair(p.first, "reserved4==target_id(int)", p.second, it->second);
            ++r4printed;
        }
    }

    if (printed == 0) {
        std::cout << "\n无 trackId 交集，尝试按位置最近匹配（最多3条）...\n";
        std::vector<std::pair<uint32_t, OldSnap>> olds;
        std::vector<std::pair<uint32_t, NewSnap>> news;
        {
            std::lock_guard<std::mutex> lk(g_mu);
            for (const auto& p : g_old_by_track_id) {
                olds.push_back(p);
            }
            for (const auto& p : g_new_by_radar_track_id) {
                news.push_back(p);
            }
        }
        int near = 0;
        for (const auto& o : olds) {
            if (near >= 3) {
                break;
            }
            auto bm = bestNewForOld(o.first, o.second);
            if (!bm) {
                continue;
            }
            NewSnap n;
            {
                std::lock_guard<std::mutex> lk(g_mu);
                auto it = g_new_by_radar_track_id.find(o.first);
                if (it != g_new_by_radar_track_id.end()) {
                    n = it->second;
                } else {
                    n = g_new_by_target_id_int.at(o.first);
                }
            }
            printPair(o.first, bm->key.c_str(), o.second, n);
            ++near;
        }
    }

    // 同一条航迹：new.target_id == old.reserved4 && new.external == old.trackId
    {
        std::lock_guard<std::mutex> lk(g_mu);
        for (const auto& p : g_old_by_reserved4) {
            if (p.first == 0) {
                continue;
            }
            auto nit = g_new_by_target_id_int.find(p.first);
            if (nit == g_new_by_target_id_int.end()) {
                continue;
            }
            const TrackDataClass& o = p.second.sample;
            const NewFieldBundle& nf = nit->second.fields;
            uint32_t ext = 0;
            if (!parseUint(nf.external_target_id, ext) || ext != static_cast<uint32_t>(o.trackId())) {
                continue;
            }
            std::cout << "\n########## 选定同一条航迹（身份对齐）##########\n";
            std::cout << "关联: new.target_id == old.reserved4 == old.uniqueId == " << p.first << "\n";
            std::cout << "      new.external_target_id == old.trackId == " << o.trackId() << "\n\n";
            std::cout << "[老 TrackDataClass 原始值]\n";
            std::cout << "  trackId=" << o.trackId() << " reserved4=" << o.reserved4()
                      << " uniqueId=" << o.uniqueId() << "\n";
            std::cout << "  latitude=" << o.latitude() << " longitude=" << o.longitude()
                      << " height=" << o.height() << "\n";
            std::cout << "  distance=" << o.distance() << " azimuth=" << o.azimuth()
                      << " speed=" << o.speed() << " course=" << o.course() << "\n";
            std::cout << "  sizeMetres=" << o.sizeMetres() << " sizeDegrees=" << o.sizeDegrees()
                      << " threatScore=" << o.threatScore() << " sensors=" << o.sensors() << "\n";
            std::cout << "  trackID[0..2]=" << o.trackID().at(0) << "," << o.trackID().at(1)
                      << "," << o.trackID().at(2) << "\n\n";
            std::cout << "[新 TargetObject 候选字段]\n";
            std::cout << "  target_id=" << nf.target_id << " external_target_id=" << nf.external_target_id
                      << " has_radar=" << nf.has_radar << "\n";
            std::cout << "  ① kin.position lat/lon/alt: " << nf.kin_lat << ", " << nf.kin_lon
                      << ", " << nf.kin_alt << "\n";
            std::cout << "  ② radar lat/lon/alt: " << nf.radar_lat << ", " << nf.radar_lon
                      << ", " << nf.radar_alt << "\n";
            std::cout << "  ① fused_range_m: " << nf.fused_range << "  ② radar.range_m: " << nf.radar_range
                      << "\n";
            std::cout << "  ① fused_azimuth_deg: " << nf.fused_az << "  ② radar.azimuth_deg: " << nf.radar_az
                      << "\n";
            std::cout << "  ① kin.speed: " << nf.kin_speed << "  vel_enu(e,n,u): " << nf.vel_e << ","
                      << nf.vel_n << "," << nf.vel_u << "\n";
            std::cout << "  ② kin course atan2: " << nf.kin_course << "  ① kin yaw: " << nf.kin_yaw << "\n";
            std::cout << "  threat_level=" << nf.threat_level << " threat_conf=" << nf.threat_conf
                      << " fusion_src_count=" << nf.fusion_src_count << "\n\n";
            printPair(p.first, "canonical", p.second, nit->second);
            std::cout << "\n########## SPxPacketTrackExtended 填写结论 ##########\n";
            std::cout << "norm.min.id / map键(Mode0): external_target_id -> " << o.trackId() << "\n";
            std::cout << "latDegs: target_kinematics.position.latitude\n";
            std::cout << "longDegs: target_kinematics.position.longitude\n";
            std::cout << "altitudeMetres: target_kinematics.position.altitude\n";
            std::cout << "norm.min.rangeMetres: fused_range_m\n";
            std::cout << "norm.min.azimuthDegrees: fused_azimuth_deg\n";
            std::cout << "norm.min.speedMps: target_kinematics.speed\n";
            std::cout << "norm.min.courseDegrees: target_kinematics.target_orientation.yaw (非 atan2)\n";
            std::cout << "secondary.uniqueID: target_id 或 radar.unique_id\n";
            std::cout << "norm.reserved3: priority.threat 映射分\n";
            break;
        }
    }

    std::cout << "\n========== 取值建议（基于差值 [OK] 最少的路径）==========\n";
    std::cout << "运行后查看各字段行尾 [OK]/[DIFF]：lat/lon 优先选标记 [OK] 的 kin 或 radar；\n";
    std::cout << "distance/azimuth 优先 fused_* 或 radar_* 中与老字段一致者；\n";
    std::cout << "course 对比 course(kin_atan2)；speed 对比 speed(kin)。\n";

    // 打印几条仅老结构样本，便于确认 trackId/reserved4 分布
    {
        std::lock_guard<std::mutex> lk(g_mu);
        int shown = 0;
        for (const auto& p : g_old_by_track_id) {
            if (shown >= 3) {
                break;
            }
            const auto& st = p.second.sample;
            std::cout << "\n[old-only] trackId=" << st.trackId() << " reserved4=" << st.reserved4()
                      << " lat=" << st.latitude() << " lon=" << st.longitude()
                      << " dist=" << st.distance() << " spd=" << st.speed()
                      << " course=" << st.course() << " uniqueId=" << st.uniqueId() << "\n";
            ++shown;
        }
    }

    if (part) {
        part->delete_contained_entities();
        DomainParticipantFactory::get_instance()->delete_participant(part);
    }
    return 0;
}
