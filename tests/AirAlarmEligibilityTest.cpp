#include "dialog/alarm/AirAlarmEligibility.h"
#include "dialog/alarm/AlarmTargetEligibility.h"

#include <QCoreApplication>
#include <QDebug>

#include <cstdlib>
#include <cstring>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

SPxPacketTrackExtended droneWithSourceId(int sourceId, int slot = 0)
{
    SPxPacketTrackExtended track;
    std::memset(&track, 0, sizeof(track));
    track.norm.min.reserved1 = 3;
    track.fusion.trackID[slot] = static_cast<uint32_t>(sourceId);
    return track;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    AlarmLogicConfig config;
    config.mode = 0;
    config.birdSkipTrackIds = {4001, 4002, 4003, 4004, 4008, 4011, 4021};

    for (int sourceId : {4001, 4002, 4003, 4004, 4011, 4021}) {
        const auto decision = AirAlarmEligibility::decide(
            droneWithSourceId(sourceId), config);
        CHECK(QStringLiteral("自报位无人机 %1 免告警").arg(sourceId),
              decision.skip
                  && decision.reason == QStringLiteral("bird_skip_fusion_id")
                  && decision.matchedFusionTrackId == sourceId);
    }

    const auto legacyEight = AirAlarmEligibility::decide(
        droneWithSourceId(4008), config);
    CHECK("既有 4008 豁免保持不变", legacyEight.skip);

    const auto laterSlot = AirAlarmEligibility::decide(
        droneWithSourceId(4011, 7), config);
    CHECK("白名单检查全部融合源槽位",
          laterSlot.skip && laterSlot.matchedFusionTrackId == 4011);

    const auto ordinaryDrone = AirAlarmEligibility::decide(
        droneWithSourceId(4005), config);
    CHECK("非白名单无人机继续参与告警", !ordinaryDrone.skip);

    SPxPacketTrackExtended nonDrone = droneWithSourceId(4005);
    nonDrone.norm.min.reserved1 = 1;
    const auto nonDroneDecision = AirAlarmEligibility::decide(nonDrone, config);
    CHECK("Mode=0 非无人机不进对空规则",
          nonDroneDecision.skip
              && nonDroneDecision.reason == QStringLiteral("not_drone"));

    config.mode = 1;
    CHECK("Mode=1 保持原有非无人机行为",
          !AirAlarmEligibility::decide(nonDrone, config).skip);
    CHECK("Mode=1 仍应应用无人机白名单",
          AirAlarmEligibility::decide(droneWithSourceId(4001), config).skip);

    SPxPacketTrackExtended allowedSelfReport = droneWithSourceId(4005);
    AlarmTargetEligibility::markExplicitSelfReport(allowedSelfReport);
    CHECK("方案过滤开启时 5 号自报位无人机放行",
          !AirAlarmEligibility::decide(allowedSelfReport, config, true).skip);
    CHECK("方案过滤开启时其他无人机排除",
          AirAlarmEligibility::decide(droneWithSourceId(4008), config, true).skip);

    SPxPacketTrackExtended virtualNonDrone = nonDrone;
    AlarmTargetEligibility::markExplicitVirtual(virtualNonDrone);
    CHECK("方案过滤开启时明确虚兵放行",
          !AirAlarmEligibility::decide(virtualNonDrone, config, true).skip);

    qInfo() << "Air alarm eligibility tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
