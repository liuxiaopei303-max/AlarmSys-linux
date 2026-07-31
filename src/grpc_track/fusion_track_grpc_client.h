#pragma once

#include <QString>

#include <atomic>
#include <memory>
#include <thread>

/** FusionTrack gRPC(:60056)：yuan_yao/jing_zi_tou → m_mapRadarTrack */
class FusionTrackGrpcClient
{
public:
    explicit FusionTrackGrpcClient(const QString& target);
    ~FusionTrackGrpcClient();
    void start();
    void stop();

private:
    void runLoop();
    void subscribeOnce();

    QString target_;
    std::atomic<bool> stop_{false};
    std::unique_ptr<std::thread> thread_;
};
