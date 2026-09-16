#include "TrackAlarmThread.h"
#include "AlarmAreaGeometryParser.h"
#include "AirAlarmEligibility.h"
#include "AlarmContentBuilder.h"
#include "AlarmFileLogger.h"
#include "ThreatAssessmentCalculator.h"
#include "NoAlarmAreaPolicy.h"
#include "alarm_geoproj.h"
#include "grpc_alarm/AlarmGrpcSnapshotMapping.h"
#include "grpc_track/new_track_struct_grpc_convert.h"
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
	const AlarmAreaPointList parsed = parseAlarmAreaPointList(areaPoints);
	if (!parsed.valid)
		return;
	for (const QPointF& point : parsed.points) {
		if (areaType == 4)
			alertArea.m_road.append(point);
		else if (areaType == 3)
			alertArea.m_alertAreaPolygon.append(point);
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
		{
			const qint64 t0 = QDateTime::currentMSecsSinceEpoch();
			processAlarms();
			const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - t0;
			if (elapsed > 3000) {
				// processAlarms 超 3s 记一条警告，便于定位慢的根因
				qWarning() << "TrackAlarmThread: processAlarms SLOW elapsed=" << elapsed << "ms";
			}
		}
		gConfig->SendAllAlarmEventMsg();
		// tickAlarmGrpcSnapshot 已移至独立线程（startGrpcSnapshotTimer），此处不再调用
		// gConfig->tickAlarmGrpcSnapshot();
		// 热更新、人工确认和 gRPC 快照线程都会访问 m_mapAlarmData。旧实现未加锁
		// 并在 remove() 前反向移动 QMap 迭代器；方案快速切换时容器被并发 clear，
		// 迭代器会失效并永久卡在 QMapNodeBase::previousNode()。统一在同一互斥量
		// 下正向 erase，既避免数据竞争，也不依赖删除节点后的前驱指针。
		{
			QMutexLocker alarmDataLocker(&gConfig->m_alarmDataMutex);
			const qint64 timeNow = QDateTime::currentMSecsSinceEpoch();
			for (auto it = gConfig->m_mapAlarmData.begin();
				 it != gConfig->m_mapAlarmData.end();) {
				const qint64 timeAlarm = QDateTime::fromString(
					it.value().time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"))
					.toMSecsSinceEpoch();
				const qint64 staleMs =
					gConfig->m_alarmLogic.alarmMapPruneNonFirstStaleMs;
				if (timeAlarm > 0 && timeNow - timeAlarm > staleMs) {
					it.value().alarm_status = 1;
					it = gConfig->m_mapAlarmData.erase(it);
				} else {
					++it;
				}
			}
		}

		msleep(100); // 每3秒检查一次
	}
}

void TrackAlarmThread::SaveToDB(AlarmRule info, qint64 targetId, float lat, float lon, float speed, float dir, float dis, int TargetType, int threatScore, int timestampSec, int radarSourceId, const QString& alarmContent, int eventStage, int taskStatus, const QString& escalationReason)
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
		// 旧规则路径调用默认值 3，代表已形成正式告警；三态路径显式传入真实阶段。
		newAlarm.event_stage = eventStage;
		newAlarm.task_status = taskStatus;
		newAlarm.alarm_content = alarmContent;
		newAlarm.escalation_reason = escalationReason;
		if (!escalationReason.isEmpty())
			newAlarm.escalation_evidence = alarmContent;
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
			sendAlarm.event_stage = qMax(sendAlarm.event_stage, eventStage);
			sendAlarm.task_status = qMax(sendAlarm.task_status, taskStatus);
			sendAlarm.unique_id = targetId;
			sendAlarm.track_id = 0;
			if (radarSourceId != 0)
				sendAlarm.targettype = radarSourceId;
			sendAlarm.group_id = info.group_id;
			sendAlarm.area_id = info.area_id;
			if (!alarmContent.isEmpty()) {
				sendAlarm.alarm_content = alarmContent;
			}
			if (!escalationReason.isEmpty()) {
				sendAlarm.escalation_reason = escalationReason;
				sendAlarm.escalation_evidence = alarmContent;
			}
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

DataAccessLayer::DetectionTypeResult TrackAlarmThread::cognitiveEvidenceForUniqueId(
	qint64 uniqueId)
{
	if (uniqueId <= 0)
		return DataAccessLayer::DetectionTypeResult();
	const auto cached = m_cognitiveEvidenceCache.constFind(uniqueId);
	if (cached != m_cognitiveEvidenceCache.constEnd())
		return cached.value();
	const DataAccessLayer::DetectionTypeResult result =
		gConfig->dbHelper.getDetectionTypesByReId(uniqueId);
	m_cognitiveEvidenceCache.insert(uniqueId, result);
	return result;
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
				SPxPacketTrackExtended radarHit;
				bool foundRadar = false;
				if (m_mapRadarTrack.contains(candidateTrackId)) {
					radarHit = m_mapRadarTrack.value(candidateTrackId);
					foundRadar = true;
				} else {
					const auto uid = static_cast<uint32_t>(candidateTrackId);
					const auto localId = static_cast<uint32_t>(candidateTrackId);
					for (auto it = m_mapRadarTrack.constBegin(); it != m_mapRadarTrack.constEnd(); ++it) {
						if (it.value().secondary.uniqueID == uid || it.value().norm.min.id == localId) {
							radarHit = it.value();
							foundRadar = true;
							break;
						}
					}
				}
				if (!foundRadar) {
					logAlarmTraceThrottled(
						QStringLiteral("upd_skip_radar_%1").arg(candidateTrackId),
						QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:not_in_radar_map conditionId:%2")
							.arg(candidateTrackId)
							.arg(info.condition_id),
						10000);
					continue;
				}
				track = radarHit;
			}
			else if (type == 3)
			{
				SPxPacketTrackExtended birdHit;
				bool foundBird = false;
				if (m_mapBirdRadarTrack.contains(candidateTrackId)) {
					birdHit = m_mapBirdRadarTrack.value(candidateTrackId);
					foundBird = true;
				} else {
					const auto uid = static_cast<uint32_t>(candidateTrackId);
					const auto localId = static_cast<uint32_t>(candidateTrackId);
					for (auto it = m_mapBirdRadarTrack.constBegin(); it != m_mapBirdRadarTrack.constEnd(); ++it) {
						if (it.value().secondary.uniqueID == uid || it.value().norm.min.id == localId) {
							birdHit = it.value();
							foundBird = true;
							break;
						}
					}
				}
				if (!foundBird) {
					logAlarmTraceThrottled(
						QStringLiteral("upd_skip_bird_%1").arg(candidateTrackId),
						QStringLiteral("updataAlarmTrackToDB skip------trackId:%1 reason:not_in_bird_fuse_map conditionId:%2 birdMapSize:%3")
							.arg(candidateTrackId)
							.arg(info.condition_id)
							.arg(m_mapBirdRadarTrack.size()),
						10000);
					continue;
				}
				track = birdHit;
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

			const qint64 targetId = candidateTrackId;
			const qint64 cognitiveUniqueId = track.secondary.uniqueID > 0
				? static_cast<qint64>(track.secondary.uniqueID) : targetId;

			// 当前方案精确免告警区作用于对海融合和对空航迹。它是“禁止创建新事件”边界，
			// 因此放在知识库/黑名单等直告路径之前；已有发布告警仍沿原路径持续。
			QString exactNoAlarmAreaKey;
			const bool supportsExactNoAlarmArea = type == 0 || type == 3;
			const bool insideExactNoAlarmArea = supportsExactNoAlarmArea
				&& findConfiguredNoAlarmArea(
					QPointF(track.latDegs, track.longDegs), type, &exactNoAlarmAreaKey);
			const bool hasPublishedBeforeExactArea = insideExactNoAlarmArea
				&& trackHasPublishedAlarm(
					gConfig, targetId, al.trackAlreadyHasAlarmWindowMs);
			if (NoAlarmAreaPolicy::shouldSuppressNewEvent(
					insideExactNoAlarmArea, hasPublishedBeforeExactArea)) {
				const QString noAlarmDomain = type == 3
					? QStringLiteral("AIR") : QStringLiteral("SURFACE");
				logAlarmTraceThrottled(
					QStringLiteral("upd_skip_exact_noalarm_%1_%2")
						.arg(targetId).arg(info.condition_id),
					QStringLiteral(
						"updataAlarmTrackToDB skip------targetId:%1 "
						"reason:exact_no_alarm_area area:%2 domain:%3 conditionId:%4")
						.arg(targetId).arg(exactNoAlarmAreaKey).arg(noAlarmDomain).arg(info.condition_id),
					10000);
				continue;
			}
			if (insideExactNoAlarmArea && hasPublishedBeforeExactArea) {
				const QString noAlarmDomain = type == 3
					? QStringLiteral("AIR") : QStringLiteral("SURFACE");
				logAlarmTraceThrottled(
					QStringLiteral("upd_exact_noalarm_keep_%1").arg(targetId),
					QStringLiteral(
						"updataAlarmTrackToDB exact_no_alarm_area_keep------targetId:%1 "
						"area:%2 domain:%3 hasPublishedAlarm:1 conditionId:%4")
						.arg(targetId).arg(exactNoAlarmAreaKey).arg(noAlarmDomain).arg(info.condition_id),
					30000);
			}

			// 对海知识库命中是最高优先级的业务告警证据：当前规则的区域候选已经
			// 在调用本函数前形成，命中后只保留 SaveToDB 内的人工删除过滤。
			if (type == 0) {
				const DataAccessLayer::DetectionTypeResult cognitive =
					cognitiveEvidenceForUniqueId(cognitiveUniqueId);
				if (cognitive.archiveVisit.matched) {
					AreaEscalationEvaluator::Result archiveResult;
					archiveResult.targetId = targetId;
					archiveResult.domain = AreaEscalationEvaluator::TargetDomain::Surface;
					archiveResult.laneId = QStringLiteral("legacy");
					archiveResult.trackType = info.track_type;
					archiveResult.stage = AreaEscalationEvaluator::Stage::Alarm;
					archiveResult.disposition =
						AreaEscalationEvaluator::Disposition::VerifySuccess;
					archiveResult.reason = QStringLiteral("archive_visit");
					archiveResult.conditionId = info.condition_id;
					archiveResult.eventArea = {info.group_id, info.area_id};
					archiveResult.score = qBound(0, al.defaultThreatScore, 100);
					archiveResult.insideAlarm = true;
					archiveResult.currentPosition = QPointF(track.latDegs, track.longDegs);
					archiveResult.courseDeg = track.norm.min.courseDegrees;
					archiveResult.speedMps = track.norm.min.speedMps;
					archiveResult.hardConditions = QStringLiteral("archive_visit_bypass");
					archiveResult.archiveTargetLabel =
						cognitive.archiveVisit.targetLabel;
					const QString archiveContent =
						buildAreaEscalationContent(archiveResult);
					logAlarmTrace(QStringLiteral(
						"updataAlarmTrackToDB pass------targetId:%1 path:archive_visit "
						"cognitiveUniqueId:%2 archiveTarget:%3 area:%4/%5 "
						"speed:%6 course:%7 bypass:all_regular_filters conditionId:%8")
						.arg(targetId).arg(cognitiveUniqueId)
						.arg(cognitive.archiveVisit.targetLabel)
						.arg(info.group_id).arg(info.area_id)
						.arg(track.norm.min.speedMps, 0, 'f', 2)
						.arg(track.norm.min.courseDegrees, 0, 'f', 2)
						.arg(info.condition_id));
					SaveToDB(info, targetId, track.latDegs, track.longDegs,
						track.norm.min.speedMps, track.norm.min.courseDegrees,
						track.norm.min.rangeMetres, type, archiveResult.score,
						static_cast<int>(track.msgTimeSecs), radarSourceId,
						archiveContent, static_cast<int>(archiveResult.stage), 3,
						archiveResult.reason);
					continue;
				}
			}

			// 告警规则判断逻辑：优先处理黑白名单，然后进行其他条件过滤
			bool passFilter = true;
			bool blacklistAlarm = false; // 黑名单直接告警标志
			bool whitelistSkip = false;  // 白名单直接忽略标志
			QString failReason;
			float height = track.altitudeMetres;
			const int filterLookupKey =
				(targetId > 0 && targetId <= static_cast<qint64>(std::numeric_limits<int>::max()))
					? static_cast<int>(targetId)
					: 0;
			const bool isAirTrackForContent = (info.track_type > 0) || (type == 3);
			ThreatAssessmentResult lastThreatBreakdown;
			bool hasLastThreatBreakdown = false;
			auto buildContent = [&](const QString& triggerPath, bool hasProtectArea, double attackAngle,
						int threatScoreForContent,
						const DataAccessLayer::DetectionTypeResult* det, bool hasDet) {
				AlarmContentBuildInput in;
				in.rule = &info;
				in.cfg = gConfig;
				in.uniqueId = targetId;
				in.speedMps = track.norm.min.speedMps;
				in.courseDeg = track.norm.min.courseDegrees;
				in.attackAngleDeg = attackAngle;
				in.heightM = height;
				in.threatScore = threatScoreForContent;
				in.hasProtectArea = hasProtectArea;
				in.isAirTrack = isAirTrackForContent;
				const QString trackTypeFallback =
					NewTrackStructGrpcConvert::resolveTargetTypeForScoring(QString(), track);
				if (trackTypeFallback == QLatin1String("ship")) {
					in.fallbackTargetType = trackTypeFallback;
				}
				in.triggerPath = triggerPath;
				if (hasLastThreatBreakdown) {
					in.hasThreatBreakdown = true;
					in.threatBreakdown = lastThreatBreakdown;
				}
				if (hasDet && det) {
					in.detection = *det;
					in.hasDetection = true;
				}
				return buildRuleAlarmContent(in);
			};

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
				const QString blacklistContent = buildContent(
					QStringLiteral("blacklist"), false, track.norm.min.courseDegrees,
					al.defaultThreatScore, nullptr, false);
				SaveToDB(info, targetId, track.latDegs, track.longDegs, track.norm.min.speedMps, track.norm.min.courseDegrees, track.norm.min.rangeMetres, type, al.defaultThreatScore, static_cast<int>(track.msgTimeSecs), radarSourceId, blacklistContent);
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
						const qint64 uniqueId = cognitiveUniqueId;
						DataAccessLayer::DetectionTypeResult detectionResult =
							cognitiveEvidenceForUniqueId(uniqueId);

						if (!detectionResult.found) {
							// 对海：无认知结果则不通过；对空(type==3)：放行，仍计算威胁度
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
						}

						// 威胁度判断：对海需有认知结果；对空无认知结果也计算（类型分按 reserved1/未知）
						if ((info.threat_level1 > 0 || info.threat_level2 > 0) && passFilter) {
							ThreatAssessmentParams threatParams;
							bool foundParams = false;
							for (const ThreatAssessmentParams& params : gConfig->m_listThreatAssessmentParams) {
								if (params.groupId == info.group_id && params.areaId == info.area_id) {
									threatParams = params;
									foundParams = true;
									break;
								}
							}
							if (!foundParams) {
								threatParams = ThreatAssessmentParams(info.group_id, info.area_id);
							}

							lastThreatBreakdown = calculateThreatAssessment(
								track, detectionResult, threatParams, hasProtectArea, protectCenter, angleToCheck);
							hasLastThreatBreakdown = true;
							const double calculatedThreatLevel = lastThreatBreakdown.totalThreatLevel;
							lastCalculatedThreatScore = qBound(0, static_cast<int>(qRound(calculatedThreatLevel)), 100);

							if (info.threat_level1 > 0 && calculatedThreatLevel >= info.threat_level1) {
								blacklistAlarm = true;
								logAlarmTrace(QStringLiteral("updataAlarmTrackToDB pass------trackId:%1 path:threat_direct score:%2 type:%3 speed:%4 angle:%5 dist:%6 threshold:%7 conditionId:%8")
									.arg(track.norm.min.id)
									.arg(calculatedThreatLevel, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedTypeScore, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedSpeedScore, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedAngleScore, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedDistanceScore, 0, 'f', 1)
									.arg(info.threat_level1)
									.arg(info.condition_id));
							}
							else if (info.threat_level1 > 0 && calculatedThreatLevel < info.threat_level1) {
								passFilter = false;
								failReason = QStringLiteral("threat_score_low score:%1 type:%2 speed:%3 angle:%4 dist:%5 threshold:%6 uniqueId:%7")
									.arg(calculatedThreatLevel, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedTypeScore, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedSpeedScore, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedAngleScore, 0, 'f', 1)
									.arg(lastThreatBreakdown.weightedDistanceScore, 0, 'f', 1)
									.arg(info.threat_level1)
									.arg(uniqueId);
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
					// 证据链只在「首次触发 / 条件再次满足」时写入触发时刻速度航向；
					// 纯持续告警（当前不满足规则）保留首次 content，勿用当前值覆盖。
					QString ruleContent;
					if (!(trackAlreadyHasAlarm && !passFilter)) {
						const qint64 detectionKey =
							track.secondary.uniqueID > 0 ? track.secondary.uniqueID : targetId;
						DataAccessLayer::DetectionTypeResult detectionForContent =
							cognitiveEvidenceForUniqueId(detectionKey);
						const bool hasDetectionForContent = detectionForContent.found;
						QString triggerPath = QStringLiteral("rule_match");
						if (info.threat_level1 > 0
							&& lastCalculatedThreatScore >= info.threat_level1) {
							triggerPath = QStringLiteral("threat_direct");
						}
						ruleContent = buildContent(
							triggerPath,
							hasProtectArea,
							angleToCheck,
							lastCalculatedThreatScore,
							&detectionForContent,
							hasDetectionForContent);
					}
					SaveToDB(info, targetId, track.latDegs, track.longDegs, track.norm.min.speedMps, track.norm.min.courseDegrees, track.norm.min.rangeMetres, type, lastCalculatedThreatScore, static_cast<int>(track.msgTimeSecs), radarSourceId, ruleContent);
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
		} else if (alertArea1.m_alertAreaType == 3
			&& alertArea1.m_alertAreaPolygon.size() < 3) {
			qWarning() << "getAlarmArea skip type=3 insufficient polygon points group" << m_area1.groupID
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

QString escalationSourceRuleId(const AlarmRule& rule)
{
	const QString prefix = QStringLiteral("threat_rule_%1_%2_")
		.arg(rule.group_id)
		.arg(rule.area_id);
	if (rule.condition_id.startsWith(prefix))
		return rule.condition_id.mid(prefix.size());
	return rule.condition_id;
}

QString escalationLaneKey(int trackType, const QString& sourceRuleId)
{
	return QStringLiteral("%1|%2").arg(trackType).arg(sourceRuleId);
}

QString escalationTrackStateKey(
	AreaEscalationEvaluator::TargetDomain domain,
	const QString& laneId,
	qint64 targetId)
{
	return QStringLiteral("%1|%2|%3")
		.arg(static_cast<int>(domain))
		.arg(laneId)
		.arg(targetId);
}

QString escalationDomainName(AreaEscalationEvaluator::TargetDomain domain)
{
	return domain == AreaEscalationEvaluator::TargetDomain::Air
		? QStringLiteral("AIR") : QStringLiteral("SURFACE");
}

bool TrackAlarmThread::findAlarmArea(int groupId, int areaId, AlarmArea* out) const
{
	if (out == nullptr || !m_maparea.contains(groupId))
		return false;
	for (const AlarmArea& area : m_maparea.value(groupId)) {
		if (area.areaID == areaId) {
			*out = area;
			return true;
		}
	}
	return false;
}

bool TrackAlarmThread::findConfiguredNoAlarmArea(
	const QPointF& point, int trackType, QString* matchedAreaKey) const
{
	if (matchedAreaKey != nullptr)
		matchedAreaKey->clear();
	if (gConfig == nullptr || gConfig->m_activeSchemeNoAlarmAreas.isEmpty())
		return false;

	for (auto groupIt = m_maparea.constBegin(); groupIt != m_maparea.constEnd(); ++groupIt) {
		for (const AlarmArea& area : groupIt.value()) {
			if (!NoAlarmAreaPolicy::appliesTo(
					gConfig->m_activeSchemeNoAlarmAreas,
					area.groupID, area.areaID, trackType)
				|| !containsCurrentPoint(area, point)) {
				continue;
			}
			if (matchedAreaKey != nullptr)
				*matchedAreaKey = NoAlarmAreaPolicy::areaKey(area.groupID, area.areaID);
			return true;
		}
	}
	return false;
}

AreaEscalationProtectionResolver::Context TrackAlarmThread::resolveProtectionContext(
	const AlarmRule& rule,
	AreaEscalationEvaluator::TargetDomain domain) const
{
	AreaEscalationProtectionResolver::Request request;
	request.config = gConfig->m_areaEscalation;
	request.activeSchemeId = gConfig->m_activeAlarmSchemeId;
	request.targetDomain = escalationDomainName(domain);

	const QString areaKey = QStringLiteral("%1_%2").arg(rule.group_id).arg(rule.area_id);
	if (gConfig->m_mapSchemeProtectAreas.contains(areaKey)) {
		const QPair<int, int> protectKey = gConfig->m_mapSchemeProtectAreas.value(areaKey);
		AlarmArea protectArea;
		if (findAlarmArea(protectKey.first, protectKey.second, &protectArea)
			&& protectArea.m_alertAreaType == 2) {
			request.databaseCircleAvailable = true;
			request.databaseCenter = protectArea.m_startP;
			request.databaseRadiusMeters =
				AreaEscalationProtectionResolver::distanceMeters(
					protectArea.m_startP, protectArea.m_endP);
		}
	}
	return AreaEscalationProtectionResolver::resolve(request);
}

bool TrackAlarmThread::containsCurrentPoint(const AlarmArea& area, const QPointF& point) const
{
	switch (area.m_alertAreaType) {
	case 1:
		return area.m_alertAreaRect.normalized().contains(point);
	case 2: {
		const double radius = CommonFunc::GetDistance2(
			area.m_startP.x(), area.m_startP.y(), area.m_endP.x(), area.m_endP.y());
		const double distance = CommonFunc::GetDistance2(
			area.m_startP.x(), area.m_startP.y(), point.x(), point.y());
		return distance <= radius;
	}
	case 3:
		return area.m_alertAreaPolygon.containsPoint(point, Qt::OddEvenFill);
	default:
		return false;
	}
}

AreaEscalationEvaluator::AreaDefinition TrackAlarmThread::toEscalationArea(
	const AlarmArea& area) const
{
	AreaEscalationEvaluator::AreaDefinition out;
	out.key = {area.groupID, area.areaID};
	out.name = area.areaName;
	switch (area.m_alertAreaType) {
	case 1:
		out.shape = AreaEscalationEvaluator::Shape::Rectangle;
		out.rectangle = area.m_alertAreaRect.normalized();
		break;
	case 2:
		out.shape = AreaEscalationEvaluator::Shape::Circle;
		out.circleCenter = area.m_startP;
		out.circleRadius = CommonFunc::GetDistance2(
			area.m_startP.x(), area.m_startP.y(), area.m_endP.x(), area.m_endP.y());
		out.geographic = true;
		break;
	case 3:
		out.shape = AreaEscalationEvaluator::Shape::Polygon;
		out.polygon = area.m_alertAreaPolygon;
		break;
	default:
		out.shape = AreaEscalationEvaluator::Shape::Invalid;
		break;
	}
	return out;
}

bool TrackAlarmThread::configureAreaEscalation(const QList<AlarmRule>& rules)
{
	if (m_areaEscalationGeneration == gConfig->m_alarmConfigGeneration)
		return m_areaEscalationActive;

	m_areaEscalationGeneration = gConfig->m_alarmConfigGeneration;
	m_areaEscalationActive = false;
	m_areaEscalationBindings.clear();
	m_areaEscalationClaimedConditionIds.clear();
	m_areaEscalationPreviousSpeed.clear();

	if (!gConfig->m_areaEscalation.enabled) {
		AreaEscalationEvaluator::PolicyDefinition disabled;
		m_areaEscalationEvaluator.reset(disabled);
		qInfo() << "AreaEscalation disabled by configuration";
		return false;
	}

	QMap<QString, QList<AlarmRule>> warningAreas;
	QMap<QString, QList<AlarmRule>> alarmAreas;
	for (const AlarmRule& rule : rules) {
		if (!rule.alarmstate || rule.group_id < 0 || rule.area_id < 0)
			continue;
		const QString key = QStringLiteral("%1/%2").arg(rule.group_id).arg(rule.area_id);
		if (rule.alarm_level == 2)
			warningAreas[key].append(rule);
		else if (rule.alarm_level == 3)
			alarmAreas[key].append(rule);
	}

	// 没有预警/告警角色时才保留旧规则路径；仅 A 或仅 B 也由新引擎统一处理。
	if (warningAreas.isEmpty() && alarmAreas.isEmpty()) {
		AreaEscalationEvaluator::PolicyDefinition disabled;
		m_areaEscalationEvaluator.reset(disabled);
		qInfo().noquote() << QStringLiteral(
			"AreaEscalation no warning/alarm roles, legacy compatibility mode "
			"warningAreaCount=%1 alarmAreaCount=%2 generation=%3")
			.arg(warningAreas.size()).arg(alarmAreas.size()).arg(m_areaEscalationGeneration);
		return false;
	}

	auto fail = [&](const QString& reason) {
		AreaEscalationEvaluator::PolicyDefinition invalid;
		m_areaEscalationEvaluator.reset(invalid);
		m_areaEscalationBindings.clear();
		m_areaEscalationClaimedConditionIds.clear();
		qCritical().noquote() << QStringLiteral(
			"AreaEscalation 多区域策略失败，安全回退旧逻辑: %1 warningAreaCount=%2 alarmAreaCount=%3")
			.arg(reason).arg(warningAreas.size()).arg(alarmAreas.size());
		AlarmFileLogger::logNewAlarmTrack(
			QStringLiteral("AreaEscalation policy invalid------reason:%1 warningAreaCount:%2 alarmAreaCount:%3")
				.arg(reason).arg(warningAreas.size()).arg(alarmAreas.size()));
		return false;
	};

	AreaEscalationEvaluator::PolicyDefinition policy;
	policy.enabled = true;
	policy.dwellMs = 7000;
	QSet<QString> bindingKeys;
	QStringList bindingDescriptions;

	auto appendRole = [&](const QMap<QString, QList<AlarmRule>>& areas,
		AreaEscalationEvaluator::AreaRole role) -> bool {
		for (auto areaIt = areas.constBegin(); areaIt != areas.constEnd(); ++areaIt) {
			const QList<AlarmRule>& areaRules = areaIt.value();
			if (areaRules.isEmpty())
				return fail(QStringLiteral("区域 %1 未绑定规则").arg(areaIt.key()));
			AlarmArea alarmArea;
			if (!findAlarmArea(areaRules.first().group_id, areaRules.first().area_id, &alarmArea))
				return fail(QStringLiteral("区域 %1 几何未加载").arg(areaIt.key()));
			const AreaEscalationEvaluator::AreaDefinition definition = toEscalationArea(alarmArea);
			if (!definition.isValid())
				return fail(QStringLiteral("区域 %1 几何无效").arg(areaIt.key()));
			if (role == AreaEscalationEvaluator::AreaRole::Warning)
				policy.warningAreas.append(definition);
			else
				policy.alarmAreas.append(definition);

			for (const AlarmRule& rule : areaRules) {
				if (rule.track_type != 0 && rule.track_type != 3)
					return fail(QStringLiteral("区域 %1 含不支持 track_type=%2 condition=%3")
						.arg(areaIt.key()).arg(rule.track_type).arg(rule.condition_id));
				if (!(0 <= rule.threat_level1 && rule.threat_level1 < rule.threat_level2
					  && rule.threat_level2 <= 100)) {
					return fail(QStringLiteral("区域 %1 阈值无效 condition=%2 threshold=%3/%4")
						.arg(areaIt.key(), rule.condition_id)
						.arg(rule.threat_level1).arg(rule.threat_level2));
				}
				const QString bindingKey = QStringLiteral("%1|%2")
					.arg(areaIt.key()).arg(rule.track_type);
				if (bindingKeys.contains(bindingKey))
					return fail(QStringLiteral("同一区域同一 track_type 只能绑定一条规则: %1")
						.arg(bindingKey));
				bindingKeys.insert(bindingKey);

				AreaEscalationBinding binding;
				binding.area = definition;
				binding.runtimeArea = alarmArea;
				binding.role = role;
				binding.domain = rule.track_type == 3
					? AreaEscalationEvaluator::TargetDomain::Air
					: AreaEscalationEvaluator::TargetDomain::Surface;
				binding.laneId = escalationDomainName(binding.domain);
				binding.trackType = rule.track_type;
				binding.rule = rule;
				m_areaEscalationBindings.append(binding);
				m_areaEscalationClaimedConditionIds.insert(rule.condition_id);
				bindingDescriptions.append(QStringLiteral("%1:%2:%3:%4/%5")
					.arg(role == AreaEscalationEvaluator::AreaRole::Warning
						? QStringLiteral("A") : QStringLiteral("B"),
						definition.key.toString(), binding.laneId)
					.arg(rule.threat_level1).arg(rule.threat_level2));
			}
		}
		return true;
	};

	if (!appendRole(warningAreas, AreaEscalationEvaluator::AreaRole::Warning)
		|| !appendRole(alarmAreas, AreaEscalationEvaluator::AreaRole::Alarm)) {
		return false;
	}
	if (m_areaEscalationBindings.isEmpty())
		return fail(QStringLiteral("多区域策略未形成任何规则绑定"));
	if (!policy.warningAreas.isEmpty())
		policy.warningArea = policy.warningAreas.first();
	if (!policy.alarmAreas.isEmpty())
		policy.alarmArea = policy.alarmAreas.first();

	QString error;
	if (!m_areaEscalationEvaluator.reset(policy, &error))
		return fail(error);
	m_areaEscalationActive = true;
	QStringList warningKeys;
	QStringList alarmKeys;
	for (const auto& area : policy.warningAreas) warningKeys.append(area.key.toString());
	for (const auto& area : policy.alarmAreas) alarmKeys.append(area.key.toString());
	qInfo().noquote() << QStringLiteral(
		"AreaEscalation 多区域策略成功 A=[%1] B=[%2] bindings=[%3] generation=%4")
		.arg(warningKeys.join(QLatin1Char(',')), alarmKeys.join(QLatin1Char(',')),
			 bindingDescriptions.join(QLatin1Char(',')))
		.arg(m_areaEscalationGeneration);
	AlarmFileLogger::logNewAlarmTrack(QStringLiteral(
		"AreaEscalation policy ready------A:[%1] B:[%2] bindings:[%3] generation:%4")
		.arg(warningKeys.join(QLatin1Char(',')), alarmKeys.join(QLatin1Char(',')),
			 bindingDescriptions.join(QLatin1Char(',')))
		.arg(m_areaEscalationGeneration));
	return true;
}

AreaEscalationEvaluator::AreaEvidence TrackAlarmThread::evaluateAreaEscalationEvidence(
	const AlarmRule& rule,
	const SPxPacketTrackExtended& track,
	const DataAccessLayer::DetectionTypeResult& detection,
	bool previousSpeedPassed,
	bool insideArea)
{
	AreaEscalationEvaluator::AreaEvidence evidence;
	evidence.available = insideArea;
	evidence.conditionId = rule.condition_id;
	if (!insideArea)
		return evidence;
	if (rule.track_type == 0 && detection.archiveVisit.matched) {
		evidence.archiveVisitMatched = true;
		evidence.archiveTargetLabel = detection.archiveVisit.targetLabel;
		evidence.archiveThreatScore = qBound(
			0, gConfig->m_alarmLogic.defaultThreatScore, 100);
	}

	QStringList failures;
	auto reject = [&](bool* field, const QString& reason) {
		*field = false;
		failures.append(reason);
	};

	const double speed = track.norm.min.speedMps;
	bool currentSpeedPassed = true;
	if (rule.speed_condition == 1)
		currentSpeedPassed = speed < rule.speed;
	else if (rule.speed_condition == 2)
		currentSpeedPassed = speed > rule.speed;
	if (!currentSpeedPassed)
		reject(&evidence.hard.speed, QStringLiteral("speed"));
	if (rule.speed_condition > 0 && !(previousSpeedPassed && currentSpeedPassed))
		reject(&evidence.hard.speedDoubleCheck, QStringLiteral("speed_double_check"));

	if (rule.height_min != rule.height_max
		&& (track.altitudeMetres < rule.height_min || track.altitudeMetres > rule.height_max)) {
		reject(&evidence.hard.height, QStringLiteral("height"));
	}

	const AreaEscalationEvaluator::TargetDomain targetDomain = rule.track_type == 3
		? AreaEscalationEvaluator::TargetDomain::Air
		: AreaEscalationEvaluator::TargetDomain::Surface;
	const AreaEscalationProtectionResolver::Context protection =
		resolveProtectionContext(rule, targetDomain);
	const bool hasProtectArea = protection.available;
	const QPointF protectCenter = protection.center;

	double angleToCheck = track.norm.min.courseDegrees;
	if (hasProtectArea) {
		const double bearingToCenter = AreaEscalationProtectionResolver::bearingDegrees(
			QPointF(track.latDegs, track.longDegs), protectCenter);
		angleToCheck = AreaEscalationProtectionResolver::attackAngleDegrees(
			track.norm.min.courseDegrees, bearingToCenter);
	}
	if (rule.course_min != rule.course_max) {
		const bool anglePassed = rule.course_min <= rule.course_max
			? (angleToCheck >= rule.course_min && angleToCheck <= rule.course_max)
			: (angleToCheck >= rule.course_min || angleToCheck <= rule.course_max);
		if (!anglePassed)
			reject(&evidence.hard.entryAngle, QStringLiteral("entry_angle"));
	}

	if (rule.dist_to_protect_area >= 0 && hasProtectArea) {
		const double centerDistance = AreaEscalationProtectionResolver::distanceMeters(
			QPointF(track.latDegs, track.longDegs), protectCenter);
		const double edgeDistance = std::max(0.0, centerDistance - protection.radiusMeters);
		if (edgeDistance >= rule.dist_to_protect_area)
			reject(&evidence.hard.protectDistance, QStringLiteral("protect_distance"));
	}
	if (rule.entry_time > 0 && hasProtectArea) {
		const double seconds = calculateTimeToProtectArea(
			track, protectCenter, protection.radiusMeters);
		if (seconds < 0.0 || seconds > rule.entry_time * 60.0)
			reject(&evidence.hard.entryTime, QStringLiteral("entry_time"));
	}

	const qint64 targetId = track.secondary.uniqueID > 0
		? static_cast<qint64>(track.secondary.uniqueID)
		: static_cast<qint64>(track.norm.min.id);
	const int filterKey = targetId > 0 && targetId <= std::numeric_limits<int>::max()
		? static_cast<int>(targetId) : 0;
	if (filterKey > 0 && gConfig->m_mapTargetInfoFilter.contains(filterKey)) {
		const TargetInfoFilter attrs = gConfig->m_mapTargetInfoFilter.value(filterKey);
		if (rule.whitelist_judge == 1 && attrs.black_white_attr != 1)
			reject(&evidence.hard.targetAttributes, QStringLiteral("whitelist"));
		if (rule.affiliation_judge > 0
			&& (rule.affiliation_judge & (1 << attrs.affiliation_attr)) == 0)
			reject(&evidence.hard.targetAttributes, QStringLiteral("affiliation"));
		if ((rule.corp_judge == 1 && attrs.corp_attr != 1)
			|| (rule.corp_judge == 2 && attrs.corp_attr == 1))
			reject(&evidence.hard.targetAttributes, QStringLiteral("corp"));
	}

	// 识别结果与 B 区事件的运动/空间硬条件分属不同证据路径：
	// 1. 无识别结果时仍按现有威胁评分回退口径形成 LOW/MEDIUM 事件；
	// 2. MinIO 图片另由 opticSeen 独立触发 HIGH；
	// 3. 否则“optic 或 B 区连续 7 秒”会错误退化成必须 optic。
	evidence.hard.detection = detection.found;
	if (detection.found) {
		const QString resolvedType = !detection.finalTargetType.isEmpty()
			? detection.finalTargetType : detection.camTargetType;
		if (rule.targetattr_type > 0) {
			const int typeBit = convertTargetTypeStringToBitmask(resolvedType);
			if (typeBit == 0 || (rule.targetattr_type & typeBit) == 0)
				reject(&evidence.hard.targetType, QStringLiteral("target_type"));
		}
		if (rule.camera_detect_type > 0) {
			const int bit = convertTargetTypeStringToBitmask(detection.camTargetType);
			if (bit == 0 || (rule.camera_detect_type & bit) == 0)
				reject(&evidence.hard.targetType, QStringLiteral("camera_type"));
		}
		if (rule.llm_detect_type > 0) {
			const int bit = convertTargetTypeStringToBitmask(detection.llmTargetType);
			if (bit == 0 || (rule.llm_detect_type & bit) == 0)
				reject(&evidence.hard.targetType, QStringLiteral("llm_type"));
		}
		if (rule.uav_detect_type > 0) {
			const int bit = convertTargetTypeStringToBitmask(detection.uavTargetType);
			if (bit == 0 || (rule.uav_detect_type & bit) == 0)
				reject(&evidence.hard.targetType, QStringLiteral("uav_type"));
		}
	}

	ThreatAssessmentParams threatParams(rule.group_id, rule.area_id);
	for (const ThreatAssessmentParams& params : gConfig->m_listThreatAssessmentParams) {
		if (params.groupId == rule.group_id && params.areaId == rule.area_id) {
			threatParams = params;
			break;
		}
	}
	const ThreatAssessmentResult assessment = calculateThreatAssessment(
		track, detection, threatParams, hasProtectArea, protectCenter, angleToCheck);
	evidence.score = qBound(0, static_cast<int>(qRound(assessment.totalThreatLevel)), 100);
	evidence.hard.failure = failures.join(QLatin1Char('|'));
	return evidence;
}

void TrackAlarmThread::processAreaEscalation()
{
	QHash<QString, AreaEscalationEvaluator::TargetSnapshot> snapshotsByTarget;
	QSet<AreaEscalationEvaluator::TargetKey> liveTargets;
	QSet<QString> liveSpeedKeys;
	QHash<QString, DataAccessLayer::DetectionTypeResult> detectionCache;
	QHash<QString, bool> opticCache;
	QHash<QString, bool> noAlarmCache;
	QHash<QString, bool> strictNoAlarmCache;

	auto businessTargetId = [](qint64 mapKey, const SPxPacketTrackExtended& track) {
		if (mapKey > 0) return mapKey;
		if (track.secondary.uniqueID > 0)
			return static_cast<qint64>(track.secondary.uniqueID);
		return static_cast<qint64>(track.norm.min.id);
	};

	auto appendBindingSnapshots = [&](const AreaEscalationBinding& binding, const auto& trackMap) {
		for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
			const SPxPacketTrackExtended& track = it.value();
			const qint64 targetId = businessTargetId(static_cast<qint64>(it.key()), track);
			if (targetId <= 0) continue;
			const auto domain = binding.domain;
			liveTargets.insert({domain, targetId});

			if (gConfig->isUniqueIdAlarmFiltered(targetId)) {
				m_areaEscalationEvaluator.clearTarget(domain, targetId);
				continue; // 人工结束事件同时清资格
			}
			if (domain == AreaEscalationEvaluator::TargetDomain::Air) {
				const AirAlarmEligibility::Decision admission =
					AirAlarmEligibility::decide(track, gConfig->m_alarmLogic);
				if (admission.skip) {
					m_areaEscalationEvaluator.clearTarget(domain, targetId);
					logAlarmTraceThrottled(
						QStringLiteral("area_escalation_air_skip_%1_%2")
							.arg(targetId).arg(admission.reason),
						QStringLiteral(
							"AreaEscalation air target skipped------target:%1 reason:%2 "
							"fusionTid:%3 conditionId:%4")
							.arg(targetId).arg(admission.reason)
							.arg(admission.matchedFusionTrackId)
							.arg(binding.rule.condition_id),
						10000);
					continue;
				}
			}

			const QString targetKey = QStringLiteral("%1|%2")
				.arg(static_cast<int>(domain)).arg(targetId);
			const QString speedKey = QStringLiteral("%1|%2")
				.arg(binding.rule.condition_id).arg(targetId);
			liveSpeedKeys.insert(speedKey);

			const QPointF point(track.latDegs, track.longDegs);
			const bool insideArea = containsCurrentPoint(binding.runtimeArea, point);
			const bool previousSpeedPassed =
				m_areaEscalationPreviousSpeed.value(speedKey, false);

			AreaEscalationEvaluator::TargetSnapshot& snapshot = snapshotsByTarget[targetKey];
			snapshot.targetId = targetId;
			snapshot.domain = domain;
			snapshot.laneId = binding.laneId;
			snapshot.position = point;
			snapshot.courseDeg = track.norm.min.courseDegrees;
			snapshot.speedMps = track.norm.min.speedMps;
			if (!noAlarmCache.contains(targetKey)) {
				bool insideLegacyNoAlarm = false;
				for (int groupId : gConfig->m_alarmLogic.noAlarmGroupIds) {
					if (groupId >= 0 && isTrackInGroupAreaByGroupId(point, groupId)) {
						insideLegacyNoAlarm = true;
						break;
					}
				}
				QString exactNoAlarmAreaKey;
				const int noAlarmTrackType =
					domain == AreaEscalationEvaluator::TargetDomain::Air ? 3 : 0;
				const bool insideExactNoAlarm =
					findConfiguredNoAlarmArea(
						point, noAlarmTrackType, &exactNoAlarmAreaKey);
				const bool hasPublished = (insideLegacyNoAlarm || insideExactNoAlarm)
					&& trackHasPublishedAlarm(
					gConfig, targetId, gConfig->m_alarmLogic.trackAlreadyHasAlarmWindowMs);
				const bool exactSuppression =
					NoAlarmAreaPolicy::shouldSuppressNewEvent(
						insideExactNoAlarm, hasPublished);
				noAlarmCache.insert(
					targetKey, (insideLegacyNoAlarm && !hasPublished) || exactSuppression);
				strictNoAlarmCache.insert(targetKey, exactSuppression);
				if (insideExactNoAlarm) {
					const QString action = exactSuppression
						? QStringLiteral("suppress_new") : QStringLiteral("keep_existing");
					logAlarmTraceThrottled(
						QStringLiteral("area_escalation_exact_noalarm_%1_%2")
							.arg(targetId).arg(action),
						QStringLiteral(
							"AreaEscalation exact no-alarm area------domain:%1 "
							"target:%2 area:%3 action:%4 hasPublishedAlarm:%5")
							.arg(escalationDomainName(domain)).arg(targetId)
							.arg(exactNoAlarmAreaKey).arg(action).arg(hasPublished ? 1 : 0),
						exactSuppression ? 10000 : 30000);
				}
			}
			snapshot.suppressNewEvent = noAlarmCache.value(targetKey, false);
			snapshot.suppressAllNewEvents =
				strictNoAlarmCache.value(targetKey, false);

			const qint64 detectionId = track.secondary.uniqueID > 0
				? static_cast<qint64>(track.secondary.uniqueID) : targetId;
			if (insideArea && !detectionCache.contains(targetKey))
				detectionCache.insert(
					targetKey, cognitiveEvidenceForUniqueId(detectionId));
			const DataAccessLayer::DetectionTypeResult detection = detectionCache.value(targetKey);

			AreaEscalationEvaluator::AreaObservation observation;
			observation.area = binding.area.key;
			observation.role = binding.role;
			observation.laneId = binding.laneId;
			observation.trackType = binding.trackType;
			observation.threatThreshold = binding.rule.threat_level1;
			observation.prewarningThreshold = binding.rule.threat_level2;
			observation.evidence = evaluateAreaEscalationEvidence(
				binding.rule, track, detection, previousSpeedPassed, insideArea);
			if (insideArea && binding.role == AreaEscalationEvaluator::AreaRole::Alarm) {
				if (!opticCache.contains(targetKey)) {
					opticCache.insert(targetKey,
						gConfig->dbHelper.hasMinioMultiMetadataForUniqueId(
							detectionId, QStringLiteral("%")));
				}
				observation.evidence.opticSeen = opticCache.value(targetKey, false);
				if (observation.evidence.score < observation.threatThreshold
					|| !observation.evidence.hard.allPassed()) {
					logAlarmTraceThrottled(
						QStringLiteral("area_escalation_block_%1_%2")
							.arg(speedKey, binding.area.key.toString()),
						QStringLiteral(
							"AreaEscalation blocked in B------domain:%1 lane:%2 target:%3 area:%4 "
							"score:%5 thresholds:%6/%7 speed:%8 course:%9 hard:[%10]")
							.arg(escalationDomainName(domain), binding.laneId)
							.arg(targetId).arg(binding.area.key.toString())
							.arg(observation.evidence.score)
							.arg(observation.threatThreshold)
							.arg(observation.prewarningThreshold)
							.arg(track.norm.min.speedMps, 0, 'f', 2)
							.arg(track.norm.min.courseDegrees, 0, 'f', 2)
							.arg(observation.evidence.hard.summary()),
						5000);
				}
			}
			snapshot.observations.append(observation);

			if (insideArea) {
				bool currentSpeedPassed = true;
				if (binding.rule.speed_condition == 1)
					currentSpeedPassed = track.norm.min.speedMps < binding.rule.speed;
				else if (binding.rule.speed_condition == 2)
					currentSpeedPassed = track.norm.min.speedMps > binding.rule.speed;
				m_areaEscalationPreviousSpeed.insert(speedKey, currentSpeedPassed);
			}
		}
	};

	for (const AreaEscalationBinding& binding : m_areaEscalationBindings) {
		if (binding.domain == AreaEscalationEvaluator::TargetDomain::Air)
			appendBindingSnapshots(binding, m_mapBirdRadarTrack);
		else
			appendBindingSnapshots(binding, m_mapFuseTrack);
	}

	// 生命周期以各实时航迹表为准；非融合表只证明航迹仍存在，不参与 A/B 区域判定。
	auto addSpxLiveness = [&](const auto& trackMap,
		AreaEscalationEvaluator::TargetDomain domain, bool keepMapKey) {
		for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
			const SPxPacketTrackExtended& track = it.value();
			if (keepMapKey && static_cast<qint64>(it.key()) > 0)
				liveTargets.insert({domain, static_cast<qint64>(it.key())});
			if (track.secondary.uniqueID > 0)
				liveTargets.insert({domain, static_cast<qint64>(track.secondary.uniqueID)});
			if (track.norm.min.id > 0)
				liveTargets.insert({domain, static_cast<qint64>(track.norm.min.id)});
		}
	};
	addSpxLiveness(m_mapFuseTrack, AreaEscalationEvaluator::TargetDomain::Surface, true);
	addSpxLiveness(m_mapBirdRadarTrack, AreaEscalationEvaluator::TargetDomain::Air, true);
	// 原始雷达可能是海/空融合的上游，只作双域存活证据，避免融合点短暂缺失误删资格。
	addSpxLiveness(m_mapRadarTrack, AreaEscalationEvaluator::TargetDomain::Surface, false);
	addSpxLiveness(m_mapRadarTrack, AreaEscalationEvaluator::TargetDomain::Air, false);
	for (auto it = m_mapAISTrack.constBegin(); it != m_mapAISTrack.constEnd(); ++it) {
		if (it.key() > 0)
			liveTargets.insert({AreaEscalationEvaluator::TargetDomain::Surface,
				static_cast<qint64>(it.key())});
		if (it.value().MMSI > 0)
			liveTargets.insert({AreaEscalationEvaluator::TargetDomain::Surface,
				static_cast<qint64>(it.value().MMSI)});
	}

	for (auto it = m_areaEscalationPreviousSpeed.begin();
		 it != m_areaEscalationPreviousSpeed.end();) {
		if (!liveSpeedKeys.contains(it.key())) it = m_areaEscalationPreviousSpeed.erase(it);
		else ++it;
	}

	const QList<AreaEscalationEvaluator::TargetSnapshot> snapshots = snapshotsByTarget.values();
	const QList<AreaEscalationEvaluator::Result> results =
		m_areaEscalationEvaluator.evaluateCycle(
			snapshots, liveTargets, QDateTime::currentMSecsSinceEpoch());
	for (const AreaEscalationEvaluator::Result& result : results)
		applyAreaEscalationResult(result);
}

void TrackAlarmThread::applyAreaEscalationResult(
	const AreaEscalationEvaluator::Result& result)
{
	if (result.conditionId.isEmpty())
		return;
	const bool isAir = result.domain == AreaEscalationEvaluator::TargetDomain::Air;
	const auto& trackMap = isAir ? m_mapBirdRadarTrack : m_mapFuseTrack;
	if (!trackMap.contains(result.targetId)) {
		logAlarmTraceThrottled(
			QStringLiteral("area_escalation_missing_track_%1_%2")
				.arg(static_cast<int>(result.domain)).arg(result.targetId),
			QStringLiteral("AreaEscalation result skipped------domain:%1 target:%2 reason:track_map_missing")
				.arg(escalationDomainName(result.domain)).arg(result.targetId),
			5000);
		return;
	}
	const SPxPacketTrackExtended& track = trackMap.value(result.targetId);
	AlarmRule rule = gConfig->m_mapAlarmRule.value(result.conditionId);
	if (rule.condition_id.isEmpty()) {
		for (const AreaEscalationBinding& binding : m_areaEscalationBindings) {
			if (binding.domain == result.domain && binding.area.key == result.eventArea) {
				rule = binding.rule;
				break;
			}
		}
	}
	if (rule.condition_id.isEmpty())
		return;
	QString content;

	const bool publishStageChange = result.stageChanged
		&& isAlarmEventStagePublishable(static_cast<int>(result.stage));
	if (publishStageChange) {
		if (result.reason == QLatin1String("archive_visit")) {
			// 知识库命中不再生成速度/航向/评分规则文案；证据由升级模块确定性组装。
			content = buildAreaEscalationContent(result);
		} else {
			const DataAccessLayer::DetectionTypeResult detection =
				cognitiveEvidenceForUniqueId(result.targetId);
			// 双样本速度锁存的第二个样本只确认速度；正文与评分应按升级模块
			// 返回的触发证据位置/航向构造，不能用第二样本覆盖入区证据。
			SPxPacketTrackExtended evidenceTrack = track;
			evidenceTrack.latDegs = result.currentPosition.x();
			evidenceTrack.longDegs = result.currentPosition.y();
			evidenceTrack.norm.min.courseDegrees = result.courseDeg;
			evidenceTrack.norm.min.speedMps = result.speedMps;
			const AreaEscalationProtectionResolver::Context protection =
				resolveProtectionContext(rule, result.domain);
			const bool hasProtectArea = protection.available;
			const QPointF protectCenter = protection.center;

			double attackAngle = evidenceTrack.norm.min.courseDegrees;
			if (hasProtectArea) {
				const double bearingToCenter = AreaEscalationProtectionResolver::bearingDegrees(
					QPointF(evidenceTrack.latDegs, evidenceTrack.longDegs), protectCenter);
				attackAngle = AreaEscalationProtectionResolver::attackAngleDegrees(
					evidenceTrack.norm.min.courseDegrees, bearingToCenter);
			}

			ThreatAssessmentParams threatParams(rule.group_id, rule.area_id);
			for (const ThreatAssessmentParams& params : gConfig->m_listThreatAssessmentParams) {
				if (params.groupId == rule.group_id && params.areaId == rule.area_id) {
					threatParams = params;
					break;
				}
			}
			const ThreatAssessmentResult assessment = calculateThreatAssessment(
				evidenceTrack, detection, threatParams, hasProtectArea, protectCenter, attackAngle);
			AlarmContentBuildInput readableInput;
			readableInput.rule = &rule;
			readableInput.cfg = gConfig;
			readableInput.uniqueId = result.targetId;
			readableInput.speedMps = evidenceTrack.norm.min.speedMps;
			readableInput.courseDeg = evidenceTrack.norm.min.courseDegrees;
			readableInput.attackAngleDeg = attackAngle;
			readableInput.heightM = evidenceTrack.altitudeMetres;
			readableInput.threatScore = result.score;
			readableInput.hasProtectArea = hasProtectArea;
			readableInput.protectionReferenceSource = protection.sourceName();
			readableInput.protectionReferenceCenter = protection.center;
			readableInput.protectionReferenceRadiusM = protection.radiusMeters;
			readableInput.isAirTrack = isAir;
			const QString trackTypeFallback =
				NewTrackStructGrpcConvert::resolveTargetTypeForScoring(QString(), evidenceTrack);
			if (trackTypeFallback == QLatin1String("ship")) {
				readableInput.fallbackTargetType = trackTypeFallback;
			}
			readableInput.hasThreatBreakdown = true;
			readableInput.threatBreakdown = assessment;
			readableInput.triggerPath = result.reason == QLatin1String("direct_entry")
				? QStringLiteral("threat_direct") : QStringLiteral("rule_match");
			readableInput.detection = detection;
			readableInput.hasDetection = detection.found;
			const QString readableRuleContent = buildRuleAlarmContent(readableInput);
			content = buildAreaEscalationContent(result, readableRuleContent);
		}

		SaveToDB(rule, result.targetId, track.latDegs, track.longDegs,
			track.norm.min.speedMps, track.norm.min.courseDegrees,
			track.norm.min.rangeMetres, isAir ? track.norm.min.reserved1 : 0, result.score,
			static_cast<int>(track.msgTimeSecs), isAir ? 9 : 0, content,
			static_cast<int>(result.stage),
			result.disposition == AreaEscalationEvaluator::Disposition::VerifySuccess ? 3 : 0,
			result.reason);
	}

	const QString nowText = QDateTime::currentDateTime().toString(
		QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
	{
		QMutexLocker locker(&gConfig->m_alarmDataMutex);
		for (auto it = gConfig->m_mapAlarmData.begin(); it != gConfig->m_mapAlarmData.end(); ++it) {
			AlarmData& alarm = it.value();
			if (alarm.condition_id != result.conditionId
				|| static_cast<qint64>(alarm.unique_id) != result.targetId
				|| alarm.alarm_status == 2) {
				continue;
			}
			alarm.event_stage = static_cast<int>(result.stage);
			alarm.track_duration = result.alarmDwellMs / 1000.0;
			alarm.alarm_environment = static_cast<int>(isAir
				? AlarmTargetEnvironment::Air : AlarmTargetEnvironment::Surface);
			alarm.task_status = result.disposition == AreaEscalationEvaluator::Disposition::VerifySuccess ? 3 : 0;
			alarm.threatScore = result.score;
			alarm.group_id = result.eventArea.groupId;
			alarm.area_id = result.eventArea.areaId;
			alarm.targetlat = track.latDegs;
			alarm.targetlon = track.longDegs;
			alarm.targetspeed = track.norm.min.speedMps;
			alarm.targetdir = track.norm.min.courseDegrees;
			alarm.time = nowText; // 仅刷新内存快照；空白区域不产生 DB UPDATE
			if (publishStageChange) {
				alarm.escalation_reason = result.reason;
				alarm.escalation_evidence = content;
				alarm.alarm_content = content;
			}
		}
	}

	if (result.stageChanged) {
		const AreaEscalationProtectionResolver::Context protection =
			resolveProtectionContext(rule, result.domain);
		const QString log = QStringLiteral(
			"AreaEscalation upgrade------domain:%1 lane:%2 target:%3 qualificationArea:%4/%5 eventArea:%6/%7 stage:%8 reason:%9 "
			"qualificationTime:%10 alarmEntryTime:%11 dwellMs:%12 previous:(%13,%14) current:(%15,%16) "
			"course:%17 speed:%18 score:%19 thresholds:%20/%21 hard:[%22] warningToAlarmMs:%23 geometry:[%24] "
			"protectReference:%25 center:(%26,%27) radiusM:%28 dbCenter:(%29,%30)")
			.arg(escalationDomainName(result.domain), result.laneId)
			.arg(result.targetId)
			.arg(result.qualificationArea.groupId).arg(result.qualificationArea.areaId)
			.arg(result.eventArea.groupId).arg(result.eventArea.areaId)
			.arg(AreaEscalationEvaluator::stageName(result.stage), result.reason)
			.arg(result.qualificationTimeMs).arg(result.alarmEntryTimeMs).arg(result.alarmDwellMs)
			.arg(result.previousPosition.x(), 0, 'f', 8).arg(result.previousPosition.y(), 0, 'f', 8)
			.arg(result.currentPosition.x(), 0, 'f', 8).arg(result.currentPosition.y(), 0, 'f', 8)
			.arg(result.courseDeg, 0, 'f', 2).arg(result.speedMps, 0, 'f', 2)
			.arg(result.score).arg(result.threatThreshold).arg(result.prewarningThreshold)
			.arg(result.hardConditions).arg(result.warningToAlarmMs).arg(result.entryGeometry)
			.arg(protection.sourceName())
			.arg(protection.center.x(), 0, 'f', 8).arg(protection.center.y(), 0, 'f', 8)
			.arg(protection.radiusMeters, 0, 'f', 1)
			.arg(protection.databaseCenter.x(), 0, 'f', 8)
			.arg(protection.databaseCenter.y(), 0, 'f', 8);
		qInfo().noquote() << log;
		AlarmFileLogger::logNewAlarmTrack(log);
	}
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
	// 一轮处理只能观察一个完整方案快照。热更新会等待本轮结束，再原子替换
	// 规则并清旧事件；下一轮必定按新 generation reset A/B 状态。
	QReadLocker alarmConfigLocker(&gConfig->m_alarmConfigLock);
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
		// 尾迹只拷贝当前在航迹表中的 key，避免整表深拷贝随残留 key 膨胀拖慢循环
		auto copyTrailForLiveTracks = [](auto& dst, const auto& src, const auto& trackMap) {
			dst.clear();
			for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
				const auto trailIt = src.constFind(it.key());
				if (trailIt != src.constEnd()) {
					dst.insert(it.key(), trailIt.value());
				}
			}
		};
		copyTrailForLiveTracks(m_mapFuseTrail, gConfig->m_mapFuseTrail, gConfig->m_mapFuseTrack);
		copyTrailForLiveTracks(m_mapRadarTrail, gConfig->m_mapRadarTrail, gConfig->m_mapRadarTrack);
		copyTrailForLiveTracks(m_mapBirdRadarTrail, gConfig->m_mapBirdFuseTrail, gConfig->m_mapBirdFuseTrack);
		m_mapAISTrail = gConfig->m_mapAISTrail;
	}
	m_cognitiveEvidenceCache.clear();
	int trailCount = 2;
	QList< AlarmRule> waringList = gConfig->m_mapAlarmRule.values();
	const bool areaEscalationActive = configureAreaEscalation(waringList);
	if (areaEscalationActive)
		processAreaEscalation();
	//qDebug() << "TrackAlarmThread: processAlarms: waringList size" << waringList.size();
	for (int i = 0; i < waringList.size(); i++)
	{
		//qDebug() << "TrackAlarmThread: processAlarms: waringList" << i;
		AlarmRule info = waringList.at(i);
		if (areaEscalationActive
			&& m_areaEscalationClaimedConditionIds.contains(info.condition_id)) {
			continue; // 新三态已统一研判，本轮不得再输出旧停留/黄色逻辑
		}
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

ThreatAssessmentResult TrackAlarmThread::calculateThreatAssessment(const SPxPacketTrackExtended& track, const DataAccessLayer::DetectionTypeResult& detectionResult, const ThreatAssessmentParams& threatParams, bool hasProtectArea, const QPointF& protectCenter, double entryAngle)
{
	ThreatAssessmentContext context;
	context.hasProtectArea = hasProtectArea;
	context.protectCenter = protectCenter;
	context.directionAngleDeg = hasProtectArea
		? entryAngle : track.norm.min.courseDegrees;
	context.useBasePoint = gConfig->m_struBasicConfig.m_nUseBasePoint == 1;
	context.basePoint = QPointF(
		gConfig->m_struBasicConfig.m_dBasePointLat,
		gConfig->m_struBasicConfig.m_dBasePointLon);
	return calculateTargetThreatAssessment(
		track, detectionResult.finalTargetType, threatParams, context).assessment;
}

double TrackAlarmThread::calculateThreatLevel(const SPxPacketTrackExtended& track, const DataAccessLayer::DetectionTypeResult& detectionResult, const ThreatAssessmentParams& threatParams, bool hasProtectArea, const QPointF& protectCenter, double entryAngle)
{
	return calculateThreatAssessment(track, detectionResult, threatParams, hasProtectArea, protectCenter, entryAngle).totalThreatLevel;
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
