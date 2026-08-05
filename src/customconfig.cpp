#include "customconfig.h"
#include "dialog/alarm/AlarmContentBuilder.h"
#include "grpc_alarm/AlarmGrpcSnapshotClient.hpp"
#include "grpc_alarm/AlarmGrpcDestroySubscriber.hpp"
#include "grpc_target_type/TargetTypeGrpcClient.hpp"
#include <QSettings>
#include <QDebug>
#include <QTextCodec>
#include <QNetworkInterface>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QDir>
#include <QProcess>
#include <QStandardPaths>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QFuture>
#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QReadLocker>
#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QDateTime>
#include <limits>
#include <cstring>
#include "dialog/alarm/AlarmFileLogger.h"
#include "fastdds_newtrack/NewTrackStructAlarmPublisherApp.hpp"
#include "fastdds_newtrack/NewTrackStructSubscriberApp.hpp"
#include "grpc_track/fusion_track_grpc_client.h"
#include "grpc_track/new_track_struct_grpc_client.h"

namespace {

/** DDS 嵌套 int32 trackId 字段兼容；主通道为 string trackId = target_id */
int legacyDdsTrackId(qint64 targetId)
{
    if (targetId <= 0) {
        return 0;
    }
    if (targetId > static_cast<qint64>(std::numeric_limits<int32_t>::max())) {
        return static_cast<int32_t>(targetId & 0x7FFFFFFF);
    }
    return static_cast<int>(targetId);
}

void parseCsvIntSet(const QString& csv, QSet<int>* out)
{
    out->clear();
    for (const QString& p : csv.split(',', QString::SkipEmptyParts)) {
        bool ok = false;
        const int v = p.trimmed().toInt(&ok);
        if (ok)
            out->insert(v);
    }
}

/** QSettings 会把逗号分隔的 ini 值读成 QStringList，直接 toString() 会得到空串 */
void parseIntSetFromSettings(QSettings& settings, const QString& key, QSet<int>* out)
{
    out->clear();
    const QVariant v = settings.value(key);
    if (v.canConvert<QStringList>()) {
        const QStringList parts = v.toStringList();
        if (!parts.isEmpty()) {
            for (const QString& p : parts) {
                bool ok = false;
                const int n = p.trimmed().toInt(&ok);
                if (ok)
                    out->insert(n);
            }
            return;
        }
    }
    const QString csv = v.toString().trimmed();
    if (!csv.isEmpty())
        parseCsvIntSet(csv, out);
}

bool trackDurationExceeded(const AlarmData& ad, int seconds)
{
    if (seconds <= 0)
        return false;
    const QString ot = ad.origintime.isEmpty() ? ad.time : ad.origintime;
    QDateTime t0 = QDateTime::fromString(ot, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    if (!t0.isValid())
        t0 = QDateTime::fromString(ot, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    if (!t0.isValid())
        return false;
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - t0.toMSecsSinceEpoch();
    return elapsed >= static_cast<qint64>(seconds) * 1000;
}

bool evalOneRowDetectionRules(const QString& jsonStr, const AlarmData& ad, DataAccessLayer& db)
{
    if (jsonStr.trimmed().isEmpty())
        return true;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    const QJsonObject o = doc.object();
    const bool needOptic = o.contains(QStringLiteral("optic")) && o.value(QStringLiteral("optic")).toBool();
    const int trackDurSec = o.contains(QStringLiteral("track_duration"))
        ? o.value(QStringLiteral("track_duration")).toInt(0) : 0;
    const bool needTrackDur = o.contains(QStringLiteral("track_duration")) && trackDurSec > 0;
    if (!needOptic && !needTrackDur)
        return true;
    bool ok = true;
    if (needOptic) {
        const QString likePattern = o.contains(QStringLiteral("file_name_like"))
            ? o.value(QStringLiteral("file_name_like")).toString()
            : QStringLiteral("%");
        ok = ok && db.hasMinioMultiMetadataForUniqueId(ad.unique_id, likePattern);
    }
    if (needTrackDur)
        ok = ok && trackDurationExceeded(ad, trackDurSec);
    return ok;
}

bool ruleHasDetectionConstraints(const QList<AlarmIdentificationRuleSub>& subs)
{
    for (const AlarmIdentificationRuleSub& s : subs) {
        if (s.enabled && !s.detection_rules_json.trimmed().isEmpty())
            return true;
    }
    return false;
}

bool evalSingleRuleCriteriaIntersection(const QList<AlarmIdentificationRuleSub>& subs, const AlarmData& ad, DataAccessLayer& db)
{
    QMap<QString, QList<AlarmIdentificationRuleSub>> byCriteria;
    for (const AlarmIdentificationRuleSub& s : subs) {
        if (!s.enabled)
            continue;
        const QString key = s.criteria_id.trimmed().isEmpty()
            ? QStringLiteral("__default__")
            : s.criteria_id.trimmed();
        byCriteria[key].append(s);
    }

    for (auto it = byCriteria.begin(); it != byCriteria.end(); ++it) {
        bool groupOk = true;
        const QList<AlarmIdentificationRuleSub>& groupRows = it.value();
        for (const AlarmIdentificationRuleSub& row : groupRows) {
            if (!evalOneRowDetectionRules(row.detection_rules_json, ad, db)) {
                groupOk = false;
                break;
            }
        }
        if (groupOk) {
            return true;
        }
    }
    return false;
}

bool evalIdentificationRulesUnion(
    const QMap<QString, QStringList>& areaToRuleIds,
    const QMap<QString, QList<AlarmIdentificationRuleSub>>& mapSubs,
    const AlarmData& ad,
    DataAccessLayer& db,
    bool& outAnyRuleHadConstraints)
{
    outAnyRuleHadConstraints = false;
    const QString areaKey = QString::number(ad.group_id) + QLatin1Char('_') + QString::number(ad.area_id);
    QStringList ruleIds = areaToRuleIds.value(areaKey);
    if (!ruleIds.contains(ad.condition_id))
        ruleIds.append(ad.condition_id);

    for (const QString& rid : ruleIds) {
        const QList<AlarmIdentificationRuleSub> subs = mapSubs.value(rid);
        if (!ruleHasDetectionConstraints(subs))
            continue;
        outAnyRuleHadConstraints = true;
        if (evalSingleRuleCriteriaIntersection(subs, ad, db))
            return true;
    }
    return false;
}

struct UniqueTrackHit {
    SPxPacketTrackExtended track;
    bool isAirTrack = false;
    bool found = false;
};

UniqueTrackHit findTrackByUniqueId(CustomConfig* cfg, qint64 uniqueId)
{
    UniqueTrackHit hit;
    if (!cfg || uniqueId <= 0)
        return hit;

    QReadLocker rl(&cfg->m_trackDataLock);
    const auto tryMapQint64 = [&](const QMap<qint64, SPxPacketTrackExtended>& trackMap, bool isAir) {
        if (hit.found)
            return;
        auto it = trackMap.constFind(uniqueId);
        if (it != trackMap.constEnd()) {
            hit.track = it.value();
            hit.isAirTrack = isAir;
            hit.found = true;
        }
    };
    const auto tryMapInt = [&](const QMap<int, SPxPacketTrackExtended>& trackMap, bool isAir) {
        if (hit.found || uniqueId > static_cast<qint64>(std::numeric_limits<int>::max()))
            return;
        auto it = trackMap.constFind(static_cast<int>(uniqueId));
        if (it != trackMap.constEnd()) {
            hit.track = it.value();
            hit.isAirTrack = isAir;
            hit.found = true;
        }
    };
    // map key 通常=target_id；若 key 与 secondary.uniqueID 不一致，再按 uniqueID 扫一遍
    const auto scanBySecondaryUniqueId = [&](const QMap<qint64, SPxPacketTrackExtended>& trackMap, bool isAir) {
        if (hit.found)
            return;
        for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
            if (static_cast<qint64>(it.value().secondary.uniqueID) == uniqueId) {
                hit.track = it.value();
                hit.isAirTrack = isAir;
                hit.found = true;
                return;
            }
        }
    };
    const auto scanBySecondaryUniqueIdInt = [&](const QMap<int, SPxPacketTrackExtended>& trackMap, bool isAir) {
        if (hit.found || uniqueId > static_cast<qint64>(std::numeric_limits<int>::max()))
            return;
        for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
            if (static_cast<qint64>(it.value().secondary.uniqueID) == uniqueId) {
                hit.track = it.value();
                hit.isAirTrack = isAir;
                hit.found = true;
                return;
            }
        }
    };
    tryMapQint64(cfg->m_mapBirdFuseTrack, true);
    tryMapInt(cfg->m_mapBirdRadarTrack, true);
    tryMapQint64(cfg->m_mapFuseTrack, false);
    tryMapInt(cfg->m_mapRadarTrack, false);
    scanBySecondaryUniqueId(cfg->m_mapBirdFuseTrack, true);
    scanBySecondaryUniqueIdInt(cfg->m_mapBirdRadarTrack, true);
    scanBySecondaryUniqueId(cfg->m_mapFuseTrack, false);
    scanBySecondaryUniqueIdInt(cfg->m_mapRadarTrack, false);
    return hit;
}

qint64 effectiveUniqueIdFromTrackHit(const UniqueTrackHit& hit, qint64 requestUniqueId)
{
    if (!hit.found)
        return requestUniqueId;
    const qint64 uid = static_cast<qint64>(hit.track.secondary.uniqueID);
    return uid > 0 ? uid : requestUniqueId;
}

AlarmData buildManualAlarmDataFromTrack(
    const SPxPacketTrackExtended& track, qint64 uniqueId, int defaultThreatScore, const AlarmData* existing)
{
    AlarmData ad;
    ad.alarm_id = QStringLiteral("manual-") + QString::number(uniqueId);
    ad.condition_id = QStringLiteral("manual_confirm");
    ad.mode_id = 1;
    ad.track_id = 0;
    ad.unique_id = uniqueId;
    ad.targetlat = track.latDegs;
    ad.targetlon = track.longDegs;
    ad.targetspeed = track.norm.min.speedMps;
    ad.targetdir = track.norm.min.courseDegrees;
    ad.targetdist = track.norm.min.rangeMetres / 1852.0;
    // reserved3 在融合航迹上是威胁分映射，不是目标类型；人工告警不从 reserved3 取 type/score
    ad.targettype = existing ? existing->targettype : 0;
    ad.targetbehavior = 0;
    ad.time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    ad.origintime = (existing && !existing->origintime.isEmpty())
        ? existing->origintime
        : ad.time;
    ad.alarm_status = 0;
    ad.task_status = 0;
    ad.alarm_count = existing ? existing->alarm_count + 1 : 1;
    ad.threatScore = qMax(
        existing ? existing->threatScore : 0,
        defaultThreatScore > 0 ? defaultThreatScore : 90);
    ad.threat_time_ms = existing && existing->threat_time_ms > 0
        ? existing->threat_time_ms
        : QDateTime::currentMSecsSinceEpoch();
    ad.group_id = 0;
    ad.area_id = 0;
    return ad;
}

void syncManualAlarmsBeforePublish(CustomConfig* cfg)
{
    if (!cfg)
        return;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const int defaultThreat = cfg->m_alarmLogic.defaultThreatScore;

    for (auto it = cfg->m_mapManualAlarmData.begin(); it != cfg->m_mapManualAlarmData.end(); ) {
        const qint64 uniqueId = static_cast<qint64>(it.value().unique_id);
        const UniqueTrackHit hit = findTrackByUniqueId(cfg, uniqueId);
        if (!hit.found) {
            const qint64 alarmMs = QDateTime::fromString(
                it.value().time, QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")).toMSecsSinceEpoch();
            if (alarmMs > 0 && nowMs - alarmMs > 600 * 1000) {
                cfg->m_mapManualAlarmAirTrack.remove(uniqueId);
                it = cfg->m_mapManualAlarmData.erase(it);
            } else {
                ++it;
            }
            continue;
        }

        const AlarmData prev = it.value();
        AlarmData updated = buildManualAlarmDataFromTrack(hit.track, uniqueId, defaultThreat, &prev);
        updated.alarm_count = prev.alarm_count;
        updated.origintime = prev.origintime;
        updated.threat_time_ms = prev.threat_time_ms;
        updated.alarm_content = buildManualAlarmContent(
            cfg,
            uniqueId,
            hit.isAirTrack,
            updated.targetspeed,
            updated.targetdir);
        it.value() = updated;
        cfg->m_mapManualAlarmAirTrack.insert(uniqueId, hit.isAirTrack);
        ++it;
    }
}

bool isManualAlarmId(const QString& alarmId)
{
    return alarmId.startsWith(QStringLiteral("manual-"));
}

} // namespace

quint32 CustomConfig::radarDataSourceCode(const SPxPacketTrackExtended& track)
{
    return track.secondary.reserved[0];
}

void CustomConfig::setRadarDataSourceCode(SPxPacketTrackExtended& track, quint32 sourceCode)
{
    track.secondary.reserved[0] = sourceCode;
}

bool CustomConfig::hasRadarTrack(int id, quint32 sourceCode) const
{
    SPxPacketTrackExtended unused;
    return tryGetRadarTrack(id, &unused, sourceCode);
}

bool CustomConfig::tryGetRadarTrack(int id, SPxPacketTrackExtended* out, quint32 sourceCode) const
{
    if (id <= 0 || out == nullptr) {
        return false;
    }
    QReadLocker rl(&m_trackDataLock);
    const auto matchSource = [sourceCode](const SPxPacketTrackExtended& t) {
        return sourceCode == 0 || radarDataSourceCode(t) == sourceCode;
    };
    auto it = m_mapRadarTrack.constFind(id);
    if (it != m_mapRadarTrack.cend() && matchSource(it.value())) {
        *out = it.value();
        return true;
    }
    const auto uid = static_cast<uint32_t>(id);
    const auto localId = static_cast<uint32_t>(id);
    for (auto jt = m_mapRadarTrack.constBegin(); jt != m_mapRadarTrack.constEnd(); ++jt) {
        if (!matchSource(jt.value())) {
            continue;
        }
        if (jt.value().secondary.uniqueID == uid || jt.value().norm.min.id == localId) {
            *out = jt.value();
            return true;
        }
    }
    return false;
}

SPxPacketTrackExtended CustomConfig::radarTrackValue(int id, quint32 sourceCode) const
{
    SPxPacketTrackExtended track;
    std::memset(&track, 0, sizeof(track));
    tryGetRadarTrack(id, &track, sourceCode);
    return track;
}

bool CustomConfig::tryGetRadarTrackBySourceId(int id, const QString& dataSourceId, SPxPacketTrackExtended* out) const
{
    const quint32 code = m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.value(dataSourceId.trimmed(), 0);
    return tryGetRadarTrack(id, out, code);
}

CustomConfig* CustomConfig::instance = nullptr;
QMutex CustomConfig::mutex;

CustomConfig::CustomConfig()
{
    m_alarmGrpcClient = std::make_unique<AlarmGrpcSnapshotClient>();
    m_alarmDestroyGrpcSubscriber = std::make_unique<AlarmGrpcDestroySubscriber>();
    m_targetTypeGrpcClient = std::make_unique<TargetTypeGrpcClient>();
    LoadConfig();

    m_dbInitSuccess = dbHelper.initDatabase();

    m_listTargetInfo = dbHelper.getAllTargetInfo();
    m_mapAlarmRule =  dbHelper.getAlarmRule();
    m_mapAlarmIdentificationRulesSub = dbHelper.getAlarmIdentificationRulesSub();
    m_mapAreaKeyToIdentificationRuleIds = dbHelper.getIdentificationRuleIdsByAreaForActiveScheme();
    m_mapAreaKeyToVerificationRuleIds = dbHelper.getVerificationRuleIdsByAreaForActiveScheme();
    m_mapAreaKeyToHandleRules = dbHelper.getAreaHandleRulesByAreaForActiveScheme();
    m_alarmArea = dbHelper.getAreaInfo();
    m_mapTargetInfoFilter = dbHelper.getTargetInfoFilter();
    m_listThreatAssessmentParams = dbHelper.getThreatAssessmentParams();
    m_mapSchemeProtectAreas = dbHelper.getActiveSchemeProtectAreas();

    m_gNetworkManager = new QNetworkAccessManager;
    m_gNetworkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
}


CustomConfig::~CustomConfig()
{
    stopGrpcSnapshotTimer();
    stopAlarmDestroyGrpcSubscriber();
}

void CustomConfig::startGrpcSnapshotTimer()
{
    if (m_grpcSnapshotThread != nullptr) {
        return;
    }
    if (!m_alarmGrpcClient || !m_alarmGrpcClient->isEnabled()) {
        qInfo() << "GrpcSnapshotTimer: gRPC 告警快照未启用，跳过独立线程启动";
        return;
    }

    m_grpcSnapshotThread = new QThread(this);
    QTimer* timer = new QTimer();   // 无 parent，随后移入线程
    timer->setInterval(1000);
    timer->moveToThread(m_grpcSnapshotThread);

    // 线程启动后立即开始计时
    QObject::connect(m_grpcSnapshotThread, &QThread::started, timer,
                     [timer]() { timer->start(); });

    // 每秒触发：在 m_grpcSnapshotThread 线程中执行（DirectConnection）
    // tickAlarmGrpcSnapshot 内部已持有 m_alarmDataMutex，线程安全
    QObject::connect(timer, &QTimer::timeout, this,
                     [this]() { tickAlarmGrpcSnapshot(); },
                     Qt::DirectConnection);

    // 线程结束时清理 timer
    QObject::connect(m_grpcSnapshotThread, &QThread::finished,
                     timer, &QObject::deleteLater);

    m_grpcSnapshotThread->setObjectName(QStringLiteral("GrpcSnapshot"));
    m_grpcSnapshotThread->start();
    qInfo() << "GrpcSnapshotTimer: 独立推送线程已启动（与 processAlarms 解耦）";
}

void CustomConfig::stopGrpcSnapshotTimer()
{
    if (m_grpcSnapshotThread == nullptr) {
        return;
    }
    m_grpcSnapshotThread->quit();
    if (!m_grpcSnapshotThread->wait(3000)) {
        qWarning() << "GrpcSnapshotTimer: 线程未在 3s 内退出，强制终止";
        m_grpcSnapshotThread->terminate();
        m_grpcSnapshotThread->wait(1000);
    }
    m_grpcSnapshotThread = nullptr;
    qInfo() << "GrpcSnapshotTimer: 独立推送线程已停止";
}

void CustomConfig::CleanupDDSResources(int domainId)
{
    qDebug() << "正在清理DDS资源，域ID:" << domainId;
    
#ifdef Q_OS_WIN
    // Windows平台下清理共享内存和信号量
    try {
        // 清理共享内存文件
        QString sharedMemPattern = QString("fastrtps_*_%1_*").arg(domainId);
        QStringList filters;
        filters << sharedMemPattern;
        
        // 查找临时目录中的共享内存文件
        QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        QStringList files = tempDir.entryList(filters, QDir::Files);
        
        foreach(QString file, files) {
            QString filePath = tempDir.absoluteFilePath(file);
            QFile::remove(filePath);
            qDebug() << "已删除共享内存文件:" << filePath;
        }
        
        // 使用Windows API清理信号量
        QProcess process;
        QString command = "powershell -Command \"Get-Process | Where-Object {$_.Modules.ModuleName -like '*fastrtps*'} | Select-Object -Property Id\"";
        process.start(command);
        process.waitForFinished();
        QString output = process.readAllStandardOutput();
        
        if (!output.isEmpty()) {
            qDebug() << "发现可能的FastDDS进程:" << output;
        }
    } catch (std::exception& e) {
        qDebug() << "清理DDS资源时发生异常:" << e.what();
    }
#else
    // Linux平台下清理共享内存和信号量
    try {
        // 使用ipcs和ipcrm命令清理共享内存和信号量
        QProcess process;
        process.start("ipcs -m");
        process.waitForFinished();
        QString output = process.readAllStandardOutput();
        
        QStringList lines = output.split("\n");
        foreach(QString line, lines) {
            if (line.contains(QString("fastrtps_%1").arg(domainId))) {
                QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                if (parts.size() > 1) {
                    QString shmId = parts[1];
                    QProcess::execute(QString("ipcrm -m %1").arg(shmId));
                    qDebug() << "已删除共享内存ID:" << shmId;
                }
            }
        }
        
        // 清理信号量
        process.start("ipcs -s");
        process.waitForFinished();
        output = process.readAllStandardOutput();
        
        lines = output.split("\n");
        foreach(QString line, lines) {
            if (line.contains(QString("fastrtps_%1").arg(domainId))) {
                QStringList parts = line.split(QRegExp("\\s+"), QString::SkipEmptyParts);
                if (parts.size() > 1) {
                    QString semId = parts[1];
                    QProcess::execute(QString("ipcrm -s %1").arg(semId));
                    qDebug() << "已删除信号量ID:" << semId;
                }
            }
        }
    } catch (std::exception& e) {
        qDebug() << "清理DDS资源时发生异常:" << e.what();
    }
#endif

    // 短暂延迟，确保资源被完全释放
    QThread::msleep(500);
    qDebug() << "DDS资源清理完成";
}

void CustomConfig::InitFastdds()
{
    // 确保域ID不超过250
    if (m_struBasicConfig.m_nPPIDDSPort > 250) {
        qDebug() << "配置的域ID超过最大值250，将使用默认值0";
        m_struBasicConfig.m_nPPIDDSPort = 0;
        SaveConfig();
    }
    
    // 先清理可能存在的DDS资源
    //CleanupDDSResources(m_struBasicConfig.m_nPPIDDSPort);
    //CleanupDDSResources(m_struBasicConfig.m_nAlarmEventDDSPort);

    int alarmeventDDSPort = m_struBasicConfig.m_nAlarmEventDDSPort;
    int alarmeventDDSPortSingle = m_struBasicConfig.m_nAlarmEventDDSPortSingle;
    int trackDDSPort = m_struBasicConfig.m_nTrackDDSPort;

    CleanupDDSResources(alarmeventDDSPort);
    CleanupDDSResources(alarmeventDDSPortSingle);
    CleanupDDSResources(trackDDSPort);

    //初始化alarmevent发送者（可由 DDS/PublishAlarmEventDds=0 关闭）
    if (m_bPublishAlarmEventDds) {
        try {
            m_alarmevnetPublisherMulti = new AlarmEventPublisherApp(alarmeventDDSPort,0);
            std::thread alarmThread(&AlarmEventApplication::run, m_alarmevnetPublisherMulti);
            alarmThread.detach();

            m_alarmevnetPublisherSingle = new AlarmEventPublisherApp(alarmeventDDSPortSingle, 1);
            std::thread alarmThreadSingle(&AlarmEventApplication::run, m_alarmevnetPublisherSingle);
            alarmThreadSingle.detach();
        }
        catch (std::exception& e) {
            qDebug() << "AlarmEvent 发布者初始化失败:" << e.what();
            m_alarmevnetPublisherMulti = nullptr;
            m_alarmevnetPublisherSingle = nullptr;
        }
    } else {
        m_alarmevnetPublisherMulti = nullptr;
        m_alarmevnetPublisherSingle = nullptr;
        qInfo() << "AlarmEvent DDS 发布已关闭（PublishAlarmEventDds=0），告警走 gRPC UpdateAlarmSnapshot";
    }

    // 航迹订阅：FusionTrackTransport / RadarTrackTransport = dds|grpc
    const int trackEnableOldSubscriber =
        QSettings(QStringLiteral("Config.ini"), QSettings::IniFormat)
            .value(QStringLiteral("DDS/TrackEnableOldSubscriber"), 1)
            .toInt();
    const QString fusionTx = m_struBasicConfig.m_strFusionTrackTransport.trimmed().toLower();
    const QString radarTx = m_struBasicConfig.m_strRadarTrackTransport.trimmed().toLower();
    const bool fusionUseGrpc = (fusionTx == QStringLiteral("grpc"));
    const bool radarUseGrpc = (radarTx == QStringLiteral("grpc"));

    // 旧 TrackDataClass：仅在雷达仍走 DDS 且显式开启时启动（避免与 FusionTrack gRPC 双点）
    if (trackEnableOldSubscriber != 0 && !radarUseGrpc) {
        try {
            m_trackSubscriber = new TrackClassSubscriberApp(trackDDSPort);
            std::thread trackThread(&TrackClassApplication::run, m_trackSubscriber);
            trackThread.detach();
            qDebug() << "TrackClassSubscriber 已启用 domain=" << trackDDSPort;
        }
        catch (std::exception& e) {
            qWarning() << "TrackClassSubscriber 初始化失败:" << e.what();
            m_trackSubscriber = nullptr;
        }
    } else if (trackEnableOldSubscriber != 0 && radarUseGrpc) {
        qInfo() << "TrackClassSubscriber 已跳过（RadarTrackTransport=grpc）";
    } else {
        qDebug() << "TrackClassSubscriber 已禁用";
    }

    // 融合：gRPC 或 DDS（告警 NewTrackStruct 发布者始终走 DDS）
    if (fusionUseGrpc) {
        try {
            m_newTrackStructGrpcClient =
                new NewTrackStructGrpcClient(m_struBasicConfig.m_strNewTrackStructGrpcAddr);
            m_newTrackStructGrpcClient->start();
            qInfo() << "融合航迹通道 FusionTrackTransport=grpc →"
                    << m_struBasicConfig.m_strNewTrackStructGrpcAddr;
        } catch (std::exception& e) {
            qWarning() << "NewTrackStructGrpcClient 启动失败:" << e.what();
            m_newTrackStructGrpcClient = nullptr;
        }
    } else {
        try {
            m_newTrackStructSubscriber = new NewTrackStructSubscriberApp(trackDDSPort);
            qInfo() << "融合航迹通道 FusionTrackTransport=dds domain=" << trackDDSPort;
        } catch (std::exception& e) {
            qWarning() << "NewTrackStructSubscriber 初始化失败:" << e.what();
            m_newTrackStructSubscriber = nullptr;
        }
    }

    if (m_bPublishAlarmEventDds) {
        try {
            m_newTrackStructAlarmPublisherMulti = new NewTrackStructAlarmPublisherApp(trackDDSPort, 0);
            m_newTrackStructAlarmPublisherSingle = new NewTrackStructAlarmPublisherApp(trackDDSPort, 1);
        }
        catch (std::exception& e) {
            qDebug() << "NewTrackStruct 告警发布者初始化失败:" << e.what();
            m_newTrackStructAlarmPublisherMulti = nullptr;
            m_newTrackStructAlarmPublisherSingle = nullptr;
        }
    } else {
        m_newTrackStructAlarmPublisherMulti = nullptr;
        m_newTrackStructAlarmPublisherSingle = nullptr;
        qInfo() << "NewTrackStructAlarm DDS 发布已关闭（随 PublishAlarmEventDds=0）";
    }

    if (radarUseGrpc) {
        try {
            m_fusionTrackGrpcClient =
                new FusionTrackGrpcClient(m_struBasicConfig.m_strFusionTrackStreamGrpcAddr);
            m_fusionTrackGrpcClient->start();
            qInfo() << "雷达航迹通道 RadarTrackTransport=grpc →"
                    << m_struBasicConfig.m_strFusionTrackStreamGrpcAddr
                    << "routes" << m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.keys()
                    << "radarCodes" << m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.keys();
        } catch (std::exception& e) {
            qWarning() << "FusionTrackGrpcClient 启动失败:" << e.what();
            m_fusionTrackGrpcClient = nullptr;
        }
    } else {
        qInfo() << "雷达航迹通道 RadarTrackTransport=dds"
                << (trackEnableOldSubscriber != 0 && !radarUseGrpc
                        ? "(TrackClassSubscriber)"
                        : "(需 DDS/TrackEnableOldSubscriber=1)");
    }

    if (m_suspiciousTarget.enabled && m_suspiciousTarget.publishDds) {
        try {
            m_suspiciousTargetPublisherMulti = new NewTrackStructAlarmPublisherApp(
                trackDDSPort, 0, m_suspiciousTarget.ddsTopic);
            m_suspiciousTargetPublisherSingle = new NewTrackStructAlarmPublisherApp(
                trackDDSPort, 1, m_suspiciousTarget.ddsTopic);
            qInfo() << "SuspiciousTarget DDS 发布已启用 topic=" << m_suspiciousTarget.ddsTopic
                    << " domain=" << trackDDSPort;
        }
        catch (std::exception& e) {
            qDebug() << "SuspiciousTarget DDS 发布者初始化失败:" << e.what();
            m_suspiciousTargetPublisherMulti = nullptr;
            m_suspiciousTargetPublisherSingle = nullptr;
        }
    } else if (m_suspiciousTarget.enabled) {
        qInfo() << "SuspiciousTarget DDS 已关闭（PublishDds=0），embedInTarget="
                << m_suspiciousTarget.embedInTarget
                << " ruleId=" << m_suspiciousTarget.embedRuleId;
    }

    //try{
    //    m_recognitionPublisher = new FastddsRecognitionPublisherApp(m_struBasicConfig.m_nRecognitionDDSPort);
    //    std::thread recognitionThread(&FastddsRecognitionApplication::run, m_recognitionPublisher);
    //    recognitionThread.detach();
    //}
    //catch (std::exception& e) {
    //    qDebug() << "识别信息发布者初始化失败:" << e.what();    
    //}

    if (m_bPublishAlarmEventDds) {
        m_bFastDDSInitialized = (m_alarmevnetPublisherMulti != nullptr);
        if (!m_bFastDDSInitialized) {
            qDebug() << "FastDDS 告警发布未就绪：AlarmEvent DataWriter 未创建，前端将收不到 DDS 告警";
        } else {
            qDebug() << "FastDDS 初始化完成，AlarmEvent domain=" << alarmeventDDSPort
                     << " single=" << alarmeventDDSPortSingle;
        }
    } else {
        // 告警 DDS 关闭时仍视为「初始化完成」，其它航迹/gRPC 通道独立；告警发送侧自行短路
        m_bFastDDSInitialized = true;
        qInfo() << "FastDDS 航迹侧初始化完成；AlarmEvent DDS 未启用（PublishAlarmEventDds=0）";
    }

    emit sig_initfastddsFinished();
}

void CustomConfig::DestoryFastdds()
{
    qDebug() << "正在销毁FastDDS资源...";
    
    // 保存当前使用的域ID，用于后续清理
    int currentDomainId = m_struBasicConfig.m_nPPIDDSPort;

    if (m_alarmevnetPublisherMulti != nullptr)
    {
        try {
            delete m_alarmevnetPublisherMulti;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁外部发布者时发生异常:" << e.what();
        }
        m_alarmevnetPublisherMulti = nullptr;
    }

    if (m_alarmevnetPublisherSingle != nullptr)
    {
        try {
            delete m_alarmevnetPublisherSingle;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁外部发布者时发生异常:" << e.what();
        }
        m_alarmevnetPublisherSingle = nullptr;
    }

    if (m_newTrackStructAlarmPublisherMulti != nullptr)
    {
        try {
            delete m_newTrackStructAlarmPublisherMulti;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁NewTrackStruct组播发布者异常:" << e.what();
        }
        m_newTrackStructAlarmPublisherMulti = nullptr;
    }

    if (m_newTrackStructAlarmPublisherSingle != nullptr)
    {
        try {
            delete m_newTrackStructAlarmPublisherSingle;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁NewTrackStruct单播发布者异常:" << e.what();
        }
        m_newTrackStructAlarmPublisherSingle = nullptr;
    }

    if (m_suspiciousTargetPublisherMulti != nullptr)
    {
        try {
            delete m_suspiciousTargetPublisherMulti;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁SuspiciousTarget组播发布者异常:" << e.what();
        }
        m_suspiciousTargetPublisherMulti = nullptr;
    }

    if (m_suspiciousTargetPublisherSingle != nullptr)
    {
        try {
            delete m_suspiciousTargetPublisherSingle;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁SuspiciousTarget单播发布者异常:" << e.what();
        }
        m_suspiciousTargetPublisherSingle = nullptr;
    }

    if (m_trackSubscriber != nullptr)
    {
        try {
            m_trackSubscriber->stop();
            delete m_trackSubscriber;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁TrackClass订阅者异常:" << e.what();
        }
        m_trackSubscriber = nullptr;
    }

    if (m_newTrackStructSubscriber != nullptr)
    {
        try {
            m_newTrackStructSubscriber->stop();
            delete m_newTrackStructSubscriber;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁NewTrackStruct订阅者异常:" << e.what();
        }
        m_newTrackStructSubscriber = nullptr;
    }

    if (m_newTrackStructGrpcClient != nullptr)
    {
        try {
            m_newTrackStructGrpcClient->stop();
            delete m_newTrackStructGrpcClient;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁 NewTrackStructGrpcClient 异常:" << e.what();
        }
        m_newTrackStructGrpcClient = nullptr;
    }

    if (m_fusionTrackGrpcClient != nullptr)
    {
        try {
            m_fusionTrackGrpcClient->stop();
            delete m_fusionTrackGrpcClient;
        }
        catch (const std::exception& e) {
            qDebug() << "销毁 FusionTrackGrpcClient 异常:" << e.what();
        }
        m_fusionTrackGrpcClient = nullptr;
    }

    //if (m_fastddsPublisher != nullptr)
    //{
    //    try {
    //        delete m_fastddsPublisher;
    //    }
    //    catch (const std::exception& e) {
    //        qDebug() << "销毁外部发布者时发生异常:" << e.what();
    //    }
    //    m_fastddsPublisher = nullptr;
    //}
    
    // 清理可能残留的 DDS 共享内存/信号量（多域与 Windows 版对齐）
    CleanupDDSResources(currentDomainId);
    CleanupDDSResources(m_struBasicConfig.m_nAlarmEventDDSPort);
    CleanupDDSResources(m_struBasicConfig.m_nAlarmEventDDSPortSingle);
    CleanupDDSResources(m_struBasicConfig.m_nTrackDDSPort);
    // 重置初始化标志
    m_bFastDDSInitialized = false;
    
    qDebug() << "FastDDS资源销毁完成";
}

void CustomConfig::reloadAlarmConfigFromDb(const QString& scope)
{
    if (!m_dbInitSuccess && !dbHelper.initDatabase()) {
        qWarning() << "reloadAlarmConfigFromDb: 数据库未就绪";
        return;
    }
    const QString s = scope.trimmed().isEmpty() ? QStringLiteral("all") : scope.trimmed();
    const bool all = (s == QLatin1String("all"));
    if (all || s == QLatin1String("alarm_rules")) {
        // 数据库读取放在锁外；完整快照在一个写锁临界区内一次性发布。
        const QMap<QString, AlarmRule> nextAlarmRules = dbHelper.getAlarmRule();
        const QList<AreaInfo> nextAlarmAreas = dbHelper.getAreaInfo();
        const QMap<QString, QPair<int, int>> nextProtectAreas =
            dbHelper.getActiveSchemeProtectAreas();
        int clearedAlarmCount = 0;
        {
            QWriteLocker configLocker(&m_alarmConfigLock);
            m_mapAlarmRule = nextAlarmRules;
            m_alarmArea = nextAlarmAreas;
            m_mapSchemeProtectAreas = nextProtectAreas;
            // 方案切换/规则重载是事件生命周期边界：清除旧方案内存快照，
            // 避免演示方案与日常方案往返时继续上报旧 condition_id。
            QMutexLocker locker(&m_alarmDataMutex);
            clearedAlarmCount = m_mapAlarmData.size();
            m_mapAlarmData.clear();
            ++m_alarmConfigGeneration;
        }
        qInfo() << "热更新 alarm_rules: alarm_setting" << m_mapAlarmRule.size()
                << "area" << m_alarmArea.size()
                << "cleared_active_events" << clearedAlarmCount;
    }
    if (all || s == QLatin1String("threat_params")) {
        m_listThreatAssessmentParams = dbHelper.getThreatAssessmentParams();
        qInfo() << "热更新 threat_params:" << m_listThreatAssessmentParams.size();
    }
    if (all || s == QLatin1String("identification")) {
        m_mapAlarmIdentificationRulesSub = dbHelper.getAlarmIdentificationRulesSub();
        m_mapAreaKeyToIdentificationRuleIds = dbHelper.getIdentificationRuleIdsByAreaForActiveScheme();
        qInfo() << "热更新 identification: rules" << m_mapAlarmIdentificationRulesSub.size()
                << "area_bindings" << m_mapAreaKeyToIdentificationRuleIds.size();
    }
    if (all || s == QLatin1String("verification")) {
        m_mapAreaKeyToVerificationRuleIds = dbHelper.getVerificationRuleIdsByAreaForActiveScheme();
        qInfo() << "热更新 verification: area_bindings" << m_mapAreaKeyToVerificationRuleIds.size();
    }
    if (all || s == QLatin1String("handle_rules")) {
        m_mapAreaKeyToHandleRules = dbHelper.getAreaHandleRulesByAreaForActiveScheme();
        qInfo() << "热更新 handle_rules: area_bindings" << m_mapAreaKeyToHandleRules.size();
    }
}

ThreatAssessmentParams CustomConfig::getThreatAssessmentParams(int groupId, int areaId)
{
    // 从内存中查找对应的威胁度参数
    for (const ThreatAssessmentParams& params : m_listThreatAssessmentParams) {
        if (params.groupId == groupId && params.areaId == areaId) {
            return params;
        }
    }
    
    // 如果内存中没有找到，从数据库实时查询
    ThreatAssessmentParams params = dbHelper.getThreatAssessmentParams(groupId, areaId);
    
    // 如果数据库中找到了，添加到内存中以便下次快速访问
    if (params.groupId == groupId && params.areaId == areaId) {
        m_listThreatAssessmentParams.append(params);
    }
    
    return params;
}

void CustomConfig::SendMsg(QString msg)
{
    Q_UNUSED(msg);
    qDebug() << "SendMsg: no-op (FastddsMsgPublisherApp not linked in Linux headless build; see customconfig.h)";
}

void CustomConfig::SendNewAlarm(QString alarmid)
{
    if (!m_bPublishAlarmEventDds) {
        return;
    }
    if (!m_bFastDDSInitialized || m_alarmevnetPublisherMulti == nullptr) {
        qDebug() << "FastDDS未初始化或告警事件发布者为空，无法发送告警消息";
        return;
    }

    try {
        // 创建AlarmEvent结构体
        AlarmEvent alarmEvent;

        // 创建告警信息
        AlarmInfo alarmInfo;
        QString strDate = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        alarmEvent.eventId("newAlarm" + strDate.toStdString());
        alarmInfo.alarmId(alarmid.toStdString());
        // 将告警信息添加到告警事件中
        std::vector<AlarmInfo> alarmInfoList = alarmEvent.alarm();
        alarmInfoList.push_back(alarmInfo);
        alarmEvent.alarm(alarmInfoList);
        // 发布告警事件
        bool r = m_alarmevnetPublisherMulti->sendNewAlarm(alarmEvent);
        //qDebug() << "成功发送" << m_mapAlarmData.size() << "条告警数据";

    }
    catch (const std::exception& e) {
        qDebug() << "发送告警消息时发生异常:" << e.what();
    }
    catch (...) {
        qDebug() << "发送告警消息时发生未知异常";
    }
}

void CustomConfig::setTargetType(const QString& id, int type)
{
    QMutexLocker locker(&m_alarmDataMutex);
    m_mapTargetType[id] = type;
    qDebug() << "设置目标类型: id=" << id << ", type=" << type;
}

void CustomConfig::SendNewTrackStructAlarmMsg(const AlarmEvent* alarmEvent)
{
    if (!m_bPublishAlarmEventDds) {
        return;
    }
    if (!m_bFastDDSInitialized) {
        return;
    }
    if (m_newTrackStructSubscriber == nullptr && m_newTrackStructGrpcClient == nullptr) {
        return;
    }

    if (m_newTrackStructAlarmPublisherMulti == nullptr && m_newTrackStructAlarmPublisherSingle == nullptr) {
        return;
    }

    if (alarmEvent == nullptr || alarmEvent->alarm().empty()) {
        return;
    }

    // 用告警唯一 ID 过滤，避免对空/对海 trackId 重号导致串告警。
    QSet<QString> alarmUniqueIdStrs;
    for (const AlarmInfo& ai : alarmEvent->alarm()) {
        const QString alarmId = QString::fromStdString(ai.alarmId()).trimmed();
        if (alarmId.isEmpty()) {
            continue;
        }
        const auto it = m_mapAlarmData.constFind(alarmId);
        if (it != m_mapAlarmData.constEnd() && it.value().unique_id > 0) {
            alarmUniqueIdStrs.insert(QString::number(it.value().unique_id));
            continue;
        }
        if (isManualAlarmId(alarmId)) {
            const auto manualIt = m_mapManualAlarmData.constFind(alarmId);
            if (manualIt != m_mapManualAlarmData.constEnd() && manualIt.value().unique_id > 0) {
                alarmUniqueIdStrs.insert(QString::number(manualIt.value().unique_id));
            }
        }
    }
    if (alarmUniqueIdStrs.isEmpty()) {
        return;
    }

    TargetFull::TargetOutputSet merged;
    const bool gotMerged =
        (m_newTrackStructSubscriber && m_newTrackStructSubscriber->build_merged_latest(merged))
        || (m_newTrackStructGrpcClient && m_newTrackStructGrpcClient->build_merged_latest(merged));
    if (!gotMerged) {
        return;
    }

    std::vector<TargetFull::TargetObject> filteredTargets;
    filteredTargets.reserve(merged.targets().size());
    for (const auto& target : merged.targets()) {
        const QString targetId = QString::fromStdString(target.target_id()).trimmed();
        const bool keep = !targetId.isEmpty() && alarmUniqueIdStrs.contains(targetId);
        if (keep) {
            filteredTargets.push_back(target);
        }
    }

    TargetFull::TargetOutputSet filtered;
    filtered.targets(std::move(filteredTargets));
    filtered.groups(std::vector<TargetFull::GroupObject>());
    merged = std::move(filtered);

    if (merged.targets().empty() && merged.groups().empty()) {
        return;
    }

    bool ok = false;
    if (m_newTrackStructAlarmPublisherMulti) {
        ok = m_newTrackStructAlarmPublisherMulti->send(merged) || ok;
    }
    if (m_newTrackStructAlarmPublisherSingle) {
        ok = m_newTrackStructAlarmPublisherSingle->send(merged) || ok;
    }
    if (!ok) {
        qDebug() << "NewTrackStructAlarm发送失败";
    }
}

void CustomConfig::setSuspiciousUniqueIds(const QSet<QString>& seaIds, const QSet<QString>& airIds)
{
    QMutexLocker locker(&m_suspiciousIdsMutex);
    m_suspiciousSeaUniqueIds = seaIds;
    m_suspiciousAirUniqueIds = airIds;
}

QSet<QString> CustomConfig::suspiciousSeaUniqueIds() const
{
    QMutexLocker locker(&m_suspiciousIdsMutex);
    return m_suspiciousSeaUniqueIds;
}

QSet<QString> CustomConfig::suspiciousAirUniqueIds() const
{
    QMutexLocker locker(&m_suspiciousIdsMutex);
    return m_suspiciousAirUniqueIds;
}

int CustomConfig::SendSuspiciousTargetMsg(const QSet<QString>& suspiciousUniqueIds)
{
    if (!m_suspiciousTarget.publishDds) {
        return 0;
    }
    if (!m_bFastDDSInitialized) {
        return 0;
    }
    if (m_newTrackStructSubscriber == nullptr && m_newTrackStructGrpcClient == nullptr) {
        return 0;
    }
    if (m_suspiciousTargetPublisherMulti == nullptr && m_suspiciousTargetPublisherSingle == nullptr) {
        return 0;
    }
    if (suspiciousUniqueIds.isEmpty()) {
        return 0;
    }

    TargetFull::TargetOutputSet merged;
    const bool gotMerged =
        (m_newTrackStructSubscriber && m_newTrackStructSubscriber->build_merged_latest(merged))
        || (m_newTrackStructGrpcClient && m_newTrackStructGrpcClient->build_merged_latest(merged));
    if (!gotMerged) {
        qDebug() << "SuspiciousTarget DDS 跳过: build_merged_latest 失败 ids=" << suspiciousUniqueIds.size();
        return 0;
    }

    std::vector<TargetFull::TargetObject> filteredTargets;
    filteredTargets.reserve(static_cast<size_t>(suspiciousUniqueIds.size()));
    QSet<QString> remainingIds = suspiciousUniqueIds;
    for (const auto& target : merged.targets()) {
        const QString targetId = QString::fromStdString(target.target_id()).trimmed();
        if (!targetId.isEmpty() && remainingIds.contains(targetId)) {
            filteredTargets.push_back(target);
            remainingIds.remove(targetId);
        }
    }

    // 合并 batch 未命中时仍发布最小 TargetObject（仅 target_id），保证下游能收到 ID 列表
    for (const QString& id : remainingIds) {
        TargetFull::TargetObject stub;
        stub.target_id(id.toStdString());
        filteredTargets.push_back(std::move(stub));
    }

    if (filteredTargets.empty()) {
        return 0;
    }

    TargetFull::TargetOutputSet filtered;
    filtered.targets(std::move(filteredTargets));
    filtered.groups(std::vector<TargetFull::GroupObject>());

    bool ok = false;
    if (m_suspiciousTargetPublisherMulti) {
        ok = m_suspiciousTargetPublisherMulti->send(filtered) || ok;
    }
    if (m_suspiciousTargetPublisherSingle) {
        ok = m_suspiciousTargetPublisherSingle->send(filtered) || ok;
    }
    if (!ok) {
        qDebug() << "SuspiciousTarget DDS 发送失败 topic=" << m_suspiciousTarget.ddsTopic
                 << "targets=" << filtered.targets().size();
        return 0;
    }
    return static_cast<int>(filtered.targets().size());
}

bool CustomConfig::alarmEventPushFullEnabled() const
{
    QMutexLocker locker(&m_alarmEventPushModeMutex);
    return m_bAlarmEventPushFullEnabled;
}

void CustomConfig::setAlarmEventPushFullEnabled(bool on)
{
    QMutexLocker locker(&m_alarmEventPushModeMutex);
    m_bAlarmEventPushFullEnabled = on;
    qDebug() << "告警事件推送模式:" << (on ? "完整(SendAllAlarmEventMsg)" : "仅DDS轻量空事件");
}

void CustomConfig::tickAlarmGrpcSnapshot()
{
    if (!m_alarmGrpcClient || !m_alarmGrpcClient->isEnabled()) {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (m_lastAlarmGrpcPushMs != 0 && nowMs - m_lastAlarmGrpcPushMs < 1000) {
        return;
    }

    m_lastAlarmGrpcPushMs = nowMs;
    m_alarmGrpcClient->pushSnapshot(this);
}

AlarmEffectiveDisposition CustomConfig::resolveRuleAlarmDisposition(const AlarmData& alarmData)
{
    AlarmEffectiveDisposition out;
    // 三态升级的处置口径由升级原因决定，不能再被识别规则事后覆盖：
    // 持资格直接入 B 为 UNASSIGNED；光电/连续 7 秒为 VERIFY_SUCCESS。
    if (alarmData.event_stage == 3) {
        if (alarmData.escalation_reason == QLatin1String("direct_entry")) {
            out.task_status = 0;
            return out;
        }
        if (alarmData.escalation_reason == QLatin1String("optic")
            || alarmData.escalation_reason == QLatin1String("alarm_area_dwell")) {
            out.task_status = 3;
            return out;
        }
    }
    switch (alarmData.task_status) {
    case 0:
        out.task_status = 0;
        break;
    case 1:
        out.task_status = 1;
        break;
    case 2:
        out.task_status = 2;
        break;
    default:
        out.task_status = 3;
        break;
    }

    bool anyRuleHadConstraints = false;
    const bool verifyOk = evalIdentificationRulesUnion(
        m_mapAreaKeyToIdentificationRuleIds,
        m_mapAlarmIdentificationRulesSub,
        alarmData,
        dbHelper,
        anyRuleHadConstraints);
    const bool manuallyConfirmed = m_setManualConfirmedUniqueIds.contains(
        static_cast<qint64>(alarmData.unique_id));
    if (manuallyConfirmed || (anyRuleHadConstraints && verifyOk)) {
        out.task_status = 3;

        if (!m_mapAlarmIdentificationResolvedTime.contains(alarmData.alarm_id)) {
            const QString resolvedAt = m_mapManualConfirmResolvedTimeByUniqueId.value(
                static_cast<qint64>(alarmData.unique_id),
                QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")));
            m_mapAlarmIdentificationResolvedTime.insert(alarmData.alarm_id, resolvedAt);

            if (m_alarmLogic.saveAlarmToDb != 0) {
                dbHelper.setSystemEvaluationAlarmTimeOnce(
                    alarmData.alarm_id,
                    static_cast<qint64>(alarmData.unique_id),
                    alarmData.threat_time_ms,
                    QDateTime::currentMSecsSinceEpoch());
            }

            const QString log = QStringLiteral("new alarm-------trackId:%1   timeNow:%2   timeNowStr:%3")
                                    .arg(alarmData.track_id)
                                    .arg(QDateTime::currentDateTime().toSecsSinceEpoch())
                                    .arg(QDateTime::currentDateTime().toString(
                                        QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz")));
            AlarmFileLogger::logNewAlarmTrack(log);
        }

        out.resolved_time_str = m_mapAlarmIdentificationResolvedTime.value(alarmData.alarm_id);
        if (out.resolved_time_str.isEmpty()) {
            out.resolved_time_str = m_mapManualConfirmResolvedTimeByUniqueId.value(
                static_cast<qint64>(alarmData.unique_id));
        }
    }

    return out;
}

void CustomConfig::SendAllAlarmEventMsg()
{
    if (!m_bPublishAlarmEventDds) {
        return;
    }
    if (!m_bFastDDSInitialized || m_alarmevnetPublisherMulti == nullptr) {
        static qint64 s_lastWarnMs = 0;
        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
        if (s_lastWarnMs == 0 || nowMs - s_lastWarnMs >= 60000) {
            s_lastWarnMs = nowMs;
            qDebug() << "FastDDS 告警发布不可用（未初始化或 DataWriter 创建失败），无法发送 AlarmEvent";
        }
        return;
    }

    bool fullPush;
    {
        QMutexLocker locker(&m_alarmEventPushModeMutex);
        fullPush = true;
    }
    if (!fullPush) {
        try {
            AlarmEvent alarmEvent;
            const QString strDate = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_hhmmss"));
            alarmEvent.eventId((QStringLiteral("alarmevent_dds_") + strDate).toStdString());
            if (m_alarmevnetPublisherMulti)
                m_alarmevnetPublisherMulti->sendLst(alarmEvent);
            if (m_alarmevnetPublisherSingle)
                m_alarmevnetPublisherSingle->sendLst(alarmEvent);
            SendNewTrackStructAlarmMsg(&alarmEvent);
        } catch (const std::exception& e) {
            qDebug() << "DDS轻量告警事件发送异常:" << e.what();
        } catch (...) {
            qDebug() << "DDS轻量告警事件发送未知异常";
        }
        return;
    }

    try {
        // 创建AlarmEvent结构体
        AlarmEvent alarmEvent;

        QString strDate = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        
        alarmEvent.eventId("alarmevent_" + strDate.toStdString());
        
        // 锁定告警数据访问
        QMutexLocker locker(&m_alarmDataMutex);
        syncManualAlarmsBeforePublish(this);
        
        // 检查是否有告警数据
        if (m_mapAlarmData.isEmpty() && m_mapManualAlarmData.isEmpty()) {
            //qDebug() << "没有告警数据可发送";
            if (m_alarmevnetPublisherMulti)
                m_alarmevnetPublisherMulti->sendLst(alarmEvent);
            if (m_alarmevnetPublisherSingle)
                m_alarmevnetPublisherSingle->sendLst(alarmEvent);
            SendNewTrackStructAlarmMsg(&alarmEvent);
            return;
        }
        
        // 找出threatScore最高的5个告警，并建立alarm_id到alarmLevel的映射
        QMap<QString, int> alarmLevelMap; // alarm_id -> alarmLevel (5,4,3,2,1 或 0)
        
        // 收集所有有效的告警数据（排除过期告警）
        QList<QPair<QString, int>> validAlarms; // <alarm_id, threatScore>
        qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
        
        for (auto iter = m_mapAlarmData.begin(); iter != m_mapAlarmData.end(); ++iter) {
            const AlarmData& alarmData = iter.value();

            if (m_mapAlarmRule.value(alarmData.condition_id).track_type > 0)
            {
                bool hasTrack = false;
                {
                    QReadLocker rl(&m_trackDataLock);
                    hasTrack = m_mapBirdFuseTrack.contains(static_cast<qint64>(alarmData.unique_id));
                }
                if (!hasTrack) {
                    continue;
                }
            }
            else
            {
                bool hasTrack = false;
                {
                    QReadLocker rl(&m_trackDataLock);
                    hasTrack = m_mapFuseTrack.contains(static_cast<qint64>(alarmData.unique_id));
                }
                if (!hasTrack) {
                    continue;
                }
            }

            qint64 time_alarm = QDateTime::fromString(alarmData.time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
            if (time_now - time_alarm <= 5 * 1000) {
                // 只处理10分钟内的告警
                validAlarms.append(QPair<QString, int>(alarmData.alarm_id, alarmData.threatScore));
            }
        }

        for (auto iter = m_mapManualAlarmData.begin(); iter != m_mapManualAlarmData.end(); ++iter) {
            const AlarmData& alarmData = iter.value();
            const UniqueTrackHit hit = findTrackByUniqueId(this, static_cast<qint64>(alarmData.unique_id));
            if (!hit.found)
                continue;
            qint64 time_alarm = QDateTime::fromString(alarmData.time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
            if (time_now - time_alarm <= 5 * 1000) {
                validAlarms.append(QPair<QString, int>(alarmData.alarm_id, alarmData.threatScore));
            }
        }
        
        // 按threatScore降序排序（threatScore相同时按alarm_id排序，保证稳定性）
        std::sort(validAlarms.begin(), validAlarms.end(), 
            [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                if (a.second != b.second) {
                    return a.second > b.second; // threatScore降序
                }
                return a.first < b.first; // threatScore相同时按alarm_id升序
            });
        
        // 为前5个告警分配级别 5, 4, 3, 2, 1
        int topCount = qMin(5, validAlarms.size());
        for (int i = 0; i < topCount; ++i) {
            alarmLevelMap[validAlarms[i].first] = 5 - i; // 5, 4, 3, 2, 1
        }
        
        // 其他告警级别为0（会在遍历时设置）
        
        // 遍历告警数据并转换为AlarmEvent结构体
        for (auto iter = m_mapAlarmData.begin(); iter != m_mapAlarmData.end(); ++iter) {
            const AlarmData& alarmData = iter.value();
            // 灭告警：按 unique_id 过滤（对海/对空 type 任一命中即不发布）
            if (isUniqueIdAlarmFiltered(static_cast<qint64>(alarmData.unique_id))) {
                continue;
            }
            qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
            qint64 time_alarm = QDateTime::fromString(iter.value().time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
            if (time_now - time_alarm > 600 * 1000)
            {
                continue;
            }

            // 创建告警信息
            AlarmInfo alarmInfo;
            
            // 设置告警类型
            std::vector<AlarmType> alarmTypes;
            
            // 根据mode_id设置告警类型
            //switch (alarmData.mode_id) {
            //    case 1: // 区域告警
                    alarmTypes.push_back(AlarmType::TRACK_AREA);
            //        break;
            //    case 2: // 目标行为
            //        alarmTypes.push_back(casia::event::alarm::AlarmType::TARGET_BEHAVIOR);
            //        break;
            //    case 3: // 目标属性
            //        alarmTypes.push_back(casia::event::alarm::AlarmType::TARGET_ATTR);
            //        break;
            //    case 4: // 设备
            //        alarmTypes.push_back(casia::event::alarm::AlarmType::DEVICE);
            //        break;
            //    default: // 其他
            //        alarmTypes.push_back(casia::event::alarm::AlarmType::CUSTOM_TYPE);
            //        break;
            //}
            alarmInfo.alarmType(alarmTypes);

            // 设置告警状态
            casia::event::alarm::AlarmStatus status;
            switch (alarmData.alarm_status) {
                case 0: // 新生成
                    status = AlarmStatus::ACTIVE;
                    break;
                case 1: // 已读
                    status = AlarmStatus::FOCUS;
                    break;
                case 2: // 删除
                    status = AlarmStatus::DELETED;
                    break;
                default:
                    status = AlarmStatus::ACTIVE;
                    break;
            }
            alarmInfo.status(status);

            //告警规则id
            std::vector<string> ruleIDs;
            ruleIDs.push_back(alarmData.condition_id.toStdString());

            alarmInfo.alarmRuleId(ruleIDs);
            alarmInfo.alarmId(alarmData.alarm_id.toStdString());
            
            const AlarmEffectiveDisposition effDisp = resolveRuleAlarmDisposition(alarmData);
            casia::event::alarm::AlarmTaskStatus taskStatus;
            switch (effDisp.task_status) {
            case 0:
                taskStatus = casia::event::alarm::AlarmTaskStatus::UNASSIGNED;
                break;
            case 1:
                taskStatus = casia::event::alarm::AlarmTaskStatus::ASSIGNED;
                break;
            case 2:
                taskStatus = casia::event::alarm::AlarmTaskStatus::VERIFY_FAILED;
                break;
            default:
                taskStatus = casia::event::alarm::AlarmTaskStatus::VERIFY_SUCCESS;
                break;
            }
            alarmInfo.taskStatus(taskStatus);
            
            // 设置告警内容
            QString alarmContent = QString("告警ID: %1, 告警规则: %2").arg(alarmData.alarm_id).arg(alarmData.condition_id);
            alarmInfo.alarmContent(alarmContent.toStdString());
            
            // 设置告警级别：根据threatScore排名，前5名依次为5,4,3,2,1，其他为0
            int alarmLevel = 0;
            if (alarmLevelMap.contains(alarmData.alarm_id)) {
                alarmLevel = alarmLevelMap[alarmData.alarm_id];
            }
            alarmInfo.alarmContent(QString::number(alarmLevel).toStdString());
            alarmInfo.alarmLevel(alarmData.threatScore);
            
            QString areaName = "";
            QString areaID = "";
            for (const AlarmRule& alarmSetting : m_mapAlarmRule) {
                if (alarmSetting.area_id == alarmData.area_id &&
                    alarmSetting.group_id == alarmData.group_id) {
                    areaName = alarmSetting.area_name;
                    areaID = QString::number(alarmData.group_id) + "_" + QString::number(alarmData.area_id);
                    break;
                }
            }
            // 设置区域ID
            QString strAreaID = QString("%1_%2").arg(alarmData.group_id).arg(alarmData.area_id);
            alarmInfo.areaId(strAreaID.toStdString());
            alarmInfo.areaName(areaName.toStdString());
            
            // 设置航迹ID
            alarmInfo.trackId(QString::number(alarmData.unique_id).toStdString());
            
            // 设置目标类型和行为ID
            alarmInfo.classId(alarmData.targettype);
            alarmInfo.behaviorId(alarmData.targetbehavior);
            
            // 设置位置信息
            casia::event::alarm::GeoPosition position;
            position.longitude(alarmData.targetlon);
            position.latitude(alarmData.targetlat);
            position.altitude(0.0); // 高度默认为0
            alarmInfo.position(position);
            
            // 设置时间信息（每次随告警刷新；resolvedTime 仅在鉴定通过时写入且首次固定）
            alarmInfo.updateTime(alarmData.time.toStdString());
            alarmInfo.resolvedTime(effDisp.resolved_time_str.toStdString());

            // 创建目标框信息
            BoundingBox targetBox;
            targetBox.trackId(legacyDdsTrackId(alarmData.unique_id));
            
            // 创建航迹数据
            TrackData trackData;
            casia::event::alarm::TrackType trackType;
            if (m_mapAlarmRule.value(alarmData.condition_id).track_type > 0)
            {
                trackType = casia::event::alarm::TrackType::BIRD;
            }
            else
            {
                trackType = casia::event::alarm::TrackType::FUSE;
            }
            trackData.trackId(legacyDdsTrackId(alarmData.unique_id));
            trackData.mmsi(legacyDdsTrackId(alarmData.unique_id));
            trackData.longitude(alarmData.targetlon);
            trackData.latitude(alarmData.targetlat);
            trackData.course(alarmData.targetdir);
            trackData.speed(alarmData.targetspeed);
            trackData.trackType(trackType);
            
            // 设置目标框和航迹数据
            alarmInfo.targetBox(targetBox);
            alarmInfo.track(trackData);
            
            // 将告警信息添加到告警事件中
            std::vector<AlarmInfo> alarmInfoList = alarmEvent.alarm();
            alarmInfoList.push_back(alarmInfo);
            alarmEvent.alarm(alarmInfoList);
        }

        // 前端手动确认告警（不在规则 m_mapAlarmData 中）
        for (auto iter = m_mapManualAlarmData.begin(); iter != m_mapManualAlarmData.end(); ++iter) {
            const AlarmData& alarmData = iter.value();
            const bool isAirTrack = m_mapManualAlarmAirTrack.value(
                static_cast<qint64>(alarmData.unique_id), false);
            if (isUniqueIdAlarmFiltered(static_cast<qint64>(alarmData.unique_id))) {
                continue;
            }

            qint64 time_now = QDateTime::currentDateTime().toMSecsSinceEpoch();
            qint64 time_alarm = QDateTime::fromString(alarmData.time, "yyyy-MM-dd hh:mm:ss.zzz").toMSecsSinceEpoch();
            if (time_now - time_alarm > 600 * 1000)
                continue;

            const UniqueTrackHit hit = findTrackByUniqueId(this, static_cast<qint64>(alarmData.unique_id));
            if (!hit.found)
                continue;

            AlarmInfo alarmInfo;
            std::vector<AlarmType> alarmTypes;
            alarmTypes.push_back(AlarmType::TRACK_AREA);
            alarmInfo.alarmType(alarmTypes);
            alarmInfo.status(AlarmStatus::ACTIVE);
            alarmInfo.alarmRuleId(std::vector<string>{alarmData.condition_id.toStdString()});
            alarmInfo.alarmId(alarmData.alarm_id.toStdString());
            alarmInfo.taskStatus(casia::event::alarm::AlarmTaskStatus::VERIFY_SUCCESS);

            int alarmLevel = 0;
            if (alarmLevelMap.contains(alarmData.alarm_id))
                alarmLevel = alarmLevelMap[alarmData.alarm_id];
            alarmInfo.alarmContent(QString::number(alarmLevel).toStdString());
            alarmInfo.alarmLevel(alarmData.threatScore);
            alarmInfo.areaId("");
            alarmInfo.areaName("");
            alarmInfo.trackId(QString::number(alarmData.unique_id).toStdString());
            alarmInfo.classId(alarmData.targettype);
            alarmInfo.behaviorId(alarmData.targetbehavior);

            casia::event::alarm::GeoPosition position;
            position.longitude(alarmData.targetlon);
            position.latitude(alarmData.targetlat);
            position.altitude(0.0);
            alarmInfo.position(position);

            alarmInfo.updateTime(alarmData.time.toStdString());
            const QString resolvedAt = m_mapManualConfirmResolvedTimeByUniqueId.value(
                static_cast<qint64>(alarmData.unique_id),
                alarmData.time);
            alarmInfo.resolvedTime(resolvedAt.toStdString());

            BoundingBox targetBox;
            targetBox.trackId(legacyDdsTrackId(alarmData.unique_id));

            TrackData trackData;
            trackData.trackId(legacyDdsTrackId(alarmData.unique_id));
            trackData.mmsi(legacyDdsTrackId(alarmData.unique_id));
            trackData.longitude(alarmData.targetlon);
            trackData.latitude(alarmData.targetlat);
            trackData.course(alarmData.targetdir);
            trackData.speed(alarmData.targetspeed);
            trackData.trackType(isAirTrack ? casia::event::alarm::TrackType::BIRD
                                           : casia::event::alarm::TrackType::FUSE);

            alarmInfo.targetBox(targetBox);
            alarmInfo.track(trackData);

            std::vector<AlarmInfo> alarmInfoList = alarmEvent.alarm();
            alarmInfoList.push_back(alarmInfo);
            alarmEvent.alarm(alarmInfoList);
        }
        
        // 处理 m_mapTargetType 中不在 m_mapAlarmData 中的 target_id
        QSet<QString> processedTargetIds;
        for (auto iter = m_mapAlarmData.begin(); iter != m_mapAlarmData.end(); ++iter) {
            processedTargetIds.insert(QString::number(iter.value().unique_id));
        }
        for (auto iter = m_mapManualAlarmData.begin(); iter != m_mapManualAlarmData.end(); ++iter) {
            processedTargetIds.insert(QString::number(iter.value().unique_id));
        }

        for (auto iter = m_mapTargetType.begin(); iter != m_mapTargetType.end(); ++iter) {
            const QString targetIdStr = iter.key();

            if (processedTargetIds.contains(targetIdStr)) {
                continue;
            }

            bool targetIdOk = false;
            const qint64 targetId = targetIdStr.toLongLong(&targetIdOk);
            if (!targetIdOk || targetId <= 0) {
                continue;
            }
            if (isUniqueIdAlarmFiltered(targetId)) {
                continue;
            }

            SPxPacketTrackExtended track;
            bool foundTrack = false;
            casia::event::alarm::TrackType trackType = casia::event::alarm::TrackType::FUSE;

            {
                QReadLocker rl(&m_trackDataLock);
                if (m_mapFuseTrack.contains(targetId)) {
                    track = m_mapFuseTrack.value(targetId);
                    foundTrack = true;
                    trackType = casia::event::alarm::TrackType::FUSE;
                } else if (m_mapBirdFuseTrack.contains(targetId)) {
                    track = m_mapBirdFuseTrack.value(targetId);
                    foundTrack = true;
                    trackType = casia::event::alarm::TrackType::BIRD;
                }
            }

            if (!foundTrack) {
                continue;
            }

            AlarmInfo alarmInfo;

            std::vector<AlarmType> alarmTypes;
            alarmTypes.push_back(AlarmType::TRACK_AREA);
            alarmInfo.alarmType(alarmTypes);

            alarmInfo.status(AlarmStatus::ACTIVE);

            alarmInfo.alarmId("-1");

            const bool manuallyConfirmed = m_setManualConfirmedUniqueIds.contains(targetId);
            QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            if (manuallyConfirmed) {
                alarmInfo.taskStatus(casia::event::alarm::AlarmTaskStatus::VERIFY_SUCCESS);
                const QString resolvedAt = m_mapManualConfirmResolvedTimeByUniqueId.value(targetId, currentTime);
                alarmInfo.resolvedTime(resolvedAt.toStdString());
            } else {
                alarmInfo.taskStatus(casia::event::alarm::AlarmTaskStatus::UNASSIGNED);
            }

            QString alarmContent = QString("目标类型更新: target_id %1").arg(targetId);
            alarmInfo.alarmContent(alarmContent.toStdString());

            alarmInfo.alarmLevel(1);

            alarmInfo.areaId("");
            alarmInfo.areaName("");

            alarmInfo.trackId(targetIdStr.toStdString());

            alarmInfo.classId(iter.value());
            alarmInfo.behaviorId(0);

            casia::event::alarm::GeoPosition position;
            position.longitude(track.longDegs);
            position.latitude(track.latDegs);
            position.altitude(track.altitudeMetres);
            alarmInfo.position(position);

            alarmInfo.updateTime(currentTime.toStdString());

            BoundingBox targetBox;
            targetBox.trackId(legacyDdsTrackId(targetId));
            alarmInfo.targetBox(targetBox);

            TrackData trackData;
            trackData.trackId(legacyDdsTrackId(targetId));
            trackData.longitude(track.longDegs);
            trackData.latitude(track.latDegs);
            trackData.course(track.norm.min.courseDegrees);
            trackData.speed(track.norm.min.speedMps);
            trackData.trackType(trackType);
            alarmInfo.track(trackData);

            std::vector<AlarmInfo> alarmInfoList = alarmEvent.alarm();
            alarmInfoList.push_back(alarmInfo);
            alarmEvent.alarm(alarmInfoList);
        }
        
        // 发布告警事件
        if(m_alarmevnetPublisherMulti)
            m_alarmevnetPublisherMulti->sendLst(alarmEvent);
        if(m_alarmevnetPublisherSingle)
            m_alarmevnetPublisherSingle->sendLst(alarmEvent);
        SendNewTrackStructAlarmMsg(&alarmEvent);
        qDebug() << "成功发送" << m_mapAlarmData.size() << "条告警数据";
        
    } catch (const std::exception& e) {
        qDebug() << "发送告警消息时发生异常:" << e.what();
    } catch (...) {
        qDebug() << "发送告警消息时发生未知异常";
    }
}

void CustomConfig::pruneExpiredAlarmFiltersLocked(qint64 nowMs)
{
    for (auto it = m_mapAlarmFilterExpireMs.begin(); it != m_mapAlarmFilterExpireMs.end(); ) {
        if (it.value() <= nowMs) {
            qInfo() << "alarm_filter expired type=" << it.key().first
                    << "target_id=" << it.key().second;
            it = m_mapAlarmFilterExpireMs.erase(it);
        } else {
            ++it;
        }
    }
}

void CustomConfig::removeAlarmFilterForUniqueId(qint64 uniqueId)
{
    if (uniqueId <= 0) {
        return;
    }
    QMutexLocker locker(&m_alarmFilterMutex);
    const bool had0 = m_mapAlarmFilterExpireMs.remove(qMakePair(0, uniqueId)) > 0;
    const bool had1 = m_mapAlarmFilterExpireMs.remove(qMakePair(1, uniqueId)) > 0;
    if (had0 || had1) {
        qInfo() << "alarm_filter cleared by manual confirm target_id=" << uniqueId;
    }
}

void CustomConfig::addAlarmFilter(int type, qint64 targetId)
{
    if (targetId <= 0) {
        return;
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 expireMs = nowMs + qMax<qint64>(1000, m_alarmLogic.alarmFilterTtlMs);
    {
        QMutexLocker locker(&m_alarmFilterMutex);
        m_mapAlarmFilterExpireMs.insert(qMakePair(type, targetId), expireMs);
        qInfo() << "alarm_filter add type=" << type << "target_id=" << targetId
                << "expire_ms=" << expireMs << "ttl_ms=" << m_alarmLogic.alarmFilterTtlMs;
    }

    // 从内存告警表剔除该目标，避免 TrackAlarmThread 下一轮继续刷新并发布
    {
        QMutexLocker locker(&m_alarmDataMutex);
        QList<QString> ruleKeys;
        for (auto it = m_mapAlarmData.constBegin(); it != m_mapAlarmData.constEnd(); ++it) {
            if (static_cast<qint64>(it.value().unique_id) == targetId) {
                ruleKeys.append(it.key());
            }
        }
        for (const QString& k : ruleKeys) {
            m_mapAlarmData.remove(k);
        }

        QList<QString> manualKeys;
        for (auto it = m_mapManualAlarmData.constBegin(); it != m_mapManualAlarmData.constEnd(); ++it) {
            if (static_cast<qint64>(it.value().unique_id) == targetId) {
                manualKeys.append(it.key());
            }
        }
        for (const QString& k : manualKeys) {
            m_mapManualAlarmData.remove(k);
        }
    }
    m_mapTargetType.remove(QString::number(targetId));
}

bool CustomConfig::isUniqueIdAlarmFiltered(qint64 uniqueId)
{
    if (uniqueId <= 0) {
        return false;
    }
    QMutexLocker locker(&m_alarmFilterMutex);
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    pruneExpiredAlarmFiltersLocked(nowMs);
    const auto isActive = [&](int type) {
        const auto it = m_mapAlarmFilterExpireMs.constFind(qMakePair(type, uniqueId));
        return it != m_mapAlarmFilterExpireMs.constEnd() && it.value() > nowMs;
    };
    return isActive(0) || isActive(1);
}

bool CustomConfig::confirmAlarmByUniqueId(
    qint64 uniqueId, QString* outMessage, const ManualConfirmTrackHint* trackHint)
{
    if (uniqueId <= 0) {
        if (outMessage)
            *outMessage = QStringLiteral("invalid uniqueId");
        return false;
    }

    // 手动标蓝/确认告警：解除该 unique_id 的灭告警 filter，恢复规则与手动告警发布
    removeAlarmFilterForUniqueId(uniqueId);

    QMutexLocker locker(&m_alarmDataMutex);
    const QString nowStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    if (!m_mapManualConfirmResolvedTimeByUniqueId.contains(uniqueId))
        m_mapManualConfirmResolvedTimeByUniqueId.insert(uniqueId, nowStr);
    m_setManualConfirmedUniqueIds.insert(uniqueId);

    const int confirmThreatFloor = m_alarmLogic.defaultThreatScore > 0
        ? m_alarmLogic.defaultThreatScore
        : 90;

    bool hadRuleAlarm = false;
    for (auto it = m_mapAlarmData.begin(); it != m_mapAlarmData.end(); ++it) {
        AlarmData& ad = it.value();
        if (static_cast<qint64>(ad.unique_id) != uniqueId)
            continue;
        hadRuleAlarm = true;
        // 右键设为蓝方：规则告警级别至少升到默认高威胁（严重）
        if (ad.threatScore < confirmThreatFloor)
            ad.threatScore = confirmThreatFloor;
        if (m_mapAlarmIdentificationResolvedTime.contains(ad.alarm_id))
            continue;
        m_mapAlarmIdentificationResolvedTime.insert(
            ad.alarm_id, m_mapManualConfirmResolvedTimeByUniqueId.value(uniqueId));
        if (m_alarmLogic.saveAlarmToDb != 0) {
            dbHelper.setSystemEvaluationAlarmTimeOnce(
                ad.alarm_id,
                uniqueId,
                ad.threat_time_ms,
                nowMs);
        }
        const QString log = QStringLiteral("manual confirm alarm------trackId:%1 uniqueId:%2 alarmId:%3")
            .arg(ad.track_id)
            .arg(uniqueId)
            .arg(ad.alarm_id);
        AlarmFileLogger::logNewAlarmTrack(log);
    }

    if (hadRuleAlarm) {
        if (outMessage)
            *outMessage = QStringLiteral("ok");
        return true;
    }

    UniqueTrackHit hit = findTrackByUniqueId(this, uniqueId);
    bool usedTrackHint = false;
    if (!hit.found && trackHint && trackHint->valid) {
        // 内存表已 prune / 本进程未订阅该 topic：用前端附带运动学建 manual alarm（不改 map 超时）
        std::memset(&hit.track, 0, sizeof(hit.track));
        hit.track.latDegs = trackHint->latDegs;
        hit.track.longDegs = trackHint->lonDegs;
        hit.track.norm.min.speedMps = trackHint->speedMps;
        hit.track.norm.min.courseDegrees = trackHint->courseDeg;
        if (uniqueId <= static_cast<qint64>(std::numeric_limits<uint32_t>::max()))
            hit.track.secondary.uniqueID = static_cast<uint32_t>(uniqueId);
        hit.track.msgTimeSecs = static_cast<uint32_t>(QDateTime::currentSecsSinceEpoch());
        hit.isAirTrack = trackHint->isAirTrack;
        hit.found = true;
        usedTrackHint = true;
    }
    if (!hit.found) {
        if (outMessage)
            *outMessage = QStringLiteral("track not found for uniqueId (not in fuse/radar map)");
        return false;
    }

    const qint64 storageUniqueId = effectiveUniqueIdFromTrackHit(hit, uniqueId);
    if (storageUniqueId != uniqueId) {
        removeAlarmFilterForUniqueId(storageUniqueId);
    }
    m_setManualConfirmedUniqueIds.insert(storageUniqueId);
    if (!m_mapManualConfirmResolvedTimeByUniqueId.contains(storageUniqueId)) {
        m_mapManualConfirmResolvedTimeByUniqueId.insert(storageUniqueId, nowStr);
    }

    const QString manualAlarmId = QStringLiteral("manual-") + QString::number(storageUniqueId);
    const AlarmData* existing = m_mapManualAlarmData.contains(manualAlarmId)
        ? &m_mapManualAlarmData[manualAlarmId]
        : nullptr;
    AlarmData manualAlarm = buildManualAlarmDataFromTrack(
        hit.track, storageUniqueId, m_alarmLogic.defaultThreatScore, existing);
    manualAlarm.alarm_content = buildManualAlarmContent(
        this,
        storageUniqueId,
        hit.isAirTrack,
        manualAlarm.targetspeed,
        manualAlarm.targetdir);
    m_mapManualAlarmData.insert(manualAlarmId, manualAlarm);
    m_mapManualAlarmAirTrack.insert(storageUniqueId, hit.isAirTrack);

    const QString log = QStringLiteral("manual confirm new alarm------trackId:%1 uniqueId:%2 alarmId:%3 air:%4 reqUniqueId:%5 hint:%6")
        .arg(manualAlarm.track_id)
        .arg(storageUniqueId)
        .arg(manualAlarmId)
        .arg(hit.isAirTrack ? 1 : 0)
        .arg(uniqueId)
        .arg(usedTrackHint ? 1 : 0);
    AlarmFileLogger::logNewAlarmTrack(log);

    if (outMessage) {
        *outMessage = usedTrackHint
            ? QStringLiteral("ok (manual alarm created from client track hint)")
            : QStringLiteral("ok (manual alarm created for dds)");
    }
    return true;
}


void CustomConfig::LoadConfig()
{
    QSettings settings("Config.ini", QSettings::IniFormat);

    m_struBasicConfig.m_nStyleMode = settings.value("Basic/StyleMode").toInt();
    m_struBasicConfig.m_nRadarCnt = settings.value("Basic/RadarCnt").toInt();
    m_struBasicConfig.m_nCameraCnt = settings.value("Basic/CameraCnt").toInt();
    m_struBasicConfig.m_nPlatformCnt = settings.value("Basic/PlatformCnt").toInt();
    m_struBasicConfig.m_nRefpCnt = settings.value("Basic/RefpCnt").toString().toInt();
    m_struBasicConfig.m_bEnableWideCam = settings.value("Basic/EnableWideCam").toBool();
    m_struBasicConfig.m_bShipPlatform = settings.value("Basic/ShipPlatform").toBool();
    m_struBasicConfig.m_bShowFrameRate = settings.value("Basic/ShowFrameRate").toBool();
    m_struBasicConfig.m_nRefreshInterval = settings.value("Basic/RefreshInterval").toInt();
    m_struBasicConfig.m_nAutoHandleInterval = settings.value("Basic/AutoHandleInterval").toInt();
    m_struBasicConfig.m_nCameraDecodeType = settings.value("Basic/CameraDecodeType").toInt();
    m_struBasicConfig.m_nCameraTopRoot = settings.value("Basic/CameraTopRoot").toInt();
    m_struBasicConfig.m_nUsePtzModel = settings.value("Basic/UsePTZModel").toInt();
    m_struBasicConfig.m_nPPIDDSPort = settings.value("Basic/PPIDDSPort").toInt();
    if (settings.contains("Basic/DebugMode"))
        m_struBasicConfig.m_bDebugMode = settings.value("Basic/DebugMode").toBool();
    if(settings.contains("Basic/UseRemoteCameraControl"))
        m_struBasicConfig.m_bUseRemoteCameraControl = settings.value("Basic/UseRemoteCameraControl").toBool();
    m_struBasicConfig.m_strControlIP = settings.value("Basic/RemoteControllP").toString();
    m_struBasicConfig.m_nControlPort = settings.value("Basic/RemoteControlPort").toInt();
    if (settings.contains("Basic/UseSingleTrackMetaTask"))
        m_struBasicConfig.m_bUseSingleTrackMeataTask = settings.value("Basic/UseSingleTrackMetaTask").toBool();
    m_struBasicConfig.m_strSingleTrackServerIP = settings.value("Basic/SingleTrackServerlP").toString();
    m_struBasicConfig.m_nSingleTrackServerPort = settings.value("Basic/SingleTrackServerPort").toInt();
    if(settings.contains("Basic/CheckOffsetHS"))
        m_struBasicConfig.m_dCheckOffsetHS = settings.value("Basic/CheckOffsetHS").toFloat();

    if (settings.contains("Basic/RecvSinglePortDDS"))
        m_struBasicConfig.m_bUseSingleDDSPort = settings.value("Basic/RecvSinglePortDDS").toBool();
    if (settings.contains("Basic/RemoteControlDDSPort"))
        m_struBasicConfig.m_nCameraStateDDSPort = settings.value("Basic/RemoteControlDDSPort").toInt();
    if (settings.contains("Basic/MultiTrackDDSPort"))
        m_struBasicConfig.m_nCameraMultiTrackRectDDSPort = settings.value("Basic/MultiTrackDDSPort").toInt();
    if (settings.contains("Basic/SingleTrackDDSPort"))
        m_struBasicConfig.m_nCameraSingleTrackRectDDSPort = settings.value("Basic/SingleTrackDDSPort").toInt();
    if (settings.contains("Basic/AlarmEventDDSPort"))
        m_struBasicConfig.m_nAlarmEventDDSPort = settings.value("Basic/AlarmEventDDSPort").toInt();
    if (settings.contains("Basic/AlarmEventDDSPortSingle"))
        m_struBasicConfig.m_nAlarmEventDDSPortSingle = settings.value("Basic/AlarmEventDDSPortSingle").toInt();
    if (settings.contains("Basic/TrackDDSPort"))
        m_struBasicConfig.m_nTrackDDSPort = settings.value("Basic/TrackDDSPort").toInt();
    {
        const QString fusionTx = settings
                                     .value(QStringLiteral("Basic/FusionTrackTransport"), QStringLiteral("dds"))
                                     .toString()
                                     .trimmed()
                                     .toLower();
        m_struBasicConfig.m_strFusionTrackTransport =
            (fusionTx == QStringLiteral("grpc")) ? QStringLiteral("grpc") : QStringLiteral("dds");
        m_struBasicConfig.m_strNewTrackStructGrpcAddr =
            settings
                .value(QStringLiteral("Basic/NewTrackStructGrpcAddr"), QStringLiteral("192.168.18.141:60055"))
                .toString()
                .trimmed();
        {
            const QVariant configured = settings.value(
                QStringLiteral("Basic/VirtualTrackSourceAllowList"),
                QStringLiteral("virtual_new_track_struct_grpc_client"));
            QStringList values = configured.canConvert<QStringList>()
                ? configured.toStringList()
                : configured.toString().split(',', QString::SkipEmptyParts);
            m_struBasicConfig.m_virtualTrackSourceAllowList.clear();
            for (const QString& value : values) {
                const QString sourceId = value.trimmed();
                if (!sourceId.isEmpty()
                    && !m_struBasicConfig.m_virtualTrackSourceAllowList.contains(sourceId)) {
                    m_struBasicConfig.m_virtualTrackSourceAllowList.append(sourceId);
                }
            }
            if (m_struBasicConfig.m_virtualTrackSourceAllowList.isEmpty()) {
                m_struBasicConfig.m_virtualTrackSourceAllowList.append(
                    QStringLiteral("virtual_new_track_struct_grpc_client"));
            }
        }
        const QString radarTx = settings
                                    .value(QStringLiteral("Basic/RadarTrackTransport"), QStringLiteral("dds"))
                                    .toString()
                                    .trimmed()
                                    .toLower();
        m_struBasicConfig.m_strRadarTrackTransport =
            (radarTx == QStringLiteral("grpc")) ? QStringLiteral("grpc") : QStringLiteral("dds");
        m_struBasicConfig.m_strFusionTrackStreamGrpcAddr =
            settings
                .value(QStringLiteral("Basic/FusionTrackStreamGrpcAddr"), QStringLiteral("192.168.18.141:60056"))
                .toString()
                .trimmed();
        qInfo() << "NewTrackStruct 虚兵来源白名单"
                << m_struBasicConfig.m_virtualTrackSourceAllowList;

        {
            const QString routesCsv = settings
                                          .value(QStringLiteral("Basic/FusionTrackGrpcSourceRoutes"),
                                                 QStringLiteral(
                                                     "yuan_yao:radar,jing_zi_tou:radar,udp_xpf_track:radar,"
                                                     "ais:ais,tan_niao:bird,auto_bird:bird,"
                                                     "ku_lei_da:bird,udp_fanwucar_track:bird"))
                                          .toString()
                                          .trimmed();
            m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.clear();
            static const QSet<QString> kValidBuckets = {
                QStringLiteral("radar"),
                QStringLiteral("ais"),
                QStringLiteral("bird"),
            };
            for (const QString& part : routesCsv.split(',', QString::SkipEmptyParts)) {
                const QStringList kv = part.trimmed().split(':');
                if (kv.size() < 2) {
                    continue;
                }
                const QString ds = kv.at(0).trimmed();
                QString bucket = kv.at(1).trimmed().toLower();
                // AlarmSys 探鸟统一进 m_mapBirdRadarTrack（兼容 camServer 的 auto_bird/ku_bird）
                if (bucket == QLatin1String("auto_bird") || bucket == QLatin1String("ku_bird")) {
                    bucket = QStringLiteral("bird");
                }
                if (ds.isEmpty() || !kValidBuckets.contains(bucket)) {
                    continue;
                }
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(ds, bucket);
            }
            if (m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.isEmpty()) {
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("yuan_yao"), QStringLiteral("radar"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("jing_zi_tou"), QStringLiteral("radar"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("udp_xpf_track"), QStringLiteral("radar"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("ais"), QStringLiteral("ais"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("tan_niao"), QStringLiteral("bird"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("auto_bird"), QStringLiteral("bird"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("ku_lei_da"), QStringLiteral("bird"));
                m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute.insert(QStringLiteral("udp_fanwucar_track"), QStringLiteral("bird"));
            }
        }
        {
            const QString sourcesCsv = settings
                                           .value(QStringLiteral("Basic/RadarTrackGrpcSources"),
                                                  QStringLiteral("yuan_yao:1,jing_zi_tou:2,udp_xpf_track:3"))
                                           .toString()
                                           .trimmed();
            m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.clear();
            for (const QString& part : sourcesCsv.split(',', QString::SkipEmptyParts)) {
                const QStringList kv = part.trimmed().split(':');
                if (kv.size() < 2) {
                    continue;
                }
                const QString ds = kv.at(0).trimmed();
                bool ok = false;
                const quint32 code = kv.at(1).trimmed().toUInt(&ok);
                if (ds.isEmpty() || !ok || code == 0) {
                    continue;
                }
                m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.insert(ds, code);
            }
            if (m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.isEmpty()) {
                m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.insert(QStringLiteral("yuan_yao"), 1);
                m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.insert(QStringLiteral("jing_zi_tou"), 2);
                m_struBasicConfig.m_mapRadarTrackGrpcSourceCode.insert(QStringLiteral("udp_xpf_track"), 3);
            }
        }
        {
            const QString slotsCsv = settings
                                         .value(QStringLiteral("Basic/RadarFusionTrackIdSlots"),
                                                QStringLiteral("0:yuan_yao,2:jing_zi_tou"))
                                         .toString()
                                         .trimmed();
            m_struBasicConfig.m_vRadarFusionTrackIdSlots.clear();
            for (const QString& part : slotsCsv.split(',', QString::SkipEmptyParts)) {
                const QStringList kv = part.trimmed().split(':');
                if (kv.size() < 2) {
                    continue;
                }
                bool ok = false;
                const int slot = kv.at(0).trimmed().toInt(&ok);
                const QString ds = kv.at(1).trimmed();
                if (!ok || slot < 0 || slot >= 8 || ds.isEmpty()) {
                    continue;
                }
                m_struBasicConfig.m_vRadarFusionTrackIdSlots.append(qMakePair(slot, ds));
            }
            if (m_struBasicConfig.m_vRadarFusionTrackIdSlots.isEmpty()) {
                m_struBasicConfig.m_vRadarFusionTrackIdSlots.append(
                    qMakePair(0, QStringLiteral("yuan_yao")));
                m_struBasicConfig.m_vRadarFusionTrackIdSlots.append(
                    qMakePair(2, QStringLiteral("jing_zi_tou")));
            }
        }
        qInfo() << "FusionTrackGrpcSourceRoutes" << m_struBasicConfig.m_mapFusionTrackGrpcSourceRoute;
        qInfo() << "RadarTrackGrpcSources" << m_struBasicConfig.m_mapRadarTrackGrpcSourceCode;
    }

    m_struBasicConfig.m_bEnableLaserRadar = settings.value("Basic/EnableLaserRadar").toBool();
    m_struBasicConfig.m_bEnableMilliwaveRadar = settings.value("Basic/EnableMilliwaveRadar").toBool();
    m_struBasicConfig.m_strVesselName = settings.value("Basic/VesselName").toString();
    m_struBasicConfig.m_strAisNum = settings.value("Basic/AisNum").toString();
    m_struBasicConfig.m_nVesselLength = settings.value("Basic/VesselLength").toInt();
    m_struBasicConfig.m_nVesselWidth = settings.value("Basic/VesselWidth").toInt();
    m_struBasicConfig.m_nAlarmCPA = settings.value("Basic/AlarmCPA").toInt();
    m_struBasicConfig.m_nAlarmTCPA = settings.value("Basic/AlarmTCPA").toInt();
    m_struBasicConfig.m_nAlarmTime = settings.value("Basic/AlarmTCPA").toInt();
    m_struBasicConfig.m_strEmailAddr = settings.value("Basic/EmailAddr").toString();

    m_struBasicConfig.m_strADSBIP = settings.value("Basic/ADSBIP").toString();
    m_struBasicConfig.m_nADSBPort = settings.value("Basic/ADSBPort").toInt();

    m_struBasicConfig.m_nGpsProto = settings.value("Basic/GpsProto").toInt();
    m_struBasicConfig.m_nGpsInputType = settings.value("Basic/GpsInputType").toInt();
    m_struBasicConfig.m_strGpsIP = settings.value("Basic/GpsIP").toString();
    m_struBasicConfig.m_nGpsPort = settings.value("Basic/GpsPort").toInt();
    m_struBasicConfig.m_strGpsSerialPort = settings.value("Basic/GpsSerialPort").toString();
    m_struBasicConfig.m_nGpsSerialBaud = settings.value("Basic/GpsSerialBaud").toInt();

    m_struBasicConfig.m_nCompProto = settings.value("Basic/CompProto").toInt();
    m_struBasicConfig.m_nCompInputType = settings.value("Basic/CompInputType").toInt();
    m_struBasicConfig.m_strCompIP = settings.value("Basic/CompIP").toString();
    m_struBasicConfig.m_nCompPort = settings.value("Basic/CompPort").toInt();
    m_struBasicConfig.m_strCompSerialPort = settings.value("Basic/CompSerialPort").toString();
    m_struBasicConfig.m_nCompSerialBaud = settings.value("Basic/CompSerialBaud").toInt();

    m_struBasicConfig.m_nDeptInputType = settings.value("Basic/DeptInputType").toInt();
    m_struBasicConfig.m_nDeptProto = settings.value("Basic/DeptProto").toInt();
    m_struBasicConfig.m_strDeptIP = settings.value("Basic/DeptIP").toString();
    m_struBasicConfig.m_nDeptPort = settings.value("Basic/DeptPort").toInt();
    m_struBasicConfig.m_strDeptSerialPort = settings.value("Basic/DeptSerialPort").toString();
    m_struBasicConfig.m_nDeptSerialBaud = settings.value("Basic/DeptSerialBaud").toInt();

    m_struBasicConfig.m_nWeatherInputType = settings.value("Basic/WeatherInputType").toInt();
    m_struBasicConfig.m_nWeatherProto = settings.value("Basic/WeatherProto").toInt();
    m_struBasicConfig.m_strWeatherIP = settings.value("Basic/WeatherIP").toString();
    m_struBasicConfig.m_nWeatherPort = settings.value("Basic/WeatherPort").toInt();
    m_struBasicConfig.m_strWeatherSerialPort = settings.value("Basic/WeatherSerialPort").toString();
    m_struBasicConfig.m_nWeatherSerialBaud = settings.value("Basic/WeatherSerialBaud").toInt();

    m_struBasicConfig.m_nMergeInputType = settings.value("Basic/MergeInputType").toInt();
    m_struBasicConfig.m_nMergeProto = settings.value("Basic/MergeProto").toInt();
    m_struBasicConfig.m_strMergeIP = settings.value("Basic/MergeIP").toString();
    m_struBasicConfig.m_nMergePort = settings.value("Basic/MergePort").toInt();
    m_struBasicConfig.m_strMergeSerialPort = settings.value("Basic/MergeSerialPort").toString();
    m_struBasicConfig.m_nMergeSerialBaud = settings.value("Basic/MergeSerialBaud").toInt();

    m_struBasicConfig.m_nAISInputType = settings.value("Basic/AISInputType").toInt();
    m_struBasicConfig.m_nAISProto = settings.value("Basic/AISProto").toInt();
    m_struBasicConfig.m_strAISIP = settings.value("Basic/AISIP").toString();
    m_struBasicConfig.m_nAISPort = settings.value("Basic/AISPort").toInt();
    m_struBasicConfig.m_strAISSerialPort = settings.value("Basic/AISSerialPort").toString();
    m_struBasicConfig.m_nAISSerialBaud = settings.value("Basic/AISSerialBaud").toInt();

    m_struBasicConfig.m_strSurroundCam0 = settings.value("Basic/SurroundCam0").toString();
    m_struBasicConfig.m_strSurroundCam1 = settings.value("Basic/SurroundCam1").toString();
    m_struBasicConfig.m_strSurroundCam2 = settings.value("Basic/SurroundCam2").toString();
    m_struBasicConfig.m_strSurroundCam3 = settings.value("Basic/SurroundCam3").toString();
    m_struBasicConfig.m_strSurroundCam4 = settings.value("Basic/SurroundCam4").toString();

    m_struBasicConfig.m_nUseBasePoint = settings.value("Basic/UseBasePoint").toInt();
    m_struBasicConfig.m_dBasePointLon = settings.value("Basic/BasePointLon").toDouble();
    m_struBasicConfig.m_dBasePointLat = settings.value("Basic/BasePointLat").toDouble();

    m_struBasicConfig.m_strVlmServerIp = settings.value("Basic/VlmJudgeIP").toString();
    m_struBasicConfig.m_nVlmServerPort = settings.value("Basic/VlmJudgePort").toInt();
    m_struBasicConfig.m_nCollectHostPort = settings.value("Basic/CollectHostPort").toInt();
    m_struBasicConfig.m_nTaskHostPort = settings.value("Basic/TaskHostPort").toInt();
    m_struBasicConfig.m_nTaskHandleHostPort = settings.value("Basic/TaskHandleHostPort").toInt();
    m_struBasicConfig.m_strTaskServerIp = settings.value("Basic/TaskServerIP").toString();
    m_struBasicConfig.m_nTaskServerPort = settings.value("Basic/TaskServerPort").toInt();
    m_struBasicConfig.m_strCollectServerIp = settings.value("Basic/CollectServerIP").toString();
    m_struBasicConfig.m_nCollectServerPort = settings.value("Basic/CollectServerPort").toInt();
    m_struBasicConfig.m_strUavServerIp = settings.value("Basic/UavServerIP").toString();
    m_struBasicConfig.m_nUavServerPort = settings.value("Basic/UavServerPort").toInt();
    m_struBasicConfig.m_strUavVideoHostIP = settings.value("Basic/UUAVVideoHostIP").toString();
    m_struBasicConfig.m_nUavVideoDetectPort = settings.value("Basic/UAVVideoDetectPort").toInt();
    m_struBasicConfig.m_strUavVideoDetectIP = settings.value("Basic/UAVVideoDetectIP").toString();
    m_struBasicConfig.m_nUavVideoDetectPort1 = settings.value("Basic/UAVVideoDetectPort1").toInt();
    m_struBasicConfig.m_nUavVideoDetectPort2 = settings.value("Basic/UAVVideoDetectPort2").toInt();
    m_struBasicConfig.m_nUavVideoDetectPort3 = settings.value("Basic/UAVVideoDetectPort3").toInt(); 
    m_struBasicConfig.m_strAudioTransIp = settings.value("Basic/AudioTransIP").toString();
    m_struBasicConfig.m_nAudioTransPort = settings.value("Basic/AudioTransPort").toInt();
    m_struBasicConfig.m_nAudioTransWithIdentifyPort = settings.value("Basic/AudioTransWithIdentifyPort").toInt();
    m_struBasicConfig.m_bUseChatRoute = settings.value("Basic/UseChatRoute").toBool();
    m_struBasicConfig.m_strAliAudioTransIp = settings.value("Basic/AliAudioTransIP").toString();
    m_struBasicConfig.m_nAliAudioTransPort = settings.value("Basic/AliAudioTransPort").toInt();
    m_struBasicConfig.m_nVideoTransPort = settings.value("Basic/VideoTransPort").toInt();    
    m_struBasicConfig.m_strShipFileIp = settings.value("Basic/ShipFileServerIP").toString();
    m_struBasicConfig.m_nShipFilePort = settings.value("Basic/ShipFileServerPort").toInt();
    m_struBasicConfig.m_strSelfPoseTrackServerIp = settings.value("Basic/SelfPoseTrackServerIP").toString();
    m_struBasicConfig.m_nSelfPoseTrackServerPort = settings.value("Basic/SelfPoseTrackServerPort").toInt();
    m_struBasicConfig.m_nSelfPoseAirTrackServerPort = settings.value("Basic/SelfPoseAirTrackServerPort").toInt();

    m_struBasicConfig.m_strRecvSelfPoseIP = settings.value("Basic/RecvSelfPoseIP").toString();
    m_struBasicConfig.m_nRecvSelfPosePort = settings.value("Basic/RecvSelfPosePort").toInt();
    //m_struBasicConfig.m_strUAVVideoDetectIP = settings.value("Basic/UAVVideoDetectIP").toString();
    //m_struBasicConfig.m_nUAVVideoDetectPort = settings.value("Basic/UAVVideoDetectPort").toInt();

    m_struBasicConfig.m_nLLMMode = settings.value("Basic/LLMMode").toInt();
    m_struBasicConfig.m_nAutoHandleMode = settings.value("Basic/AutoHandleMode").toInt();

    m_vRadarConfig.clear();
    for (int i = 0; i < 4; i++)
    {
        RadarAttr radar;
        radar.m_nRadarId = i;
        radar.m_strRadarName = settings.value(QString("RadarAttr%1/RadarName").arg(radar.m_nRadarId)).toString();
        radar.m_bEnable = settings.value(QString("RadarAttr%1/RadarEnable").arg(radar.m_nRadarId)).toBool();
        radar.m_nResolution = settings.value(QString("RadarAttr%1/RadarResolution").arg(radar.m_nRadarId)).toInt();
        radar.m_dPRF = settings.value(QString("RadarAttr%1/RadarPRF").arg(radar.m_nRadarId)).toDouble();
        radar.m_dAzi = settings.value(QString("RadarAttr%1/RadarAzi").arg(radar.m_nRadarId)).toDouble();
        radar.m_dDuraton = settings.value(QString("RadarAttr%1/RadarDuration").arg(radar.m_nRadarId)).toDouble();
        radar.m_nPosType = settings.value(QString("RadarAttr%1/RadarPosType").arg(radar.m_nRadarId)).toDouble();
        radar.m_nRefPoint = settings.value(QString("RadarAttr%1/RadarRefp").arg(radar.m_nRadarId)).toInt();
        radar.m_dOffsetX = settings.value(QString("RadarAttr%1/RadarOffsetX").arg(radar.m_nRadarId)).toDouble();
        radar.m_dOffsetY = settings.value(QString("RadarAttr%1/RadarOffsetY").arg(radar.m_nRadarId)).toDouble();
        radar.m_dLon = settings.value(QString("RadarAttr%1/RadarLon").arg(radar.m_nRadarId)).toDouble();
        radar.m_dLat = settings.value(QString("RadarAttr%1/RadarLat").arg(radar.m_nRadarId)).toDouble();
        radar.m_dAlt = settings.value(QString("RadarAttr%1/RadarAlt").arg(radar.m_nRadarId)).toDouble();
        radar.m_dHeight = settings.value(QString("RadarAttr%1/RadarHeight").arg(radar.m_nRadarId)).toDouble();
        radar.m_dSpiral = settings.value(QString("RadarAttr%1/RadarSpiral").arg(radar.m_nRadarId)).toDouble();
        radar.m_nGpsFmt = settings.value(QString("RadarAttr%1/RadarGpsFmt").arg(radar.m_nRadarId)).toInt();
        radar.m_strGpsIP = settings.value(QString("RadarAttr%1/RadarGpsIP").arg(radar.m_nRadarId)).toString();
        radar.m_nGpsPort = settings.value(QString("RadarAttr%1/RadarGpsPort").arg(radar.m_nRadarId)).toInt();
        radar.m_strGpsInputInterface = settings.value(QString("RadarAttr%1/RadarGpsIf").arg(radar.m_nRadarId)).toString();
        radar.m_nVideoFmt = settings.value(QString("RadarAttr%1/RadarVideoFmt").arg(radar.m_nRadarId)).toInt();
        radar.m_strVideoInputIP = settings.value(QString("RadarAttr%1/RadarVideoIP").arg(radar.m_nRadarId)).toString();
        radar.m_nVideoInputPort = settings.value(QString("RadarAttr%1/RadarVideoPort").arg(radar.m_nRadarId)).toInt();
        radar.m_strVideoInputInterface = settings.value(QString("RadarAttr%1/RadarVideoIf").arg(radar.m_nRadarId)).toString();
        radar.m_nTrackFmt = settings.value(QString("RadarAttr%1/RadarTrackFmt").arg(radar.m_nRadarId)).toInt();
        radar.m_strTrackInputIP = settings.value(QString("RadarAttr%1/RadarTrackIP").arg(radar.m_nRadarId)).toString();
        radar.m_nTrackInputPort = settings.value(QString("RadarAttr%1/RadarTrackPort").arg(radar.m_nRadarId)).toInt();
        radar.m_strTrackInputInterface = settings.value(QString("RadarAttr%1/RadarTrackIf").arg(radar.m_nRadarId)).toString();
        radar.m_nRadarType = settings.value(QString("RadarAttr%1/RadarType").arg(radar.m_nRadarId)).toInt();
        radar.m_nCtrlConf = settings.value(QString("RadarAttr%1/RadarCtrlConf").arg(radar.m_nRadarId)).toInt();
        radar.m_nRange = settings.value(QString("RadarAttr%1/RadarRange").arg(radar.m_nRadarId)).toDouble();
        radar.m_dGain = settings.value(QString("RadarAttr%1/RadarGain").arg(radar.m_nRadarId)).toDouble();

        m_vRadarConfig.push_back(radar);
    }
 

    m_listRange << "1NM" << "3NM" << "6NM" << "12NM" << "24NM" << "48NM" << "96NM";

    m_listBowOrient << "C up" << "H up" << "N up";

    m_listAlarmTbl << "状态" << "时间" << "名称" << "航迹" << "CPA" << "TCPA" << "触发器";

    m_listTrackCommon << "名称" << "类型" << "ID" << "分类" << "警报" << "位置"
        << "距离" << "方位" << "SOG" << "COG" << "备注";
    m_listTrackMerge << "二次";

    m_listTrackAIS << "AIS MMSI" << "AIS IMO" << "AIS呼号" << "AIS船名"
        << "AIS导航状态" << "AIS长度" << "AIS吃水" << "AIS转弯角速度"
        << "AIS船型" << "AIS目的地" << "AIS预计到达时间" << "AIS尺寸";


    m_listEquipTbl << "序号" << "设备名称" << "设备状态";

    m_listChannelTbl << "序号" << "信道名称" << "信道状态";

    m_listTemplateTbl << "" << "任务名称";

    m_listHistoryTbl << "任务名称" << "采集员" << "采集时间" << "采集时长(秒)" << "采集状态";

    m_alarmLogic.mode = settings.value("AlarmLogic/Mode", 0).toInt();
    m_alarmLogic.saveAlarmToDb = settings.value("AlarmLogic/SaveAlarmToDb", 0).toInt();
    m_alarmLogic.refreshTargetInfoFilterEachLoop = settings.value("AlarmLogic/RefreshTargetInfoFilterEachLoop", 1).toInt();
    m_alarmLogic.alarmMapPruneNonFirstStaleMs = settings.value("AlarmLogic/AlarmMapPruneNonFirstStaleMs", 5000).toInt();
    m_alarmLogic.dedupSameAlarmWindowMs = settings.value("AlarmLogic/DedupSameAlarmWindowMs", 600000).toInt();
    m_alarmLogic.dedupMatchConditionAndTrack = settings.value("AlarmLogic/DedupMatchConditionAndTrack", 0).toInt();
    m_alarmLogic.originTimeFromTrackMsg = settings.value("AlarmLogic/OriginTimeFromTrackMsg", 1).toInt();
    m_alarmLogic.originTrackTimeIsMs = settings.value("AlarmLogic/OriginTrackTimeIsMs", 0).toInt();
    m_alarmLogic.birdRadarSourceId = settings.value("AlarmLogic/BirdRadarSourceId", 9).toInt();
    m_alarmLogic.birdFilterMode = settings.value("AlarmLogic/BirdFilterMode", 0).toInt();
    m_alarmLogic.birdSelfReportSubstrings = settings.value("AlarmLogic/BirdSelfReportSubstrings", QStringLiteral("自报位")).toString();
    if (settings.contains(QStringLiteral("AlarmLogic/BirdSkipTrackIds")))
        parseIntSetFromSettings(settings, QStringLiteral("AlarmLogic/BirdSkipTrackIds"), &m_alarmLogic.birdSkipTrackIds);
    if (m_alarmLogic.birdSkipTrackIds.isEmpty())
        parseCsvIntSet(QStringLiteral("4001,4002,4003,4004,4008,4011"), &m_alarmLogic.birdSkipTrackIds);
    {
        QStringList skipParts;
        for (int sid : m_alarmLogic.birdSkipTrackIds) {
            skipParts.append(QString::number(sid));
        }
        skipParts.sort();
        AlarmFileLogger::logNewAlarmTrack(
            QStringLiteral("AlarmLogic bird filter config------BirdRadarSourceId:%1 BirdFilterMode:%2 BirdSkipTrackIds:[%3]")
                .arg(m_alarmLogic.birdRadarSourceId)
                .arg(m_alarmLogic.birdFilterMode)
                .arg(skipParts.join(QLatin1Char(','))));
    }
    if (settings.contains(QStringLiteral("AlarmLogic/AreaGroupIdAllowList")))
        parseIntSetFromSettings(settings, QStringLiteral("AlarmLogic/AreaGroupIdAllowList"), &m_alarmLogic.areaGroupIdAllow);
    if (m_alarmLogic.areaGroupIdAllow.isEmpty())
        parseCsvIntSet(QStringLiteral("1,2"), &m_alarmLogic.areaGroupIdAllow);
    if (settings.contains(QStringLiteral("AlarmLogic/NoAlarmGroupIdList"))) {
        parseIntSetFromSettings(settings, QStringLiteral("AlarmLogic/NoAlarmGroupIdList"), &m_alarmLogic.noAlarmGroupIds);
    } else if (settings.contains(QStringLiteral("AlarmLogic/NoAlarmGroupId"))) {
        const int legacyNoAlarmGroupId = settings.value(QStringLiteral("AlarmLogic/NoAlarmGroupId")).toInt();
        m_alarmLogic.noAlarmGroupIds.clear();
        if (legacyNoAlarmGroupId >= 0)
            m_alarmLogic.noAlarmGroupIds.insert(legacyNoAlarmGroupId);
    } else {
        parseCsvIntSet(QStringLiteral("1,2"), &m_alarmLogic.noAlarmGroupIds);
    }
    m_alarmLogic.fuseMapRequireContain = settings.value("AlarmLogic/FuseMapRequireContain", 1).toInt();
    m_alarmLogic.trackAlreadyHasAlarmWindowMs = settings.value("AlarmLogic/TrackAlreadyHasAlarmWindowMs", 60000).toInt();
    m_alarmLogic.alarmFilterTtlMs = settings.value(QStringLiteral("AlarmLogic/AlarmFilterTtlMs"), 3600000).toInt();
    m_alarmLogic.speedDoubleCheckFuseTrackOnly = settings.value("AlarmLogic/SpeedDoubleCheckFuseTrackOnly", 1).toInt();
    m_alarmLogic.defaultThreatScore = settings.value("AlarmLogic/DefaultThreatScore", 90).toInt();

    m_areaEscalation.enabled = settings.value(QStringLiteral("AreaEscalation/Enabled"), 0).toInt();
    qInfo() << "AreaEscalation 配置: enabled=" << m_areaEscalation.enabled;

    m_suspiciousTarget.enabled = settings.value(QStringLiteral("SuspiciousTarget/Enabled"), 0).toInt();
    m_suspiciousTarget.judgeIntervalMs = settings.value(QStringLiteral("SuspiciousTarget/JudgeIntervalMs"), 5000).toInt();
    m_suspiciousTarget.speedLowThresholdMps =
        settings.value(QStringLiteral("SuspiciousTarget/SpeedLowThresholdMps"), 3.0).toDouble();
    m_suspiciousTarget.speedHighThresholdMps =
        settings.value(QStringLiteral("SuspiciousTarget/SpeedHighThresholdMps"), 8.0).toDouble();
    m_suspiciousTarget.protectAngleMaxDeg =
        settings.value(QStringLiteral("SuspiciousTarget/ProtectAngleMaxDeg"), 30.0).toDouble();
    m_suspiciousTarget.speedCheckClearIntervalMin =
        settings.value(QStringLiteral("SuspiciousTarget/SpeedCheckClearIntervalMin"), 30).toInt();
    m_suspiciousTarget.embedInTarget =
        settings.value(QStringLiteral("SuspiciousTarget/EmbedInTarget"), 1).toInt();
    m_suspiciousTarget.publishDds =
        settings.value(QStringLiteral("SuspiciousTarget/PublishDds"), 0).toInt();
    m_suspiciousTarget.ddsTopic = settings.value(
        QStringLiteral("SuspiciousTarget/DdsTopic"), QStringLiteral("NewTrackStructSuspicious")).toString();
    m_suspiciousTarget.embedRuleId = settings.value(
        QStringLiteral("SuspiciousTarget/EmbedRuleId"), QStringLiteral("suspicious_target")).toString();
    if (m_suspiciousTarget.embedRuleId.trimmed().isEmpty()) {
        m_suspiciousTarget.embedRuleId = QStringLiteral("suspicious_target");
    }
    if (m_suspiciousTarget.judgeIntervalMs < 1000) {
        m_suspiciousTarget.judgeIntervalMs = 1000;
    }
    qInfo() << "SuspiciousTarget 配置: enabled=" << m_suspiciousTarget.enabled
            << "intervalMs=" << m_suspiciousTarget.judgeIntervalMs
            << "speedLow=" << m_suspiciousTarget.speedLowThresholdMps
            << "speedHigh=" << m_suspiciousTarget.speedHighThresholdMps
            << "angleMax=" << m_suspiciousTarget.protectAngleMaxDeg
            << "embedInTarget=" << m_suspiciousTarget.embedInTarget
            << "publishDds=" << m_suspiciousTarget.publishDds
            << "embedRuleId=" << m_suspiciousTarget.embedRuleId
            << "topic=" << m_suspiciousTarget.ddsTopic;

    // 默认关闭 AlarmEvent DDS；Nexus 走 gRPC UpdateAlarmSnapshot。改 PublishAlarmEventDds=1 可恢复。
    m_bPublishAlarmEventDds =
        settings.value(QStringLiteral("DDS/PublishAlarmEventDds"), 0).toInt() != 0;
    qInfo() << "DDS/PublishAlarmEventDds=" << (m_bPublishAlarmEventDds ? 1 : 0)
            << (m_bPublishAlarmEventDds
                    ? "(AlarmEvent/NewTrackStructAlarm DDS 开启)"
                    : "(告警 DDS 关闭，走 gRPC UpdateAlarmSnapshot)");

    const bool grpcEnabled = settings.value(QStringLiteral("GrpcAlarm/Enabled"), 1).toInt() != 0;
    const QString grpcHost = settings.value(QStringLiteral("GrpcAlarm/Host"), QStringLiteral("192.168.18.141")).toString();
    const int grpcPort = settings.value(QStringLiteral("GrpcAlarm/Port"), 25051).toInt();
    const QString grpcProducer = settings.value(QStringLiteral("GrpcAlarm/ProducerId"), QStringLiteral("AlarmSys-linux")).toString();
    m_alarmGrpcClient->configure(grpcHost.toStdString(), grpcPort, grpcEnabled, grpcProducer.toStdString());
    qInfo() << "GrpcAlarm 配置: enabled=" << grpcEnabled
            << "host=" << grpcHost << "port=" << grpcPort;
    // 启动独立 gRPC 快照推送线程（与 processAlarms 解耦，防止 processAlarms 慢时快照中断导致 NexusUI 告警闪烁）
    startGrpcSnapshotTimer();

    const bool destroyGrpcEnabled = settings.value(QStringLiteral("GrpcDestroy/Enabled"), 1).toInt() != 0;
    const QString destroyGrpcHost = settings.value(QStringLiteral("GrpcDestroy/Host"), grpcHost).toString();
    const int destroyGrpcPort = settings.value(QStringLiteral("GrpcDestroy/Port"), 50061).toInt();
    if (m_alarmDestroyGrpcSubscriber) {
        m_alarmDestroyGrpcSubscriber->configure(
            destroyGrpcHost.toStdString(), destroyGrpcPort, destroyGrpcEnabled);
        qInfo() << "GrpcDestroy 配置: enabled=" << destroyGrpcEnabled
                << "host=" << destroyGrpcHost << "port=" << destroyGrpcPort;
    }

    const bool targetTypeGrpcEnabled =
        settings.value(QStringLiteral("GrpcTargetType/Enabled"), 1).toInt() != 0;
    const QString targetTypeGrpcHost =
        settings.value(QStringLiteral("GrpcTargetType/Host"), QStringLiteral("192.168.18.141")).toString();
    const int targetTypeGrpcPort =
        settings.value(QStringLiteral("GrpcTargetType/Port"), 60054).toInt();
    if (m_targetTypeGrpcClient) {
        m_targetTypeGrpcClient->configure(
            targetTypeGrpcHost.toStdString(), targetTypeGrpcPort, targetTypeGrpcEnabled);
        qInfo() << "GrpcTargetType 配置: enabled=" << targetTypeGrpcEnabled
                << "host=" << targetTypeGrpcHost << "port=" << targetTypeGrpcPort;
    }

    m_systemAlarmGrpcEnabled =
        settings.value(QStringLiteral("GrpcSystemAlarm/Enabled"), 1).toInt() != 0;
    m_systemAlarmGrpcListen =
        settings.value(QStringLiteral("GrpcSystemAlarm/Listen"), QStringLiteral("192.168.18.141")).toString();
    m_systemAlarmGrpcPort =
        settings.value(QStringLiteral("GrpcSystemAlarm/Port"), 25071).toInt();
    qInfo() << "GrpcSystemAlarm 配置: enabled=" << m_systemAlarmGrpcEnabled
            << "listen=" << m_systemAlarmGrpcListen << "port=" << m_systemAlarmGrpcPort;
}

void CustomConfig::startAlarmDestroyGrpcSubscriber()
{
    if (m_alarmDestroyGrpcSubscriber) {
        m_alarmDestroyGrpcSubscriber->start(this);
    }
}

void CustomConfig::stopAlarmDestroyGrpcSubscriber()
{
    if (m_alarmDestroyGrpcSubscriber) {
        m_alarmDestroyGrpcSubscriber->stop();
    }
}

bool CustomConfig::pushTargetTypeGrpcUpdate(qint64 targetId, const QString& targetType)
{
    if (!m_targetTypeGrpcClient || !m_targetTypeGrpcClient->isEnabled()) {
        return false;
    }
    return m_targetTypeGrpcClient->updateTargetType(targetId, targetType);
}

void CustomConfig::SaveConfig()
{
    QSettings settings("Config.ini", QSettings::IniFormat); // 当前目录的INI文件
    qDebug() << "save state" << endl;
    for (int i = 0; i < 16; i++)
    {
        settings.beginGroup(QString("PTZ%1").arg(i));
        settings.setValue("MainCamID", m_vPtzConfig[i].m_nMainCamID);
        settings.setValue("SideCamEnable", m_vPtzConfig[i].m_nSideCamEnable);
        settings.setValue("SideCamID", m_vPtzConfig[i].m_nSideCamID);
        settings.setValue("ShowAR", m_vPtzConfig[i].m_nShowAR);
        settings.setValue("ShowDetect", m_vPtzConfig[i].m_nShowDetect);
        settings.setValue("ShowTrack", m_vPtzConfig[i].m_nShowTrack);
        settings.setValue("ShowRadar", m_vPtzConfig[i].m_nShowRadar);
        settings.setValue("ShowFuse", m_vPtzConfig[i].m_nShowFuse);

        settings.endGroup();
    }

    settings.beginGroup("PTZWILD");
    settings.setValue("MainCamID", WildCamConfAttr.m_nMainCamID);
    settings.setValue("SideCamEnable", WildCamConfAttr.m_nSideCamEnable);
    settings.setValue("SideCamID", WildCamConfAttr.m_nSideCamID);
    settings.endGroup();
}

void CustomConfig::setConfigPath(string path)
{
    ifstream File(path);
    if (File)
        m_configPath = path;
}
void CustomConfig::SetLog(string logMes)
{
    logMes = QString::fromUtf8(logMes.c_str()).toStdString();
    time_t now = time(0);
    tm* t_tm = localtime(&now);
    string  tim = to_string(t_tm->tm_year + 1900) + "-" + to_string(t_tm->tm_mon + 1) + "-" + to_string(t_tm->tm_mday) + "-" + to_string(t_tm->tm_hour) + "-" + to_string(t_tm->tm_min) + "-" + to_string(t_tm->tm_sec);

    logMes = tim + ":" + logMes;
    fstream infile("log.iscas", ios::out | std::ios::app);
    if (!infile.is_open()) return;

    infile.write(logMes.data(), logMes.size());
    infile.write("\n", sizeof(char));
    infile.flush();
    infile.close();
}

void CustomConfig::setConfig(string name, string value)
{
    m_configMap[name] = value;
}

string CustomConfig::valS(string paramName)
{
    if (m_configMap.find(paramName) != m_configMap.end()) return m_configMap[paramName];
    return "";
}

int CustomConfig::valI(string paramName)
{
    string str = valS(paramName);
    int value = 0;
    if (str != "") value = stoi(str);
    return value;
}

float CustomConfig::valF(string paramName)
{
    string str = valS(paramName);
    float value = 0;
    if (str != "") value = stof(str);
    return value;
}
