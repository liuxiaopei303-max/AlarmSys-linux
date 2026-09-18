#pragma once

#include <QThread>
#include "datastruct/commonStruct.h"
#include "dialog/alarm/AreaEscalationEvaluator.h"
#include "dialog/alarm/AreaEscalationProtectionResolver.h"
#include "dialog/alarm/AngleDurationGate.h"
#include"customconfig.h"
class TrackAlarmThread : public QThread
{
    Q_OBJECT
public:
    explicit TrackAlarmThread(int alarmType, QObject* parent = nullptr);
    void stop();
    CustomConfig* gConfig;
protected:
    void run() override;

private:
    struct AreaEscalationBinding {
        AreaEscalationEvaluator::AreaDefinition area;
        AlarmArea runtimeArea;
        AreaEscalationEvaluator::AreaRole role = AreaEscalationEvaluator::AreaRole::Warning;
        AreaEscalationEvaluator::TargetDomain domain = AreaEscalationEvaluator::TargetDomain::Surface;
        QString laneId;
        int trackType = 0;
        AlarmRule rule;
    };

    void processAlarms();
    void updataAlarmTrackToDB(QSet<qint64> trackID, AlarmRule info,int type,int radarSourceId = 0);
    bool isTrackInGroupArea(QPointF pt);
    bool isTrackInGroupAreaByGroupId(QPointF pt, int groupId);
    bool isTrackInOtherAlarmArea(QPolygonF polyNow, QPointF pt, QList< AlarmRule> waringList,int index);
    void SaveToDB(AlarmRule info, qint64 targetId, float lat, float lon, float speed, float dir, float dis, int TargetType, int threatScore, int timestampSec, int radarSourceId = 0, const QString& alarmContent = QString(), int eventStage = 3, int taskStatus = 0, const QString& escalationReason = QString());
    DataAccessLayer::DetectionTypeResult cognitiveEvidenceForUniqueId(qint64 uniqueId);
    int convertTargetTypeStringToBitmask(const QString& targetType);
    double calculateThreatLevel(const SPxPacketTrackExtended& track, const DataAccessLayer::DetectionTypeResult& detectionResult, const ThreatAssessmentParams& threatParams, bool hasProtectArea = false, const QPointF& protectCenter = QPointF(), double entryAngle = 0.0);
    ThreatAssessmentResult calculateThreatAssessment(const SPxPacketTrackExtended& track, const DataAccessLayer::DetectionTypeResult& detectionResult, const ThreatAssessmentParams& threatParams, bool hasProtectArea = false, const QPointF& protectCenter = QPointF(), double entryAngle = 0.0);
    double calculateTimeToProtectArea(const SPxPacketTrackExtended& track, const QPointF& protectCenter, double protectRadius);
    bool configureAreaEscalation(const QList<AlarmRule>& rules);
    void processAreaEscalation();
    AreaEscalationEvaluator::AreaEvidence evaluateAreaEscalationEvidence(
        const AlarmRule& rule,
        const SPxPacketTrackExtended& track,
        const DataAccessLayer::DetectionTypeResult& detection,
        bool previousSpeedPassed,
        bool insideArea,
        qint64 targetId, double trackAgeSeconds);
    void applyAreaEscalationResult(const AreaEscalationEvaluator::Result& result);
    bool findAlarmArea(int groupId, int areaId, AlarmArea* out) const;
    bool findConfiguredNoAlarmArea(
        const QPointF& point, int trackType, QString* areaKey) const;
    bool containsCurrentPoint(const AlarmArea& area, const QPointF& point) const;
    AreaEscalationEvaluator::AreaDefinition toEscalationArea(const AlarmArea& area) const;
    AreaEscalationProtectionResolver::Context resolveProtectionContext(
        const AlarmRule& rule) const;
    int m_alarmType;
    bool m_running;
    float m_uavLat;
    float m_uavLon;
    QList<AlarmArea>m_listGroupArea;
    QList<AlarmData>m_listAlarmData;
    QMap<int, QList<AlarmArea>>m_maparea;
    QMap<int, SPxPacketTrackExtended> m_mapRadarTrack;
    QMap<qint64, SPxPacketTrackExtended> m_mapBirdRadarTrack;
    QMap<int, AISTrack> m_mapAISTrack;
    QMap<qint64, SPxPacketTrackExtended> m_mapFuseTrack;
    QMap<qint64, QList<QPointF>>m_mapFuseTrail;
    QMap<int, QList<QPointF>>m_mapRadarTrail;
    QMap<int, QList<QPointF>>m_mapAISTrail;
    QMap<qint64, QList<QPointF>>m_mapBirdRadarTrail;
    
    // 速度判定状态（键为 target_id 或雷达/AIS 业务 id）
    QMap<qint64, bool> m_mapSpeedLastCheck;
    qint64 m_lastSpeedCheckMapClearMs = 0; // ��¼ÿ��Ŀ���ϴ��ٶ��ж����
    AreaEscalationEvaluator m_areaEscalationEvaluator;
    bool m_areaEscalationActive = false;
    quint64 m_areaEscalationGeneration = 0;
    QList<AreaEscalationBinding> m_areaEscalationBindings;
    QSet<QString> m_areaEscalationClaimedConditionIds;
    QHash<QString, bool> m_areaEscalationPreviousSpeed;
    QHash<QString, qint64> m_demoFirstSeenMs;
    AngleDurationGate m_regularAngleDuration;
    AngleDurationGate m_areaAngleDuration;
    /** 单处理周期认知结果缓存；避免同一目标因多区域/多规则重复点查数据库。 */
    QHash<qint64, DataAccessLayer::DetectionTypeResult> m_cognitiveEvidenceCache;
signals:

    void newAlarmDetected(QString);

   // void newAlarmDetected(const AlarmData& data);
private slots:
    void getAlarmArea();

    


};
