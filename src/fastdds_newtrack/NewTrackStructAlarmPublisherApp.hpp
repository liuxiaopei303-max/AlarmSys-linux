#pragma once

#include <mutex>

#include <QString>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
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

class NewTrackStructAlarmPublisherApp : public eprosima::fastdds::dds::DataWriterListener
{
public:
    /** @param topic_override 非空时覆盖 Config.ini DDS/NewTrackAlarmTopic */
    NewTrackStructAlarmPublisherApp(const int& domain_id, int mode, const QString& topic_override = QString());
    ~NewTrackStructAlarmPublisherApp();

    bool send(const TargetFull::TargetOutputSet& msg);

    void on_publication_matched(
        eprosima::fastdds::dds::DataWriter* writer,
        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;

private:
    std::shared_ptr<eprosima::fastdds::dds::DomainParticipantFactory> factory_;
    eprosima::fastdds::dds::DomainParticipant* participant_ = nullptr;
    eprosima::fastdds::dds::Publisher* publisher_ = nullptr;
    eprosima::fastdds::dds::Topic* topic_ = nullptr;
    eprosima::fastdds::dds::DataWriter* writer_ = nullptr;
    eprosima::fastdds::dds::TypeSupport type_;
    std::mutex writer_mutex_;
};

