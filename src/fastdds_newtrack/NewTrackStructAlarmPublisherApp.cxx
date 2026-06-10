#include "NewTrackStructAlarmPublisherApp.hpp"

#include <stdexcept>

#include <QSettings>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/qos/DataWriterQos.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>

using namespace eprosima::fastdds::dds;

namespace {
struct NewTrackAlarmDdsConfig
{
    bool use_profile = true;
    QString xml_multi = QStringLiteral("newtrack_alarm_multi.xml");
    QString participant_profile_multi = QStringLiteral("participant_newtrack_alarm_multi");
    QString xml_single = QStringLiteral("newtrack_alarm_single.xml");
    QString participant_profile_single = QStringLiteral("participant_newtrack_alarm_single");
    QString topic_name = QStringLiteral("NewTrackStructAlarm");
};

NewTrackAlarmDdsConfig loadNewTrackAlarmDdsConfig()
{
    NewTrackAlarmDdsConfig c;
    QSettings s(QStringLiteral("Config.ini"), QSettings::IniFormat);
    c.use_profile = s.value(QStringLiteral("DDS/NewTrackAlarmUseProfile"), 1).toInt() != 0;
    c.xml_multi = s.value(QStringLiteral("DDS/NewTrackAlarmXmlMulti"), c.xml_multi).toString();
    c.participant_profile_multi =
        s.value(QStringLiteral("DDS/NewTrackAlarmParticipantProfileMulti"), c.participant_profile_multi).toString();
    c.xml_single = s.value(QStringLiteral("DDS/NewTrackAlarmXmlSingle"), c.xml_single).toString();
    c.participant_profile_single =
        s.value(QStringLiteral("DDS/NewTrackAlarmParticipantProfileSingle"), c.participant_profile_single).toString();
    c.topic_name = s.value(QStringLiteral("DDS/NewTrackAlarmTopic"), c.topic_name).toString();
    return c;
}
} // namespace

NewTrackStructAlarmPublisherApp::NewTrackStructAlarmPublisherApp(const int& domain_id, int mode)
    : factory_(nullptr)
    , type_(new TargetFull::TargetOutputSetPubSubType())
{
    const NewTrackAlarmDdsConfig dds_cfg = loadNewTrackAlarmDdsConfig();
    const QString xml_file = (mode == 0) ? dds_cfg.xml_multi : dds_cfg.xml_single;
    const QString participant_profile =
        (mode == 0) ? dds_cfg.participant_profile_multi : dds_cfg.participant_profile_single;

    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("newtrack_struct_alarm_pub_participant");
    factory_ = DomainParticipantFactory::get_shared_instance();
    if (dds_cfg.use_profile)
    {
        DomainParticipantFactory::get_instance()->load_XML_profiles_file(xml_file.toStdString());
        participant_ = DomainParticipantFactory::get_instance()->create_participant_with_profile(
            domain_id, participant_profile.toStdString());
    }
    else
    {
        participant_ = factory_->create_participant(domain_id, pqos, nullptr, StatusMask::none());
    }

    if (participant_ == nullptr)
    {
        throw std::runtime_error("NewTrackStructAlarm participant initialization failed");
    }

    type_.register_type(participant_);

    PublisherQos pub_qos = PUBLISHER_QOS_DEFAULT;
    participant_->get_default_publisher_qos(pub_qos);
    publisher_ = participant_->create_publisher(pub_qos, nullptr, StatusMask::none());
    if (publisher_ == nullptr)
    {
        throw std::runtime_error("NewTrackStructAlarm publisher initialization failed");
    }

    TopicQos topic_qos = TOPIC_QOS_DEFAULT;
    participant_->get_default_topic_qos(topic_qos);
    topic_ = participant_->create_topic(dds_cfg.topic_name.toStdString(), type_.get_type_name(), topic_qos);
    if (topic_ == nullptr)
    {
        throw std::runtime_error("NewTrackStructAlarm topic initialization failed");
    }

    DataWriterQos writer_qos = DATAWRITER_QOS_DEFAULT;
    publisher_->get_default_datawriter_qos(writer_qos);
    writer_qos.reliability().kind = ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
    writer_qos.durability().kind = DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS;
    writer_qos.history().kind = HistoryQosPolicyKind::KEEP_ALL_HISTORY_QOS;
    writer_ = publisher_->create_datawriter(topic_, writer_qos, this, StatusMask::all());
    if (writer_ == nullptr)
    {
        throw std::runtime_error("NewTrackStructAlarm writer initialization failed");
    }
}

NewTrackStructAlarmPublisherApp::~NewTrackStructAlarmPublisherApp()
{
    if (participant_ != nullptr)
    {
        participant_->delete_contained_entities();
        if (factory_)
        {
            factory_->delete_participant(participant_);
        }
        else
        {
            DomainParticipantFactory::get_instance()->delete_participant(participant_);
        }
    }
}

void NewTrackStructAlarmPublisherApp::on_publication_matched(
    DataWriter* /*writer*/, const PublicationMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        std::cout << "NewTrackStructAlarm publisher matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        std::cout << "NewTrackStructAlarm publisher unmatched." << std::endl;
    }
}

bool NewTrackStructAlarmPublisherApp::send(const TargetFull::TargetOutputSet& msg)
{
    std::lock_guard<std::mutex> lock(writer_mutex_);
    if (writer_ == nullptr)
    {
        return false;
    }
    TargetFull::TargetOutputSet sample = msg;
    return (RETCODE_OK == writer_->write(&sample));
}

