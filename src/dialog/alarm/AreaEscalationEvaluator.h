#pragma once

#include <QHash>
#include <QList>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QSet>
#include <QString>
#include <functional>

/**
 * “威胁 -> 预警 -> 正式告警”深模块。
 *
 * 外部接口只接受一轮完整航迹快照；区域几何、周期顺序隔离、资格、B 入区边沿、
 * 连续停留、速度二次确认锁存、只升不降和失效清理全部隐藏在实现中。
 */
class AreaEscalationEvaluator
{
public:
    enum class Stage { None = 0, Threat = 1, Prewarning = 2, Alarm = 3 };
    enum class Disposition { Unassigned = 0, VerifySuccess = 3 };
    enum class Shape { Invalid = 0, Rectangle = 1, Circle = 2, Polygon = 3 };
    enum class TargetDomain { Surface = 0, Air = 1 };
    enum class AreaRole { Warning = 0, Alarm = 1 };

    struct AreaKey {
        int groupId = -1;
        int areaId = -1;

        bool operator==(const AreaKey& other) const
        {
            return groupId == other.groupId && areaId == other.areaId;
        }
        QString toString() const;
    };

    struct AreaDefinition {
        AreaKey key;
        Shape shape = Shape::Invalid;
        QRectF rectangle;
        QPointF circleCenter;
        double circleRadius = 0.0;
        bool geographic = false; // true 时 circleRadius 为米，point 为 (lat, lon)
        QPolygonF polygon;
        QString name;

        bool isValid() const;
    };

    /** 同一 A/B 业务区域可同时承载海上和空中两条独立规则通道。 */
    struct RulePair {
        QString laneId;
        TargetDomain domain = TargetDomain::Surface;
        int trackType = 0;
        int threatThreshold = 20;
        int prewarningThreshold = 60;
        QString warningRuleId;
        QString alarmRuleId;

        bool isValid(QString* error = nullptr) const;
    };

    struct PairDefinition {
        bool enabled = false;
        bool demoParallelUpgrade = false; // only opted-in demo scheme; all legacy policies unchanged
        AreaDefinition warningArea;
        AreaDefinition alarmArea;
        /** 多区域策略；为空时兼容旧单 A/B 字段。 */
        QList<AreaDefinition> warningAreas;
        QList<AreaDefinition> alarmAreas;
        int threatThreshold = 20;
        int prewarningThreshold = 60;
        qint64 dwellMs = 7000;
        QString warningRuleId;
        QString alarmRuleId;
        QList<RulePair> rulePairs;

        bool isValid(QString* error = nullptr) const;
    };

    /** 新多区域接口名；PairDefinition 保留为源码兼容别名。 */
    using PolicyDefinition = PairDefinition;

    struct TargetKey {
        TargetDomain domain = TargetDomain::Surface;
        qint64 targetId = 0;

        bool operator==(const TargetKey& other) const
        {
            return domain == other.domain && targetId == other.targetId;
        }
    };

    struct HardConditions {
        bool speed = true;
        bool speedDoubleCheck = true;
        bool height = true;
        bool entryAngle = true;
        bool heading = true;
        bool angleDuration = true;
        bool trackAge = true;
        bool opticRequired = true;
        bool recognition = true;
        bool targetType = true;
        bool targetAttributes = true;
        // 是否存在识别结果，仅作为 optic/类型证据记录。无识别结果
        // 不能阻断基础事件或 B 区连续 7 秒合格升级。
        bool detection = true;
        bool protectDistance = true;
        bool entryTime = true;
        QString failure;

        bool allPassed() const;
        bool allExceptSpeedDoubleCheck() const;
        QString summary() const;
    };

    struct AreaEvidence {
        bool available = false;
        int score = 0;
        HardConditions hard;
        QString conditionId;
        bool opticSeen = false;
        /** 对海知识库 archive_status=true；只在 B 区作为独立 HIGH 证据。 */
        bool archiveVisitMatched = false;
        QString archiveTargetLabel;
        int archiveThreatScore = 90;
    };

    /** 一个区域在本处理周期的规则证据；区域角色和阈值均来自启用方案。 */
    struct AreaObservation {
        AreaKey area;
        AreaRole role = AreaRole::Warning;
        QString laneId;
        int trackType = 0;
        int threatThreshold = 20;
        int prewarningThreshold = 60;
        bool entryOnly = false;
        bool ignoreThreatScore = false;
        AreaEvidence evidence;
    };

    struct TargetSnapshot {
        qint64 targetId = 0;
        TargetDomain domain = TargetDomain::Surface;
        QString laneId;
        QPointF position; // x=lat, y=lon，与现有 AlarmSys 区域坐标约定一致
        double courseDeg = 0.0;
        double speedMps = 0.0;
        QList<AreaObservation> observations;
        /** 位于免告警区；只在尚无活动事件时阻止创建。 */
        bool suppressNewEvent = false;
        /** 精确免告警区；连知识库直告也只允许已有活动事件继续。 */
        bool suppressAllNewEvents = false;
        // 以下单 A/B 证据字段保留给旧调用者和既有测试。
        AreaEvidence warning;
        AreaEvidence alarm;
    };

    struct Result {
        qint64 targetId = 0;
        TargetDomain domain = TargetDomain::Surface;
        QString laneId;
        int trackType = 0;
        Stage stage = Stage::None;
        Disposition disposition = Disposition::Unassigned;
        QString reason;
        QString conditionId;
        AreaKey eventArea;
        AreaKey qualificationArea;
        int score = 0;
        int threatThreshold = 20;
        int prewarningThreshold = 60;
        bool insideWarning = false;
        bool insideAlarm = false;
        bool qualified = false;
        qint64 qualificationTimeMs = 0;
        /** 兼容字段名：SURFACE 为 B 区连续合格起始，AIR 为 B 区几何进入时间。 */
        qint64 alarmEntryTimeMs = 0;
        qint64 alarmDwellMs = 0;
        qint64 warningToAlarmMs = -1;
        QPointF previousPosition;
        QPointF currentPosition;
        bool hasPreviousPosition = false;
        double courseDeg = 0.0;
        double speedMps = 0.0;
        QString hardConditions;
        QString entryGeometry;
        QString archiveTargetLabel;
        bool stageChanged = false;
    };

    using Clock = std::function<qint64()>;

    explicit AreaEscalationEvaluator(Clock clock = Clock());

    /** 配置重载 seam：校验 A/B 角色后原子替换，并清空所有旧航迹状态。 */
    bool reset(const PairDefinition& pair, QString* error = nullptr);

    /** 处理一轮完整实时航迹快照；默认以快照 ID 集合作为活动航迹集合。 */
    QList<Result> evaluateCycle(const QList<TargetSnapshot>& snapshots);
    QList<Result> evaluateCycle(const QList<TargetSnapshot>& snapshots, qint64 nowMs);

    /**
     * 区域判定仍只使用 snapshots；liveTargetIds 是所有实时航迹表的 ID 并集，
     * 仅用于生命周期清理，避免融合点暂缺而雷达/AIS 航迹仍在时误删资格。
     */
    QList<Result> evaluateCycle(
        const QList<TargetSnapshot>& snapshots,
        const QSet<qint64>& liveTargetIds,
        qint64 nowMs);
    QList<Result> evaluateCycle(
        const QList<TargetSnapshot>& snapshots,
        const QSet<TargetKey>& liveTargets,
        qint64 nowMs);

    /** 人工结束单个活动事件。 */
    void clearTarget(qint64 targetId);
    void clearTarget(TargetDomain domain, qint64 targetId);
    bool isEnabled() const;
    PairDefinition pairDefinition() const;

    static QString stageName(Stage stage);
    static QString dispositionName(Disposition disposition);

private:
    struct AreaTrackState {
        AreaKey area;
        AreaRole role = AreaRole::Warning;
        bool seen = false;
        bool inside = false;
        /** SURFACE HIGH 连续资格起始（失效即清零）；AIR 保持几何进区时间。 */
        qint64 alarmEntryTimeMs = 0;
        bool speedEntryLatched = false;
        quint64 speedEntryCycle = 0;
        AreaEvidence speedEntryEvidence;
        QPointF speedEntryPrevious;
        QPointF speedEntryCurrent;
        double speedEntryCourseDeg = 0.0;
        QString speedEntryGeometry;
    };

    struct TrackState {
        TargetDomain domain = TargetDomain::Surface;
        qint64 targetId = 0;
        QString laneId;
        Stage stage = Stage::None;
        Disposition disposition = Disposition::Unassigned;
        QString reason;
        QString conditionId;
        AreaKey eventArea;
        int score = 0;
        bool qualified = false;
        qint64 qualificationTimeMs = 0;
        quint64 qualificationCycle = 0;
        AreaKey qualificationArea;
        QString archiveTargetLabel;
        bool seen = false;
        QPointF previousPosition;
        bool hasPreviousPosition = false;
        int trackType = 0;
        int threatThreshold = 20;
        int prewarningThreshold = 60;
        QHash<QString, AreaTrackState> areas;
    };

    static bool contains(const AreaDefinition& area, const QPointF& point);
    static QString describeEntry(
        const AreaDefinition& area, const QPointF& previous, const QPointF& current);
    Result makeResult(
        const TargetSnapshot& snapshot,
        const TrackState& state, bool stageChanged,
        bool insideWarning, bool insideAlarm, qint64 nowMs, const QString& hard,
        const QString& entryGeometry) const;
    void upgrade(
        TrackState* state, Stage stage, Disposition disposition, const QString& reason,
        const AreaEvidence& evidence, const AreaKey& area, bool* changed);
    QList<RulePair> effectiveRulePairs() const;
    QList<AreaDefinition> effectiveWarningAreas() const;
    QList<AreaDefinition> effectiveAlarmAreas() const;
    bool findAreaDefinition(AreaRole role, const AreaKey& key, AreaDefinition* out) const;
    QList<AreaObservation> effectiveObservations(
        const TargetSnapshot& snapshot, const RulePair* legacyRulePair) const;
    bool findRulePair(const TargetSnapshot& snapshot, RulePair* out) const;
    static QString stateKey(TargetDomain domain, const QString& laneId, qint64 targetId);

    Clock m_clock;
    PairDefinition m_pair;
    bool m_enabled = false;
    quint64 m_cycle = 0;
    QHash<QString, TrackState> m_tracks;
};

uint qHash(const AreaEscalationEvaluator::AreaKey& key, uint seed = 0);
uint qHash(const AreaEscalationEvaluator::TargetKey& key, uint seed = 0);
