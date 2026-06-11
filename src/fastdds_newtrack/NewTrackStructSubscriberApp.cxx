#include "NewTrackStructSubscriberApp.hpp"

#include <cmath>
#include <stdexcept>

#include <QDateTime>
#include <QPointF>
#include <QProcessEnvironment>
#include <QReadLocker>
#include <QSettings>
#include <QWriteLocker>
#include <QDebug>

#include "NewTrackStructConvert.hpp"

#include "customconfig.h"
#include "dialog/alarm/AlarmFileLogger.h"
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/qos/SubscriberQos.hpp>

using namespace eprosima::fastdds::dds;

namespace {
struct NewTrackSubDdsConfig
{
    bool use_profile = true;
    QString xml_file = QStringLiteral("newtrack_sub_recv.xml");
    QString participant_profile = QStringLiteral("participant_newtrack_sub");
    QString topic_fuse = QStringLiteral("TrackTopic_NewStruct_FuseTrack");
    QString topic_bird = QStringLiteral("TrackTopic_NewStruct_FuseBirdTrack");
};

NewTrackSubDdsConfig loadNewTrackSubDdsConfig()
{
    NewTrackSubDdsConfig c;
    QSettings s(QStringLiteral("Config.ini"), QSettings::IniFormat);
    c.use_profile = s.value(QStringLiteral("DDS/NewTrackSubUseProfile"), 1).toInt() != 0;
    c.xml_file = s.value(QStringLiteral("DDS/NewTrackSubXml"), c.xml_file).toString();
    c.participant_profile =
        s.value(QStringLiteral("DDS/NewTrackSubParticipantProfile"), c.participant_profile).toString();
    c.topic_fuse = s.value(QStringLiteral("DDS/NewTrackSubTopicFuse"), c.topic_fuse).toString();
    c.topic_bird = s.value(QStringLiteral("DDS/NewTrackSubTopicBird"), c.topic_bird).toString();
    return c;
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

void logTrackDdsFieldCompare(const TargetFull::TargetOutputSet& sample)
{
    static qint64 s_lastLogMs = 0;
    static int s_logged = 0;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (s_logged >= 5 && nowMs - s_lastLogMs < 30000) {
        return;
    }
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }

    for (const TargetFull::TargetObject& t : sample.targets()) {
        const TargetFull::RadarObservedTargetProfile* r = findRadarProfile(t);
        const uint32_t radarId = r ? static_cast<uint32_t>(r->track_id()) : 0U;
        uint32_t targetIdInt = 0U;
        bool ok = false;
        const QString tidStr = QString::fromStdString(t.target_id()).trimmed();
        targetIdInt = tidStr.toUInt(&ok);

        QReadLocker rl(&cfg->m_trackDataLock);
        int oldId = -1;
        if (radarId != 0U && cfg->m_mapFuseTrack.contains(static_cast<int>(radarId))) {
            oldId = static_cast<int>(radarId);
        } else if (ok && cfg->m_mapFuseTrack.contains(static_cast<int>(targetIdInt))) {
            oldId = static_cast<int>(targetIdInt);
        } else {
            continue;
        }

        const SPxPacketTrackExtended& oldTr = cfg->m_mapFuseTrack.value(oldId);
        const auto& kin = t.target_kinematics();
        const auto& pos = kin.position();
        const double kinCourse = courseFromEnu(kin.velocity_enu().e(), kin.velocity_enu().n());

        qInfo().noquote()
            << "[TrackDDS-Compare] matchKey=oldId" << oldId
            << "new.target_id=" << tidStr
            << "radar.track_id=" << radarId
            << "radar.unique_id=" << (r ? r->unique_id() : 0UL);
        qInfo().noquote()
            << "  lat: old=" << oldTr.latDegs << " kin=" << pos.latitude()
            << " radar=" << (r ? r->latDegs() : 0.0)
            << " | lon: old=" << oldTr.longDegs << " kin=" << pos.longitude()
            << " radar=" << (r ? r->longDegs() : 0.0);
        qInfo().noquote()
            << "  alt: old=" << oldTr.altitudeMetres << " kin=" << pos.altitude()
            << " radar=" << (r ? r->altitudeMetres() : 0.0);
        qInfo().noquote()
            << "  range: old=" << oldTr.norm.min.rangeMetres << " fused=" << t.fused_range_m()
            << " radar=" << (r ? r->range_m() : 0.0);
        qInfo().noquote()
            << "  az: old=" << oldTr.norm.min.azimuthDegrees << " fused=" << t.fused_azimuth_deg()
            << " radar=" << (r ? r->azimuth_deg() : 0.0);
        qInfo().noquote()
            << "  speed: old=" << oldTr.norm.min.speedMps << " kin=" << kin.speed();
        qInfo().noquote()
            << "  course: old=" << oldTr.norm.min.courseDegrees << " kin_atan2=" << kinCourse;
        qInfo().noquote()
            << "  unique: old=" << oldTr.secondary.uniqueID
            << " radar=" << (r ? r->unique_id() : 0UL);
        qInfo().noquote()
            << "  fusionID[0]: old=" << oldTr.fusion.trackID[0]
            << " new0=" << (r && !r->fusionSources().empty() ? r->fusionSources().front().trackId() : 0UL);

        ++s_logged;
        s_lastLogMs = nowMs;
        if (s_logged >= 5) {
            break;
        }
    }
}

void clearStaleTrackMaps()
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const QDateTime curTime = QDateTime::currentDateTime();
    QWriteLocker wl(&cfg->m_trackDataLock);

    auto pruneMapQint64 = [&](QMap<qint64, SPxPacketTrackExtended>& trackMap, int staleSecs) {
        const QList<qint64> keys = trackMap.keys();
        for (qint64 key : keys) {
            if (!trackMap.contains(key)) {
                continue;
            }
            const SPxPacketTrackExtended track = trackMap.value(key);
            const QDateTime time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(track.msgTimeSecs));
            if (time.secsTo(curTime) > staleSecs) {
                trackMap.remove(key);
            }
        }
    };

    pruneMapQint64(cfg->m_mapFuseTrack, 8);
    pruneMapQint64(cfg->m_mapBirdFuseTrack, 4);
}

void appendTrailPoint(QList<QPointF>& trail, const SPxPacketTrackExtended& track, int maxPoints)
{
    const QPointF pt(track.latDegs, track.longDegs);
    if (trail.contains(pt)) {
        return;
    }
    if (trail.size() > maxPoints) {
        trail.removeFirst();
    }
    trail.append(pt);
}

void applyTargetOutputSet(const TargetFull::TargetOutputSet& sample, bool isBirdTopic)
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const int fuseMode = cfg->m_alarmLogic.mode;

    QWriteLocker wl(&cfg->m_trackDataLock);
    for (const TargetFull::TargetObject& t : sample.targets()) {
        SPxPacketTrackExtended track;
        const int mapMode = isBirdTopic ? 0 : fuseMode;
        if (!NewTrackStructConvert::targetToSpxExtended(t, mapMode, track, isBirdTopic)) {
            continue;
        }

        qint64 targetId = 0;
        if (!NewTrackStructConvert::parseTargetId(t, &targetId) || targetId <= 0) {
            continue;
        }

        if (isBirdTopic) {
            NewTrackStructConvert::applyBirdFuseExtras(t, track);
            uint32_t prevUnique = 0U;
            if (cfg->m_mapBirdFuseTrack.contains(targetId)) {
                prevUnique = cfg->m_mapBirdFuseTrack.value(targetId).secondary.uniqueID;
                track.secondary.uniqueID =
                    NewTrackStructConvert::preserveStableUniqueId(prevUnique, track.secondary.uniqueID);
            }
            cfg->m_mapBirdFuseTrack.insert(targetId, track);
            appendTrailPoint(cfg->m_mapBirdFuseTrail[targetId], track, 10);

            int fusionSourceCount = 0;
            for (const TargetFull::TargetSourceItem& src : t.sources()) {
                if (!src.source_profile().has_radar_source()) {
                    continue;
                }
                fusionSourceCount += static_cast<int>(
                    src.source_profile().radar_source().target_profile().fusionSources().size());
            }
            const int fusionTid0 = static_cast<int>(track.fusion.trackID[0]);
            const bool inSkipList = cfg->m_alarmLogic.birdSkipTrackIds.contains(fusionTid0);
            if (fusionTid0 == 0 || inSkipList) {
                AlarmFileLogger::logNewAlarmTrack(
                    QStringLiteral("BirdDDS ingest------mapKey:%1 target_id:%2 external_id:%3 fusionTid0:%4 fusionSourceCount:%5 inSkipList:%6 reserved1:%7")
                        .arg(targetId)
                        .arg(QString::fromStdString(t.target_id()))
                        .arg(QString::fromStdString(t.external_target_id()))
                        .arg(fusionTid0)
                        .arg(fusionSourceCount)
                        .arg(inSkipList ? 1 : 0)
                        .arg(track.norm.min.reserved1));
            } else if (track.secondary.uniqueID != prevUnique) {
                AlarmFileLogger::logNewAlarmTrack(
                    QStringLiteral("BirdDDS fusion------mapKey:%1 target_id:%2 uniqueID:%3 prevUnique:%4 fusionTid0:%5")
                        .arg(targetId)
                        .arg(QString::fromStdString(t.target_id()))
                        .arg(track.secondary.uniqueID)
                        .arg(prevUnique)
                        .arg(fusionTid0));
            }
        } else {
            if (cfg->m_mapFuseTrack.contains(targetId)) {
                const uint32_t prevUnique = cfg->m_mapFuseTrack.value(targetId).secondary.uniqueID;
                track.secondary.uniqueID =
                    NewTrackStructConvert::preserveStableUniqueId(prevUnique, track.secondary.uniqueID);
            }
            cfg->m_mapFuseTrack.insert(targetId, track);
            const int trailMax = (fuseMode == 1) ? 15 : 10;
            appendTrailPoint(cfg->m_mapFuseTrail[targetId], track, trailMax);
        }
    }
}
} // namespace

NewTrackStructSubscriberApp::NewTrackStructSubscriberApp(const int& domain_id)
    : factory_(nullptr)
    , type_(new TargetFull::TargetOutputSetPubSubType())
{
    const NewTrackSubDdsConfig dds_cfg = loadNewTrackSubDdsConfig();
    topic_name_fuse_ = dds_cfg.topic_fuse.toStdString();
    topic_name_bird_ = dds_cfg.topic_bird.toStdString();

    DomainParticipantQos pqos = PARTICIPANT_QOS_DEFAULT;
    pqos.name("newtrack_struct_sub_participant");
    factory_ = DomainParticipantFactory::get_shared_instance();
    if (dds_cfg.use_profile)
    {
        DomainParticipantFactory::get_instance()->load_XML_profiles_file(dds_cfg.xml_file.toStdString());
        participant_ = DomainParticipantFactory::get_instance()->create_participant_with_profile(
            domain_id, dds_cfg.participant_profile.toStdString());
    }
    else
    {
        participant_ = factory_->create_participant(domain_id, pqos, nullptr, StatusMask::none());
    }
    if (participant_ == nullptr)
    {
        throw std::runtime_error("NewTrackStruct subscriber participant initialization failed");
    }

    type_.register_type(participant_);

    SubscriberQos sub_qos = SUBSCRIBER_QOS_DEFAULT;
    participant_->get_default_subscriber_qos(sub_qos);
    subscriber_ = participant_->create_subscriber(sub_qos, nullptr, StatusMask::none());
    if (subscriber_ == nullptr)
    {
        throw std::runtime_error("NewTrackStruct subscriber initialization failed");
    }

    topic_fuse_ = participant_->create_topic(topic_name_fuse_, type_.get_type_name(), TOPIC_QOS_DEFAULT);
    topic_bird_ = participant_->create_topic(topic_name_bird_, type_.get_type_name(), TOPIC_QOS_DEFAULT);
    if (topic_fuse_ == nullptr || topic_bird_ == nullptr)
    {
        throw std::runtime_error("NewTrackStruct topic initialization failed");
    }

    DataReaderQos reader_qos = DATAREADER_QOS_DEFAULT;
    subscriber_->get_default_datareader_qos(reader_qos);
    reader_qos.reliability().kind = ReliabilityQosPolicyKind::RELIABLE_RELIABILITY_QOS;
    reader_qos.durability().kind = DurabilityQosPolicyKind::TRANSIENT_LOCAL_DURABILITY_QOS;
    reader_qos.history().kind = HistoryQosPolicyKind::KEEP_ALL_HISTORY_QOS;
    reader_fuse_ = subscriber_->create_datareader(topic_fuse_, reader_qos, this, StatusMask::all());
    reader_bird_ = subscriber_->create_datareader(topic_bird_, reader_qos, this, StatusMask::all());
    if (reader_fuse_ == nullptr || reader_bird_ == nullptr)
    {
        throw std::runtime_error("NewTrackStruct reader initialization failed");
    }
}

NewTrackStructSubscriberApp::~NewTrackStructSubscriberApp()
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

void NewTrackStructSubscriberApp::stop()
{
    stop_.store(true);
}

void NewTrackStructSubscriberApp::on_subscription_matched(
    DataReader* /*reader*/, const SubscriptionMatchedStatus& info)
{
    if (info.current_count_change == 1)
    {
        std::cout << "NewTrackStruct subscriber matched." << std::endl;
    }
    else if (info.current_count_change == -1)
    {
        std::cout << "NewTrackStruct subscriber unmatched." << std::endl;
    }
}

void NewTrackStructSubscriberApp::on_data_available(DataReader* reader)
{
    if (stop_.load())
    {
        return;
    }

    TargetFull::TargetOutputSet sample;
    SampleInfo info;
    const std::string topic_name = reader->get_topicdescription()->get_name();
    const bool isFuse = (topic_name == topic_name_fuse_);
    const bool isBird = (topic_name == topic_name_bird_);
    TargetFull::TargetOutputSet batch;
    bool gotBatch = false;

    while ((!stop_.load()) && (RETCODE_OK == reader->take_next_sample(&sample, &info)))
    {
        if (!info.valid_data)
        {
            continue;
        }

        batch = sample;
        gotBatch = true;

        std::lock_guard<std::mutex> lock(data_mutex_);
        if (isFuse)
        {
            latest_fuse_ = sample;
            has_fuse_ = true;
        }
        else if (isBird)
        {
            latest_bird_ = sample;
            has_bird_ = true;
        }
    }

    if (!gotBatch) {
        return;
    }

    if (isFuse) {
        applyTargetOutputSet(batch, false);
        if (qEnvironmentVariableIntValue("ALARMSYS_COMPARE_TRACK_DDS") != 0) {
            logTrackDdsFieldCompare(batch);
        }
    } else if (isBird) {
        applyTargetOutputSet(batch, true);
    }

    static qint64 s_lastClearMs = 0;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (nowMs - s_lastClearMs >= 2000) {
        clearStaleTrackMaps();
        s_lastClearMs = nowMs;
    }
}

bool NewTrackStructSubscriberApp::latest_fuse(TargetFull::TargetOutputSet& out) const
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (!has_fuse_)
    {
        return false;
    }
    out = latest_fuse_;
    return true;
}

bool NewTrackStructSubscriberApp::latest_bird(TargetFull::TargetOutputSet& out) const
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (!has_bird_)
    {
        return false;
    }
    out = latest_bird_;
    return true;
}

bool NewTrackStructSubscriberApp::build_merged_latest(TargetFull::TargetOutputSet& out) const
{
    std::lock_guard<std::mutex> lock(data_mutex_);
    if (!has_fuse_ && !has_bird_)
    {
        return false;
    }

    out = TargetFull::TargetOutputSet();
    std::vector<TargetFull::TargetObject> merged_targets;
    std::vector<TargetFull::GroupObject> merged_groups;

    if (has_fuse_)
    {
        const auto& t = latest_fuse_.targets();
        merged_targets.insert(merged_targets.end(), t.begin(), t.end());
        const auto& g = latest_fuse_.groups();
        merged_groups.insert(merged_groups.end(), g.begin(), g.end());
    }
    if (has_bird_)
    {
        const auto& t = latest_bird_.targets();
        merged_targets.insert(merged_targets.end(), t.begin(), t.end());
        const auto& g = latest_bird_.groups();
        merged_groups.insert(merged_groups.end(), g.begin(), g.end());
    }

    out.targets(std::move(merged_targets));
    out.groups(std::move(merged_groups));
    return true;
}

