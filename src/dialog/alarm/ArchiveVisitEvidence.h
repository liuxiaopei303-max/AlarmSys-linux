#pragma once

#include <QString>

/**
 * 知识库来访结果的告警语义。
 *
 * 数据库字段仍由相机系统维护；AlarmSys 只把 archive_status 的字符串协议
 * 收敛成一个确定的布尔证据，避免调用者各自实现宽松且不一致的解析。
 */
struct ArchiveVisitEvidence
{
    bool matched = false;
    QString targetLabel;

    static ArchiveVisitEvidence fromDatabase(
        const QString& archiveStatus,
        const QString& archiveTargetType);
};
