#pragma once

#include <QString>

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#pragma push_macro("signals")
#undef signals
#include "NewTrackRealTimeStatus.hpp"
#pragma pop_macro("signals")

/** NewTrackStruct gRPC(:60055)：按 environment 写入 m_mapFuseTrack / m_mapBirdFuseTrack */
class NewTrackStructGrpcClient
{
public:
    explicit NewTrackStructGrpcClient(const QString& target);
    ~NewTrackStructGrpcClient();
    void start();
    void stop();

    /** 供告警/可疑目标 DDS 发布：合并最近海/空 batch（至少含 target_id） */
    bool build_merged_latest(TargetFull::TargetOutputSet& out) const;
    void storeLatest(bool isBird, TargetFull::TargetOutputSet&& batch);

private:
    void runLoop();
    void subscribeOnce();

    QString target_;
    std::atomic<bool> stop_{false};
    std::unique_ptr<std::thread> thread_;
    std::unique_ptr<std::thread> prune_thread_;

    mutable std::mutex latest_mutex_;
    TargetFull::TargetOutputSet latest_fuse_;
    TargetFull::TargetOutputSet latest_bird_;
    bool has_fuse_ = false;
    bool has_bird_ = false;
};
