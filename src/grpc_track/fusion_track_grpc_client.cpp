#include "fusion_track_grpc_client.h"

#include "customconfig.h"

#include "fusion_track_stream.grpc.pb.h"
#include "fusion_track_stream.pb.h"

#include <QDateTime>
#include <QDebug>
#include <QWriteLocker>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <thread>

#include <grpcpp/grpcpp.h>

namespace {

using trackmanager::grpc::fusion_track::FusionTrackStreamService;
using trackmanager::grpc::fusion_track::SubscribeFusionTrackRequest;
using trackmanager::grpc::fusion_track::TrackDataClass;
using trackmanager::grpc::fusion_track::TrackDataClassBatch;

constexpr int kStaleSecs = 20;

QString routeBucketFor(const std::string& dataSourceId)
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return {};
    }
    return cfg->m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.value(
        QString::fromStdString(dataSourceId));
}

quint32 radarSourceCodeFor(const std::string& dataSourceId)
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return 0;
    }
    return cfg->m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.value(
        QString::fromStdString(dataSourceId), 0);
}

void trackDataClassToSpx(const TrackDataClass& st, SPxPacketTrackExtended& track, quint32 sourceCode)
{
    std::memset(&track, 0, sizeof(track));
    const int trackid = static_cast<int>(st.trackid());
    track.norm.min.id = static_cast<uint32_t>(trackid);
    track.longDegs = static_cast<float>(st.longitude());
    track.latDegs = static_cast<float>(st.latitude());
    track.altitudeMetres = static_cast<float>(st.height());
    track.norm.min.rangeMetres = static_cast<float>(st.distance());
    track.norm.min.azimuthDegrees = static_cast<float>(st.azimuth());
    track.norm.min.courseDegrees = static_cast<float>(st.course());
    track.norm.min.speedMps = static_cast<float>(st.speed());
    track.norm.min.sizeDegrees = static_cast<float>(st.sizedegrees());
    track.norm.min.sizeMetres = static_cast<float>(st.sizemetres());
    track.norm.reserved1 = static_cast<uint32_t>(st.trackcategoryid());
    track.msgTimeSecs = static_cast<uint32_t>(QDateTime::currentDateTime().toSecsSinceEpoch());
    const int n = st.fusion_track_ids_size();
    for (int i = 0; i < 8 && i < n; ++i) {
        track.fusion.trackID[i] = st.fusion_track_ids(i);
    }
    const auto now = std::chrono::system_clock::now();
    track.msgTimeUsecs = static_cast<uint32_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count());
    track.secondary.uniqueID = st.uniqueid();
    track.norm.reserved3 = st.reserved4();
    track.fusion.sensors = st.sensors();
    CustomConfig::setRadarDataSourceCode(track, sourceCode);
}

int spxMapKeyFromTrack(const TrackDataClass& st)
{
    const int uniqueId = static_cast<int>(st.uniqueid());
    if (uniqueId > 0) {
        return uniqueId;
    }
    return static_cast<int>(st.trackid());
}

void appendTrail(QMap<int, QList<QPointF>>& trailMap, int mapKey, double lat, double lon)
{
    QList<QPointF>& trail = trailMap[mapKey];
    const QPointF pt(lat, lon);
    if (trail.contains(pt)) {
        return;
    }
    if (trail.size() > 10) {
        trail.removeFirst();
    }
    trail.append(pt);
}

void eraseStaleSpxMap(
    QMap<int, SPxPacketTrackExtended>& trackMap,
    QMap<int, QList<QPointF>>* trailMap,
    const QDateTime& curTime)
{
    for (auto iter = trackMap.begin(); iter != trackMap.end();) {
        const QDateTime time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(iter.value().msgTimeSecs));
        if (time.secsTo(curTime) > kStaleSecs) {
            if (trailMap) {
                trailMap->remove(iter.key());
            }
            iter = trackMap.erase(iter);
        } else {
            ++iter;
        }
    }
}

void clearStaleIngestedTracks()
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const QDateTime curTime = QDateTime::currentDateTime();
    const qint64 nowSecs = curTime.toSecsSinceEpoch();
    QWriteLocker wl(&cfg->m_trackDataLock);

    eraseStaleSpxMap(cfg->m_mapRadarTrack, &cfg->m_mapRadarTrail, curTime);
    eraseStaleSpxMap(cfg->m_mapBirdRadarTrack, &cfg->m_mapBirdRadarTrail, curTime);

    for (auto iter = cfg->m_mapAISTrack.begin(); iter != cfg->m_mapAISTrack.end();) {
        if (iter.value().msgTimeSecs > 0
            && (nowSecs - static_cast<qint64>(iter.value().msgTimeSecs)) > kStaleSecs) {
            cfg->m_mapAISTrail.remove(iter.key());
            iter = cfg->m_mapAISTrack.erase(iter);
        } else {
            ++iter;
        }
    }
    for (auto it = cfg->m_mapAISTrail.begin(); it != cfg->m_mapAISTrail.end();) {
        if (!cfg->m_mapAISTrack.contains(it.key())) {
            it = cfg->m_mapAISTrail.erase(it);
        } else {
            ++it;
        }
    }
}

void applyBatch(const TrackDataClassBatch& batch)
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const int n = batch.tracks_size();
    const int ns = batch.sources_size();
    if (n <= 0) {
        return;
    }

    QWriteLocker wl(&cfg->m_trackDataLock);
    int radarN = 0;
    int aisN = 0;
    int birdN = 0;

    for (int i = 0; i < n; ++i) {
        const std::string ds = (i < ns) ? batch.sources(i).datasourceid() : std::string();
        const QString bucket = routeBucketFor(ds);
        if (bucket.isEmpty()) {
            continue;
        }
        const TrackDataClass& st = batch.tracks(i);

        if (bucket == QLatin1String("radar")) {
            const quint32 sourceCode = radarSourceCodeFor(ds);
            if (sourceCode == 0) {
                continue;
            }
            SPxPacketTrackExtended track;
            trackDataClassToSpx(st, track, sourceCode);
            const int mapKey = spxMapKeyFromTrack(st);
            if (mapKey <= 0) {
                continue;
            }
            cfg->m_mapRadarTrack.insert(mapKey, track);
            appendTrail(cfg->m_mapRadarTrail, mapKey, track.latDegs, track.longDegs);
            ++radarN;
            continue;
        }

        if (bucket == QLatin1String("ais")) {
            AISTrack ais{};
            ais.name = QString::fromStdString(st.trackalias());
            ais.MMSI = st.mmsi() != 0U ? st.mmsi() : st.trackid();
            if (ais.MMSI == 0U) {
                ais.MMSI = st.uniqueid();
            }
            if (ais.MMSI == 0U) {
                continue;
            }
            ais.shipCargoType = st.trackcategoryid();
            ais.courseDeg = st.course();
            ais.headingDeg = st.course();
            ais.latDeg = st.latitude();
            ais.longDeg = st.longitude();
            ais.speedMps = st.speed();
            ais.msgTimeSecs = static_cast<int>(QDateTime::currentSecsSinceEpoch());
            const int key = static_cast<int>(ais.MMSI);
            cfg->m_mapAISTrack.insert(key, ais);
            appendTrail(cfg->m_mapAISTrail, key, ais.latDeg, ais.longDeg);
            ++aisN;
            continue;
        }

        if (bucket == QLatin1String("bird")) {
            SPxPacketTrackExtended track;
            trackDataClassToSpx(st, track, 0);
            const int mapKey = spxMapKeyFromTrack(st);
            if (mapKey <= 0) {
                continue;
            }
            cfg->m_mapBirdRadarTrack.insert(mapKey, track);
            appendTrail(cfg->m_mapBirdRadarTrail, mapKey, track.latDegs, track.longDegs);
            ++birdN;
        }
    }

    if (radarN > 0 || aisN > 0 || birdN > 0) {
        static qint64 s_lastLogMs = 0;
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - s_lastLogMs >= 5000) {
            qInfo() << "[FusionTrackGrpc] ingest radar=" << radarN
                    << "ais=" << aisN
                    << "bird=" << birdN
                    << "radarMap=" << cfg->m_mapRadarTrack.size()
                    << "aisMap=" << cfg->m_mapAISTrack.size()
                    << "birdMap=" << cfg->m_mapBirdRadarTrack.size();
            s_lastLogMs = nowMs;
        }
    }
}

} // namespace

FusionTrackGrpcClient::FusionTrackGrpcClient(const QString& target)
    : target_(target.trimmed().isEmpty() ? QStringLiteral("192.168.18.141:60056") : target.trimmed())
{
}

FusionTrackGrpcClient::~FusionTrackGrpcClient()
{
    stop();
}

void FusionTrackGrpcClient::start()
{
    if (thread_ && thread_->joinable()) {
        return;
    }
    stop_.store(false);
    thread_ = std::make_unique<std::thread>(&FusionTrackGrpcClient::runLoop, this);
    qInfo() << "[FusionTrackGrpc] started →" << target_;
}

void FusionTrackGrpcClient::stop()
{
    stop_.store(true);
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();
}

void FusionTrackGrpcClient::runLoop()
{
    double backoff = 2.0;
    while (!stop_.load()) {
        try {
            subscribeOnce();
            backoff = 2.0;
        } catch (const std::exception& e) {
            if (stop_.load()) {
                break;
            }
            qWarning() << "[FusionTrackGrpc] exception:" << e.what();
        }
        if (stop_.load()) {
            break;
        }
        const int waitMs = static_cast<int>(backoff * 1000.0);
        for (int i = 0; i < waitMs / 100 && !stop_.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        backoff = std::min(backoff * 2.0, 30.0);
    }
}

void FusionTrackGrpcClient::subscribeOnce()
{
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(64 * 1024 * 1024);
    auto channel = grpc::CreateCustomChannel(
        target_.toStdString(), grpc::InsecureChannelCredentials(), args);
    auto stub = FusionTrackStreamService::NewStub(channel);
    if (!stub) {
        throw std::runtime_error("FusionTrackStreamService NewStub failed");
    }

    grpc::ClientContext context;
    SubscribeFusionTrackRequest req;
    auto stream = stub->Subscribe(&context, req);
    if (!stream) {
        throw std::runtime_error("FusionTrack Subscribe returned null");
    }
    qInfo() << "[FusionTrackGrpc] connected" << target_
            << "routes"
            << (CustomConfig::getInstance()
                    ? CustomConfig::getInstance()->m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute
                    : QHash<QString, QString>());

    TrackDataClassBatch batch;
    qint64 lastClearMs = 0;
    while (!stop_.load() && stream->Read(&batch)) {
        applyBatch(batch);
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - lastClearMs >= 2000) {
            clearStaleIngestedTracks();
            lastClearMs = nowMs;
        }
    }
    const grpc::Status st = stream->Finish();
    if (!stop_.load()) {
        qWarning() << "[FusionTrackGrpc] stream ended"
                   << static_cast<int>(st.error_code())
                   << QString::fromStdString(st.error_message());
    }
}
