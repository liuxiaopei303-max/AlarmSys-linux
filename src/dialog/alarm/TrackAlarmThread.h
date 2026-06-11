#pragma once

#include <QThread>
#include "datastruct/commonStruct.h"
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
    void processAlarms();
    void updataAlarmTrackToDB(QSet<qint64> trackID, AlarmRule info,int type,int radarSourceId = 0);
    bool isTrackInGroupArea(QPointF pt);
    bool isTrackInGroupAreaByGroupId(QPointF pt, int groupId);
    bool isTrackInOtherAlarmArea(QPolygonF polyNow, QPointF pt, QList< AlarmRule> waringList,int index);
    void SaveToDB(AlarmRule info, qint64 targetId, float lat, float lon, float speed, float dir, float dis, int TargetType, int threatScore, int timestampSec, int radarSourceId = 0);
    int convertTargetTypeStringToBitmask(const QString& targetType);
    double calculateThreatLevel(const SPxPacketTrackExtended& track, const DataAccessLayer::DetectionTypeResult& detectionResult, const ThreatAssessmentParams& threatParams, bool hasProtectArea = false, const QPointF& protectCenter = QPointF(), double entryAngle = 0.0);
    double calculateTimeToProtectArea(const SPxPacketTrackExtended& track, const QPointF& protectCenter, double protectRadius);
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
signals:

    void newAlarmDetected(QString);

   // void newAlarmDetected(const AlarmData& data);
private slots:
    void getAlarmArea();

    


}; 