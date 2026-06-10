/**
 * 20s 订阅 NewTrackStructAlarm（participant_newtrack_alarm_multi / domain 141）
 * 统计接收频率与航迹 ID 列表。
 */
#include "NewTrackRealTimeStatusPubSubTypes.hpp"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
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
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace eprosima::fastdds::dds;

namespace {

constexpr int kDomainId = 141;
constexpr char kXmlFile[] = "newtrack_alarm_multi.xml";
constexpr char kProfile[] = "participant_newtrack_alarm_multi";
constexpr char kTopic[] = "NewTrackStructAlarm";

std::set<std::string> collectTrackIds(const TargetFull::TargetOutputSet& sample)
{
    std::set<std::string> ids;
    for (const TargetFull::TargetObject& t : sample.targets()) {
        if (!t.target_id().empty()) {
            ids.insert("target_id:" + t.target_id());
        }
        if (!t.external_target_id().empty()) {
            ids.insert("external:" + t.external_target_id());
        }
        for (const TargetFull::AlternateId& alt : t.alternate_ids()) {
            if (!alt.id().empty()) {
                ids.insert("alt:" + alt.type() + "=" + alt.id());
            }
        }
        for (const TargetFull::TargetSourceItem& src : t.sources()) {
            if (!src.source_track_id().empty()) {
                ids.insert("source_track_id:" + src.source_track_id());
            }
        }
    }
    return ids;
}

class AlarmSubListener : public DataReaderListener
{
public:
    void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
    {
        if (info.current_count_change == 1) {
            std::cout << "[sub] publisher matched, total=" << info.current_count << std::endl;
            matched_.store(true);
        } else if (info.current_count_change == -1) {
            std::cout << "[sub] publisher unmatched, total=" << info.current_count << std::endl;
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
            std::lock_guard<std::mutex> lock(mutex_);
            ++sample_count_;
            recv_times_ms_.push_back(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count());
            const auto ids = collectTrackIds(sample);
            for (const std::string& id : ids) {
                all_ids_.insert(id);
            }
            last_ids_ = ids;
            last_target_count_ = static_cast<int>(sample.targets().size());
            last_group_count_ = static_cast<int>(sample.groups().size());
        }
    }

    std::mutex mutex_;
    std::atomic<bool> matched_{false};
    int sample_count_ = 0;
    int last_target_count_ = 0;
    int last_group_count_ = 0;
    std::vector<long long> recv_times_ms_;
    std::set<std::string> all_ids_;
    std::set<std::string> last_ids_;
};

} // namespace

int main(int argc, char* argv[])
{
    const int duration_sec = (argc > 1) ? std::atoi(argv[1]) : 20;
    const std::string xml_path = (argc > 2) ? argv[2] : kXmlFile;

    DomainParticipantFactory::get_instance()->load_XML_profiles_file(xml_path);
    DomainParticipant* participant = DomainParticipantFactory::get_instance()->create_participant_with_profile(
        kDomainId, kProfile);
    if (participant == nullptr) {
        std::cerr << "create_participant failed, xml=" << xml_path << std::endl;
        return 1;
    }

    TypeSupport type(new TargetFull::TargetOutputSetPubSubType());
    type.register_type(participant);

    Topic* topic = participant->create_topic(kTopic, type.get_type_name(), TOPIC_QOS_DEFAULT);
    Subscriber* subscriber = participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT);
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    subscriber->get_default_datareader_qos(rqos);
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = TRANSIENT_LOCAL_DURABILITY_QOS;

    AlarmSubListener listener;
    DataReader* reader = subscriber->create_datareader(topic, rqos, &listener);
    if (reader == nullptr) {
        std::cerr << "create_datareader failed" << std::endl;
        return 1;
    }

    std::cout << "Listening domain=" << kDomainId << " topic=" << kTopic
              << " profile=" << kProfile << " xml=" << xml_path
              << " duration=" << duration_sec << "s" << std::endl;

    const auto t0 = std::chrono::steady_clock::now();
    while (std::chrono::steady_clock::now() - t0 < std::chrono::seconds(duration_sec)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    std::lock_guard<std::mutex> lock(listener.mutex_);
    const int n = listener.sample_count_;
    double hz = (duration_sec > 0) ? static_cast<double>(n) / duration_sec : 0.0;

    std::cout << "\n========== NewTrackStructAlarm 20s 统计 ==========" << std::endl;
    std::cout << "matched=" << (listener.matched_.load() ? "yes" : "no") << std::endl;
    std::cout << "samples=" << n << " avg_hz=" << hz << std::endl;
    std::cout << "last_packet targets=" << listener.last_target_count_
              << " groups=" << listener.last_group_count_ << std::endl;

    if (n >= 2 && listener.recv_times_ms_.size() >= 2) {
        long long sum_delta = 0;
        for (size_t i = 1; i < listener.recv_times_ms_.size(); ++i) {
            sum_delta += listener.recv_times_ms_[i] - listener.recv_times_ms_[i - 1];
        }
        const double avg_interval_ms =
            static_cast<double>(sum_delta) / static_cast<double>(listener.recv_times_ms_.size() - 1);
        std::cout << "avg_interval_ms=" << avg_interval_ms
                  << " (~" << (avg_interval_ms > 0 ? 1000.0 / avg_interval_ms : 0) << " Hz)" << std::endl;
    }

    std::cout << "\n--- 全部 trackId / 标识 (共 " << listener.all_ids_.size() << " 个) ---" << std::endl;
    for (const std::string& id : listener.all_ids_) {
        std::cout << "  " << id << std::endl;
    }

    std::cout << "\n--- 最后一包标识 ---" << std::endl;
    for (const std::string& id : listener.last_ids_) {
        std::cout << "  " << id << std::endl;
    }

    participant->delete_contained_entities();
    DomainParticipantFactory::get_instance()->delete_participant(participant);
    return 0;
}
