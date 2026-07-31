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

bool isRadarSource(const std::string& dataSourceId)
{
    return dataSourceId == "yuan_yao" || dataSourceId == "jing_zi_tou";
}

void trackDataClassToSpx(const TrackDataClass& st, SPxPacketTrackExtended& track)
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
}

void clearStaleRadarTracks()
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const QDateTime curTime = QDateTime::currentDateTime();
    QWriteLocker wl(&cfg->m_trackDataLock);
    for (auto iter = cfg->m_mapRadarTrack.begin(); iter != cfg->m_mapRadarTrack.end();) {
        const QDateTime time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(iter.value().msgTimeSecs));
        if (time.secsTo(curTime) > 20) {
            iter = cfg->m_mapRadarTrack.erase(iter);
        } else {
            ++iter;
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
    for (int i = 0; i < n; ++i) {
        const std::string ds = (i < ns) ? batch.sources(i).datasourceid() : std::string();
        if (!isRadarSource(ds)) {
            continue;
        }
        const TrackDataClass& st = batch.tracks(i);
        SPxPacketTrackExtended track;
        trackDataClassToSpx(st, track);
        const int trackid = static_cast<int>(st.trackid());
        if (trackid < 0) {
            continue;
        }
        cfg->m_mapRadarTrack.insert(trackid, track);
        ++radarN;
    }
    if (radarN > 0) {
        static qint64 s_lastLogMs = 0;
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - s_lastLogMs >= 5000) {
            qInfo() << "[FusionTrackGrpc] radar ingest" << radarN
                    << "mapSize=" << cfg->m_mapRadarTrack.size();
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
    qInfo() << "[FusionTrackGrpc] connected" << target_;

    TrackDataClassBatch batch;
    qint64 lastClearMs = 0;
    while (!stop_.load() && stream->Read(&batch)) {
        applyBatch(batch);
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - lastClearMs >= 2000) {
            clearStaleRadarTracks();
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
