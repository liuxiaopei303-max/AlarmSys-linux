#pragma once

#include <QMap>
#include <QSet>
#include <QThread>

#include "customconfig.h"
#include "datastruct/commonStruct.h"

class SuspiciousTargetThread : public QThread
{
    Q_OBJECT
public:
    explicit SuspiciousTargetThread(QObject* parent = nullptr);
    void stop();

protected:
    void run() override;

private:
    void processSuspiciousTargets();
    void loadAlarmAreas();
    void collectProtectCenters(QList<QPointF>* outCenters) const;
    void evaluateTrack(
        qint64 trackId,
        const SPxPacketTrackExtended& track,
        const QList<QPointF>& protectCenters,
        const SuspiciousTargetConfig& cfg,
        QMap<qint64, bool>* lowCheckMap,
        QMap<qint64, bool>* highCheckMap,
        QSet<QString>* suspiciousIds);

    /** 速度二次确认，与 TrackAlarmThread::updataAlarmTrackToDB 一致 */
    bool applySpeedDoubleCheck(qint64 trackId, bool currentOk, QMap<qint64, bool>* lastCheckMap);

    CustomConfig* gConfig = nullptr;
    bool m_running = false;
    QMap<int, QList<AlarmArea>> m_maparea;
    /** 对海融合航迹速度确认状态 */
    QMap<qint64, bool> m_mapSpeedLowLastCheckSea;
    QMap<qint64, bool> m_mapSpeedHighLastCheckSea;
    /** 对空融合航迹（drone）速度确认状态，与对海分表避免 target_id 重号 */
    QMap<qint64, bool> m_mapSpeedLowLastCheckAir;
    QMap<qint64, bool> m_mapSpeedHighLastCheckAir;
    qint64 m_lastSpeedCheckMapClearMs = 0;
};
