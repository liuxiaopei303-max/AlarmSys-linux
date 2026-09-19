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

    /**
     * 知识库命中是独立的对海直告警证据，不依赖方案、区域或威胁规则。
     * 人工结束过滤仍保留，避免操作员结束后立即重建事件。
     */
    bool shouldTriggerGlobalSurfaceAlarm(bool manuallyFiltered) const;
};
