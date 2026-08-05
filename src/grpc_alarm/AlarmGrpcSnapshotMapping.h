#pragma once

/** AlarmData 内存事件域；数值与现有 gRPC EnvironmentType 的 SURFACE=4/AIR=3 保持一致。 */
enum class AlarmTargetEnvironment {
    Unknown = -1,
    Air = 3,
    Surface = 4,
};

/**
 * AlarmItem.level 的稳定输出契约。数值与现有 protobuf ThreatLevel 一致，
 * 但这里不依赖生成代码，便于独立单元测试。
 */
enum class AlarmSnapshotLevel {
    Low = 0,
    Medium = 1,
    High = 2,
};

inline AlarmSnapshotLevel alarmSnapshotLevelFromEventStage(int eventStage)
{
    if (eventStage >= 3)
        return AlarmSnapshotLevel::High;
    if (eventStage == 2)
        return AlarmSnapshotLevel::Medium;
    return AlarmSnapshotLevel::Low;
}

/** 旧规则路径一旦发布事件，其语义就是正式告警，不再按固定分数区间猜 level。 */
inline int legacyPublishedAlarmStage()
{
    return 3;
}

inline AlarmTargetEnvironment alarmEnvironmentFromTrackType(int trackType)
{
    // 0=融合海上，2=AIS；1/3/4 为现有对空来源。
    switch (trackType) {
    case 1:
    case 3:
    case 4:
        return AlarmTargetEnvironment::Air;
    case 0:
    case 2:
    default:
        return AlarmTargetEnvironment::Surface;
    }
}

inline AlarmTargetEnvironment resolveAlarmEnvironment(
    AlarmTargetEnvironment eventEnvironment, int ruleTrackType)
{
    if (eventEnvironment == AlarmTargetEnvironment::Surface
        || eventEnvironment == AlarmTargetEnvironment::Air) {
        return eventEnvironment;
    }
    return alarmEnvironmentFromTrackType(ruleTrackType);
}
