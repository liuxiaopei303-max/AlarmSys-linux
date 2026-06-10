/**
 * 订阅新 TargetOutputSet（融合+鸟情），统计 classified_type 取值。
 * 用法: ./track_classified_type_probe [秒数，默认10]
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
#include <fastdds/dds/topic/TypeSupport.hpp>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

using namespace eprosima::fastdds::dds;

namespace {

constexpr int kDomainId = 141;
constexpr char kNewXml[] = "newtrack_sub_recv.xml";
constexpr char kNewProfile[] = "participant_newtrack_sub";
constexpr char kTopicFuse[] = "TrackTopic_NewStruct_FuseTrack";
constexpr char kTopicBird[] = "TrackTopic_NewStruct_FuseBirdTrack";

const char* unitTypeName(TargetFull::UnitType t)
{
    switch (t) {
    case TargetFull::UnitType::UNKNOWN: return "UNKNOWN";
    case TargetFull::UnitType::DRONE: return "DRONE";
    case TargetFull::UnitType::BIRD: return "BIRD";
    case TargetFull::UnitType::HELICOPTER: return "HELICOPTER";
    case TargetFull::UnitType::FIXED_WING: return "FIXED_WING";
    case TargetFull::UnitType::MISSILE: return "MISSILE";
    case TargetFull::UnitType::BUOY: return "BUOY";
    case TargetFull::UnitType::SURFACE_SHIP: return "SURFACE_SHIP";
    case TargetFull::UnitType::SUBMARINE: return "SUBMARINE";
    case TargetFull::UnitType::GROUND_VEHICLE: return "GROUND_VEHICLE";
    case TargetFull::UnitType::PERSON: return "PERSON";
    case TargetFull::UnitType::ANIMAL: return "ANIMAL";
    case TargetFull::UnitType::OTHER: return "OTHER";
    default: return "INVALID";
    }
}

std::mutex g_mu;
std::map<std::string, uint64_t> g_byTopicType; // "topic|TYPE" -> count
std::map<int, uint64_t> g_rawEnum;             // raw enum int -> count
std::atomic<uint64_t> g_samples{0};
std::atomic<uint64_t> g_targets{0};
std::atomic<bool> g_matched{false};

class ProbeListener : public DataReaderListener
{
public:
    explicit ProbeListener(std::string topicLabel)
        : topic_label_(std::move(topicLabel))
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
                const TargetFull::UnitType ct = t.classified_type();
                const int raw = static_cast<int>(ct);
                const std::string key = topic_label_ + "|" + unitTypeName(ct);
                std::lock_guard<std::mutex> lk(g_mu);
                ++g_byTopicType[key];
                ++g_rawEnum[raw];
                ++g_targets;
            }
        }
    }

private:
    std::string topic_label_;
};

} // namespace

int main(int argc, char* argv[])
{
    int seconds = 10;
    if (argc >= 2) {
        seconds = std::atoi(argv[1]);
        if (seconds <= 0) {
            seconds = 10;
        }
    }

    auto* factory = DomainParticipantFactory::get_instance();
    factory->load_XML_profiles_file(kNewXml);
    DomainParticipant* participant = factory->create_participant_with_profile(kDomainId, kNewProfile);
    if (!participant) {
        std::cerr << "participant create failed\n";
        return 1;
    }

    TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
    type.register_type(participant);

    Subscriber* sub = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    Topic* topicFuse = participant->create_topic(kTopicFuse, type.get_type_name(), TOPIC_QOS_DEFAULT);
    Topic* topicBird = participant->create_topic(kTopicBird, type.get_type_name(), TOPIC_QOS_DEFAULT);

    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;
    rqos.history().kind = KEEP_ALL_HISTORY_QOS;

    ProbeListener listenerFuse("Fuse");
    ProbeListener listenerBird("Bird");
    DataReader* readerFuse = sub->create_datareader(topicFuse, rqos, &listenerFuse);
    DataReader* readerBird = sub->create_datareader(topicBird, rqos, &listenerBird);

    std::cout << "domain=" << kDomainId << " listen " << seconds << "s ...\n";
    std::this_thread::sleep_for(std::chrono::seconds(seconds));

    std::cout << "\n========== classified_type 统计 ==========\n";
    std::cout << "matched=" << (g_matched.load() ? "yes" : "no")
              << " samples=" << g_samples.load()
              << " targets=" << g_targets.load() << "\n\n";

    std::cout << "[按 topic + 类型名]\n";
    std::lock_guard<std::mutex> lk(g_mu);
    for (const auto& p : g_byTopicType) {
        std::cout << "  " << p.first << "  count=" << p.second << "\n";
    }

    std::cout << "\n[按枚举整型值]\n";
    for (const auto& p : g_rawEnum) {
        std::cout << "  raw=" << p.first << "  count=" << p.second << "\n";
    }

    std::cout << "\nIDL UnitType: 0=UNKNOWN 1=DRONE 2=BIRD 3=HELICOPTER ...\n";

    participant->delete_contained_entities();
    factory->delete_participant(participant);
    return 0;
}
