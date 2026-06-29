#include "TrackAlarmThread.h"
#include "AlarmFileLogger.h"
#include "alarm_geoproj.h"
#include <QDebug>
#include "common/commonfunc.h"
#include <QHash>
#include <QReadLocker>
#include <algorithm>
#include <cmath>
#include <limits>

double rad2deg(double rad)
{
	return rad * 180.0 / M_PI;
}

double radians(double degree)
{
	return degree * M_PI / 180;
}

double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
	// 将经纬度转换为弧度
	lat1 = radians(lat1);
	lon1 = radians(lon1);
	lat2 = radians(lat2);
	lon2 = radians(lon2);
	// 计算经度差
	double deltaLon = lon2 - lon1;
	// 计算方位角的正切值
	double y = sin(deltaLon) * cos(lat2);
	double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(deltaLon);
	double bearingRad = atan2(y, x);
	// 将方位角转换为度并确保其在0到360度之间
	double bearingDeg = rad2deg(bearingRad);
	if (bearingDeg < 0) {
		bearingDeg += 360;
	}
	return bearingDeg;
}






QPolygonF createCirclePolygon(const QPointF& center, double radius, int precision = 100)
{
	QPolygonF circle;
	for (int i = 0; i < precision; ++i)
	{
		double angle = 2 * M_PI * i / precision;
		double x = center.x() + radius * cos(angle);
		double y = center.y() + radius * sin(angle);
		circle << QPointF(x, y);
	}
	return circle;
}

namespace {

void logAlarmTrace(const QString& content)
{
	AlarmFileLogger::logNewAlarmTrack(content);
}

void logAlarmTraceThrottled(const QString& key, const QString& content, qint64 intervalMs = 10000)
{
	static QHash<QString, qint64> lastMs;
	const qint64 now = QDateTime::currentMSecsSinceEpoch();
	const qint64 prev = lastMs.value(key, 0);
	if (prev > 0 && (now - prev) < intervalMs)
		return;
	lastMs[key] = now;
	logAlarmTrace(content);
}

bool trackHasPublishedAlarm(CustomConfig* cfg, qint64 targetId, qint64 windowMs)
{
	if (cfg == nullptr || targetId <= 0 || windowMs <= 0)
		return false;
	QMutexLocker locker(&cfg->m_alarmDataMutex);
	const qint64 timeNow = QDateTime::currentDateTime().toMSecsSinceEpoch();
	for (QMap<QString, AlarmData>::const_iterator it = cfg->m_mapAlarmData.constBegin();
	     it != cfg->m_mapAlarmData.constEnd();
	     ++it) {
		if (static_cast<qint64>(it.value().unique_id) != targetId || it.value().alarm_status == 2)
			continue;
		QDateTime t0 = QDateTime::fromString(it.value().time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
		if (!t0.isValid())
			t0 = QDateTime::fromString(it.value().time, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
		if (!t0.isValid())
			continue;
		if (timeNow - t0.toMSecsSinceEpoch() < windowMs)
			return true;
	}
	return false;
}

QString birdSkipIdsToString(const QSet<int>& ids)
{
	QStringList parts;
	parts.reserve(ids.size());
	for (int id : ids) {
		parts.append(QString::number(id));
	}
	parts.sort();
	return parts.join(QLatin1Char(','));
}

/** 若 fusion.trackID[0..7] 任一在 birdSkipTrackIds 中则返回该 id，否则返回 0 */
int findBirdSkipFusionTrackId(const SPxPacketTrackExtended& tr, const QSet<int>& skipIds)
{
	for (int i = 0; i < 8; ++i) {
		const int tid = static_cast<int>(tr.fusion.trackID[i]);
		if (tid > 0 && skipIds.contains(tid))
			return tid;
	}
	return 0;
}

void logBirdAreaCandidate(qint64 mapKey, const SPxPacketTrackExtended& tr, const QString& conditionId)
{
	logAlarmTraceThrottled(
		QStringLiteral("bird_area_cand_%1").arg(mapKey),
		QStringLiteral("processAlarms bird_area_candidate------mapKey:%1 normId:%2 fusionTid0:%3 reserved1:%4 conditionId:%5")
			.arg(mapKey)
			.arg(tr.norm.min.id)
			.arg(static_cast<int>(tr.fusion.trackID[0]))
			.arg(tr.norm.min.reserved1)
			.arg(conditionId),
		5000);
}

bool parseCoordPair(const QString& csv, QPointF* out)
{
	if (out == nullptr)
		return false;
	const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
	if (parts.size() < 2)
		return false;
	bool okX = false;
	bool okY = false;
	const float x = parts.at(0).trimmed().toFloat(&okX);
	const float y = parts.at(1).trimmed().toFloat(&okY);
	if (!okX || !okY)
		return false;
	*out = QPointF(x, y);
	return true;
}

bool parseAreaRect(const QString& csv, QRectF* out)
{
	if (out == nullptr)
		return false;
	const QStringList parts = csv.split(QLatin1Char(','), QString::SkipEmptyParts);
	if (parts.size() < 4)
		return false;
	bool ok = false;
	const float x1 = parts.at(0).trimmed().toFloat(&ok);
	if (!ok)
		return false;
	const float y1 = parts.at(1).trimmed().toFloat(&ok);
	if (!ok)
		return false;
	const float x2 = parts.at(2).trimmed().toFloat(&ok);
	if (!ok)
		return false;
	const float y2 = parts.at(3).trimmed().toFloat(&ok);
	if (!ok)
		return false;
	*out = QRectF(QPointF(x1, y1), QPointF(x2, y2));
	return true;
}

void appendPolygonPoints(const QString& areaPoints, int areaType, AlarmArea& alertArea)
{
	const QStringList parts = areaPoints.split(QLatin1Char(','), QString::SkipEmptyParts);
	if (parts.isEmpty())
		return;
	bool okCount = false;
	const int pSize = parts.at(0).trimmed().toInt(&okCount);
	if (!okCount || pSize < 2)
		return;
	const int required = 1 + (pSize - 1) * 2;
	if (parts.size() < required)
		return;

	for (int j = 0; j < pSize - 1; ++j) {
		const int idxX = j * 2 + 1;
		const int idxY = j * 2 + 2;
		bool okX = false;
		bool okY = false;
		const float x = parts.at(idxX).trimmed().toFloat(&okX);
		const float y = parts.at(idxY).trimmed().toFloat(&okY);
		if (!okX || !okY)
			continue;
		const QPointF pt(x, y);
		if (areaType == 4)
			alertArea.m_road.append(pt);
		else if (areaType == 3)
			alertArea.m_alertAreaPolygon.append(pt);
	}
}

} // namespace

TrackAlarmThread::TrackAlarmThread(int alarmType, QObject* parent)
	: QThread(parent), m_alarmType(alarmType), m_running(false)
{
	gConfig = CustomConfig::getInstance();
	//getAlarmArea();
}

void TrackAlarmThread::stop()
{
	m_running = false;
}

void TrackAlarmThread::run()
{
	m_running = true;
	//msleep(1000 * 5); // 每3秒检查一次
	while (m_running) {
		const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
		const qint64 speedCheckClearIntervalMs = 30LL * 60 * 1000;
		if (m_lastSpeedCheckMapClearMs == 0) {
			m_lastSpeedCheckMapClearMs = nowMs;
		} else if (nowMs - m_lastSpeedCheckMapClearMs >= speedCheckClearIntervalMs) {
			const int clearedCount = m_mapSpeedLastCheck.size();
			m_mapSpeedLastCheck.clear();
			m_lastSpeedCheckMapClearMs = nowMs;
			qDebug() << "TrackAlarmThread: cleared m_mapSpeedLastCheck, removed" << clearedCount << "entries";
		}

		// 根据告警类型处理不同的业务逻辑
		if (gConfig->m_alarmLogic.refreshTargetInfoFilterEachLoop)
			gConfig->m_mapTargetInfoFilter = gConfig->dbHelper.getTargetInfoFilter();
 		processAlarms();
		gConfig->SendAllAlarmEventMsg();
		gConfig->tickAlarmGrpcSnapshot();
		QMap<QString, AlarmData>::iterator it = gConfig->m_mapAlarmData.end();

		// 先检查是否为空
		if (gConfig->m_mapAlarmData.isEmpty()) {
			// 处理空容器的情况
			msleep(100); // 每3秒检查一次
			continue; // 或其他适当的处理方式
		}

		// 先将迭代器移动到最后一个有效元素
		it--;

		while (it != gConfig->m_mapAlarmData.begin())
		{
			
			qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
			qint64 time_alarm = QDateTime::fromString(it.value().time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
			if (time_now - time_alarm > gConfig->m_alarmLogic.alarmMapPruneNonFirstStaleMs)
			{
					
				it.value().alarm_status = 1;
				QString alarmID = it.value().alarm_id;
				it--;
				//bool success = gConfig->dbHelper.updateAlarmData(it.value().alarm_id, it.value());
				gConfig->m_mapAlarmData.remove(alarmID);
			}
			else
			{
				it--;
			}
			
			
		}


		if (it == gConfig->m_mapAlarmData.begin())
		{
			qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
			qint64 time_alarm = QDateTime::fromString(it.value().time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
			if (time_now - time_alarm > gConfig->m_alarmLogic.alarmMapPruneNonFirstStaleMs)
			{

				it.value().alarm_status = 1;
				QString alarmID = it.value().alarm_id;
				//bool success = gConfig->dbHelper.updateAlarmData(it.value().alarm_id, it.value());
				gConfig->m_mapAlarmData.remove(alarmID);
			}
		}

		msleep(100); // 每3秒检查一次
	}
}

void TrackAlarmThread::SaveToDB(AlarmRule info, qint64 targetId, float lat, float lon, float speed, float dir, float dis, int TargetType, int threatScore, int timestampSec, int radarSourceId)
{
	if (targetId <= 0) {
		logAlarmTrace(QStringLiteral("SaveToDB skip invalid target_id------targetId:%1 conditionId:%2")
			.arg(targetId)
			.arg(info.condition_id));
		return;
	}
	if (gConfig->isUniqueIdAlarmFiltered(targetId)) {
		logAlarmTraceThrottled(
			QStringLiteral("save_filtered_%1").arg(targetId),
			QStringLiteral("SaveToDB skip filtered------targetId:%1 conditionId:%2")
				.arg(targetId)
				.arg(info.condition_id),
			30000);
		return;
	}

	const AlarmLogicConfig& al = gConfig->m_alarmLogic;
	const bool saveDb = al.saveAlarmToDb != 0;
	AlarmData dbWriteAlarm;
	bool dbWriteIsInsert = false;
	bool dbWritePending = false;

	{
	QMutexLocker locker(&gConfig->m_alarmDataMutex);
	QMap<QString, AlarmData>::iterator it = gConfig->m_mapAlarmData.begin();
	bool isNew = true;
	AlarmData sendAlarm;
	const qint64 dedupMs = al.dedupSameAlarmWindowMs;

	auto matchesDedupKey = [&](const AlarmData& ad) {
		if (al.dedupMatchConditionAndTrack)
			return ad.condition_id == info.condition_id
				&& static_cast<qint64>(ad.unique_id) == targetId;
		return static_cast<qint64>(ad.unique_id) == targetId;
	};

	while (it != gConfig->m_mapAlarmData.end())
	{
		if (matchesDedupKey(it.value()))
		{
			qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
			qint64 time_alarm = QDateTime::fromString(it.value().time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
			const qint64 ageMs = time_now - time_alarm;
			if (ageMs < dedupMs)
			{
				isNew = false;
				sendAlarm = it.value();
				logAlarmTraceThrottled(
					QStringLiteral("dedup_hit_%1_%2").arg(targetId).arg(info.condition_id),
					QStringLiteral("SaveToDB dedup hit------targetId:%1 conditionId:%2 alarmId:%3 ageMs:%4 dedupMs:%5 nextCount:%6")
						.arg(targetId)
						.arg(info.condition_id)
						.arg(sendAlarm.alarm_id)
						.arg(ageMs)
						.arg(dedupMs)
						.arg(sendAlarm.alarm_count + 1),
					30000);
				break;
			}
			logAlarmTraceThrottled(
				QStringLiteral("dedup_expired_%1_%2").arg(targetId).arg(info.condition_id),
				QStringLiteral("SaveToDB dedup expired------targetId:%1 conditionId:%2 prevAlarmId:%3 ageMs:%4 dedupMs:%5")
					.arg(targetId)
					.arg(info.condition_id)
					.arg(it.value().alarm_id)
					.arg(ageMs)
					.arg(dedupMs),
				10000);
		}
		it++;
	}

	auto buildOriginTime = [&]() -> QString {
		if (!al.originTimeFromTrackMsg)
			return QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
		if (al.originTrackTimeIsMs)
			return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestampSec)).toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
		return QDateTime::fromSecsSinceEpoch(static_cast<qint64>(timestampSec)).toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
	};

	if (isNew)
	{
		AlarmData newAlarm;
		newAlarm.alarm_id = QUuid::createUuid().toString();
		newAlarm.alarm_id.remove(QChar('{'));
		newAlarm.alarm_id.remove(QChar('}'));
		newAlarm.condition_id = info.condition_id;
		newAlarm.mode_id = 1;
		newAlarm.alarm_count = 1;
		newAlarm.threatScore = threatScore;
		newAlarm.unique_id = targetId;
		newAlarm.track_id = 0;
		if (gConfig->m_struBasicConfig.m_nUseBasePoint == 1)
		{
			newAlarm.targetdist = CommonFunc::GetDistance(lon, lat, gConfig->m_struBasicConfig.m_dBasePointLon, gConfig->m_struBasicConfig.m_dBasePointLat)/1852.0;
			newAlarm.targetdir = CommonFunc::getAngle2(lat, lon, gConfig->m_struBasicConfig.m_dBasePointLat, gConfig->m_struBasicConfig.m_dBasePointLon);
		}
		else
		{
			newAlarm.targetdist = dis / 1852.0;
			newAlarm.targetdir = dir;
		}

		newAlarm.targetlat = lat;
		newAlarm.targetlon = lon;
		newAlarm.targetspeed = speed;
		if(radarSourceId != 0)
			newAlarm.targettype = radarSourceId;
		newAlarm.group_id = info.group_id;
		newAlarm.area_id = info.area_id;
		
		newAlarm.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
		newAlarm.origintime = buildOriginTime();
		newAlarm.alarm_status = 0;
		newAlarm.threat_time_ms = al.originTimeFromTrackMsg
			? (al.originTrackTimeIsMs ? static_cast<qint64>(timestampSec)
						  : static_cast<qint64>(timestampSec) * 1000LL)
			: QDateTime::currentMSecsSinceEpoch();
		gConfig->m_mapAlarmData.insert(newAlarm.alarm_id, newAlarm);

		int timestampNow = QDateTime::currentDateTime().toSecsSinceEpoch();
		qint64 timestrampOrigin = static_cast<qint64>(timestampSec);

		QString log = "new threat------targetId:"+ QString::number(newAlarm.unique_id)+"   timeNow:" + QString::number(timestampNow)+ "   timeNowStr:"+QDateTime::fromSecsSinceEpoch(timestampNow).toString("yyyy-MM-dd hh:mm:ss.zzz")+ "   timeOrigin:"+QString::number(timestrampOrigin)+ "   timeOriginStr:"+buildOriginTime();
		AlarmFileLogger::logNewAlarmTrack(log);

		qDebug() << "new Alarm target_id=" << newAlarm.unique_id << endl;
		logAlarmTrace(QStringLiteral("SaveToDB new------targetId:%1 conditionId:%2 alarmId:%3 threatScore:%4 speed:%5 type:%6")
			.arg(newAlarm.unique_id)
			.arg(info.condition_id)
			.arg(newAlarm.alarm_id)
			.arg(threatScore)
			.arg(speed)
			.arg(TargetType));
		if (saveDb) {
			dbWriteIsInsert = true;
			dbWritePending = true;
			dbWriteAlarm = newAlarm;
		}
	}
	else
	{
			sendAlarm.targetlat = lat;
			sendAlarm.targetlon = lon;
			sendAlarm.targetspeed = speed;
			sendAlarm.alarm_count += 1;
			sendAlarm.threatScore = threatScore;
			sendAlarm.unique_id = targetId;
			sendAlarm.track_id = 0;
			if (radarSourceId != 0)
				sendAlarm.targettype = radarSourceId;
			sendAlarm.group_id = info.group_id;
			sendAlarm.area_id = info.area_id;
			sendAlarm.time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
			if (gConfig->m_struBasicConfig.m_nUseBasePoint == 1)
			{
				sendAlarm.targetdist = CommonFunc::GetDistance( lon, lat,  gConfig->m_struBasicConfig.m_dBasePointLon,gConfig->m_struBasicConfig.m_dBasePointLat)/1852.0;
				sendAlarm.targetdir = CommonFunc::getAngle2(lat, lon, gConfig->m_struBasicConfig.m_dBasePointLat, gConfig->m_struBasicConfig.m_dBasePointLon);
			}
			else
			{
				sendAlarm.targetdist = dis / 1852.0;
				sendAlarm.targetdir = dir;
			}

			 if (sendAlarm.alarm_status ==0)
			{
				m_listAlarmData.append(sendAlarm);
				gConfig->m_mapAlarmData.insert(sendAlarm.alarm_id, sendAlarm);
				if (saveDb) {
					dbWriteIsInsert = false;
					dbWritePending = true;
					dbWriteAlarm = sendAlarm;
				}
			}
			else
			{
				logAlarmTraceThrottled(
					QStringLiteral("save_status_%1_%2").arg(targetId).arg(sendAlarm.alarm_status),
					QStringLiteral("SaveToDB skip db update------targetId:%1 alarmId:%2 alarm_status:%3 count:%4")
						.arg(sendAlarm.unique_id)
						.arg(sendAlarm.alarm_id)
						.arg(sendAlarm.alarm_status)
						.arg(sendAlarm.alarm_count),
					30000);
			}
			
			gConfig->m_mapAlarmData.insert(sendAlarm.alarm_id, sendAlarm);
			qDebug() << "old Alarm target_id=" << sendAlarm.unique_id <<"count="<<sendAlarm.alarm_count << endl;
			logAlarmTraceThrottled(
				QStringLiteral("save_old_%1_%2").arg(targetId).arg(info.condition_id),
				QStringLiteral("SaveToDB old------targetId:%1 conditionId:%2 alarmId:%3 count:%4 threatScore:%5 speed:%6")
					.arg(sendAlarm.unique_id)
					.arg(info.condition_id)
					.arg(sendAlarm.alarm_id)
					.arg(sendAlarm.alarm_count)
					.arg(threatScore)
					.arg(speed),
				30000);
	}

	} // 释放 m_alarmDataMutex，避免 DB 阻塞拖死 gRPC/告警线程

	if (dbWritePending) {
		if (dbWriteIsInsert) {
			gConfig->dbHelper.addAlarmData(dbWriteAlarm);
		} else {
			const bool success = gConfig->dbHelper.updateAlarmData(dbWriteAlarm.alarm_id, dbWriteAlarm);
			if (!success) {
				qDebug() << "updatafailed target_id=" << dbWriteAlarm.unique_id << endl;
				logAlarmTrace(QStringLiteral("SaveToDB db update failed------targetId:%1 alarmId:%2 count:%3")
					.arg(dbWriteAlarm.unique_id)
					.arg(dbWriteAlarm.alarm_id)
					.arg(dbWriteAlarm.alarm_count));
			}
		}
	}
}
bool TrackAlarmThread::isTrackInGroupArea(QPointF pt)
{
	bool isInGroup = false;
	//QPointF pt = QPointF(track.latDegs, track.longDegs);
	const QSet<int>& allowG = gConfig->m_alarmLogic.areaGroupIdAllow;
	for (int k = 0; k < m_listGroupArea.size(); k++)
	{
		int alertType = m_listGroupArea.at(k).m_alertAreaType;
		int groupid = m_listGroupArea.at(k).groupID;
		if (!allowG.isEmpty() && !allowG.contains(groupid))
			continue;

		if (alertType == 1)
		{
			QRectF rect = m_listGroupArea.at(k).m_alertAreaRect;
			if (rect.contains(pt))
			{
				isInGroup = true;
				break;

			}

		}
		else if (alertType == 2)
		{
			QPointF startPt = m_listGroupArea.at(k).m_startP;
			QPointF endPt = m_listGroupArea.at(k).m_endP;
			float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
			float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
			if (dis <= radius)
			{

				isInGroup = true;
				break;

			}

		}
		else if (alertType == 3)
		{
			QPolygonF rect = m_listGroupArea.at(k).m_alertAreaPolygon;
			if (rect.containsPoint(pt, Qt::OddEvenFill))
			{

				isInGroup = true;
				break;


			}
		}
	}
	return isInGroup;
}

bool TrackAlarmThread::isTrackInGroupAreaByGroupId(QPointF pt, int groupId)
{
	if (groupId < 0 || !m_maparea.contains(groupId))
		return false;

	const QList<AlarmArea>& areas = m_maparea[groupId];
	for (int k = 0; k < areas.size(); ++k) {
		const AlarmArea& area = areas.at(k);
		const int alertType = area.m_alertAreaType;
		if (alertType == 1) {
			if (area.m_alertAreaRect.contains(pt))
				return true;
		} else if (alertType == 2) {
			const QPointF startPt = area.m_startP;
			const QPointF endPt = area.m_endP;
			const float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
			const float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
			if (dis <= radius)
				return true;
		} else if (alertType == 3) {
			if (area.m_alertAreaPolygon.containsPoint(pt, Qt::OddEvenFill))
				return true;
		}
	}
	return false;
}

void  TrackAlarmThread::updataAlarmTrackToDB(QSet<qint64> trackID, AlarmRule info, int type, int radarSourceId)
{

	m_listAlarmData.clear(); 

	QList<qint64> listID = trackID.toList();
	logAlarmTraceThrottled(
		QStringLiteral("upd_batch_%1_%2").arg(info.condition_id).arg(type),
		QStringLiteral("updataAlarmTrackToDB enter------rule:%1 conditionId:%2 trackType:%3 radarSourceId:%4 candidateCount:%5")
			.arg(info.name)
			.arg(info.condition_id)
			.arg(type)
			.arg(radarSourceId)
			.arg(listID.size()),
		5000);
	if (type == 3) {
		const AlarmLogicConfig& alEnter = gConfig->m_alarmLogic;
		logAlarmTrace(
			QStringLiteral("updataAlarmTrackToDB bird_batch------conditionId:%1 radarSourceId:%2 cfgBirdRadarSourceId:%3 birdFilterMode:%4 skipIds:[%5] candidateCount:%6")
				.arg(info.condition_id)
				.arg(radarSourceId)
				.arg(alEnter.birdRadarSourceId)
				.arg(alEnter.birdFilterMode)
				.arg(birdSkipIdsToString(alEnter.birdSkipTrackIds))
				.arg(listID.size()));
	}
	for (int i = 0; i < listID.size(); i++)
	{
		const qint64 candidateTrackId = listID.at(i);
		if (type <= 3)
		{
			SPxPacketTrackExtended track;
			const AlarmLogicConfig& al = gConfig->m_alarmLogic;
			if (type == 0)
			{
				if (al.fuseMapRequireContain) {
					if (!m_mapFuseTrack.contains(candidateTrackId)) {
						logAlarmTraceThrottled(
							QStringLiteral("upd_skip_fuse_%1").arg(candidateTrackId),
							QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:not_in_fuse_map conditionId:%2")
								.arg(candidateTrackId)
								.arg(info.condition_id),
							10000);
						continue;
					}
					track = m_mapFuseTrack[candidateTrackId];
				} else {
					if (!m_mapFuseTrack.contains(candidateTrackId)) {
						logAlarmTraceThrottled(
							QStringLiteral("upd_skip_fuse_%1").arg(candidateTrackId),
							QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:fuse_map_missing conditionId:%2")
								.arg(candidateTrackId)
								.arg(info.condition_id),
							10000);
						continue;
					}
					track = m_mapFuseTrack[candidateTrackId];
				}
			}
			else if(type == 1)
			{
				if (!m_mapRadarTrack.contains(candidateTrackId)) {
					logAlarmTraceThrottled(
						QStringLiteral("upd_skip_radar_%1").arg(candidateTrackId),
						QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:not_in_radar_map conditionId:%2")
							.arg(candidateTrackId)
							.arg(info.condition_id),
						10000);
					continue;
				}
				track = m_mapRadarTrack[candidateTrackId];
			}
			else if (type == 3)
			{
				if (!m_mapBirdRadarTrack.contains(candidateTrackId)) {
					logAlarmTraceThrottled(
						QStringLiteral("upd_skip_bird_%1").arg(candidateTrackId),
						QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:not_in_bird_fuse_map conditionId:%2 birdMapSize:%3")
							.arg(candidateTrackId)
							.arg(info.condition_id)
							.arg(m_mapBirdRadarTrack.size()),
						10000);
					continue;
				}
				track = m_mapBirdRadarTrack[candidateTrackId];
				{
					const int fusionTid0 = static_cast<int>(track.fusion.trackID[0]);
					const int fusionTid1 = static_cast<int>(track.fusion.trackID[1]);
					const int skipFusionTid = findBirdSkipFusionTrackId(track, al.birdSkipTrackIds);
					const bool radarMatch = (radarSourceId == al.birdRadarSourceId);
					const bool inSkipList = (skipFusionTid > 0);
					logAlarmTraceThrottled(
						QStringLiteral("bird_filter_chk_%1_%2").arg(info.condition_id).arg(candidateTrackId),
						QStringLiteral("birdFilterCheck------mapKey:%1 normId:%2 fusionTid0:%3 fusionTid1:%4 sensors:%5 reserved1:%6 radarSrc:%7 cfgBirdRadarSrc:%8 radarMatch:%9 inSkipList:%10 conditionId:%11")
							.arg(candidateTrackId)
							.arg(track.norm.min.id)
							.arg(fusionTid0)
							.arg(fusionTid1)
							.arg(track.fusion.sensors)
							.arg(track.norm.min.reserved1)
							.arg(radarSourceId)
							.arg(al.birdRadarSourceId)
							.arg(radarMatch ? 1 : 0)
							.arg(inSkipList ? 1 : 0)
							.arg(info.condition_id),
						3000);
					if (!radarMatch) {
						logAlarmTraceThrottled(
							QStringLiteral("bird_filter_radar_mismatch_%1").arg(info.condition_id),
							QStringLiteral("birdFilterSkipPathOff------radarSourceId:%1 cfgBirdRadarSourceId:%2 mapKey:%3 fusionTid0:%4 conditionId:%5")
								.arg(radarSourceId)
								.arg(al.birdRadarSourceId)
								.arg(candidateTrackId)
								.arg(fusionTid0)
								.arg(info.condition_id),
							10000);
					}
				}
				if (radarSourceId == al.birdRadarSourceId) {
					// 对空融合(type==3, radarSourceId=9)：fusion.trackID[0..7] 任一在黑名单则跳过
					const int skipFusionTid = findBirdSkipFusionTrackId(track, al.birdSkipTrackIds);
					if (skipFusionTid > 0) {
						logAlarmTrace(
							QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 normId:%2 reason:bird_skip_fusion_id fusionTid:%3 conditionId:%4")
								.arg(candidateTrackId)
								.arg(track.norm.min.id)
								.arg(skipFusionTid)
								.arg(info.condition_id));
						continue;
					}
					if (static_cast<int>(track.fusion.trackID[0]) == 0) {
						logAlarmTraceThrottled(
							QStringLiteral("bird_filter_fusion0_%1").arg(candidateTrackId),
							QStringLiteral("birdFilterNoSkip fusionTid0=0------mapKey:%1 normId:%2 sensors:%3 conditionId:%4")
								.arg(candidateTrackId)
								.arg(track.norm.min.id)
								.arg(track.fusion.sensors)
								.arg(info.condition_id),
							5000);
					}
					if (al.birdFilterMode == 0) {
						QString res;
						const unsigned int sensorType = track.fusion.sensors;
						for (int si = 0; si < 3; si++)
						{
							if (sensorType >> si & 0X01)
							{
								switch (si)
								{
								case 0:  res += QStringLiteral("14S雷达 "); break;
								case 1:  res += QStringLiteral("四创雷达 "); break;
								case 2:  res += QStringLiteral("自报位 "); break;
								default: break;
								}
							}
						}
						bool skipBySensor = false;
						const QStringList subs = al.birdSelfReportSubstrings.split(QLatin1Char(','), QString::SkipEmptyParts);
						for (const QString& sub : subs) {
							const QString t = sub.trimmed();
							if (!t.isEmpty() && res.contains(t)) {
								skipBySensor = true;
								break;
							}
						}
						if (skipBySensor) {
							logAlarmTraceThrottled(
								QStringLiteral("upd_skip_sensor_%1").arg(candidateTrackId),
								QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:bird_self_report_sensor sensors:%2 conditionId:%3")
									.arg(candidateTrackId)
									.arg(res.trimmed())
									.arg(info.condition_id),
								10000);
							continue;
						}
					}
				}
			}

			// 告警规则判断逻辑：优先处理黑白名单，然后进行其他条件过滤
			bool passFilter = true;
			bool blacklistAlarm = false; // 黑名单直接告警标志
			bool whitelistSkip = false;  // 白名单直接忽略标志
			QString failReason;
			const qint64 targetId = candidateTrackId;
			float height = track.altitudeMetres;
			const int filterLookupKey =
				(targetId > 0 && targetId <= static_cast<qint64>(std::numeric_limits<int>::max()))
					? static_cast<int>(targetId)
					: 0;

			// 免告警区：位于 NoAlarmGroupIdList 指定 group 内且无已发布告警则跳过；已有告警则持续
			{
				const AlarmLogicConfig& alNoAlarm = gConfig->m_alarmLogic;
				if (!alNoAlarm.noAlarmGroupIds.isEmpty()) {
					const QPointF ptNoAlarm(track.latDegs, track.longDegs);
					int matchedNoAlarmGroupId = -1;
					for (int gid : alNoAlarm.noAlarmGroupIds) {
						if (gid < 0)
							continue;
						if (isTrackInGroupAreaByGroupId(ptNoAlarm, gid)) {
							matchedNoAlarmGroupId = gid;
							break;
						}
					}
					if (matchedNoAlarmGroupId >= 0) {
						const bool hasPublished = trackHasPublishedAlarm(
							gConfig, targetId, alNoAlarm.trackAlreadyHasAlarmWindowMs);
						if (!hasPublished) {
							logAlarmTraceThrottled(
								QStringLiteral("upd_skip_noalarm_%1_%2").arg(targetId).arg(info.condition_id),
								QStringLiteral("updataAlarmTrackToDB skip------targetId:%1 reason:no_alarm_group groupId:%2 conditionId:%3")
									.arg(targetId)
									.arg(matchedNoAlarmGroupId)
									.arg(info.condition_id),
								10000);
							continue;
						}
						logAlarmTraceThrottled(
							QStringLiteral("upd_noalarm_keep_%1").arg(targetId),
							QStringLiteral("updataAlarmTrackToDB no_alarm_zone_keep------targetId:%1 groupId:%2 hasPublishedAlarm:1 conditionId:%3")
								.arg(targetId)
								.arg(matchedNoAlarmGroupId)
								.arg(info.condition_id),
							30000);
					}
				}
			}

			// 1. 最优先：黑名单和白名单判断
			if (filterLookupKey > 0 && gConfig->m_mapTargetInfoFilter.contains(filterLookupKey)) {
				TargetInfoFilter targetInfo = gConfig->m_mapTargetInfoFilter[filterLookupKey];

				// 黑名单判断 - 如果启用黑名单且目标在黑名单中，直接告警，跳过所有其他条件
				if (info.blacklist_judge == 1 && targetInfo.black_white_attr == 2) {
					blacklistAlarm = true;
				}
				// 白名单判断 - 如果启用白名单且目标不在白名单中，直接忽略，跳过所有其他条件
				else if (info.whitelist_judge == 1 && targetInfo.black_white_attr != 1) {
					whitelistSkip = true;
					passFilter = false;
					failReason = QStringLiteral("whitelist_not_in_list");
				}
			}

			// 如果是黑名单目标，直接告警
			if (blacklistAlarm) {
				QPointF pt = QPointF(track.latDegs, track.longDegs);
				Q_UNUSED(pt);
				logAlarmTrace(QStringLiteral("updataAlarmTrackToDB pass------targetId:%1 path:blacklist conditionId:%2")
					.arg(targetId)
					.arg(info.condition_id));
				SaveToDB(info, targetId, track.latDegs, track.longDegs, track.norm.min.speedMps, track.norm.min.courseDegrees, track.norm.min.rangeMetres, type, al.defaultThreatScore, static_cast<int>(track.msgTimeSecs), radarSourceId);
			}
			// 如果是白名单过滤掉的目标，直接跳过
			else if (whitelistSkip) {
				logAlarmTraceThrottled(
					QStringLiteral("upd_whitelist_%1_%2").arg(targetId).arg(info.condition_id),
					QStringLiteral("updataAlarmTrackToDB skip------targetId:%1 reason:%2 conditionId:%3")
						.arg(targetId)
						.arg(failReason)
						.arg(info.condition_id),
					10000);
			}
			// 对于其他目标，进行完整的规则判断
			else if (passFilter) {
				int lastCalculatedThreatScore = al.defaultThreatScore;
				// 若该航迹此前已有告警（同规则、同航迹且在有效期内），则即使当前速度/方位不满足也持续告警
				bool trackAlreadyHasAlarm = false;  
				{
					QMutexLocker locker(&gConfig->m_alarmDataMutex);
					for (QMap<QString, AlarmData>::iterator itA = gConfig->m_mapAlarmData.begin(); itA != gConfig->m_mapAlarmData.end(); ++itA) {
						if (itA.value().condition_id == info.condition_id
							&& static_cast<qint64>(itA.value().unique_id) == targetId) {
							qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
							qint64 time_alarm = QDateTime::fromString(itA.value().time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
							if (time_now - time_alarm < al.trackAlreadyHasAlarmWindowMs) {
								trackAlreadyHasAlarm = true;
								logAlarmTraceThrottled(
									QStringLiteral("upd_has_alarm_%1_%2").arg(targetId).arg(info.condition_id),
									QStringLiteral("updataAlarmTrackToDB trackAlreadyHasAlarm------targetId:%1 conditionId:%2 alarmId:%3 ageMs:%4 windowMs:%5")
										.arg(targetId)
										.arg(info.condition_id)
										.arg(itA.value().alarm_id)
										.arg(time_now - time_alarm)
										.arg(al.trackAlreadyHasAlarmWindowMs),
									30000);
								break;
							}
						}
					}
				}
				// 2. 速度条件判断（融合航迹连续两次判定 / 其它类型单次判定，与 Windows 版一致）
				if (info.speed_condition > 0) {
					bool currentSpeedCheck = true; // 当前速度判定结果

					// 检查当前速度是否满足条件
					if (info.speed_condition == 1 && track.norm.min.speedMps >= info.speed) {
						currentSpeedCheck = false; // 小于条件不满足
					}
					else if (info.speed_condition == 2 && track.norm.min.speedMps <= info.speed) {
						currentSpeedCheck = false; // 大于条件不满足
					}

					const qint64 trackIdSpd = targetId;

					if (type == 0 || !al.speedDoubleCheckFuseTrackOnly) {
						if (m_mapSpeedLastCheck.contains(trackIdSpd)) {
							if (!(m_mapSpeedLastCheck[trackIdSpd] && currentSpeedCheck)) {
								passFilter = false;
								failReason = QStringLiteral("speed_double_check fail speedMps:%1 cond:%2 threshold:%3 last:%4 cur:%5")
									.arg(track.norm.min.speedMps)
									.arg(info.speed_condition)
									.arg(info.speed)
									.arg(m_mapSpeedLastCheck[trackIdSpd] ? 1 : 0)
									.arg(currentSpeedCheck ? 1 : 0);
							}
						}
						else {
							passFilter = false;
							failReason = QStringLiteral("speed_double_check first_sample speedMps:%1 cond:%2 threshold:%3 cur:%4")
								.arg(track.norm.min.speedMps)
								.arg(info.speed_condition)
								.arg(info.speed)
								.arg(currentSpeedCheck ? 1 : 0);
						}
					}
					else {
						if (!currentSpeedCheck) {
							passFilter = false;
							failReason = QStringLiteral("speed_single_check fail speedMps:%1 cond:%2 threshold:%3")
								.arg(track.norm.min.speedMps)
								.arg(info.speed_condition)
								.arg(info.speed);
						}
					}
					m_mapSpeedLastCheck[trackIdSpd] = currentSpeedCheck;
				}

				// 3. 高度范围过滤
				if (passFilter && info.height_min != info.height_max) {
					if (height < info.height_min || height > info.height_max) {
						passFilter = false;
						failReason = QStringLiteral("height_range fail height:%1 min:%2 max:%3")
							.arg(height)
							.arg(info.height_min)
							.arg(info.height_max);
					}
				}

				// 从内存中查找当前区域的详细信息（包括保护区信息）
				bool hasProtectArea = false;
				QPointF protectCenter;
				bool foundAreaInfo = false;
				AreaInfo currentAreaInfo;
				float angleToCheck = 0.0f;
				// 4. 航向角/进入角判断
				if (passFilter && info.course_min != info.course_max) {
					// 获取当前告警区域的保护区信息（从激活方案中读取）
					QString areaKey = QString("%1_%2").arg(info.group_id).arg(info.area_id);
					if (gConfig->m_mapSchemeProtectAreas.contains(areaKey)) {
						QPair<int, int> protectInfo = gConfig->m_mapSchemeProtectAreas[areaKey];
						int protectGroupID = protectInfo.first;
						int protectAreaID = protectInfo.second;

						if (protectGroupID > 0 && protectAreaID >= 0) {
							// 检查保护区是否在内存中已加载
							if (m_maparea.contains(protectGroupID)) {
								for (int b = 0; b < m_maparea[protectGroupID].size(); b++)
								{
									AlarmArea protectArea = m_maparea[protectGroupID][b];
									if (protectArea.areaID == protectAreaID)
									{
										if (protectArea.m_alertAreaType == 2) { // 圆形保护区
											hasProtectArea = true;
											protectCenter = protectArea.m_startP; // 圆心
											foundAreaInfo = true;
										}
									}
								}
							}
						}
					}

					if (hasProtectArea) {
						// 计算目标相对于保护区圆心的方位角
						double deltaLat = track.latDegs - protectCenter.x();
						double deltaLon = track.longDegs - protectCenter.y();
						//double bearingToTarget = atan2(deltaLon, deltaLat) * 180.0 / M_PI;
						//if (bearingToTarget < 0) bearingToTarget += 360.0;
						double bearingToTarget = calculateBearing(track.latDegs, track.longDegs, protectCenter.x(), protectCenter.y());

						// 计算目标航向相对于保护区圆心的绝对值（进入角）
						double trackCourse = track.norm.min.courseDegrees;
						double angleDiff = fabs(trackCourse - bearingToTarget);
						if (angleDiff > 180.0) {
							angleDiff = 360.0 - angleDiff;
						}
						angleToCheck = angleDiff;
					}
					else {
						// 没有保护区，使用目标航向角
						angleToCheck = track.norm.min.courseDegrees;
					}

					// 角度范围判断
					if (info.course_min <= info.course_max) {
						// 正常情况：course_min < course_max
						if (angleToCheck < info.course_min || angleToCheck > info.course_max) {
							passFilter = false;
							failReason = QStringLiteral("course_range fail angle:%1 min:%2 max:%3")
								.arg(angleToCheck)
								.arg(info.course_min)
								.arg(info.course_max);
						}
					}
					else {
						// 跨越0度的情况：course_min > course_max (例如：350-10度)
						if (angleToCheck < info.course_min && angleToCheck > info.course_max) {
							passFilter = false;
							failReason = QStringLiteral("course_wrap fail angle:%1 min:%2 max:%3")
								.arg(angleToCheck)
								.arg(info.course_min)
								.arg(info.course_max);
						}
					}
				}

				// 到保护区距离判断
				if (info.dist_to_protect_area >= 0 && passFilter) {
					// 重新获取保护区信息（如果前面航向角判断中没有找到）
					if (!hasProtectArea) {
						QString areaKey = QString("%1_%2").arg(info.group_id).arg(info.area_id);
						if (gConfig->m_mapSchemeProtectAreas.contains(areaKey)) {
							QPair<int, int> protectInfo = gConfig->m_mapSchemeProtectAreas[areaKey];
							int protectGroupID = protectInfo.first;
							int protectAreaID = protectInfo.second;

							if (protectGroupID > 0 && protectAreaID >= 0) {
								if (m_maparea.contains(protectGroupID)) {
									for (int b = 0; b < m_maparea[protectGroupID].size(); b++) {
										AlarmArea protectArea = m_maparea[protectGroupID][b];
										if (protectArea.areaID == protectAreaID && protectArea.m_alertAreaType == 2) {
											hasProtectArea = true;
											protectCenter = protectArea.m_startP;
											break;
										}
									}
								}
							}
						}
					}

					if (hasProtectArea) {
						// 计算目标到保护区圆心的距离（米）
						double distToCenter = CommonFunc::GetDistance(track.longDegs, track.latDegs, protectCenter.y(), protectCenter.x());

						// 获取保护区半径（从方案保护区信息中查找）
						QString areaKey = QString("%1_%2").arg(info.group_id).arg(info.area_id);
						QPair<int, int> protectInfo = gConfig->m_mapSchemeProtectAreas[areaKey];
						int protectGroupID = protectInfo.first;
						int protectAreaID = protectInfo.second;

						AlarmArea protectArea;
						for (int b = 0; b < m_maparea[protectGroupID].size(); b++) {
							if (m_maparea[protectGroupID][b].areaID == protectAreaID) {
								protectArea = m_maparea[protectGroupID][b];
								break;
							}
						}

						double protectRadius = CommonFunc::GetDistance(protectArea.m_startP.y(), protectArea.m_startP.x(),
							protectArea.m_endP.y(), protectArea.m_endP.x());

						// 计算目标到保护区圆环边缘的最小距离
						double distToProtectEdge = 0.0;
						if (distToCenter > protectRadius) {
							// 目标在保护区外，距离 = 到圆心距离 - 半径
							distToProtectEdge = distToCenter - protectRadius;
						}
						else {
							// 目标在保护区内，距离为0（或者可以设为负值表示在内部）
							distToProtectEdge = 0.0;
						}

						// 判断是否小于设置的告警距离（转换为米）
						if (distToProtectEdge < info.dist_to_protect_area) {
							// 距离小于阈值，符合告警条件，继续其他判断
							// passFilter 保持为 true
						}
						else {
							// 距离大于阈值，不符合告警条件
							passFilter = false;
							failReason = QStringLiteral("protect_dist fail edgeM:%1 threshold:%2")
								.arg(distToProtectEdge)
								.arg(info.dist_to_protect_area);
						}
					}
					else {
						// 没有保护区，该距离条件无法判断，跳过此条件
						// passFilter 保持当前状态
					}
				}

				if (info.entry_time > 0 && passFilter && hasProtectArea) {
					QString areaKey = QString("%1_%2").arg(info.group_id).arg(info.area_id);
					QPair<int, int> protectInfo = gConfig->m_mapSchemeProtectAreas[areaKey];
					int protectGroupID = protectInfo.first;
					int protectAreaID = protectInfo.second;
					AlarmArea protectArea;
					for (int b = 0; b < m_maparea[protectGroupID].size(); b++) {
						if (m_maparea[protectGroupID][b].areaID == protectAreaID) {
							protectArea = m_maparea[protectGroupID][b];
							break;
						}
					}
					double protectRadius = CommonFunc::GetDistance(protectArea.m_startP.y(), protectArea.m_startP.x(),
						protectArea.m_endP.y(), protectArea.m_endP.x());
					double timeToProtectArea = calculateTimeToProtectArea(track, protectCenter, protectRadius);
					if (timeToProtectArea < 0) {
						passFilter = false;
						failReason = QStringLiteral("entry_time invalid timeToProtect:%1").arg(timeToProtectArea);
					}
					else if (timeToProtectArea > info.entry_time * 60) {
						passFilter = false;
						failReason = QStringLiteral("entry_time too_late sec:%1 limitSec:%2")
							.arg(timeToProtectArea)
							.arg(info.entry_time * 60);
					}
				}

				// 4. 其他高级过滤条件
				if (passFilter) {
					// 4.1 从TargetInfoFilter获取基础属性信息（敌我识别、合作目标等）
					if (filterLookupKey > 0 && gConfig->m_mapTargetInfoFilter.contains(filterLookupKey)) {
						TargetInfoFilter targetInfo = gConfig->m_mapTargetInfoFilter[filterLookupKey];

						// 敌我识别判断 - 使用位掩码判断
						if (info.affiliation_judge > 0) {
							// 检查目标敌我属性是否在规则的位掩码中
							int targetAffiliationBit = 1 << targetInfo.affiliation_attr;
							if ((info.affiliation_judge & targetAffiliationBit) == 0) {
								passFilter = false; // 目标敌我属性不在规则范围内
								failReason = QStringLiteral("affiliation fail attr:%1 mask:%2")
									.arg(targetInfo.affiliation_attr)
									.arg(info.affiliation_judge);
							}
						}

						// 合作目标判断
						if (info.corp_judge > 0 && passFilter) {
							if (info.corp_judge == 1) {
								// 只处理合作目标
								if (targetInfo.corp_attr != 1) {
									passFilter = false;
									failReason = QStringLiteral("corp require_partner attr:%1").arg(targetInfo.corp_attr);
								}
							}
							else if (info.corp_judge == 2) {
								// 只处理非合作目标
								if (targetInfo.corp_attr == 1) {
									passFilter = false;
									failReason = QStringLiteral("corp require_non_partner attr:%1").arg(targetInfo.corp_attr);
								}
							}
						}
					}

					// 4.2 从认知结果表获取检测类型信息（根据reid查询）
					if (passFilter && (info.camera_detect_type > 0 || info.llm_detect_type > 0 ||info.uav_detect_type > 0 || info.threat_level1 > 0 || info.threat_level2 > 0)) 
					{

						// 使用track.secondary.uniqueID作为reid来查询认知结果
						qint64 uniqueId = track.secondary.uniqueID;
						DataAccessLayer::DetectionTypeResult detectionResult = gConfig->dbHelper.getDetectionTypesByReId(uniqueId);

						if (!detectionResult.found) {
							// 如果没有找到认知结果数据，则不通过检测相关的过滤条件
							if(type != 3) {
								passFilter = false;
								failReason = QStringLiteral("detection_not_found uniqueId:%1 trackType:%2")
									.arg(uniqueId)
									.arg(type);
							}
						}
						else {
							// 相机检测目标类型判断
							if (info.camera_detect_type > 0 && passFilter) {
								// 将字符串类型转换为位掩码进行匹配
								int camTypeBit = convertTargetTypeStringToBitmask(detectionResult.camTargetType);
								if (camTypeBit == 0 || (info.camera_detect_type & camTypeBit) == 0) {
									passFilter = false;
									failReason = QStringLiteral("camera_detect_type fail cam:%1 mask:%2")
										.arg(detectionResult.camTargetType)
										.arg(info.camera_detect_type);
								}
							}

							// 大模型研判目标类型判断
							if (info.llm_detect_type > 0 && passFilter) {
								// 将字符串类型转换为位掩码进行匹配
								int llmTypeBit = convertTargetTypeStringToBitmask(detectionResult.llmTargetType);
								if (llmTypeBit == 0 || (info.llm_detect_type & llmTypeBit) == 0) {
									passFilter = false;
									failReason = QStringLiteral("llm_detect_type fail llm:%1 mask:%2")
										.arg(detectionResult.llmTargetType)
										.arg(info.llm_detect_type);
								}
							}

							// 无人机检测目标类型判断
							if (info.uav_detect_type > 0 && passFilter) {
								// 将字符串类型转换为位掩码进行匹配
								int uavTypeBit = convertTargetTypeStringToBitmask(detectionResult.uavTargetType);
								if (uavTypeBit == 0 || (info.uav_detect_type & uavTypeBit) == 0) {
									passFilter = false;
									failReason = QStringLiteral("uav_detect_type fail uav:%1 mask:%2")
										.arg(detectionResult.uavTargetType)
										.arg(info.uav_detect_type);
								}
							}

							// 威胁度判断 - 从内存中的参数计算威胁度
							if ((info.threat_level1 > 0 || info.threat_level2 > 0) && passFilter) {
								// 从内存中查找当前区域的威胁度参数
								ThreatAssessmentParams threatParams;
								bool foundParams = false;
								for (const ThreatAssessmentParams& params : gConfig->m_listThreatAssessmentParams) {
									if (params.groupId == info.group_id && params.areaId == info.area_id) {
										threatParams = params;
										foundParams = true;
										break;
									}
								}
								
								// 如果内存中没有找到，使用默认参数
								if (!foundParams) {
									threatParams = ThreatAssessmentParams(info.group_id, info.area_id);
								}
								
								// 计算威胁度（传递保护区信息和进入角）
								double calculatedThreatLevel = calculateThreatLevel(track, detectionResult, threatParams, hasProtectArea, protectCenter, angleToCheck);
								lastCalculatedThreatScore = qBound(0, static_cast<int>(qRound(calculatedThreatLevel)), 100);
								
								if (info.threat_level1 > 0 && calculatedThreatLevel >= info.threat_level1) {
									// 超过直接告警阈值，直接告警
									blacklistAlarm = true;
									logAlarmTrace(QStringLiteral("updataAlarmTrackToDB pass------trackId:%1 path:threat_direct score:%2 threshold:%3 conditionId:%4")
										.arg(track.norm.min.id)
										.arg(calculatedThreatLevel)
										.arg(info.threat_level1)
										.arg(info.condition_id));
								}
								else if (info.threat_level1 > 0 && calculatedThreatLevel < info.threat_level1) {
									// 低于查证阈值，不告警
									passFilter = false;
									failReason = QStringLiteral("threat_score_low score:%1 threshold:%2 uniqueId:%3")
										.arg(calculatedThreatLevel)
										.arg(info.threat_level1)
										.arg(uniqueId);
								}
							}
						}
					}


				}

				// 5. 最终保存告警：规则通过或持续告警（窗口内同规则同航迹已有告警）
				if (passFilter || trackAlreadyHasAlarm) {
					const QString passPath = trackAlreadyHasAlarm
						? (passFilter ? QStringLiteral("passFilter+trackAlreadyHasAlarm") : QStringLiteral("trackAlreadyHasAlarm"))
						: QStringLiteral("passFilter");
					logAlarmTraceThrottled(
						QStringLiteral("upd_pass_%1_%2").arg(track.norm.min.id).arg(info.condition_id),
						QStringLiteral("updataAlarmTrackToDB pass------trackId:%1 path:%2 threatScore:%3 speed:%4 height:%5 reserved1:%6 conditionId:%7")
							.arg(track.norm.min.id)
							.arg(passPath)
							.arg(lastCalculatedThreatScore)
							.arg(track.norm.min.speedMps)
							.arg(height)
							.arg(track.norm.min.reserved1)
							.arg(info.condition_id),
						10000);
					SaveToDB(info, targetId, track.latDegs, track.longDegs, track.norm.min.speedMps, track.norm.min.courseDegrees, track.norm.min.rangeMetres, type, lastCalculatedThreatScore, static_cast<int>(track.msgTimeSecs), radarSourceId);
				}
				else {
					logAlarmTraceThrottled(
						QStringLiteral("upd_reject_%1_%2").arg(track.norm.min.id).arg(info.condition_id),
						QStringLiteral("updataAlarmTrackToDB reject------trackId:%1 reason:%2 speed:%3 height:%4 reserved1:%5 uniqueId:%6 conditionId:%7")
							.arg(track.norm.min.id)
							.arg(failReason.isEmpty() ? QStringLiteral("unknown") : failReason)
							.arg(track.norm.min.speedMps)
							.arg(height)
							.arg(track.norm.min.reserved1)
							.arg(track.secondary.uniqueID)
							.arg(info.condition_id),
						10000);
				}
			}
		}
	}

	gConfig->m_mapAreaAlarmData[info.name] = m_listAlarmData;
}
void TrackAlarmThread::getAlarmArea()
{
	const QList<AreaInfo> m_listGroupInfo = gConfig->m_alarmArea;
	m_maparea.clear();
	for (int i = 0; i < m_listGroupInfo.size(); i++)
	{
		const AreaInfo& m_area1 = m_listGroupInfo.at(i);
		AlarmArea alertArea1;
		alertArea1.areaName = m_area1.areaName;
		alertArea1.m_alertAreaType = m_area1.areaType;
		alertArea1.groupID = m_area1.groupID;
		alertArea1.areaID = m_area1.areaID;

		QPointF startPt;
		QPointF endPt;
		const bool hasStart = parseCoordPair(m_area1.stratPoint, &startPt);
		const bool hasEnd = parseCoordPair(m_area1.endPoint, &endPt);
		if (alertArea1.m_alertAreaType == 2 && (!hasStart || !hasEnd)) {
			qWarning() << "getAlarmArea skip type=2 missing start/end group" << m_area1.groupID
			           << "area" << m_area1.areaID << "name" << m_area1.areaName;
			continue;
		}
		if (hasStart)
			alertArea1.m_startP = startPt;
		if (hasEnd)
			alertArea1.m_endP = endPt;

		QRectF rect;
		const bool hasRect = parseAreaRect(m_area1.areaRect, &rect);
		if (hasRect)
			alertArea1.m_alertAreaRect = rect.normalized();

		appendPolygonPoints(m_area1.areaPoints, alertArea1.m_alertAreaType, alertArea1);
		if (alertArea1.m_alertAreaType == 1) {
			if (!hasRect) {
				qWarning() << "getAlarmArea skip type=1 missing area_rect group" << m_area1.groupID
				           << "area" << m_area1.areaID << "name" << m_area1.areaName;
				continue;
			}
			alertArea1.m_alertAreaPolygon = QPolygonF(alertArea1.m_alertAreaRect);
		} else if (alertArea1.m_alertAreaType == 3 && alertArea1.m_alertAreaPolygon.isEmpty()) {
			qWarning() << "getAlarmArea skip type=3 empty polygon group" << m_area1.groupID
			           << "area" << m_area1.areaID << "name" << m_area1.areaName;
			continue;
		}

		m_maparea[m_area1.groupID].append(alertArea1);
	}
	m_listGroupArea.clear();
	if (m_maparea.contains(1))
		m_listGroupArea = m_maparea.value(1);
	if (m_maparea.contains(2))
		m_listGroupArea.append(m_maparea.value(2));
}
bool TrackAlarmThread::isTrackInOtherAlarmArea(QPolygonF polyNow,QPointF pt, QList< AlarmRule> waringList,int index)
{
	
	for (int j = 0; j < waringList.size(); j++)
	{
		if (j == index)
			continue;
		
		AlarmRule info = waringList.at(j);
		if (info.alarmstate)
		{


			AlarmArea alertArea;
			if (info.group_id >= 0 && info.area_id >= 0)
			{
				//gConfig->dbHelper.getAlarmArea(info.group_id, info.area_id)
				if (m_maparea[info.group_id].size() > (info.area_id) && m_maparea[info.group_id][info.area_id].areaName.isEmpty() == false)
				{
					alertArea = m_maparea[info.group_id][info.area_id];
				}
				else
				{
					continue;
				}
			}
			QPolygonF poly = polyNow.intersected(alertArea.m_alertAreaPolygon);
			if (poly.containsPoint(pt, Qt::OddEvenFill))
			{
				if (waringList.at(index).alarm_level < info.alarm_level)
				{
					return true;
				}
			}
		}
		
	}
	return false;
}
void TrackAlarmThread::processAlarms()
{
	// 根据告警类型处理不同的业务
	//qDebug() << "0==========================" << endl;
	getAlarmArea();
	// 模拟发现新告警
	// TODO: 实际业务中替换为真实的告警检测逻辑
	// 
	//获取航迹重命名区域信息

	//QList<AreaInfo>m_listGroupInfo = gConfig->dbHelper.getGroupArea(1);
	/*m_listGroupArea.clear();
	if (m_listGroupInfo.size() > 0)
	{
		for (int i = 0; i < m_listGroupInfo.size(); i++)
		{
			AreaInfo m_area1;
			AlarmArea alertArea1;
			m_area1 = m_listGroupInfo.at(i);
			alertArea1.areaName = m_area1.areaName;
			alertArea1.m_alertAreaType = m_area1.areaType;
			QPointF pt;
			pt.setX(m_area1.stratPoint.split(',').at(0).toFloat());
			pt.setY(m_area1.stratPoint.split(',').at(1).toFloat());
			alertArea1.m_startP = pt;
			pt.setX(m_area1.endPoint.split(',').at(0).toFloat());
			pt.setY(m_area1.endPoint.split(',').at(1).toFloat());
			alertArea1.m_endP = pt;
			pt.setX(m_area1.areaRect.split(',').at(0).toFloat());
			pt.setY(m_area1.areaRect.split(',').at(1).toFloat());
			alertArea1.m_alertAreaRect.setTopLeft(pt);
			pt.setX(m_area1.areaRect.split(',').at(2).toFloat());
			pt.setY(m_area1.areaRect.split(',').at(3).toFloat());
			alertArea1.m_alertAreaRect.setBottomRight(pt);
			int pSize = m_area1.areaPoints.split(',').at(0).toInt();
			for (int i = 0; i < pSize; i++)
			{
				pt.setX(m_area1.areaPoints.split(',').at(i * 2 + 1).toFloat());
				pt.setY(m_area1.areaPoints.split(',').at(i * 2 + 2).toFloat());
				if (alertArea1.m_alertAreaType == 4)
					alertArea1.m_road.append(pt);
				else if (alertArea1.m_alertAreaType == 3)
					alertArea1.m_alertAreaPolygon.append(pt);
			}
			m_listGroupArea.append(alertArea1);
		}
	}*/

	{
		QReadLocker trackReadLocker(&gConfig->m_trackDataLock);
		m_mapFuseTrack = gConfig->m_mapFuseTrack;
		m_mapRadarTrack = gConfig->m_mapRadarTrack;
		m_mapBirdRadarTrack = gConfig->m_mapBirdFuseTrack;
		//qDebug() << "TrackAlarmThread: processAlarms: m_mapBirdRadarTrack size" << m_mapBirdRadarTrack.size();
		m_mapAISTrack = gConfig->m_mapAISTrack;
		m_mapFuseTrail = gConfig->m_mapFuseTrail;
		m_mapRadarTrail = gConfig->m_mapRadarTrail;
		m_mapBirdRadarTrail = gConfig->m_mapBirdFuseTrail;
		m_mapAISTrail = gConfig->m_mapAISTrail;
	}
	int trailCount = 2;
	QList< AlarmRule> waringList = gConfig->m_mapAlarmRule.values();
	//qDebug() << "TrackAlarmThread: processAlarms: waringList size" << waringList.size();
	for (int i = 0; i < waringList.size(); i++)
	{
		//qDebug() << "TrackAlarmThread: processAlarms: waringList" << i;
		AlarmRule info = waringList.at(i);
		//qDebug() << "TrackAlarmThread: processAlarms: info" << info.track_type;
		if (info.alarmstate)
		{
			AlarmArea alertArea;
			if (info.group_id>=0&&info.area_id>=0)
			{
				//gConfig->dbHelper.getAlarmArea(info.group_id, info.area_id)
				if (m_maparea[info.group_id].size()>(info.area_id)&&m_maparea[info.group_id][info.area_id].areaName.isEmpty()==false)
				{
					alertArea = m_maparea[info.group_id][info.area_id];
				}
				else
				{
					break;
				}
			}
			QSet<qint64> trackIdSet;//报警ID;
			if (info.group_id >= 0 && info.area_id >= 0)
			{
				//qDebug() << "1=====" << endl;
				if (info.area_judge == 1)//区域内逗留
				{
					int trackCount = 10;
					if (info.track_type == 0)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it = m_mapFuseTrack.begin();
						while (it != m_mapFuseTrack.end())
						{
							//qDebug() << "TrackAlarmThread: processAlarms: it" << it.key();
							bool findFlag = false;
							float height = it.value().altitudeMetres;
							QPointF pt = QPointF(it.value().latDegs, it.value().longDegs);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{
									
								
									if (m_mapFuseTrail[it.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										//qDebug() << "TrackAlarmThread: processAlarms: m_mapFuseTrail it key" << it.key();
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											if (rect.contains(m_mapFuseTrail[it.key()].at(i)) == false)
											{
												ret = false;
												break;
											}
										}
										if(ret)
										trackIdSet.insert(it.key());
									}
										

								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{
									//qDebug() << "=================dis<=" << endl;
									
									if (m_mapFuseTrail[it.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), m_mapFuseTrail[it.key()].at(i).x(), m_mapFuseTrail[it.key()].at(i).y());
											if (dis2 > radius)
											{
												ret = false;
												break;
											}
										}
										if (ret)
											trackIdSet.insert(it.key());
									}
								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									if (m_mapFuseTrail[it.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											if (rect.containsPoint(m_mapFuseTrail[it.key()].at(i), Qt::OddEvenFill) == false)
											{
												ret = false;
												break;
											}
										}
										if (ret)
											trackIdSet.insert(it.key());
									}



								}
							}
							/*bool ret = isTrackInOtherAlarmArea(alertArea.m_alertAreaPolygon,pt,waringList,i);
							if (ret)
								trackIdSet.remove(it.key());*/
							

							it++;
						}
						if (trackIdSet.size() > 0)
						{
							
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}


						trackIdSet.clear();
						info.isBirdTrack = 1;
						
					}
					else if (info.track_type == 1)
					{
						QMap<int, SPxPacketTrackExtended>::iterator it_radar = m_mapRadarTrack.begin();
						while (it_radar != m_mapRadarTrack.end())
						{
							QPointF pt = QPointF(it_radar.value().latDegs, it_radar.value().longDegs);
							int alertType = alertArea.m_alertAreaType;
							float height = it_radar.value().altitudeMetres;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{

									if (m_mapRadarTrail[it_radar.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											if (rect.contains(m_mapRadarTrail[it_radar.key()].at(i)) == false)
											{
												ret = false;
												break;
											}
										}
										if (ret)
											trackIdSet.insert(it_radar.key());
									}
									

								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{
									bool ret = true;
									for (int i = 0; i < trackCount; i++)
									{
										float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), m_mapRadarTrail[it_radar.key()].at(i).x(), m_mapRadarTrail[it_radar.key()].at(i).y());
										if (dis2 > radius)
										{
											ret = false;
											break;
										}
									}
									if (ret)
										trackIdSet.insert(it_radar.key());

								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									if (m_mapRadarTrail[it_radar.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											if (rect.containsPoint(m_mapRadarTrail[it_radar.key()].at(i), Qt::OddEvenFill) == false)
											{
												ret = false;
												break;
											}
										}
										if (ret)
											trackIdSet.insert(it_radar.key());
									}

								}
							}

							it_radar++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, AISTrack>::iterator it_AIS = m_mapAISTrack.begin();
						while (it_AIS != m_mapAISTrack.end())
						{
							QPointF pt = QPointF(it_AIS.value().latDeg, it_AIS.value().longDeg);
							int alertType = alertArea.m_alertAreaType;
							float height = 0;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{
									if (m_mapAISTrail[it_AIS.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											if (rect.contains(m_mapAISTrail[it_AIS.key()].at(i)) == false)
											{
												ret = false;
												break;
											}
										}
										if (ret)
											trackIdSet.insert(it_AIS.key());
									}

								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{

									if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
									trackIdSet.insert(it_AIS.value().MMSI);

								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									if (m_mapAISTrail[it_AIS.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										bool ret = true;
										for (int i = 0; i < trackCount; i++)
										{
											if (rect.containsPoint(m_mapAISTrail[it_AIS.key()].at(i),Qt::OddEvenFill) == false)
											{
												ret = false;
												break;
											}
										}
										if (ret)
											trackIdSet.insert(it_AIS.key());
									}

								}
							}

							it_AIS++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}
					else if (info.track_type == 3) 
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it_radar = m_mapBirdRadarTrack.begin();
						//qDebug() << "TrackAlarmThread: processAlarms: it_radar size" << m_mapBirdRadarTrack.size();
						while (it_radar != m_mapBirdRadarTrack.end())
						{
						
							QPointF pt = QPointF(it_radar.value().latDegs, it_radar.value().longDegs);
							float height = it_radar.value().altitudeMetres;
							int alertType = alertArea.m_alertAreaType;
							if (gConfig->m_alarmLogic.mode == 0)
							{
								const int type = it_radar.value().norm.min.reserved1;
								if (type != 3)
								{
									it_radar++;
									continue;
								}
								//qDebug() << "TrackAlarmThread: processAlarms: it_radar key" << it_radar.key() << "height" << height << "alertType" << alertType << "type" << type;
							}
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{
									if (m_mapBirdRadarTrail[it_radar.key()].size() > trailCount&&(height>=info.height_min||height<=info.height_max))
									{
										//qDebug() << "TrackAlarmThread: processAlarms: it_radar key" << it_radar.key();
										const SPxPacketTrackExtended& birdTr = it_radar.value();
										trackIdSet.insert(it_radar.key());
										logBirdAreaCandidate(it_radar.key(), birdTr, info.condition_id);
									}

								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{

									if (m_mapBirdRadarTrail[it_radar.key()].size() > trailCount&&(height>=info.height_min || height<=info.height_max)) {
										const SPxPacketTrackExtended& birdTr = it_radar.value();
										trackIdSet.insert(it_radar.key());
										logBirdAreaCandidate(it_radar.key(), birdTr, info.condition_id);
									}

								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{
									if (m_mapBirdRadarTrail[it_radar.key()].size() > trailCount && (height>info.height_min || height<info.height_max)) {
										const SPxPacketTrackExtended& birdTr = it_radar.value();
										trackIdSet.insert(it_radar.key());
										logBirdAreaCandidate(it_radar.key(), birdTr, info.condition_id);
									}


								}
							}

							it_radar++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 3, 9);
						}

					}
					else if (info.track_type == 4)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it_radar = m_mapBirdRadarTrack.begin();
						while (it_radar != m_mapBirdRadarTrack.end())
						{

							QPointF pt = QPointF(it_radar.value().latDegs, it_radar.value().longDegs);
							float height = it_radar.value().altitudeMetres;
							int type = it_radar.value().norm.min.reserved1;
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{

									if (m_mapBirdRadarTrail[it_radar.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										if (type == 3 /*|| type == 1 || type == 5 || type == 6 */)//无人机
										{
											bool ret = true;

											if (m_mapBirdRadarTrail.contains(it_radar.key()))
											{
												for (int i = 0; i < trackCount; i++)
												{
													if (m_mapBirdRadarTrail[it_radar.key()].size() - 1 >= i)
													{
														if (rect.contains(m_mapBirdRadarTrail[it_radar.key()].at(i)) == false)
														{
															ret = false;
															break;
														}
													}
												}
												if (ret)
													trackIdSet.insert(it_radar.key());
											}
										}
									}
								}
							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{

									if (m_mapBirdRadarTrail[it_radar.key()].size() > trackCount && (height >= info.height_min && height <= info.height_max))
									{
										if (type == 3 /*|| type == 1 || type == 5 || type == 6*/)//无人机
										{
											bool ret = true;
											for (int i = 0; i < trackCount; i++)
											{
												float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), m_mapBirdRadarTrail[it_radar.key()].at(i).x(), m_mapBirdRadarTrail[it_radar.key()].at(i).y());
												if (dis2 > radius)
												{
													ret = false;
													break;
												}
											}
											if (ret)
												trackIdSet.insert(it_radar.key());
										}
									}

								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									if (m_mapBirdRadarTrail[it_radar.key()].size() > trackCount && (height > info.height_min && height < info.height_max))
									{
										if (type == 3 /*|| type == 1 || type == 5 || type == 6*/)//无人机
										{
											bool ret = true;
											for (int i = 0; i < trackCount; i++)
											{
												if (rect.containsPoint(m_mapBirdRadarTrail[it_radar.key()].at(i), Qt::OddEvenFill) == false)
												{
													ret = false;
													break;
												}
											}
											if (ret)
												trackIdSet.insert(it_radar.key());
										}
									}
								}
							}

							it_radar++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1, 7);
						}
					}
				}
				
				else if (info.area_judge == 4)//进入
				{
				
				if (info.track_type == 0)
				{
					QMap<qint64, QList<QPointF>>::iterator it = m_mapFuseTrail.begin();
					while (it != m_mapFuseTrail.end())
					{
						if (it.value().size() == 0)
						{
							continue;
						}
						QPointF pt = it.value().at(0);
						//qDebug() << "itSize=" << it.value().size();
						int alertType = alertArea.m_alertAreaType;
						if (alertType == 1)
						{

							QRectF rect = alertArea.m_alertAreaRect;
							if (rect.contains(pt))
							{
								
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}
										
									 if (rect.contains(it.value().at(i))==false)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}
							}
							

						}
						else if (alertType == 2)
						{
							QPointF startPt = alertArea.m_startP;
							QPointF endPt = alertArea.m_endP;
							float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (dis <= radius)
							{
								
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}
									float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
									 if (dis2>radius)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}
							}
							

						}
						else if (alertType == 3)
						{
							QPolygonF rect = alertArea.m_alertAreaPolygon;
							if (rect.containsPoint(pt, Qt::OddEvenFill))
							{
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}

									if (rect.containsPoint(it.value().at(i), Qt::OddEvenFill) == false)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}

							}
						}

						it++;
					}

					if (trackIdSet.size() > 0)
					{
						updataAlarmTrackToDB(trackIdSet, info, 0);
					}
					trackIdSet.clear();
					info.isBirdTrack = 1;
					QMap<qint64, SPxPacketTrackExtended>::iterator it_radar = m_mapBirdRadarTrack.begin();
					while (it_radar != m_mapBirdRadarTrack.end())
					{

						QPointF pt = QPointF(it_radar.value().latDegs, it_radar.value().longDegs);
						float height = it_radar.value().altitudeMetres;
						int type = it_radar.value().norm.min.reserved1;
						int alertType = alertArea.m_alertAreaType;
						if (alertType == 1)
						{
							QRectF rect = alertArea.m_alertAreaRect;
							if (rect.contains(pt))
							{

								if (type == 3 && (height >= info.height_min && height <= info.height_max))
								{
									for (int i = 1; i < m_mapBirdRadarTrail[it_radar.key()].size(); i++)
									{
										if (i > 9)
										{
											break;
										}

										if (rect.contains(m_mapBirdRadarTrail[it_radar.key()].at(i)) == false)
										{

											trackIdSet.insert(it.key());
											break;
										}
									}
								}
							}
						}
						else if (alertType == 2)
						{
							QPointF startPt = alertArea.m_startP;
							QPointF endPt = alertArea.m_endP;
							float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (dis <= radius)
							{

								if (type == 3 && (height >= info.height_min && height <= info.height_max))
								{
									for (int i = 1; i < m_mapBirdRadarTrail[it_radar.key()].size(); i++)
									{
										if (i > 9)
										{
											break;
										}

										float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), m_mapBirdRadarTrail[it.key()].at(i).x(), m_mapBirdRadarTrail[it.key()].at(i).y());
										if (dis2 > radius)
										{
											trackIdSet.insert(it.key());
											break;
										}
									}
								}

							}

						}
						else if (alertType == 3)
						{
							QPolygonF rect = alertArea.m_alertAreaPolygon;
							if (rect.containsPoint(pt, Qt::OddEvenFill))
							{

								if (type == 3 && (height >= info.height_min && height <= info.height_max))
								{
									for (int i = 1; i < m_mapBirdRadarTrail[it_radar.key()].size(); i++)
									{
										if (i > 9)
										{
											break;
										}

										if (rect.containsPoint(m_mapBirdRadarTrail[it_radar.key()].at(i),Qt::OddEvenFill) == false)
										{

											trackIdSet.insert(it.key());
											break;
										}
									}
								}

							}
						}

						it_radar++;
					}

					if (trackIdSet.size() > 0)
					{
						updataAlarmTrackToDB(trackIdSet, info, 1);
					}

				}
				else if (info.track_type == 1)
				{
					QMap<int, QList<QPointF>>::iterator it = m_mapRadarTrail.begin();
					while (it != m_mapRadarTrail.end())
					{
						QPointF pt = it.value().at(0);
						int alertType = alertArea.m_alertAreaType;
						if (alertType == 1)
						{

							QRectF rect = alertArea.m_alertAreaRect;
							if (rect.contains(pt))
							{
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}

									if (rect.contains(it.value().at(i)) == false)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}
							}
							

						}
						else if (alertType == 2)
						{
							QPointF startPt = alertArea.m_startP;
							QPointF endPt = alertArea.m_endP;
							float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (dis <= radius)
							{
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}
									float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
									if (dis2 > radius)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}

							}
							

						}
						else if (alertType == 3)
						{
							QPolygonF rect = alertArea.m_alertAreaPolygon;
							if (rect.containsPoint(pt, Qt::OddEvenFill))
							{

								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}

									if (rect.containsPoint(it.value().at(i), Qt::OddEvenFill) == false)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}

							}
						}

						it++;
					}

					if (trackIdSet.size() > 0)
					{
						updataAlarmTrackToDB(trackIdSet, info, 1);
					}

				}
				else if (info.track_type == 2)//AIS
				{
					QMap<int, QList<QPointF>>::iterator it = m_mapAISTrail.begin();
					while (it != m_mapAISTrail.end())
					{
						QPointF pt = it.value().at(0);
						int alertType = alertArea.m_alertAreaType;
						if (alertType == 1)
						{

							QRectF rect = alertArea.m_alertAreaRect;
							if (rect.contains(pt))
							{
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}

									if (rect.contains(it.value().at(i)) == false)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}
							}


						}
						else if (alertType == 2)
						{
							QPointF startPt = alertArea.m_startP;
							QPointF endPt = alertArea.m_endP;
							float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (dis <= radius)
							{
								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}
									float dis2 = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
									if (dis2 > radius)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}

							}

						}
						else if (alertType == 3)
						{
							QPolygonF rect = alertArea.m_alertAreaPolygon;
							if (rect.containsPoint(pt, Qt::OddEvenFill))
							{

								for (int i = 1; i < it.value().size(); i++)
								{
									if (i > 9)
									{
										break;
									}

									if (rect.containsPoint(it.value().at(i), Qt::OddEvenFill) == false)
									{

										trackIdSet.insert(it.key());
										break;
									}
								}


							}
							
						}

						it++;
					}

					if (trackIdSet.size() > 0)
					{
						updataAlarmTrackToDB(trackIdSet, info, 2);
					}


				}


				}
				else if (info.area_judge == 2)//离开
				{
					if (info.track_type == 0)
					{
						QMap<qint64, QList<QPointF>>::iterator it = m_mapFuseTrail.begin();
						while (it != m_mapFuseTrail.end())
						{
							QPointF pt = it.value().at(0);
							//qDebug() << "itSize=" << it.value().size();
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{

								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{
									it++;
									continue;
								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										if (rect.contains(it.value().at(i)))
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}


							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis <= radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										if (rect.containsPoint(it.value().at(i), Qt::OddEvenFill))
										{

											trackIdSet.insert(it.key());
											qDebug() << " trackID=" << it.key() << endl;
											break;
										}
									}
								}
							}

							it++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapRadarTrail.begin();
						while (it != m_mapRadarTrail.end())
						{
							QPointF pt = it.value().at(0);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{

								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{
									it++;
									continue;
								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										if (rect.contains(it.value().at(i)))
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}


							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis <= radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										if (rect.containsPoint(it.value().at(i), Qt::OddEvenFill))
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}

							it++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)//AIS
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapAISTrail.begin();
						while (it != m_mapAISTrail.end())
						{
							QPointF pt = it.value().at(0);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{

								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{
									it++;
									continue;
								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										if (rect.contains(it.value().at(i)))
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}


							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis <= radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis <= radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										if (rect.containsPoint(it.value().at(i), Qt::OddEvenFill))
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}

							it++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}


					}


				}
				else if (info.area_judge == 3)//靠近
				{
					if (info.track_type == 0)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it = m_mapFuseTrack.begin();
						while (it != m_mapFuseTrack.end())
						{
							QPointF pt = QPointF(it.value().latDegs, it.value().longDegs);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{

									it++;
									continue;
								}
								else
								{
									QList<QPoint> m_rect;
									double x1, x2;
									SPxLatLong_t ll;
									ll.latDegs = rect.topLeft().x();
									ll.longDegs = rect.topLeft().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.topRight().x();
									ll.longDegs = rect.topRight().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.bottomRight().x();
									ll.longDegs = rect.bottomRight().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.bottomLeft().x();
									ll.longDegs = rect.bottomLeft().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);

									m_rect.append(QPoint(x1, x2));

									ll.latDegs = pt.x();
									ll.longDegs = pt.y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									QPoint ppt = QPoint(x1, x2);
									double mindis = 9999999;
									double ppm;
									AlarmGetViewPixelsPerMetre(&ppm);
									for (int i = 0; i < 4; i++)
									{
										double dis = 0;
										if (i < 3)
											dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(m_rect.at(i)), QVector2D(m_rect.at(i + 1)));
										else
											dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(m_rect.at(i)), QVector2D(m_rect.at(0)));

										float range = dis;
										if (range < 1852)
										{
											if (m_mapFuseTrail[it.key()].size() > trailCount)
											trackIdSet.insert(it.key());

											break;
										}
									}


								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis > radius && dis < radius + 1852)
								{

									if (m_mapFuseTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.key());

								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									it++;
									continue;

								}
								else
								{

									SPxLatLong_t ll;
									double x1, x2;
									ll.latDegs = pt.x();
									ll.longDegs = pt.y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									QPoint ppt = QPoint(x1, x2);
									double mindis = 9999999;
									for (int i = 0; i < rect.size(); i++)
									{

										double dis = 0;
										QPoint pe, ps;
										ll.latDegs = rect.at(i).x();
										ll.longDegs = rect.at(i).y();
										AlarmLatLongToMetres(&ll, &x1, &x2);
										ps = QPoint(x1, x2);
										if (i < rect.size() - 1)
										{
											ll.latDegs = rect.at(i + 1).x();
											ll.longDegs = rect.at(i + 1).y();
											AlarmLatLongToMetres(&ll, &x1, &x2);
											pe = QPoint(x1, x2);

										}
										else
										{
											ll.latDegs = rect.at(0).x();
											ll.longDegs = rect.at(0).y();
											AlarmLatLongToMetres(&ll, &x1, &x2);
											pe = QPoint(x1, x2);

										}
										dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(ps), QVector2D(pe));

										float range = dis;
										if (range < 1852)
										{
											if (m_mapFuseTrail[it.key()].size() > trailCount)
											trackIdSet.insert(it.key());

											break;
										}
									}

								}
							}

							it++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, SPxPacketTrackExtended>::iterator it = m_mapRadarTrack.begin();
						while (it != m_mapRadarTrack.end())
						{
							QPointF pt = QPointF(it.value().latDegs, it.value().longDegs);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{

									it++;
									continue;
								}
								else
								{
									QList<QPoint> m_rect;
									double x1, x2;
									SPxLatLong_t ll;
									ll.latDegs = rect.topLeft().x();
									ll.longDegs = rect.topLeft().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.topRight().x();
									ll.longDegs = rect.topRight().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.bottomRight().x();
									ll.longDegs = rect.bottomRight().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.bottomLeft().x();
									ll.longDegs = rect.bottomLeft().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = pt.x();
									ll.longDegs = pt.y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									QPoint ppt = QPoint(x1, x2);
									double mindis = 9999999;

									for (int i = 0; i < 4; i++)
									{
										double dis = 0;
										if (i < 3)
											dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(m_rect.at(i)), QVector2D(m_rect.at(i + 1)));
										else
											dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(m_rect.at(i)), QVector2D(m_rect.at(0)));

										float range = dis;
										if (range < 1852)
										{
											if (m_mapRadarTrail[it.key()].size() > trailCount)
											trackIdSet.insert(it.key());

											break;
										}
									}


								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis > radius && dis < radius + 1852)
								{

									if (m_mapRadarTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.key());


								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									it++;
									continue;

								}
								else
								{

									SPxLatLong_t ll;
									double x1, x2;
									ll.latDegs = pt.x();
									ll.longDegs = pt.y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									QPoint ppt = QPoint(x1, x2);
									double mindis = 9999999;
									for (int i = 0; i < rect.size(); i++)
									{

										double dis = 0;
										QPoint pe, ps;
										ll.latDegs = rect.at(i).x();
										ll.longDegs = rect.at(i).y();
										AlarmLatLongToMetres(&ll, &x1, &x2);
										ps = QPoint(x1, x2);
										if (i < rect.size() - 1)
										{
											ll.latDegs = rect.at(i + 1).x();
											ll.longDegs = rect.at(i + 1).y();
											AlarmLatLongToMetres(&ll, &x1, &x2);
											pe = QPoint(x1, x2);

										}
										else
										{
											ll.latDegs = rect.at(0).x();
											ll.longDegs = rect.at(0).y();
											AlarmLatLongToMetres(&ll, &x1, &x2);
											pe = QPoint(x1, x2);

										}
										dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(ps), QVector2D(pe));

										float range = dis;
										if (range < 1852)
										{
											//qDebug() << "trackID=" << it.key() << "dis=" << dis << endl;
											if (m_mapRadarTrail[it.key()].size() > trailCount)
											trackIdSet.insert(it.key());

											break;
										}
									}

								}
							}

							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, AISTrack>::iterator it = m_mapAISTrack.begin();
						while (it != m_mapAISTrack.end())
						{
							QPointF pt = QPointF(it.value().latDeg, it.value().longDeg);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 1)
							{
								QRectF rect = alertArea.m_alertAreaRect;
								if (rect.contains(pt))
								{

									it++;
									continue;
								}
								else
								{
									QList<QPoint> m_rect;
									double x1, x2;
									SPxLatLong_t ll;
									ll.latDegs = rect.topLeft().x();
									ll.longDegs = rect.topLeft().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.topRight().x();
									ll.longDegs = rect.topRight().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.bottomRight().x();
									ll.longDegs = rect.bottomRight().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = rect.bottomLeft().x();
									ll.longDegs = rect.bottomLeft().y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									m_rect.append(QPoint(x1, x2));

									ll.latDegs = pt.x();
									ll.longDegs = pt.y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									QPoint ppt = QPoint(x1, x2);
									double mindis = 9999999;

									for (int i = 0; i < 4; i++)
									{
										double dis = 0;
										if (i < 3)
											dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(m_rect.at(i)), QVector2D(m_rect.at(i + 1)));
										else
											dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(m_rect.at(i)), QVector2D(m_rect.at(0)));

										float range = dis;
										if (range < 1852)
										{
											if (m_mapAISTrail[it.key()].size() > trailCount)
											trackIdSet.insert(it.value().MMSI);

											break;
										}
									}


								}

							}
							else if (alertType == 2)
							{
								QPointF startPt = alertArea.m_startP;
								QPointF endPt = alertArea.m_endP;
								float radius = CommonFunc::GetDistance2(startPt.x(), startPt.y(), endPt.x(), endPt.y());
								float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
								if (dis > radius && dis < radius + 1852)
								{

									if (m_mapAISTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.value().MMSI);

								}

							}
							else if (alertType == 3)
							{
								QPolygonF rect = alertArea.m_alertAreaPolygon;
								if (rect.containsPoint(pt, Qt::OddEvenFill))
								{

									it++;
									continue;

								}
								else
								{

									SPxLatLong_t ll;
									double x1, x2;
									ll.latDegs = pt.x();
									ll.longDegs = pt.y();
									AlarmLatLongToMetres(&ll, &x1, &x2);
									QPoint ppt = QPoint(x1, x2);
									double mindis = 9999999;
									for (int i = 0; i < rect.size(); i++)
									{

										double dis = 0;
										QPoint pe, ps;
										ll.latDegs = rect.at(i).x();
										ll.longDegs = rect.at(i).y();
										AlarmLatLongToMetres(&ll, &x1, &x2);
										ps = QPoint(x1, x2);
										if (i < rect.size() - 1)
										{
											ll.latDegs = rect.at(i + 1).x();
											ll.longDegs = rect.at(i + 1).y();
											AlarmLatLongToMetres(&ll, &x1, &x2);
											pe = QPoint(x1, x2);

										}
										else
										{
											ll.latDegs = rect.at(0).x();
											ll.longDegs = rect.at(0).y();
											AlarmLatLongToMetres(&ll, &x1, &x2);
											pe = QPoint(x1, x2);

										}
										dis = CommonFunc::pointToSegmentDistance(QVector2D(ppt), QVector2D(ps), QVector2D(pe));

										float range = dis;
										if (range < 1852)
										{
											if (m_mapAISTrail[it.key()].size() > trailCount)
											trackIdSet.insert(it.value().MMSI);

											break;
										}
									}

								}
							}

							it++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}
				else if (info.speed_condition == 1)
				{
					if (info.track_type == 0)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it = m_mapFuseTrack.begin();
						while (it != m_mapFuseTrack.end())
						{

							if (it.value().norm.min.speedMps <= info.speed)
							{
								//报警
								if (m_mapFuseTrail[it.key()].size() > trailCount)
								trackIdSet.insert(it.key());
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, SPxPacketTrackExtended>::iterator it_radar = m_mapRadarTrack.begin();
						while (it_radar != m_mapRadarTrack.end())
						{

							if (it_radar.value().norm.min.speedMps <= info.speed)
							{
								//报警
								if (m_mapRadarTrail[it_radar.key()].size() > trailCount)
								trackIdSet.insert(it_radar.key());
							}



							it_radar++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, AISTrack>::iterator it_AIS = m_mapAISTrack.begin();
						while (it_AIS != m_mapAISTrack.end())
						{


							if (it_AIS.value().speedMps <= info.speed)
							{
								if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
								trackIdSet.insert(it_AIS.key());
							}


							it_AIS++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}
				}
				else if (info.speed_condition == 2)
				{
					if (info.track_type == 0)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it = m_mapFuseTrack.begin();
						while (it != m_mapFuseTrack.end())
						{

							if (it.value().norm.min.speedMps > info.speed)
							{
								//报警
								if (m_mapFuseTrail[it.key()].size() > trailCount)
								trackIdSet.insert(it.key());
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, SPxPacketTrackExtended>::iterator it_radar = m_mapRadarTrack.begin();
						while (it_radar != m_mapRadarTrack.end())
						{

							if (it_radar.value().norm.min.speedMps > info.speed)
							{
								//报警
								if (m_mapRadarTrail[it_radar.key()].size() > trailCount)
								trackIdSet.insert(it_radar.key());
							}



							it_radar++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, AISTrack>::iterator it_AIS = m_mapAISTrack.begin();
						while (it_AIS != m_mapAISTrack.end())
						{


							if (it_AIS.value().speedMps > info.speed)
							{
								if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
								trackIdSet.insert(it_AIS.key());
							}


							it_AIS++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}
				else if (info.direction == 1)//远离
				{
					if (info.track_type == 0)
					{
						QMap<qint64, QList<QPointF>>::iterator it = m_mapFuseTrail.begin();
						while (it != m_mapFuseTrail.end())
						{
							if (it.value().size() < 2)
							{
								it++;
								continue;
							}
							QPointF pt = it.value().at(0);
							QPointF pt2 = it.value().at(1);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 4)
							{

								QVector<QPointF> m_route = alertArea.m_road;
								SPxLatLong_t latlon;
								latlon.latDegs = pt.x();
								latlon.longDegs = pt.y();
								double x, y;
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt1 = QPoint(x, y);
								latlon.latDegs = pt2.x();
								latlon.longDegs = pt2.y();
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt2 = QPoint(x, y);
								int mindis = 99999;

								for (int k = 0; k < m_route.size() - 1; k++)
								{
									QPointF ps = m_route.at(k);
									SPxLatLong_t latlong;
									latlong.latDegs = ps.x();
									latlong.longDegs = ps.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									ps.setX(x);
									ps.setY(y);

									QPointF pe = m_route.at(k + 1);

									latlong.latDegs = pe.x();
									latlong.longDegs = pe.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									pe.setX(x);
									pe.setY(y);
									double dis1 = CommonFunc::pointToSegmentDistance(QVector2D(ppt1), QVector2D(ps), QVector2D(pe));
									double dis2 = CommonFunc::pointToSegmentDistance(QVector2D(ppt2), QVector2D(ps), QVector2D(pe));
									if (dis1 < 1 * 1852)
									{
										if (dis1 > dis2)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapRadarTrail.begin();
						while (it != m_mapRadarTrail.end())
						{
							QPointF pt = it.value().at(0);
							QPointF pt2 = it.value().at(1);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 4)
							{

								QVector<QPointF> m_route = alertArea.m_road;
								SPxLatLong_t latlon;
								latlon.latDegs = pt.x();
								latlon.longDegs = pt.y();
								double x, y;
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt1 = QPoint(x, y);
								latlon.latDegs = pt2.x();
								latlon.longDegs = pt2.y();
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt2 = QPoint(x, y);
								int mindis = 99999;

								for (int k = 0; k < m_route.size() - 1; k++)
								{
									QPointF ps = m_route.at(k);
									SPxLatLong_t latlong;
									latlong.latDegs = ps.x();
									latlong.longDegs = ps.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									ps.setX(x);
									ps.setY(y);

									QPointF pe = m_route.at(k + 1);

									latlong.latDegs = pe.x();
									latlong.longDegs = pe.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									pe.setX(x);
									pe.setY(y);
									double dis1 = CommonFunc::pointToSegmentDistance(QVector2D(ppt1), QVector2D(ps), QVector2D(pe));
									double dis2 = CommonFunc::pointToSegmentDistance(QVector2D(ppt2), QVector2D(ps), QVector2D(pe));
									if (dis1 < 1 * 1852)
									{
										if (dis1 > dis2)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)//AIS
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapAISTrail.begin();
						while (it != m_mapAISTrail.end())
						{
							QPointF pt = it.value().at(0);
							QPointF pt2 = it.value().at(1);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 4)
							{

								QVector<QPointF> m_route = alertArea.m_road;
								SPxLatLong_t latlon;
								latlon.latDegs = pt.x();
								latlon.longDegs = pt.y();
								double x, y;
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt1 = QPoint(x, y);
								latlon.latDegs = pt2.x();
								latlon.longDegs = pt2.y();
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt2 = QPoint(x, y);
								int mindis = 99999;

								for (int k = 0; k < m_route.size() - 1; k++)
								{
									QPointF ps = m_route.at(k);
									SPxLatLong_t latlong;
									latlong.latDegs = ps.x();
									latlong.longDegs = ps.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									ps.setX(x);
									ps.setY(y);

									QPointF pe = m_route.at(k + 1);

									latlong.latDegs = pe.x();
									latlong.longDegs = pe.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									pe.setX(x);
									pe.setY(y);
									double dis1 = CommonFunc::pointToSegmentDistance(QVector2D(ppt1), QVector2D(ps), QVector2D(pe));
									double dis2 = CommonFunc::pointToSegmentDistance(QVector2D(ppt2), QVector2D(ps), QVector2D(pe));
									if (dis1 < 1 * 1852)
									{
										if (dis1 > dis2)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}
				else if (info.direction == 2)//靠近
				{
					if (info.track_type == 0)
					{
						QMap<qint64, QList<QPointF>>::iterator it = m_mapFuseTrail.begin();
						while (it != m_mapFuseTrail.end())
						{
							QPointF pt = it.value().at(0);
							QPointF pt2 = it.value().at(1);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 4)
							{

								QVector<QPointF> m_route = alertArea.m_road;
								SPxLatLong_t latlon;
								latlon.latDegs = pt.x();
								latlon.longDegs = pt.y();
								double x, y;
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt1 = QPoint(x, y);
								latlon.latDegs = pt2.x();
								latlon.longDegs = pt2.y();
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt2 = QPoint(x, y);
								int mindis = 99999;

								for (int k = 0; k < m_route.size() - 1; k++)
								{
									QPointF ps = m_route.at(k);
									SPxLatLong_t latlong;
									latlong.latDegs = ps.x();
									latlong.longDegs = ps.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									ps.setX(x);
									ps.setY(y);

									QPointF pe = m_route.at(k + 1);

									latlong.latDegs = pe.x();
									latlong.longDegs = pe.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									pe.setX(x);
									pe.setY(y);
									double dis1 = CommonFunc::pointToSegmentDistance(QVector2D(ppt1), QVector2D(ps), QVector2D(pe));
									double dis2 = CommonFunc::pointToSegmentDistance(QVector2D(ppt2), QVector2D(ps), QVector2D(pe));
									if (dis1 < 1 * 1852)
									{
										if (dis1 < dis2)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapRadarTrail.begin();
						while (it != m_mapRadarTrail.end())
						{
							QPointF pt = it.value().at(0);
							QPointF pt2 = it.value().at(1);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 4)
							{

								QVector<QPointF> m_route = alertArea.m_road;
								SPxLatLong_t latlon;
								latlon.latDegs = pt.x();
								latlon.longDegs = pt.y();
								double x, y;
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt1 = QPoint(x, y);
								latlon.latDegs = pt2.x();
								latlon.longDegs = pt2.y();
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt2 = QPoint(x, y);
								int mindis = 99999;

								for (int k = 0; k < m_route.size() - 1; k++)
								{
									QPointF ps = m_route.at(k);
									SPxLatLong_t latlong;
									latlong.latDegs = ps.x();
									latlong.longDegs = ps.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									ps.setX(x);
									ps.setY(y);

									QPointF pe = m_route.at(k + 1);

									latlong.latDegs = pe.x();
									latlong.longDegs = pe.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									pe.setX(x);
									pe.setY(y);
									double dis1 = CommonFunc::pointToSegmentDistance(QVector2D(ppt1), QVector2D(ps), QVector2D(pe));
									double dis2 = CommonFunc::pointToSegmentDistance(QVector2D(ppt2), QVector2D(ps), QVector2D(pe));
									if (dis1 < 1 * 1852)
									{
										if (dis1 < dis2)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)//AIS
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapAISTrail.begin();
						while (it != m_mapAISTrail.end())
						{
							QPointF pt = it.value().at(0);
							QPointF pt2 = it.value().at(1);
							int alertType = alertArea.m_alertAreaType;
							if (alertType == 4)
							{

								QVector<QPointF> m_route = alertArea.m_road;
								SPxLatLong_t latlon;
								latlon.latDegs = pt.x();
								latlon.longDegs = pt.y();
								double x, y;
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt1 = QPoint(x, y);
								latlon.latDegs = pt2.x();
								latlon.longDegs = pt2.y();
								AlarmLatLongToMetres(&latlon, &x, &y);
								QPoint ppt2 = QPoint(x, y);
								int mindis = 99999;

								for (int k = 0; k < m_route.size() - 1; k++)
								{
									QPointF ps = m_route.at(k);
									SPxLatLong_t latlong;
									latlong.latDegs = ps.x();
									latlong.longDegs = ps.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									ps.setX(x);
									ps.setY(y);

									QPointF pe = m_route.at(k + 1);

									latlong.latDegs = pe.x();
									latlong.longDegs = pe.y();
									AlarmLatLongToMetres(&latlong, &x, &y);
									pe.setX(x);
									pe.setY(y);
									double dis1 = CommonFunc::pointToSegmentDistance(QVector2D(ppt1), QVector2D(ps), QVector2D(pe));
									double dis2 = CommonFunc::pointToSegmentDistance(QVector2D(ppt2), QVector2D(ps), QVector2D(pe));
									if (dis1 < 1 * 1852)
									{
										if (dis1 < dis2)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}
			}
			else if (info.group_id == -10 && info.area_id == -10)//全域
			{

				if (info.area_judge == 1)//区域内
				{
					if (info.track_type == 0)
					{


						trackIdSet = m_mapFuseTrack.keys().toSet();
						if (trackIdSet.size() > 0)
						{
							
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}


					}
					else if (info.track_type == 1)
					{

						trackIdSet.clear();
						for (int k : m_mapRadarTrack.keys()) {
							trackIdSet.insert(static_cast<qint64>(k));
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}


					}
					else if (info.track_type == 2)
					{

						trackIdSet.clear();
						for (int k : m_mapAISTrack.keys()) {
							trackIdSet.insert(static_cast<qint64>(k));
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}



			}
			else if (info.group_id == -20 && info.area_id == -20)//距离判断
			{

				if (info.area_judge == 1)//区域内
				{
					if (info.track_type == 0)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it = m_mapFuseTrack.begin();
						while (it != m_mapFuseTrack.end())
						{

							QPointF pt = QPointF(it.value().latDegs, it.value().longDegs);
							QPointF startPt = QPointF(m_uavLat, m_uavLon);
							QPointF endPt = alertArea.m_endP;
							float radius = info.distance;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.distance_condition == 1)//小于距离
							{
								if (dis <= radius)
								{
									if (m_mapFuseTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.key());
								}
							}
							else
							{
								if (dis > radius)
								{
									if (m_mapFuseTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.key());
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, SPxPacketTrackExtended>::iterator it_radar = m_mapRadarTrack.begin();
						while (it_radar != m_mapRadarTrack.end())
						{
							QPointF pt = QPointF(it_radar.value().latDegs, it_radar.value().longDegs);

							QPointF startPt = QPointF(m_uavLat, m_uavLon);

							float radius = info.distance;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.distance_condition == 1)//小于距离
							{
								if (dis <= radius)
								{
									if (m_mapRadarTrail[it_radar.key()].size() > trailCount)
									trackIdSet.insert(it_radar.key());
								}
							}
							else
							{
								if (dis > radius)
								{
									if (m_mapRadarTrail[it_radar.key()].size() > trailCount)
									trackIdSet.insert(it_radar.key());
								}
							}

							it_radar++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, AISTrack>::iterator it_AIS = m_mapAISTrack.begin();
						while (it_AIS != m_mapAISTrack.end())
						{
							QPointF pt = QPointF(it_AIS.value().latDeg, it_AIS.value().longDeg);

							QPointF startPt = QPointF(m_uavLat, m_uavLon);

							float radius = info.distance;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.distance_condition == 1)//小于距离
							{
								if (dis <= radius)
								{
									if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
									trackIdSet.insert(it_AIS.key());
								}
							}
							else
							{
								if (dis > radius)
								{
									if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
									trackIdSet.insert(it_AIS.key());
								}
							}
							it_AIS++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}
				else if (info.area_judge == 2)//离开
				{
					if (info.track_type == 0)
					{
						QMap<qint64, QList<QPointF>>::iterator it = m_mapFuseTrail.begin();
						while (it != m_mapFuseTrail.end())
						{
							QPointF pt = it.value().at(0);


							QPointF startPt = QPointF(m_uavLat, m_uavLon);
							QPointF endPt = alertArea.m_endP;
							float radius = info.distance;;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.speed_condition == 1)
							{
								if (dis <= radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis <= radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							else if (info.speed_condition == 2)
							{
								if (dis > radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis > radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}


						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}
					}


					else if (info.track_type == 1)
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapRadarTrail.begin();
						while (it != m_mapRadarTrail.end())
						{
							QPointF pt = it.value().at(0);


							QPointF startPt = QPointF(m_uavLat, m_uavLon);
							QPointF endPt = alertArea.m_endP;
							float radius = info.distance;;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.speed_condition == 1)
							{
								if (dis <= radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis <= radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							else if (info.speed_condition == 2)
							{
								if (dis > radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis > radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, QList<QPointF>>::iterator it = m_mapAISTrail.begin();
						while (it != m_mapAISTrail.end())
						{
							QPointF pt = it.value().at(0);


							QPointF startPt = QPointF(m_uavLat, m_uavLon);
							QPointF endPt = alertArea.m_endP;
							float radius = info.distance;;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.speed_condition == 1)
							{
								if (dis <= radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis <= radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							else if (info.speed_condition == 2)
							{
								if (dis > radius)
								{
									it++;
									continue;

								}
								else
								{
									for (int i = 1; i < it.value().size(); i++)
									{
										dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), it.value().at(i).x(), it.value().at(i).y());
										if (dis > radius)
										{

											trackIdSet.insert(it.key());

											break;
										}
									}
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}


				}
				else if (info.area_judge == 3)//靠近
				{
					if (info.track_type == 0)
					{
						QMap<qint64, SPxPacketTrackExtended>::iterator it = m_mapFuseTrack.begin();
						while (it != m_mapFuseTrack.end())
						{

							QPointF pt = QPointF(it.value().latDegs, it.value().longDegs);
							QPointF startPt = QPointF(m_uavLat, m_uavLon);
							QPointF endPt = alertArea.m_endP;
							float radius = info.distance;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.distance_condition == 1)//小于距离
							{
								if (dis <= radius + 1852 && dis > radius)
								{
									if (m_mapFuseTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.key());
								}
							}
							else
							{
								if (dis <= radius && dis > radius - 1852)
								{
									if (m_mapFuseTrail[it.key()].size() > trailCount)
									trackIdSet.insert(it.key());
								}
							}
							it++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 0);
						}

					}
					else if (info.track_type == 1)
					{
						QMap<int, SPxPacketTrackExtended>::iterator it_radar = m_mapRadarTrack.begin();
						while (it_radar != m_mapRadarTrack.end())
						{
							QPointF pt = QPointF(it_radar.value().latDegs, it_radar.value().longDegs);

							QPointF startPt = QPointF(m_uavLat, m_uavLon);

							float radius = info.distance;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.distance_condition == 1)//小于距离
							{
								if (dis <= radius + 1852 && dis > radius)
								{
									if (m_mapRadarTrail[it_radar.key()].size() > trailCount)
									trackIdSet.insert(it_radar.key());
								}
							}
							else
							{
								if (dis <= radius && dis > radius - 1852)
								{
									if (m_mapRadarTrail[it_radar.key()].size() > trailCount)
									trackIdSet.insert(it_radar.key());
								}
							}

							it_radar++;
						}

						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 1);
						}

					}
					else if (info.track_type == 2)
					{
						QMap<int, AISTrack>::iterator it_AIS = m_mapAISTrack.begin();
						while (it_AIS != m_mapAISTrack.end())
						{
							QPointF pt = QPointF(it_AIS.value().latDeg, it_AIS.value().longDeg);

							QPointF startPt = QPointF(m_uavLat, m_uavLon);

							float radius = info.distance;
							float dis = CommonFunc::GetDistance2(startPt.x(), startPt.y(), pt.x(), pt.y());
							if (info.distance_condition == 1)//小于距离
							{
								if (dis <= radius + 1852 && dis > radius)
								{
									if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
									trackIdSet.insert(it_AIS.key());
								}
							}
							else
							{
								if (dis <= radius && dis > radius - 1852)
								{
									if (m_mapAISTrail[it_AIS.key()].size() > trailCount)
									trackIdSet.insert(it_AIS.key());
								}
							}
							it_AIS++;
						}
						if (trackIdSet.size() > 0)
						{
							updataAlarmTrackToDB(trackIdSet, info, 2);
						}

					}

				}
			}

		}

	}


	//qDebug() << "2==========================" << endl;

	//AlarmData newAlarm;
	//// ... 设置告警数据 ...
	//emit newAlarmDetected(newAlarm);
}

int TrackAlarmThread::convertTargetTypeStringToBitmask(const QString& targetType)
{
	// 将字符串目标类型转换为位掩码
	// 根据前端定义的映射关系：
	// 海上目标：1-游艇，2-浮标，3-货轮，4-渔船
	// 空中目标：5-无人机，6-巡飞弹
	// 其他：7-未知目标

	if (targetType.isEmpty()) {
		return 0;
	}

	QString lowerType = targetType.toLower();

	// 特殊处理：ship表示所有海上目标
	if (lowerType == "ship") {
		return (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4); // 所有海上目标：游艇|浮标|货轮|渔船
	}

	// 具体海上目标类型映射
	if (lowerType == "speedboat" || lowerType == "yacht" || lowerType == "游艇") {
		return 1 << 1; // 位置1，游艇
	}
	else if (lowerType == "buoy" || lowerType == "浮标") {
		return 1 << 2; // 位置2，浮标
	}
	else if (lowerType == "cargoship" || lowerType == "cargo" || lowerType == "货轮") {
		return 1 << 3; // 位置3，货轮
	}
	else if (lowerType == "fishingboat" || lowerType == "fishing" || lowerType == "渔船") {
		return 1 << 4; // 位置4，渔船
	}
	// 空中目标类型映射
	else if (lowerType == "uav" || lowerType == "drone" || lowerType == "无人机") {
		return 1 << 5; // 位置5，无人机
	}
	else if (lowerType == "missile" || lowerType == "missle" || lowerType == "巡飞弹") {
		return 1 << 6; // 位置6，巡飞弹
	}
	// 其他类型
	else if (lowerType == "unknown" || lowerType == "未知" || lowerType == "未知目标") {
		return 1 << 7; // 位置7，未知目标
	}

	// 如果没有匹配的类型，返回0
	return 0;
}

double TrackAlarmThread::calculateThreatLevel(const SPxPacketTrackExtended& track, const DataAccessLayer::DetectionTypeResult& detectionResult, const ThreatAssessmentParams& threatParams, bool hasProtectArea, const QPointF& protectCenter, double entryAngle)
{
	// 威胁度计算基于数据库参数
	double targetTypeScore = 0.0;
	double speedScore = 0.0;
	double distanceScore = 0.0;
	double capabilityScore = 0.0;

	// 1. 根据检测结果确定目标类型评分
	QString finalTargetType = detectionResult.finalTargetType;
	if (!finalTargetType.isEmpty()) {
		if (finalTargetType == "speedboat" || finalTargetType == "yacht") {
			targetTypeScore = threatParams.seaSpeedboatScore;
		}
		else if (finalTargetType == "warship") {
			targetTypeScore = threatParams.seaWarshipScore;
		}
		else if (finalTargetType == "motorboat") {
			targetTypeScore = threatParams.seaMotorboatScore;
		}
		else if (finalTargetType == "fishingboat") {
			targetTypeScore = threatParams.seaFishingBoatScore;
		}
		else if (finalTargetType == "ship") {
			targetTypeScore = threatParams.seaShipScore;
		}
		else if (finalTargetType == "cargoship") {
			targetTypeScore = threatParams.seaCargoShipScore;
		}
		else if (finalTargetType == "buoy") {
			targetTypeScore = threatParams.seaBuoyScore;
		}
		else if (finalTargetType == "uav" || finalTargetType == "drone") {
			targetTypeScore = threatParams.airDroneScore;
		}
		else if (finalTargetType == "drone_swarm") {
			targetTypeScore = threatParams.airDroneSwarmScore;
		}
		else if (finalTargetType == "compound_wing") {
			targetTypeScore = threatParams.airCompoundWingScore;
		}
		else if (finalTargetType == "rotorcraft") {
			targetTypeScore = threatParams.airRotorcraftScore;
		}
		else if (finalTargetType == "aircraft") {
			targetTypeScore = threatParams.airAircraftScore;
		}
		else if (finalTargetType == "bird") {
			targetTypeScore = threatParams.airBirdScore;
		}
		else if (finalTargetType == "bird_flock") {
			targetTypeScore = threatParams.airBirdFlockScore;
		}
		else {
			// 默认为未知海上目标
			targetTypeScore = threatParams.seaUnknownScore;
		}
	}

	// 2. 计算速度评分
	double speed = track.norm.min.speedMps;
	if (speed < threatParams.speedThresholdLow) {
		speedScore = 0.0;
	}
	else if (speed <= threatParams.speedThresholdHigh) {
		speedScore = (speed - threatParams.speedThresholdLow) / (threatParams.speedThresholdHigh - threatParams.speedThresholdLow) * 10.0;
	}
	else {
		speedScore = 10.0;
	}

	// 3. 计算距离评分（基于保护区）
	if (hasProtectArea) {
		// 计算目标到保护区圆心的距离
		double distanceToProtectCenter = CommonFunc::GetDistance(track.longDegs, track.latDegs, protectCenter.y(), protectCenter.x());
		
		if (distanceToProtectCenter > 0 && threatParams.maxEffectiveDistance > 0) {
			// 距离越近威胁越大
			distanceScore = std::max(0.0, (threatParams.maxEffectiveDistance - distanceToProtectCenter) / threatParams.maxEffectiveDistance * 10.0);
		}
	}
	else {
		// 没有保护区时，使用基准点或自身位置
		double distance = 0.0;
		if (gConfig->m_struBasicConfig.m_nUseBasePoint == 1) {
			distance = CommonFunc::GetDistance(track.longDegs, track.latDegs, 
				gConfig->m_struBasicConfig.m_dBasePointLon, gConfig->m_struBasicConfig.m_dBasePointLat);
		}
		else {
			distance = track.norm.min.rangeMetres;
		}

		if (distance > 0 && threatParams.maxEffectiveDistance > 0) {
			distanceScore = std::max(0.0, (threatParams.maxEffectiveDistance - distance) / threatParams.maxEffectiveDistance * 10.0);
		}
	}

	// 4. 计算能力评分（基于进入角，浮标为0）
	if (finalTargetType == "buoy") {
		capabilityScore = 0.0;
	}
	else {
		if (hasProtectArea) {
			// 使用传入的进入角计算能力评分
			// 进入角越小（越直接指向保护区），威胁越大
			capabilityScore = abs((180.0 - entryAngle) / 180.0 * 10.0);
		}
		else {
			// 没有保护区时，使用目标航向角
			double courseDegrees = track.norm.min.courseDegrees;
			capabilityScore = abs((180.0 - courseDegrees) / 180.0 * 10.0);
		}
	}

	// 5. 根据权重计算总威胁度
	double totalThreatLevel = (
		targetTypeScore * threatParams.typeWeight +
		capabilityScore * threatParams.angleWeight +
		speedScore * threatParams.speedWeight +
		distanceScore * threatParams.distanceWeight
		) / 10.0 * 100.0;

	// 确保威胁度在0-100范围内
	if (totalThreatLevel < 0.0) totalThreatLevel = 0.0;
	if (totalThreatLevel > 100.0) totalThreatLevel = 100.0;

	return totalThreatLevel;
}

double TrackAlarmThread::calculateTimeToProtectArea(const SPxPacketTrackExtended& track, const QPointF& protectCenter, double protectRadius)
{
	double distToCenter = CommonFunc::GetDistance(track.longDegs, track.latDegs, protectCenter.y(), protectCenter.x());
	if (distToCenter <= protectRadius) {
		return 0.0; // 已在保护区内，到达时间为0
	}
	double currentSpeed = track.norm.min.speedMps; // 当前速度 m/s
	double currentCourse = track.norm.min.courseDegrees; // 当前航向角（度）
	if (currentSpeed <= 0.1) {
		return -1.0; // 速度太小，认为不会到达保护区
	}
	double bearingToCenter = calculateBearing(track.latDegs, track.longDegs, protectCenter.x(), protectCenter.y());
	double courseDiff = fabs(currentCourse - bearingToCenter);
	if (courseDiff > 180.0) {
		courseDiff = 360.0 - courseDiff;
	}
	if (courseDiff > 90.0) {
		return -1.0;
	}
	double courseRad = currentCourse * M_PI / 180.0;
	double targetX = (track.longDegs - protectCenter.y()) * 111320.0 * cos(protectCenter.x() * M_PI / 180.0);
	double targetY = (track.latDegs - protectCenter.x()) * 111320.0;
	double dirX = sin(courseRad);
	double dirY = cos(courseRad);
	double a = dirX * dirX + dirY * dirY;
	double b = 2.0 * (targetX * dirX + targetY * dirY);
	double c = targetX * targetX + targetY * targetY - protectRadius * protectRadius;
	double discriminant = b * b - 4.0 * a * c;
	if (discriminant < 0) {
		return -1.0; // 射线不与圆相交，不会进入保护区
	}
	double t1 = (-b - sqrt(discriminant)) / (2.0 * a);
	double t2 = (-b + sqrt(discriminant)) / (2.0 * a);
	double t = -1.0;
	if (t1 > 0 && t2 > 0) {
		t = std::min(t1, t2);
	}
	else if (t1 > 0) {
		t = t1;
	}
	else if (t2 > 0) {
		t = t2;
	}
	if (t > 0) {
		return t / currentSpeed;
	}
	else {
		return -1.0; // 按当前航向不会进入保护区
	}
}