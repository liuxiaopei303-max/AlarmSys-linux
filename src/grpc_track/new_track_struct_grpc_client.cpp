#include "new_track_struct_grpc_client.h"

#include "new_track_struct_grpc_convert.h"
#include "customconfig.h"

#ifdef signals
#pragma push_macro("signals")
#undef signals
#define WATCHSYS_RESTORE_QT_SIGNALS_MACRO
#endif
#include "target_stream.grpc.pb.h"
#include "target_stream.pb.h"
#ifdef WATCHSYS_RESTORE_QT_SIGNALS_MACRO
#pragma pop_macro("signals")
#undef WATCHSYS_RESTORE_QT_SIGNALS_MACRO
#endif

#include <QDateTime>
#include <QDebug>
#include <QPointF>
#include <QWriteLocker>

#include <algorithm>
#include <chrono>
#include <thread>

#include <grpcpp/grpcpp.h>

namespace {

using trackmanager::grpc::new_track_struct::NewTrackStructStreamService;
using trackmanager::grpc::new_track_struct::SubscribeNewTrackStructRequest;
using trackmanager::grpc::new_track_struct::TargetObject;
using trackmanager::grpc::new_track_struct::TargetOutputSet;

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

void clearStaleTrackMaps()
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const QDateTime curTime = QDateTime::currentDateTime();
    QWriteLocker wl(&cfg->m_trackDataLock);

    // 航迹过期时同步删除对应尾迹；再清掉「航迹已消失、尾迹还在」的孤儿 key
    auto pruneMap64 = [&](QMap<qint64, SPxPacketTrackExtended>& trackMap,
                          QMap<qint64, QList<QPointF>>& trailMap, int staleSecs) {
        const QList<qint64> keys = trackMap.keys();
        for (qint64 key : keys) {
            if (!trackMap.contains(key)) {
                continue;
            }
            const SPxPacketTrackExtended track = trackMap.value(key);
            const QDateTime time = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(track.msgTimeSecs));
            if (time.secsTo(curTime) > staleSecs) {
                trackMap.remove(key);
                trailMap.remove(key);
            }
        }
        // 尾迹只能挂在仍存活的航迹上：扫一遍 trail，没有对应 track 就删
        const QList<qint64> trailKeys = trailMap.keys();
        for (qint64 key : trailKeys) {
            if (!trackMap.contains(key)) {
                trailMap.remove(key);
            }
        }
    };

    pruneMap64(cfg->m_mapFuseTrack, cfg->m_mapFuseTrail, 8);
    pruneMap64(cfg->m_mapBirdFuseTrack, cfg->m_mapBirdFuseTrail, 4);
}

TargetFull::TargetObject toDdsStub(const TargetObject& t)
{
    TargetFull::TargetObject stub;
    stub.target_id(t.target_id());
    return stub;
}

void applyTargetOutputSet(NewTrackStructGrpcClient* client, const TargetOutputSet& sample)
{
    CustomConfig* cfg = CustomConfig::getInstance();
    if (!cfg) {
        return;
    }
    const int fuseMode = cfg->m_alarmLogic.mode;

    std::vector<TargetFull::TargetObject> seaStubs;
    std::vector<TargetFull::TargetObject> airStubs;
    seaStubs.reserve(static_cast<size_t>(sample.targets_size()));
    airStubs.reserve(static_cast<size_t>(sample.targets_size()));

    QWriteLocker wl(&cfg->m_trackDataLock);
    int seaN = 0;
    int airN = 0;
    int skippedDead = 0;
    int legacyConfirmed = 0;
    int skippedRoute = 0;
    QString firstLegacyConfirmed;
    QString firstRouteReject;
    for (int i = 0; i < sample.targets_size(); ++i) {
        const TargetObject& t = sample.targets(i);
        qint64 targetId = 0;
        if (!NewTrackStructGrpcConvert::parseTargetId(t, &targetId) || targetId <= 0) {
            continue;
        }
        const NewTrackStructGrpcConvert::TargetLifecycleDecision lifecycle =
            NewTrackStructGrpcConvert::decideTargetLifecycle(t);
        // 终态目标从海空两侧同时删除。终态帧可能来自 UNKNOWN 或上游域变更，
        // 不能依赖本帧 environment 决定只清一张表。
        if (lifecycle.shouldRemove()) {
            cfg->m_mapBirdFuseTrack.remove(targetId);
            cfg->m_mapBirdFuseTrail.remove(targetId);
            cfg->m_mapFuseTrack.remove(targetId);
            cfg->m_mapFuseTrail.remove(targetId);
            ++skippedDead;
            continue;
        }
        if (lifecycle.usedLegacyConfirmedCompatibility()) {
            ++legacyConfirmed;
            if (firstLegacyConfirmed.isEmpty()) {
                firstLegacyConfirmed = QStringLiteral(
                    "target=%1 source=%2 reality=%3 state=%4 reason=%5")
                    .arg(targetId)
                    .arg(lifecycle.sourceId)
                    .arg(static_cast<int>(t.reality_type()))
                    .arg(static_cast<int>(t.state()))
                    .arg(lifecycle.reason);
            }
        }

        const NewTrackStructGrpcConvert::TargetRouteDecision route =
            NewTrackStructGrpcConvert::routeTarget(
                t, cfg->m_struBasicConfig.m_virtualTrackSourceAllowList);
        if (!route.accepted()) {
            // 旧版本会把所有非 AIR（包括 UNKNOWN）写进对海表；部署后立即清掉同 ID
            // 的残留，避免继续产生无法在统一出口回填的幽灵告警。
            cfg->m_mapBirdFuseTrack.remove(targetId);
            cfg->m_mapBirdFuseTrail.remove(targetId);
            cfg->m_mapFuseTrack.remove(targetId);
            cfg->m_mapFuseTrail.remove(targetId);
            ++skippedRoute;
            if (firstRouteReject.isEmpty()) {
                firstRouteReject = QStringLiteral(
                    "target=%1 source=%2 reality=%3 environment=%4 type=%5 reason=%6")
                    .arg(targetId)
                    .arg(route.sourceId)
                    .arg(static_cast<int>(t.reality_type()))
                    .arg(static_cast<int>(t.environment()))
                    .arg(static_cast<int>(t.classified_type()))
                    .arg(route.reason);
            }
            continue;
        }
        const bool isBird = route.domain
            == NewTrackStructGrpcConvert::TargetRouteDomain::Air;

        SPxPacketTrackExtended track;
        if (!NewTrackStructGrpcConvert::targetToSpxExtended(t, track, isBird)) {
            continue;
        }

        if (isBird) {
            if (cfg->m_mapBirdFuseTrack.contains(targetId)) {
                const uint32_t prevUnique = cfg->m_mapBirdFuseTrack.value(targetId).secondary.uniqueID;
                track.secondary.uniqueID =
                    NewTrackStructGrpcConvert::preserveStableUniqueId(prevUnique, track.secondary.uniqueID);
            }
            cfg->m_mapBirdFuseTrack.insert(targetId, track);
            appendTrailPoint(cfg->m_mapBirdFuseTrail[targetId], track, 10);
            airStubs.push_back(toDdsStub(t));
            ++airN;
        } else {
            if (cfg->m_mapFuseTrack.contains(targetId)) {
                const uint32_t prevUnique = cfg->m_mapFuseTrack.value(targetId).secondary.uniqueID;
                track.secondary.uniqueID =
                    NewTrackStructGrpcConvert::preserveStableUniqueId(prevUnique, track.secondary.uniqueID);
            }
            cfg->m_mapFuseTrack.insert(targetId, track);
            const int trailMax = (fuseMode == 1) ? 15 : 10;
            appendTrailPoint(cfg->m_mapFuseTrail[targetId], track, trailMax);
            seaStubs.push_back(toDdsStub(t));
            ++seaN;
        }
    }

    if (client) {
        if (!seaStubs.empty()) {
            TargetFull::TargetOutputSet batch;
            batch.targets(std::move(seaStubs));
            client->storeLatest(false, std::move(batch));
        }
        if (!airStubs.empty()) {
            TargetFull::TargetOutputSet batch;
            batch.targets(std::move(airStubs));
            client->storeLatest(true, std::move(batch));
        }
    }

    static qint64 s_lastLogMs = 0;
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if ((seaN || airN || skippedDead || legacyConfirmed || skippedRoute)
        && nowMs - s_lastLogMs >= 5000) {
        qInfo() << "[NewTrackStructGrpc] ingest sea=" << seaN << "air=" << airN
                << "skippedDead=" << skippedDead
                << "legacyConfirmed=" << legacyConfirmed
                << "skippedRoute=" << skippedRoute
                << "fuseMap=" << cfg->m_mapFuseTrack.size()
                << "birdMap=" << cfg->m_mapBirdFuseTrack.size();
        if (!firstLegacyConfirmed.isEmpty()) {
            qInfo().noquote() << QStringLiteral("[NewTrackStructGrpc] lifecycle compatibility %1")
                .arg(firstLegacyConfirmed);
        }
        if (!firstRouteReject.isEmpty()) {
            qWarning().noquote() << QStringLiteral("[NewTrackStructGrpc] rejected %1")
                .arg(firstRouteReject);
        }
        s_lastLogMs = nowMs;
    }
}

} // namespace

NewTrackStructGrpcClient::NewTrackStructGrpcClient(const QString& target)
    : target_(target.trimmed().isEmpty() ? QStringLiteral("192.168.18.141:60055") : target.trimmed())
{
}

NewTrackStructGrpcClient::~NewTrackStructGrpcClient()
{
    stop();
}

void NewTrackStructGrpcClient::start()
{
    if (thread_ && thread_->joinable()) {
        return;
    }
    stop_.store(false);
    thread_ = std::make_unique<std::thread>(&NewTrackStructGrpcClient::runLoop, this);
    // 独立 prune：即使 Subscribe Read 阻塞/断流，仍按 msgTimeSecs 清过期航迹
    prune_thread_ = std::make_unique<std::thread>([this]() {
        while (!stop_.load()) {
            clearStaleTrackMaps();
            for (int i = 0; i < 20 && !stop_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    });
    qInfo() << "[NewTrackStructGrpc] started →" << target_;
}

void NewTrackStructGrpcClient::stop()
{
    stop_.store(true);
    if (thread_ && thread_->joinable()) {
        thread_->join();
    }
    thread_.reset();
    if (prune_thread_ && prune_thread_->joinable()) {
        prune_thread_->join();
    }
    prune_thread_.reset();
}

void NewTrackStructGrpcClient::storeLatest(bool isBird, TargetFull::TargetOutputSet&& batch)
{
    std::lock_guard<std::mutex> lock(latest_mutex_);
    if (isBird) {
        latest_bird_ = std::move(batch);
        has_bird_ = true;
    } else {
        latest_fuse_ = std::move(batch);
        has_fuse_ = true;
    }
}

bool NewTrackStructGrpcClient::build_merged_latest(TargetFull::TargetOutputSet& out) const
{
    std::lock_guard<std::mutex> lock(latest_mutex_);
    if (!has_fuse_ && !has_bird_) {
        return false;
    }
    out = TargetFull::TargetOutputSet();
    std::vector<TargetFull::TargetObject> merged_targets;
    std::vector<TargetFull::GroupObject> merged_groups;
    if (has_fuse_) {
        const auto& t = latest_fuse_.targets();
        merged_targets.insert(merged_targets.end(), t.begin(), t.end());
        const auto& g = latest_fuse_.groups();
        merged_groups.insert(merged_groups.end(), g.begin(), g.end());
    }
    if (has_bird_) {
        const auto& t = latest_bird_.targets();
        merged_targets.insert(merged_targets.end(), t.begin(), t.end());
        const auto& g = latest_bird_.groups();
        merged_groups.insert(merged_groups.end(), g.begin(), g.end());
    }
    out.targets(std::move(merged_targets));
    out.groups(std::move(merged_groups));
    return true;
}

void NewTrackStructGrpcClient::runLoop()
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
            qWarning() << "[NewTrackStructGrpc] exception:" << e.what();
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

void NewTrackStructGrpcClient::subscribeOnce()
{
    grpc::ChannelArguments args;
    args.SetMaxReceiveMessageSize(64 * 1024 * 1024);
    auto channel = grpc::CreateCustomChannel(
        target_.toStdString(), grpc::InsecureChannelCredentials(), args);
    auto stub = NewTrackStructStreamService::NewStub(channel);
    if (!stub) {
        throw std::runtime_error("NewTrackStructStreamService NewStub failed");
    }

    grpc::ClientContext context;
    SubscribeNewTrackStructRequest req;
    auto stream = stub->Subscribe(&context, req);
    if (!stream) {
        throw std::runtime_error("NewTrackStruct Subscribe returned null");
    }
    qInfo() << "[NewTrackStructGrpc] connected" << target_;

    TargetOutputSet batch;
    qint64 lastClearMs = 0;
    while (!stop_.load() && stream->Read(&batch)) {
        applyTargetOutputSet(this, batch);
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (nowMs - lastClearMs >= 2000) {
            clearStaleTrackMaps();
            lastClearMs = nowMs;
        }
    }
    const grpc::Status st = stream->Finish();
    if (!stop_.load()) {
        qWarning() << "[NewTrackStructGrpc] stream ended"
                   << static_cast<int>(st.error_code())
                   << QString::fromStdString(st.error_message());
    }
}
