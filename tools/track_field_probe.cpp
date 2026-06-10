/**
 * 在新 TargetOutputSet 融合 topic 中查找指定航迹，打印含目标值的字段路径。
 * 用法: ./track_field_probe <matchId> <needle> [秒数]
 * 例:   ./track_field_probe 176 4003 15
 * matchId: 匹配 external_target_id 或 radar.track_id 或 target_id 字符串
 */
#include "NewTrackRealTimeStatusPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/topic/Topic.hpp>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace eprosima::fastdds::dds;

namespace {

constexpr int kDomainId = 141;
constexpr char kNewXml[] = "newtrack_sub_recv.xml";
constexpr char kNewProfile[] = "participant_newtrack_sub";
constexpr char kTopicFuse[] = "TrackTopic_NewStruct_FuseTrack";

struct Hit {
    std::string path;
    std::string value;
};

std::mutex g_mu;
std::vector<Hit> g_hits;
std::atomic<uint64_t> g_samples{0};
std::atomic<bool> g_matched{false};

void addHit(const std::string& path, const std::string& value)
{
    std::lock_guard<std::mutex> lk(g_mu);
    g_hits.push_back({path, value});
}

void scanKv(const std::string& prefix, const std::vector<TargetFull::KeyValuePair>& kvs, uint64_t needle)
{
    for (size_t i = 0; i < kvs.size(); ++i) {
        const auto& kv = kvs[i];
        const std::string key = kv.key();
        const std::string val = kv.value();
        try {
            if (!val.empty() && std::stoull(val) == needle) {
                addHit(prefix + "[" + std::to_string(i) + "].value(key=" + key + ")", val);
            }
        } catch (...) {
        }
        if (val.find(std::to_string(needle)) != std::string::npos) {
            addHit(prefix + "[" + std::to_string(i) + "].value contains", val);
        }
    }
}

void scanU64(const std::string& path, uint64_t v, uint64_t needle)
{
    if (v == needle) {
        addHit(path, std::to_string(v));
    }
}

const char* envName(TargetFull::EnvironmentType e)
{
    switch (e) {
    case TargetFull::EnvironmentType::LAND: return "LAND";
    case TargetFull::EnvironmentType::AIR: return "AIR";
    case TargetFull::EnvironmentType::SURFACE: return "SURFACE";
    case TargetFull::EnvironmentType::SUBSURFACE: return "SUBSURFACE";
    case TargetFull::EnvironmentType::SPACE: return "SPACE";
    case TargetFull::EnvironmentType::MARITIME: return "MARITIME";
    default: return "UNKNOWN";
    }
}

const TargetFull::RadarObservedTargetProfile* findRadar(const TargetFull::TargetObject& t)
{
    for (const TargetFull::TargetSourceItem& src : t.sources()) {
        if (src.source_profile().has_radar_source()) {
            return &src.source_profile().radar_source().target_profile();
        }
    }
    return nullptr;
}

bool trackMatches(const TargetFull::TargetObject& t, const std::string& matchId)
{
    if (t.external_target_id() == matchId || t.target_id() == matchId) {
        return true;
    }
    const auto* r = findRadar(t);
    if (r != nullptr && std::to_string(r->track_id()) == matchId) {
        return true;
    }
    for (const TargetFull::TargetSourceItem& src : t.sources()) {
        if (src.source_track_id() == matchId) {
            return true;
        }
    }
    return false;
}

void scanTarget(const TargetFull::TargetObject& t, uint64_t needle)
{
    g_hits.clear();
    try {
        if (!t.external_target_id().empty() && std::stoull(t.external_target_id()) == needle) {
            addHit("external_target_id", t.external_target_id());
        }
        if (!t.target_id().empty() && std::stoull(t.target_id()) == needle) {
            addHit("target_id", t.target_id());
        }
    } catch (...) {
    }

    if (const auto* r = findRadar(t)) {
        scanU64("sources[].radar.target_profile.track_id", r->track_id(), needle);
        scanU64("sources[].radar.target_profile.unique_id", r->unique_id(), needle);
        for (size_t i = 0; i < r->fusionSources().size(); ++i) {
            const auto& fs = r->fusionSources()[i];
            scanU64("fusionSources[" + std::to_string(i) + "].trackId", fs.trackId(), needle);
            if (fs.dataSourceId().find(std::to_string(needle)) != std::string::npos) {
                addHit("fusionSources[" + std::to_string(i) + "].dataSourceId", fs.dataSourceId());
            }
        }
        scanKv("radar.attributes", r->attributes(), needle);
    }

    for (size_t si = 0; si < t.sources().size(); ++si) {
        const auto& src = t.sources()[si];
        const std::string p = "sources[" + std::to_string(si) + "]";
        if (src.source_track_id() == std::to_string(needle)) {
            addHit(p + ".source_track_id", src.source_track_id());
        }
        if (src.source_track_id().find(std::to_string(needle)) != std::string::npos) {
            addHit(p + ".source_track_id contains", src.source_track_id());
        }
        if (src.entity_id().find(std::to_string(needle)) != std::string::npos) {
            addHit(p + ".entity_id contains", src.entity_id());
        }
        scanKv(p + ".identity_attributes", src.identity_attributes(), needle);
        if (src.source_profile().has_radar_source()) {
            const auto& rtp = src.source_profile().radar_source().target_profile();
            scanU64(p + ".radar.track_id", rtp.track_id(), needle);
            scanU64(p + ".radar.unique_id", rtp.unique_id(), needle);
        }
        if (src.source_profile().has_ais_source()) {
            const auto& ap = src.source_profile().ais_target_profile();
            if (ap.mmsi() == std::to_string(needle)) {
                addHit(p + ".ais_target_profile.mmsi", ap.mmsi());
            }
        }
    }
}

class ProbeListener : public DataReaderListener
{
public:
    explicit ProbeListener(std::string matchId, uint64_t needle)
        : match_id_(std::move(matchId))
        , needle_(needle)
    {
    }

    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
    {
        if (info.current_count_change > 0) {
            g_matched.store(true);
        }
    }

    void on_data_available(DataReader* reader) override
    {
        TargetFull::TargetOutputSet sample;
        SampleInfo info;
        while (RETCODE_OK == reader->take_next_sample(&sample, &info)) {
            if (!info.valid_data) {
                continue;
            }
            ++g_samples;
            for (const TargetFull::TargetObject& t : sample.targets()) {
                if (!trackMatches(t, match_id_)) {
                    continue;
                }
                scanTarget(t, needle_);
                std::lock_guard<std::mutex> lk(g_mu);
                if (!g_hits.empty()) {
                    found_ = true;
                }
                last_ = t;
                got_ = true;
            }
        }
    }

    bool got() const { return got_; }
    bool found() const { return found_; }
    TargetFull::TargetObject last() const { return last_; }

private:
    std::string match_id_;
    uint64_t needle_ = 0;
    bool got_ = false;
    bool found_ = false;
    TargetFull::TargetObject last_{};
};

} // namespace

void scanAirByExternal(DomainParticipant* participant, const std::string& extId, uint64_t needle, int seconds)
{
    TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
    type.register_type(participant);
    Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    Topic* topic = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    TargetFull::TargetObject found{};
    bool got = false;
    std::vector<Hit> hits;

    class ScanListener : public DataReaderListener {
    public:
        ScanListener(std::string ext, uint64_t n, TargetFull::TargetObject* out, bool* gotFlag, std::vector<Hit>* outHits)
            : ext_(std::move(ext))
            , needle_(n)
            , out_(out)
            , got_flag_(gotFlag)
            , out_hits_(outHits)
        {
        }

        void on_data_available(DataReader* reader) override
        {
            TargetFull::TargetOutputSet sample;
            SampleInfo info;
            while (RETCODE_OK == reader->take_next_sample(&sample, &info)) {
                if (!info.valid_data) {
                    continue;
                }
                for (const TargetFull::TargetObject& t : sample.targets()) {
                    if (t.environment() != TargetFull::EnvironmentType::AIR) {
                        continue;
                    }
                    if (t.external_target_id() != ext_) {
                        continue;
                    }
                    scanTarget(t, needle_);
                    std::lock_guard<std::mutex> lk(g_mu);
                    *out_ = t;
                    *got_flag_ = true;
                    if (!g_hits.empty()) {
                        *out_hits_ = g_hits;
                    }
                }
            }
        }

    private:
        std::string ext_;
        uint64_t needle_ = 0;
        TargetFull::TargetObject* out_;
        bool* got_flag_;
        std::vector<Hit>* out_hits_;
    };

    ScanListener listener(extId, needle, &found, &got, &hits);
    sub->create_datareader(topic, rqos, &listener);
    std::cout << "scan AIR ext=" << extId << " needle=" << needle << " sec=" << seconds << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(seconds));

    if (!got) {
        std::cout << "NO_AIR_TRACK external_target_id=" << extId << "\n";
        return;
    }

    const TargetFull::TargetObject& t = found;
    std::cout << "\n========== 对空融合 ext=" << extId << " ==========\n";
    std::cout << "target_id=" << t.target_id() << "\n";
    std::cout << "environment=" << envName(t.environment())
              << " classified_type=" << static_cast<int>(t.classified_type()) << "\n";
    std::cout << "sources.size=" << t.sources().size() << "\n";
    uint64_t zibaoweiTrackId = 0;
    bool hasZibaowei = false;
    for (size_t si = 0; si < t.sources().size(); ++si) {
        const auto& src = t.sources()[si];
        const auto& sp = src.source_profile();
        std::cout << "  sources[" << si << "] entity_id=" << src.entity_id()
                  << " source_track_id=" << src.source_track_id()
                  << " has_radar_source=" << sp.has_radar_source() << "\n";
        if (!sp.has_radar_source()) {
            continue;
        }
        const auto& rtp = sp.radar_source().target_profile();
        std::cout << "    radar.target_profile.track_id=" << rtp.track_id()
                  << " unique_id=" << rtp.unique_id() << "\n";
        std::cout << "    fusionSources.size=" << rtp.fusionSources().size() << "\n";
        for (size_t fi = 0; fi < rtp.fusionSources().size(); ++fi) {
            const auto& fs = rtp.fusionSources()[fi];
            std::cout << "      fusionSources[" << fi << "] dataSourceId=" << fs.dataSourceId()
                      << " sourceName=" << fs.sourceName()
                      << " trackId=" << fs.trackId();
            if (fs.dataSourceId() == "zibaowei") {
                hasZibaowei = true;
                zibaoweiTrackId = fs.trackId();
                std::cout << "  <-- 自报位 trackId (对应老 fusion.trackID[0])";
            }
            std::cout << "\n";
        }
    }
    std::cout << "\n自报位 zibaowei.trackId=" << (hasZibaowei ? std::to_string(zibaoweiTrackId) : "(未找到)")
              << "\n";
    std::cout << "\n========== 含 " << needle << " 的字段 ==========\n";
    if (hits.empty()) {
        std::cout << "(未发现 " << needle << ")\n";
    } else {
        for (const auto& h : hits) {
            std::cout << "  " << h.path << " = " << h.value << "\n";
        }
    }
}

void scanZibaoweiTracks(DomainParticipant* participant, int seconds)
{
    TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
    type.register_type(participant);
    Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    Topic* topic = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    class ZbListener : public DataReaderListener {
    public:
        void on_data_available(DataReader* reader) override
        {
            TargetFull::TargetOutputSet sample;
            SampleInfo info;
            while (RETCODE_OK == reader->take_next_sample(&sample, &info)) {
                if (!info.valid_data) continue;
                targets_ += sample.targets().size();
                for (const TargetFull::TargetObject& t : sample.targets()) {
                    if (t.external_target_id() == "2616") {
                        seen2616_ = true;
                    }
                    for (size_t si = 0; si < t.sources().size(); ++si) {
                        const auto& sp = t.sources()[si].source_profile();
                        if (!sp.has_radar_source()) continue;
                        with_radar_++;
                        const auto& rtp = sp.radar_source().target_profile();
                        if (rtp.fusionSources().empty()) continue;
                        for (size_t fi = 0; fi < rtp.fusionSources().size(); ++fi) {
                            const auto& fs = rtp.fusionSources()[fi];
                            std::cout << "ext=" << t.external_target_id()
                                      << " env=" << envName(t.environment())
                                      << " fusionSources[" << fi << "] dataSourceId=" << fs.dataSourceId()
                                      << " trackId=" << fs.trackId()
                                      << " sourceName=" << fs.sourceName() << "\n";
                        }
                    }
                }
            }
        }
        uint64_t targets_ = 0;
        uint64_t with_radar_ = 0;
        bool seen2616_ = false;
    } zb;
    sub->create_datareader(topic, rqos, &zb);
    std::cout << "scan all fusionSources (has_radar) sec=" << seconds << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    std::cout << "stats targets=" << zb.targets_ << " with_radar=" << zb.with_radar_
              << " seen_ext_2616=" << (zb.seen2616_ ? "yes" : "no") << "\n";
}

int main(int argc, char* argv[])
{
    if (argc >= 2 && std::string(argv[1]) == "--scan-zibaowei") {
        const int seconds = (argc >= 3) ? std::atoi(argv[2]) : 25;
        auto* factory = DomainParticipantFactory::get_instance();
        factory->load_XML_profiles_file(kNewXml);
        DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
        if (!participant) return 1;
        scanZibaoweiTracks(participant, seconds);
        participant->delete_contained_entities();
        factory->delete_participant(participant);
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "--find-ext") {
        const std::string extId = argv[2];
        const uint64_t needle = (argc >= 4) ? std::stoull(argv[3]) : 0;
        const int seconds = (argc >= 5) ? std::atoi(argv[4]) : 20;
        auto* factory = DomainParticipantFactory::get_instance();
        factory->load_XML_profiles_file(kNewXml);
        DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
        if (!participant) return 1;
        TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
        type.register_type(participant);
        Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        Topic* topic = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        bool got = false;
        TargetFull::TargetObject last{};
        std::vector<Hit> hits;
        class Fnd : public DataReaderListener {
        public:
            Fnd(std::string e, uint64_t n, bool* g, TargetFull::TargetObject* o, std::vector<Hit>* h)
                : e_(std::move(e)), n_(n), g_(g), o_(o), h_(h) {}
            void on_data_available(DataReader* r) override
            {
                TargetFull::TargetOutputSet s;
                SampleInfo i;
                while (RETCODE_OK == r->take_next_sample(&s, &i)) {
                    if (!i.valid_data) continue;
                    for (const TargetFull::TargetObject& t : s.targets()) {
                        if (t.external_target_id() != e_) continue;
                        scanTarget(t, n_);
                        std::lock_guard<std::mutex> lk(g_mu);
                        *o_ = t;
                        *g_ = true;
                        if (!g_hits.empty()) *h_ = g_hits;
                    }
                }
            }
        private:
            std::string e_;
            uint64_t n_;
            bool* g_;
            TargetFull::TargetObject* o_;
            std::vector<Hit>* h_;
        } fnd(extId, needle, &got, &last, &hits);
        sub->create_datareader(topic, rqos, &fnd);
        std::cout << "find ext=" << extId << " needle=" << needle << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        if (!got) {
            std::cout << "NO ext=" << extId << "\n";
        } else {
            const auto& t = last;
            std::cout << "env=" << envName(t.environment()) << " target_id=" << t.target_id()
                      << " cls=" << static_cast<int>(t.classified_type()) << "\n";
            for (size_t si = 0; si < t.sources().size(); ++si) {
                const auto& src = t.sources()[si];
                std::cout << "  sources[" << si << "] entity=" << src.entity_id()
                          << " source_track_id=" << src.source_track_id() << "\n";
            }
            if (hits.empty()) std::cout << "needle " << needle << " NOT FOUND\n";
            else for (const auto& h : hits) std::cout << "  " << h.path << "=" << h.value << "\n";
        }
        participant->delete_contained_entities();
        factory->delete_participant(participant);
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "--list-air-ext") {
        const int seconds = (argc >= 3) ? std::atoi(argv[2]) : 15;
        auto* factory = DomainParticipantFactory::get_instance();
        factory->load_XML_profiles_file(kNewXml);
        DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
        if (!participant) return 1;
        TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
        type.register_type(participant);
        Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        Topic* topic = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        class Lst : public DataReaderListener {
        public:
            void on_data_available(DataReader* r) override
            {
                TargetFull::TargetOutputSet s;
                SampleInfo i;
                while (RETCODE_OK == r->take_next_sample(&s, &i)) {
                    if (!i.valid_data) continue;
                    for (const TargetFull::TargetObject& t : s.targets()) {
                        if (t.environment() != TargetFull::EnvironmentType::AIR) continue;
                        std::cout << "AIR ext=" << t.external_target_id()
                                  << " tid=" << t.target_id()
                                  << " src=" << t.sources().size() << "\n";
                    }
                }
            }
        } lst;
        sub->create_datareader(topic, rqos, &lst);
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        participant->delete_contained_entities();
        factory->delete_participant(participant);
        return 0;
    }
    if (argc >= 2 && std::string(argv[1]) == "--find-needle") {
        const uint64_t needle = std::stoull(argv[2]);
        const int seconds = (argc >= 3) ? std::atoi(argv[3]) : 20;
        auto* factory = DomainParticipantFactory::get_instance();
        factory->load_XML_profiles_file(kNewXml);
        DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
        if (!participant) {
            return 1;
        }
        TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
        type.register_type(participant);
        Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
        Topic* topic = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
        DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
        rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
        rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
        class FindListener : public DataReaderListener {
        public:
            explicit FindListener(uint64_t n) : needle_(n) {}
            void on_data_available(DataReader* reader) override
            {
                TargetFull::TargetOutputSet sample;
                SampleInfo info;
                while (RETCODE_OK == reader->take_next_sample(&sample, &info)) {
                    if (!info.valid_data) continue;
                    for (const TargetFull::TargetObject& t : sample.targets()) {
                        scanTarget(t, needle_);
                        std::lock_guard<std::mutex> lk(g_mu);
                        if (g_hits.empty()) continue;
                        std::cout << "HIT ext=" << t.external_target_id()
                                  << " target_id=" << t.target_id()
                                  << " env=" << envName(t.environment()) << "\n";
                        for (const auto& h : g_hits) {
                            std::cout << "    " << h.path << " = " << h.value << "\n";
                        }
                    }
                }
            }
        private:
            uint64_t needle_;
        };
        FindListener fl(needle);
        sub->create_datareader(topic, rqos, &fl);
        std::cout << "find needle=" << needle << " sec=" << seconds << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
        participant->delete_contained_entities();
        factory->delete_participant(participant);
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "--air-ext") {
        const std::string extId = argv[2];
        const uint64_t needle = (argc >= 4) ? std::stoull(argv[3]) : 0;
        const int seconds = (argc >= 5) ? std::atoi(argv[4]) : 18;
        auto* factory = DomainParticipantFactory::get_instance();
        factory->load_XML_profiles_file(kNewXml);
        DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
        if (!participant) {
            return 1;
        }
        scanAirByExternal(participant, extId, needle, seconds);
        participant->delete_contained_entities();
        factory->delete_participant(participant);
        return 0;
    }
    if (argc < 3) {
        std::cerr << "usage: track_field_probe <matchId> <needle> [seconds]\n"
                     "       track_field_probe --air-ext <external_id> [needle] [seconds]\n";
        return 1;
    }
    const std::string matchId = argv[1];
    const uint64_t needle = std::stoull(argv[2]);
    int seconds = (argc >= 4) ? std::atoi(argv[3]) : 15;

    auto* factory = DomainParticipantFactory::get_instance();
    factory->load_XML_profiles_file(kNewXml);
    DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
    if (!participant) {
        std::cerr << "participant failed\n";
        return 1;
    }

    TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
    type.register_type(participant);
    Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    Topic* topic = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    rqos.history().kind = KEEP_ALL_HISTORY_QOS;

    ProbeListener listener(matchId, needle);
    DataReader* reader = sub->create_datareader(topic, rqos, &listener);
    std::cout << "probe fuse topic matchId=" << matchId << " needle=" << needle
              << " sec=" << seconds << "\n";
    std::this_thread::sleep_for(std::chrono::seconds(seconds));

    if (!listener.got()) {
        std::cout << "NO_TRACK matched id=" << matchId << " samples=" << g_samples.load()
                  << " publisher_matched=" << (g_matched.load() ? "yes" : "no") << "\n";
        participant->delete_contained_entities();
        factory->delete_participant(participant);
        return 2;
    }

    const TargetFull::TargetObject& t = listener.last();
    std::cout << "\n========== 航迹 matchId=" << matchId << " ==========\n";
    std::cout << "target_id=" << t.target_id() << "\n";
    std::cout << "external_target_id=" << t.external_target_id() << "\n";
    std::cout << "environment=" << envName(t.environment())
              << " classified_type=" << static_cast<int>(t.classified_type()) << "\n";
    const bool isAir = (t.environment() == TargetFull::EnvironmentType::AIR);
    std::cout << "is_air_fusion_candidate=" << (isAir ? "yes" : "no")
              << " (对空融合应 environment=AIR)\n";
    const auto* r = findRadar(t);
    std::cout << "has_radar=" << (r != nullptr) << "\n";
    if (r) {
        std::cout << "radar.track_id=" << r->track_id() << " radar.unique_id=" << r->unique_id() << "\n";
        std::cout << "fusionSources.size=" << r->fusionSources().size() << "\n";
        for (size_t i = 0; i < r->fusionSources().size(); ++i) {
            const auto& fs = r->fusionSources()[i];
            std::cout << "  fusionSources[" << i << "] dataSourceId=" << fs.dataSourceId()
                      << " trackId=" << fs.trackId() << " sourceName=" << fs.sourceName() << "\n";
        }
    }
    std::cout << "sources.size=" << t.sources().size() << "\n";
    for (size_t si = 0; si < t.sources().size(); ++si) {
        const auto& src = t.sources()[si];
        std::cout << "  sources[" << si << "] entity_id=" << src.entity_id()
                  << " source_track_id=" << src.source_track_id()
                  << " has_radar=" << src.source_profile().has_radar_source()
                  << " has_ais=" << src.source_profile().has_ais_source();
        if (src.source_profile().has_ais_source()) {
            std::cout << " mmsi=" << src.source_profile().ais_target_profile().mmsi();
        }
        std::cout << "\n";
    }

    std::cout << "\n========== 含 " << needle << " 的字段 ==========\n";
    std::vector<Hit> hits;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        hits = g_hits;
    }
    if (hits.empty()) {
        std::cout << "(未在新 DDS 结构中发现值 " << needle << ")\n";
        std::cout << "说明: 老 trackID[0]=4003 在新包里可能未下发，或字段名/层级不同。\n";
    } else {
        for (const auto& h : hits) {
            std::cout << "  " << h.path << " = " << h.value << "\n";
        }
    }

    participant->delete_contained_entities();
    factory->delete_participant(participant);
    return 0;
}
