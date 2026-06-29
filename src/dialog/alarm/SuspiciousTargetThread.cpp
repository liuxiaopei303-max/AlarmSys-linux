#include "SuspiciousTargetThread.h"

#include "AlarmFileLogger.h"
#include "common/commonfunc.h"

#include <QDateTime>
#include <QDebug>
#include <QReadLocker>
#include <algorithm>
#include <cmath>

namespace {

double radians(double degree)
{
    return degree * M_PI / 180.0;
}

double rad2deg(double rad)
{
    return rad * 180.0 / M_PI;
}

double calculateBearing(double lat1, double lon1, double lat2, double lon2)
{
    lat1 = radians(lat1);
    lon1 = radians(lon1);
    lat2 = radians(lat2);
    lon2 = radians(lon2);
    const double deltaLon = lon2 - lon1;
    const double y = sin(deltaLon) * cos(lat2);
    const double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
    double bearingDeg = rad2deg(atan2(y, x));
    if (bearingDeg < 0.0) {
        bearingDeg += 360.0;
    }
    return bearingDeg;
}

double courseAngleDiff(double courseDeg, double bearingDeg)
{
    double angleDiff = fabs(courseDeg - bearingDeg);
    if (angleDiff > 180.0) {
        angleDiff = 360.0 - angleDiff;
    }
    return angleDiff;
}

bool parseCoordPair(const QString& csv, QPointF* out)
{
    if (out == nullptr) {
        return false;
    }
    const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (parts.size() < 2) {
        return false;
    }
    bool okX = false;
    bool okY = false;
    const float x = parts.at(0).trimmed().toFloat(&okX);
    const float y = parts.at(1).trimmed().toFloat(&okY);
    if (!okX || !okY) {
        return false;
    }
    *out = QPointF(x, y);
    return true;
}

bool parseAreaRect(const QString& csv, QRectF* out)
{
    if (out == nullptr) {
        return false;
    }
    const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (parts.size() < 4) {
        return false;
    }
    bool ok = false;
    const float x1 = parts.at(0).trimmed().toFloat(&ok);
    if (!ok) {
        return false;
    }
    const float y1 = parts.at(1).trimmed().toFloat(&ok);
    if (!ok) {
        return false;
    }
    const float x2 = parts.at(2).trimmed().toFloat(&ok);
    if (!ok) {
        return false;
    }
    const float y2 = parts.at(3).trimmed().toFloat(&ok);
    if (!ok) {
        return false;
    }
    *out = QRectF(QPointF(x1, y1), QPointF(x2, y2));
    return true;
}

void appendPolygonPoints(const QString& areaPoints, int areaType, AlarmArea& alertArea)
{
    const QStringList parts = areaPoints.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (parts.isEmpty()) {
        return;
    }
    bool okCount = false;
    const int pSize = parts.at(0).trimmed().toInt(&okCount);
    if (!okCount || pSize < 2) {
        return;
    }
    const int required = 1 + (pSize - 1) * 2;
    if (parts.size() < required) {
        return;
    }

    for (int j = 0; j < pSize - 1; ++j) {
        const int idxX = j * 2 + 1;
        const int idxY = j * 2 + 2;
        bool okX = false;
        bool okY = false;
        const float x = parts.at(idxX).trimmed().toFloat(&okX);
        const float y = parts.at(idxY).trimmed().toFloat(&okY);
        if (!okX || !okY) {
            continue;
        }
        const QPointF pt(x, y);
        if (areaType == 4) {
            alertArea.m_road.append(pt);
        } else if (areaType == 3) {
            alertArea.m_alertAreaPolygon.append(pt);
        }
    }
}

/** 对空融合：classified_type==DRONE 时 reserved1=3（见 NewTrackStructConvert） */
constexpr uint8_t kAirDroneReserved1 = 3;

bool isAirDroneTrack(const SPxPacketTrackExtended& track)
{
    return track.norm.min.reserved1 == kAirDroneReserved1;
}

/** 与 TrackAlarmThread 一致：fusion.trackID[0..7] 任一在 birdSkipTrackIds 中则返回该 id */
int findBirdSkipFusionTrackId(const SPxPacketTrackExtended& tr, const QSet<int>& skipIds)
{
    for (int i = 0; i < 8; ++i) {
        const int tid = static_cast<int>(tr.fusion.trackID[i]);
        if (tid > 0 && skipIds.contains(tid))
            return tid;
    }
    return 0;
}

bool isBirdSelfReportSensorFiltered(const SPxPacketTrackExtended& track, const AlarmLogicConfig& al)
{
    if (al.birdFilterMode != 0)
        return false;

    QString res;
    const unsigned int sensorType = track.fusion.sensors;
    for (int si = 0; si < 3; ++si) {
        if (sensorType >> si & 0X01) {
            switch (si) {
            case 0:  res += QStringLiteral("14S雷达 "); break;
            case 1:  res += QStringLiteral("四创雷达 "); break;
            case 2:  res += QStringLiteral("自报位 "); break;
            default: break;
            }
        }
    }

    const QStringList subs = al.birdSelfReportSubstrings.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (const QString& sub : subs) {
        const QString t = sub.trimmed();
        if (!t.isEmpty() && res.contains(t))
            return true;
    }
    return false;
}

/** 鸟情自报位过滤命中时不标可疑（与告警 bird_skip 逻辑对齐） */
bool shouldSkipBirdAirTrackForSuspicious(const SPxPacketTrackExtended& track, const AlarmLogicConfig& al)
{
    if (findBirdSkipFusionTrackId(track, al.birdSkipTrackIds) > 0)
        return true;
    return isBirdSelfReportSensorFiltered(track, al);
}

} // namespace

SuspiciousTargetThread::SuspiciousTargetThread(QObject* parent)
    : QThread(parent)
{
    gConfig = CustomConfig::getInstance();
}

void SuspiciousTargetThread::stop()
{
    m_running = false;
}

bool SuspiciousTargetThread::applySpeedDoubleCheck(
    qint64 trackId, bool currentOk, QMap<qint64, bool>* lastCheckMap)
{
    if (lastCheckMap == nullptr) {
        return false;
    }
    bool pass = false;
    if (lastCheckMap->contains(trackId)) {
        pass = (*lastCheckMap)[trackId] && currentOk;
    }
    (*lastCheckMap)[trackId] = currentOk;
    return pass;
}

void SuspiciousTargetThread::loadAlarmAreas()
{
    const QList<AreaInfo> areaList = gConfig->m_alarmArea;
    m_maparea.clear();
    for (int i = 0; i < areaList.size(); ++i) {
        const AreaInfo& areaInfo = areaList.at(i);
        AlarmArea alertArea;
        alertArea.areaName = areaInfo.areaName;
        alertArea.m_alertAreaType = areaInfo.areaType;
        alertArea.groupID = areaInfo.groupID;
        alertArea.areaID = areaInfo.areaID;

        QPointF startPt;
        QPointF endPt;
        const bool hasStart = parseCoordPair(areaInfo.stratPoint, &startPt);
        const bool hasEnd = parseCoordPair(areaInfo.endPoint, &endPt);
        if (alertArea.m_alertAreaType == 2 && (!hasStart || !hasEnd)) {
            continue;
        }
        if (hasStart) {
            alertArea.m_startP = startPt;
        }
        if (hasEnd) {
            alertArea.m_endP = endPt;
        }

        QRectF rect;
        const bool hasRect = parseAreaRect(areaInfo.areaRect, &rect);
        if (hasRect) {
            alertArea.m_alertAreaRect = rect.normalized();
        }

        appendPolygonPoints(areaInfo.areaPoints, alertArea.m_alertAreaType, alertArea);
        if (alertArea.m_alertAreaType == 1 && hasRect) {
            alertArea.m_alertAreaPolygon = QPolygonF(alertArea.m_alertAreaRect);
        } else if (alertArea.m_alertAreaType == 3 && alertArea.m_alertAreaPolygon.isEmpty()) {
            continue;
        }

        m_maparea[areaInfo.groupID].append(alertArea);
    }
}

void SuspiciousTargetThread::collectProtectCenters(QList<QPointF>* outCenters) const
{
    if (outCenters == nullptr) {
        return;
    }
    outCenters->clear();

    QSet<QString> seen;
    for (auto it = gConfig->m_mapSchemeProtectAreas.constBegin();
         it != gConfig->m_mapSchemeProtectAreas.constEnd();
         ++it) {
        const QPair<int, int> protectInfo = it.value();
        const int protectGroupId = protectInfo.first;
        const int protectAreaId = protectInfo.second;
        if (protectGroupId <= 0 || protectAreaId < 0) {
            continue;
        }

        const QString key = QStringLiteral("%1_%2").arg(protectGroupId).arg(protectAreaId);
        if (seen.contains(key)) {
            continue;
        }
        seen.insert(key);

        if (!m_maparea.contains(protectGroupId)) {
            continue;
        }
        const QList<AlarmArea>& groupAreas = m_maparea.value(protectGroupId);
        bool found = false;
        for (const AlarmArea& protectArea : groupAreas) {
            if (protectArea.areaID != protectAreaId) {
                continue;
            }
            if (protectArea.m_alertAreaType != 2) {
                break;
            }
            outCenters->append(protectArea.m_startP);
            found = true;
            break;
        }
        if (!found) {
            continue;
        }
    }
}

void SuspiciousTargetThread::evaluateTrack(
    qint64 trackId,
    const SPxPacketTrackExtended& track,
    const QList<QPointF>& protectCenters,
    const SuspiciousTargetConfig& cfg,
    QMap<qint64, bool>* lowCheckMap,
    QMap<qint64, bool>* highCheckMap,
    QSet<QString>* suspiciousIds)
{
    if (trackId <= 0 || suspiciousIds == nullptr || lowCheckMap == nullptr || highCheckMap == nullptr) {
        return;
    }

    const double speedMps = track.norm.min.speedMps;
    const double courseDeg = track.norm.min.courseDegrees;

    const bool speedHighNow = speedMps > cfg.speedHighThresholdMps;
    const bool speedHighConfirmed = applySpeedDoubleCheck(trackId, speedHighNow, highCheckMap);

    bool speedLowAngleConfirmed = false;
    if (!protectCenters.isEmpty()) {
        double minAngleDeg = 360.0;
        for (const QPointF& center : protectCenters) {
            const double bearing = calculateBearing(
                track.latDegs, track.longDegs, center.x(), center.y());
            minAngleDeg = std::min(minAngleDeg, courseAngleDiff(courseDeg, bearing));
        }

        const bool speedLowNow =
            speedMps > cfg.speedLowThresholdMps && minAngleDeg <= cfg.protectAngleMaxDeg;
        speedLowAngleConfirmed = applySpeedDoubleCheck(trackId, speedLowNow, lowCheckMap);
    } else {
        lowCheckMap->remove(trackId);
    }

    if (speedHighConfirmed || speedLowAngleConfirmed) {
        suspiciousIds->insert(QString::number(trackId));
    }
}

void SuspiciousTargetThread::processSuspiciousTargets()
{
    const SuspiciousTargetConfig& cfg = gConfig->m_suspiciousTarget;
    loadAlarmAreas();

    QList<QPointF> protectCenters;
    collectProtectCenters(&protectCenters);

    QMap<qint64, SPxPacketTrackExtended> seaFuseTrackMap;
    QMap<qint64, SPxPacketTrackExtended> airFuseTrackMap;
    {
        QReadLocker trackReadLocker(&gConfig->m_trackDataLock);
        seaFuseTrackMap = gConfig->m_mapFuseTrack;
        airFuseTrackMap = gConfig->m_mapBirdFuseTrack;
    }

    QSet<QString> suspiciousIds;
    for (auto it = seaFuseTrackMap.constBegin(); it != seaFuseTrackMap.constEnd(); ++it) {
        evaluateTrack(
            it.key(),
            it.value(),
            protectCenters,
            cfg,
            &m_mapSpeedLowLastCheckSea,
            &m_mapSpeedHighLastCheckSea,
            &suspiciousIds);
    }

    const AlarmLogicConfig& alBird = gConfig->m_alarmLogic;
    int airDroneCount = 0;
    int airBirdFilteredCount = 0;
    for (auto it = airFuseTrackMap.constBegin(); it != airFuseTrackMap.constEnd(); ++it) {
        if (!isAirDroneTrack(it.value())) {
            continue;
        }
        if (shouldSkipBirdAirTrackForSuspicious(it.value(), alBird)) {
            ++airBirdFilteredCount;
            m_mapSpeedLowLastCheckAir.remove(it.key());
            m_mapSpeedHighLastCheckAir.remove(it.key());
            continue;
        }
        ++airDroneCount;
        evaluateTrack(
            it.key(),
            it.value(),
            protectCenters,
            cfg,
            &m_mapSpeedLowLastCheckAir,
            &m_mapSpeedHighLastCheckAir,
            &suspiciousIds);
    }

    if (!suspiciousIds.isEmpty()) {
        const int sentCount = gConfig->SendSuspiciousTargetMsg(suspiciousIds);
        qDebug() << "SuspiciousTargetThread: candidates=" << suspiciousIds.size()
                 << "ddsSent=" << sentCount
                 << "seaFuse=" << seaFuseTrackMap.size()
                 << "airDrone=" << airDroneCount
                 << "airBirdFiltered=" << airBirdFilteredCount
                 << "topic=" << cfg.ddsTopic;
    }
}

void SuspiciousTargetThread::run()
{
    m_running = true;
    const SuspiciousTargetConfig& cfg = gConfig->m_suspiciousTarget;
    qInfo() << "SuspiciousTargetThread started intervalMs=" << cfg.judgeIntervalMs;

    while (m_running) {
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        const qint64 clearIntervalMs =
            static_cast<qint64>(cfg.speedCheckClearIntervalMin) * 60LL * 1000LL;
        if (m_lastSpeedCheckMapClearMs == 0) {
            m_lastSpeedCheckMapClearMs = nowMs;
        } else if (clearIntervalMs > 0 && nowMs - m_lastSpeedCheckMapClearMs >= clearIntervalMs) {
            m_mapSpeedLowLastCheckSea.clear();
            m_mapSpeedHighLastCheckSea.clear();
            m_mapSpeedLowLastCheckAir.clear();
            m_mapSpeedHighLastCheckAir.clear();
            m_lastSpeedCheckMapClearMs = nowMs;
            qDebug() << "SuspiciousTargetThread: cleared speed double-check maps";
        }

        processSuspiciousTargets();
        msleep(cfg.judgeIntervalMs);
    }
}
