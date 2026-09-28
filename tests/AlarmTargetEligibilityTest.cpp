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

SPxPacketTrackExtended drone(uint32_t sourceId, bool selfReport)
{
    SPxPacketTrackExtended track;
    std::memset(&track, 0, sizeof(track));
    track.norm.min.reserved1 = 3;
    track.fusion.trackID[0] = sourceId;
    if (selfReport)
        AlarmTargetEligibility::markExplicitSelfReport(track);
    return track;
}

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    SPxPacketTrackExtended ordinary = drone(4001, true);
    CHECK("开关关闭保持原行为",
          AlarmTargetEligibility::decide(ordinary, false).eligible);

    for (uint32_t sourceId : {4005U, 4006U, 4007U}) {
        const auto decision = AlarmTargetEligibility::decide(drone(sourceId, true), true);
        CHECK(QStringLiteral("明确自报位无人机 %1 放行").arg(sourceId),
              decision.eligible
                  && decision.matchedSelfReportTrackId == static_cast<int>(sourceId));
    }

    CHECK("4001 自报位无人机排除",
          !AlarmTargetEligibility::decide(drone(4001, true), true).eligible);
    CHECK("4002 自报位无人机排除",
          !AlarmTargetEligibility::decide(drone(4002, true), true).eligible);
    CHECK("来源不明的 4005 排除",
          !AlarmTargetEligibility::decide(drone(4005, false), true).eligible);
    SPxPacketTrackExtended mixedSources = drone(4001, true);
    mixedSources.fusion.trackID[1] = 4005;
    CHECK("非自报位槽位中的 4005 不误放行",
          !AlarmTargetEligibility::decide(mixedSources, true).eligible);

    SPxPacketTrackExtended nonDrone = drone(4005, true);
    nonDrone.norm.min.reserved1 = 1;
    CHECK("非无人机自报位排除",
          !AlarmTargetEligibility::decide(nonDrone, true).eligible);

    SPxPacketTrackExtended virtualTarget = nonDrone;
    AlarmTargetEligibility::markExplicitVirtual(virtualTarget);
    CHECK("明确虚兵不受类别限制",
          AlarmTargetEligibility::decide(virtualTarget, true).eligible);

    qInfo() << "Alarm target eligibility tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
