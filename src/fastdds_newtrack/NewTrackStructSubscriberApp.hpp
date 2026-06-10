#pragma once

#include <atomic>
#include <mutex>
#include <string>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

#ifdef signals
#pragma push_macro("signals")
#undef signals
#define ALARMSYS_RESTORE_QT_SIGNALS_MACRO
#endif
#include "NewTrackRealTimeStatus.hpp"
#include "NewTrackRealTimeStatusPubSubTypes.hpp"
#ifdef ALARMSYS_RESTORE_QT_SIGNALS_MACRO
#pragma pop_macro("signals")
#undef ALARMSYS_RESTORE_QT_SIGNALS_MACRO
#endif

class NewTrackStructSubscriberApp : public eprosima::fastdds::dds::DataReaderListener
{
public:
    explicit NewTrackStructSubscriberApp(const int& domain_id);
    ~NewTrackStructSubscriberApp();

    void stop();

    bool latest_fuse(TargetFull::TargetOutputSet& out) const;
    bool latest_bird(TargetFull::TargetOutputSet& out) const;
    bool build_merged_latest(TargetFull::TargetOutputSet& out) const;

    void on_data_available(eprosima::fastdds::dds::DataReader* reader) override;
    void on_subscription_matched(
        eprosima::fastdds::dds::DataReader* reader,
        const eprosima::fastdds::dds::SubscriptionMatchedStatus& info) override;

private:
    std::shared_ptr<eprosima::fastdds::dds::DomainParticipantFactory> factory_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Subscriber* subscriber_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_fuse_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_bird_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_fuse_ = nullptr;
    eprosima::fastdds::dds::DataReader* reader_bird_ = nullptr;
    eprosima::fastdds::dds::TypeSupport type_;

    std::string topic_name_fuse_;
    std::string topic_name_bird_;

    mutable std::mutex data_mutex_;
    TargetFull::TargetOutputSet latest_fuse_;
    TargetFull::TargetOutputSet latest_bird_;
    bool has_fuse_ = false;
    bool has_bird_ = false;
    std::atomic<bool> stop_ { false };
};

