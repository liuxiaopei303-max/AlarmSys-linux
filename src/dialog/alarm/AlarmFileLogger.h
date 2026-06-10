#pragma once

#include <QString>

class AlarmFileLogger
{
public:
    /** 航迹告警排查日志：logs/alarm_track_yyyyMMdd_HH.log */
    static void logNewAlarmTrack(const QString& content);

    /** AlarmEvent DDS 发布排查：logs/alarm_event_dds_yyyyMMdd_HH.log */
    static void logAlarmEventDds(const QString& content);

    /** gRPC 告警快照推送排查：logs/alarm_grpc_yyyyMMdd_HH.log */
    static void logGrpcAlarmSnapshot(const QString& content);

private:
    static void appendHourlyLog(const QString& filePrefix, const QString& content);
    /** 删除 logs 下修改时间超过 maxAgeMs 的 .log（默认保留 1 小时） */
    static void purgeOldLogsIfNeeded(qint64 maxAgeMs = 3600000);
};
