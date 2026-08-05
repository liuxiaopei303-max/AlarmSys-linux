#include "grpc_alarm/AlarmGrpcSnapshotMapping.h"

#include <QCoreApplication>
#include <QDebug>
#include <cstdlib>

namespace {

int failures = 0;

#define CHECK(name, expr) do { \
    if (!(expr)) { qCritical().noquote() << "FAIL" << name << "line" << __LINE__; ++failures; } \
    else { qInfo().noquote() << "PASS" << name; } \
} while (false)

} // namespace

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    CHECK("海上规则 track_type=0", alarmEnvironmentFromTrackType(0) == AlarmTargetEnvironment::Surface);
    CHECK("AIS规则 track_type=2 仍是海上", alarmEnvironmentFromTrackType(2) == AlarmTargetEnvironment::Surface);
    CHECK("对空规则 track_type=3", alarmEnvironmentFromTrackType(3) == AlarmTargetEnvironment::Air);
    CHECK("事件域数值与既有gRPC协议一致",
          static_cast<int>(AlarmTargetEnvironment::Air) == 3
              && static_cast<int>(AlarmTargetEnvironment::Surface) == 4);
    CHECK("事件真实域优先于错误规则推断",
          resolveAlarmEnvironment(AlarmTargetEnvironment::Air, 0) == AlarmTargetEnvironment::Air);
    CHECK("旧事件无真实域时按规则回退",
          resolveAlarmEnvironment(AlarmTargetEnvironment::Unknown, 3) == AlarmTargetEnvironment::Air);
    CHECK("LOW/MEDIUM/HIGH阶段输出契约",
          alarmSnapshotLevelFromEventStage(1) == AlarmSnapshotLevel::Low
              && alarmSnapshotLevelFromEventStage(2) == AlarmSnapshotLevel::Medium
              && alarmSnapshotLevelFromEventStage(3) == AlarmSnapshotLevel::High);
    CHECK("日常方案旧规则事件保持正式告警HIGH",
          alarmSnapshotLevelFromEventStage(legacyPublishedAlarmStage())
              == AlarmSnapshotLevel::High);

    qInfo() << "AlarmGrpcSnapshotMapping tests completed, failures=" << failures;
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
