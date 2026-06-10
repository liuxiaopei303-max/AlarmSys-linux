#include "customconfig.h"
#include "grpc_alarm/AlarmGrpcSnapshotClient.hpp"
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
#include <QReadLocker>
#include <algorithm>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QDateTime>
#include "dialog/alarm/AlarmFileLogger.h"
#include "fastdds_newtrack/NewTrackStructAlarmPublisherApp.hpp"
#include "fastdds_newtrack/NewTrackStructSubscriberApp.hpp"

namespace {

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
    const auto tryMap = [&](const QMap<int, SPxPacketTrackExtended>& trackMap, bool isAir) {
        if (hit.found)
            return;
        for (auto it = trackMap.constBegin(); it != trackMap.constEnd(); ++it) {
            const SPxPacketTrackExtended& tr = it.value();
            const qint64 uid = static_cast<qint64>(tr.secondary.uniqueID);
            const int normId = static_cast<int>(tr.norm.min.id);
            if (uid == uniqueId || (normId > 0 && normId == static_cast<int>(uniqueId))) {
                hit.track = tr;
                hit.isAirTrack = isAir;
                hit.found = true;
                return;
            }
        }
    };
    tryMap(cfg->m_mapBirdFuseTrack, true);
    tryMap(cfg->m_mapBirdRadarTrack, true);
    tryMap(cfg->m_mapFuseTrack, false);
    tryMap(cfg->m_mapRadarTrack, false);
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
    ad.track_id = static_cast<int>(track.norm.min.id);
    ad.unique_id = uniqueId;
    ad.targetlat = track.latDegs;
    ad.targetlon = track.longDegs;
    ad.targetspeed = track.norm.min.speedMps;
    ad.targetdir = track.norm.min.courseDegrees;
    ad.targetdist = track.norm.min.rangeMetres / 1852.0;
    ad.targettype = static_cast<int>(track.norm.reserved3);
    ad.targetbehavior = 0;
    ad.time = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    ad.origintime = (existing && !existing->origintime.isEmpty())
        ? existing->origintime
        : ad.time;
    ad.alarm_status = 0;
    ad.task_status = 0;
    ad.alarm_count = existing ? existing->alarm_count + 1 : 1;
    ad.threatScore = track.norm.reserved3 > 0
        ? static_cast<int>(track.norm.reserved3)
        : (existing ? existing->threatScore : defaultThreatScore);
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

CustomConfig* CustomConfig::instance = nullptr;
QMutex CustomConfig::mutex;

CustomConfig::CustomConfig()
{
    m_alarmGrpcClient = std::make_unique<AlarmGrpcSnapshotClient>();
    LoadConfig();

    m_dbInitSuccess = dbHelper.initDatabase();

    m_listTargetInfo = dbHelper.getAllTargetInfo();
    m_mapAlarmRule =  dbHelper.getAlarmRule();
    m_mapAlarmIdentificationRulesSub = dbHelper.getAlarmIdentificationRulesSub();
    m_mapAreaKeyToIdentificationRuleIds = dbHelper.getIdentificationRuleIdsByAreaForActiveScheme();
    m_alarmArea = dbHelper.getAreaInfo();
    m_mapTargetInfoFilter = dbHelper.getTargetInfoFilter();
    m_listThreatAssessmentParams = dbHelper.getThreatAssessmentParams();
    m_mapSchemeProtectAreas = dbHelper.getActiveSchemeProtectAreas();

    m_gNetworkManager = new QNetworkAccessManager;
    m_gNetworkManager->setNetworkAccessible(QNetworkAccessManager::Accessible);
}


CustomConfig::~CustomConfig() = default;

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

    //初始化alarmevent发送者
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

    // 航迹订阅：老 TrackDataClass / 新 NewTrackStruct 由 Config.ini [DDS]/TrackEnableOldSubscriber 控制
    const int trackEnableOldSubscriber =
        QSettings(QStringLiteral("Config.ini"), QSettings::IniFormat)
            .value(QStringLiteral("DDS/TrackEnableOldSubscriber"), 1)
            .toInt();
    if (trackEnableOldSubscriber != 0) {
        try {
            m_trackSubscriber = new TrackClassSubscriberApp(trackDDSPort);
            std::thread trackThread(&TrackClassApplication::run, m_trackSubscriber);
            trackThread.detach();
            qDebug() << "TrackClassSubscriber 已启用 domain=" << trackDDSPort;
        }
        catch (std::exception& e) {
            qDebug() << "TrackClassSubscriber 初始化失败:" << e.what();
            m_trackSubscriber = nullptr;
        }
    } else {
        qDebug() << "TrackClassSubscriber 已禁用，航迹数据由 NewTrackStruct 订阅写入";
    }

    // 初始化 NewTrackStruct 订阅与告警发布（复用当前航迹域 ID）
    try {
        m_newTrackStructSubscriber = new NewTrackStructSubscriberApp(trackDDSPort);
        m_newTrackStructAlarmPublisherMulti = new NewTrackStructAlarmPublisherApp(trackDDSPort, 0);
        m_newTrackStructAlarmPublisherSingle = new NewTrackStructAlarmPublisherApp(trackDDSPort, 1);
    }
    catch (std::exception& e) {
        qDebug() << "NewTrackStruct DDS初始化失败:" << e.what();
    }

    //try{
    //    m_recognitionPublisher = new FastddsRecognitionPublisherApp(m_struBasicConfig.m_nRecognitionDDSPort);
    //    std::thread recognitionThread(&FastddsRecognitionApplication::run, m_recognitionPublisher);
    //    recognitionThread.detach();
    //}
    //catch (std::exception& e) {
    //    qDebug() << "识别信息发布者初始化失败:" << e.what();    
    //}

    m_bFastDDSInitialized = (m_alarmevnetPublisherMulti != nullptr);
    if (!m_bFastDDSInitialized) {
        qDebug() << "FastDDS 告警发布未就绪：AlarmEvent DataWriter 未创建，前端将收不到 DDS 告警";
    } else {
        qDebug() << "FastDDS 初始化完成，AlarmEvent domain=" << alarmeventDDSPort
                 << " single=" << alarmeventDDSPortSingle;
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
    if (!m_bFastDDSInitialized || m_newTrackStructSubscriber == nullptr) {
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
    if (!m_newTrackStructSubscriber->build_merged_latest(merged)) {
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
                    hasTrack = m_mapBirdFuseTrack.contains(alarmData.track_id);
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
                    hasTrack = m_mapFuseTrack.contains(alarmData.track_id);
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
            // 告警过滤：若该 trackid 在过滤列表中且规则 track_type 与过滤 type 一致则不发送
            if (m_mapAlarmRule.contains(alarmData.condition_id)) {
                int ruleTrackType = m_mapAlarmRule.value(alarmData.condition_id).track_type;
                int apiType = (ruleTrackType > 0) ? 1 : 0; // 0-对海 1-对空
                QMutexLocker filterLocker(&m_alarmFilterMutex);
                if (m_listAlarmFilter.contains(qMakePair(apiType, alarmData.track_id))) {
                    continue;
                }
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
            alarmInfo.trackId(QString::number(alarmData.track_id).toStdString());
            
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
            targetBox.trackId(alarmData.track_id);
            
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
            trackData.trackId(alarmData.track_id);
            trackData.mmsi(alarmData.track_id);
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

            int apiType = isAirTrack ? 1 : 0;
            QMutexLocker filterLocker(&m_alarmFilterMutex);
            if (m_listAlarmFilter.contains(qMakePair(apiType, alarmData.track_id))) {
                continue;
            }
            filterLocker.unlock();

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
            alarmInfo.trackId(QString::number(alarmData.track_id).toStdString());
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
            targetBox.trackId(alarmData.track_id);

            TrackData trackData;
            trackData.trackId(alarmData.track_id);
            trackData.mmsi(alarmData.track_id);
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
        
        // 处理 m_mapTargetType 中不在 m_mapAlarmData 中的 track_id
        // 收集所有已处理的 track_id
        QSet<QString> processedTrackIds;
        for (auto iter = m_mapAlarmData.begin(); iter != m_mapAlarmData.end(); ++iter) {
            processedTrackIds.insert(QString::number(iter.value().track_id));
        }
        for (auto iter = m_mapManualAlarmData.begin(); iter != m_mapManualAlarmData.end(); ++iter) {
            processedTrackIds.insert(QString::number(iter.value().track_id));
        }
        
        // 遍历 m_mapTargetType，找出未处理的 track_id
        for (auto iter = m_mapTargetType.begin(); iter != m_mapTargetType.end(); ++iter) {
            QString trackIdStr = iter.key();
            
            // 如果这个 track_id 已经在告警数据中处理过，跳过
            if (processedTrackIds.contains(trackIdStr)) {
                continue;
            }
            
            // 尝试从航迹数据中获取信息
            bool trackIdOk = false;
            int trackId = trackIdStr.toInt(&trackIdOk);
            if (!trackIdOk) {
                continue;
            }
            
            // 尝试从融合航迹或雷达航迹中获取信息
            SPxPacketTrackExtended track;
            bool foundTrack = false;
            casia::event::alarm::TrackType trackType = casia::event::alarm::TrackType::FUSE;
            
            {
                QReadLocker rl(&m_trackDataLock);
                if (m_mapFuseTrack.contains(trackId)) {
                    track = m_mapFuseTrack.value(trackId);
                    foundTrack = true;
                    trackType = casia::event::alarm::TrackType::FUSE;
                } else if (m_mapBirdFuseTrack.contains(trackId)) {
                    track = m_mapBirdFuseTrack.value(trackId);
                    foundTrack = true;
                    trackType = casia::event::alarm::TrackType::BIRD;
                }
            }
            
            if (!foundTrack) {
                continue; // 如果找不到航迹数据，跳过
            }
            
            // 创建告警信息
            AlarmInfo alarmInfo;
            
            // 设置告警类型
            std::vector<AlarmType> alarmTypes;
            alarmTypes.push_back(AlarmType::TRACK_AREA);
            alarmInfo.alarmType(alarmTypes);
            
            // 设置告警状态为ACTIVE
            alarmInfo.status(AlarmStatus::ACTIVE);
            
            // 使用-1作为告警ID
            alarmInfo.alarmId("-1");

            const qint64 trackUniqueId = static_cast<qint64>(track.secondary.uniqueID);
            const bool manuallyConfirmed = m_setManualConfirmedUniqueIds.contains(trackUniqueId);
            QString currentTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
            if (manuallyConfirmed) {
                alarmInfo.taskStatus(casia::event::alarm::AlarmTaskStatus::VERIFY_SUCCESS);
                const QString resolvedAt = m_mapManualConfirmResolvedTimeByUniqueId.value(trackUniqueId, currentTime);
                alarmInfo.resolvedTime(resolvedAt.toStdString());
            } else {
                alarmInfo.taskStatus(casia::event::alarm::AlarmTaskStatus::UNASSIGNED);
            }
            
            // 设置告警内容
            QString alarmContent = QString("目标类型更新: 航迹ID %1").arg(trackId);
            alarmInfo.alarmContent(alarmContent.toStdString());
            
            // 设置告警级别为默认值1
            alarmInfo.alarmLevel(1);
            
            // 设置区域ID为空
            alarmInfo.areaId("");
            alarmInfo.areaName("");
            
            // 设置航迹ID
            alarmInfo.trackId(trackIdStr.toStdString());
            
            // 设置目标类型（从 m_mapTargetType 中获取）
            alarmInfo.classId(iter.value());
            alarmInfo.behaviorId(0); // 默认行为ID
            
            // 设置位置信息
            casia::event::alarm::GeoPosition position;
            position.longitude(track.longDegs);
            position.latitude(track.latDegs);
            position.altitude(track.altitudeMetres);
            alarmInfo.position(position);
            
            // 设置时间信息
            alarmInfo.updateTime(currentTime.toStdString());
            
            // 创建目标框信息
            BoundingBox targetBox;
            targetBox.trackId(trackId);
            alarmInfo.targetBox(targetBox);
            
            // 创建航迹数据
            TrackData trackData;
            trackData.trackId(trackId);
            trackData.longitude(track.longDegs);
            trackData.latitude(track.latDegs);
            trackData.course(track.norm.min.courseDegrees);
            trackData.speed(track.norm.min.speedMps);
            trackData.trackType(trackType);
            alarmInfo.track(trackData);
            
            // 将告警信息添加到告警事件中
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

void CustomConfig::addAlarmFilter(int type, int trackid)
{
    QMutexLocker locker(&m_alarmFilterMutex);
    QPair<int, int> p = qMakePair(type, trackid);
    if (!m_listAlarmFilter.contains(p)) {
        m_listAlarmFilter.append(p);
    }
}

bool CustomConfig::confirmAlarmByUniqueId(qint64 uniqueId, QString* outMessage)
{
    if (uniqueId <= 0) {
        if (outMessage)
            *outMessage = QStringLiteral("invalid uniqueId");
        return false;
    }

    QMutexLocker locker(&m_alarmDataMutex);
    const QString nowStr = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();

    if (!m_mapManualConfirmResolvedTimeByUniqueId.contains(uniqueId))
        m_mapManualConfirmResolvedTimeByUniqueId.insert(uniqueId, nowStr);
    m_setManualConfirmedUniqueIds.insert(uniqueId);

    bool hadRuleAlarm = false;
    for (auto it = m_mapAlarmData.begin(); it != m_mapAlarmData.end(); ++it) {
        AlarmData& ad = it.value();
        if (static_cast<qint64>(ad.unique_id) != uniqueId)
            continue;
        hadRuleAlarm = true;
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

    const UniqueTrackHit hit = findTrackByUniqueId(this, uniqueId);
    if (!hit.found) {
        if (outMessage)
            *outMessage = QStringLiteral("track not found for uniqueId (not in fuse/radar map)");
        return false;
    }

    const qint64 storageUniqueId = effectiveUniqueIdFromTrackHit(hit, uniqueId);
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
    m_mapManualAlarmData.insert(manualAlarmId, manualAlarm);
    m_mapManualAlarmAirTrack.insert(storageUniqueId, hit.isAirTrack);

    const QString log = QStringLiteral("manual confirm new alarm------trackId:%1 uniqueId:%2 alarmId:%3 air:%4 reqUniqueId:%5")
        .arg(manualAlarm.track_id)
        .arg(storageUniqueId)
        .arg(manualAlarmId)
        .arg(hit.isAirTrack ? 1 : 0)
        .arg(uniqueId);
    AlarmFileLogger::logNewAlarmTrack(log);

    if (outMessage)
        *outMessage = QStringLiteral("ok (manual alarm created for dds)");
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
    m_alarmLogic.noAlarmGroupId = settings.value(QStringLiteral("AlarmLogic/NoAlarmGroupId"), 1).toInt();
    m_alarmLogic.fuseMapRequireContain = settings.value("AlarmLogic/FuseMapRequireContain", 1).toInt();
    m_alarmLogic.trackAlreadyHasAlarmWindowMs = settings.value("AlarmLogic/TrackAlreadyHasAlarmWindowMs", 60000).toInt();
    m_alarmLogic.speedDoubleCheckFuseTrackOnly = settings.value("AlarmLogic/SpeedDoubleCheckFuseTrackOnly", 1).toInt();
    m_alarmLogic.defaultThreatScore = settings.value("AlarmLogic/DefaultThreatScore", 90).toInt();

    const bool grpcEnabled = settings.value(QStringLiteral("GrpcAlarm/Enabled"), 1).toInt() != 0;
    const QString grpcHost = settings.value(QStringLiteral("GrpcAlarm/Host"), QStringLiteral("192.168.18.141")).toString();
    const int grpcPort = settings.value(QStringLiteral("GrpcAlarm/Port"), 25051).toInt();
    const QString grpcProducer = settings.value(QStringLiteral("GrpcAlarm/ProducerId"), QStringLiteral("AlarmSys-linux")).toString();
    m_alarmGrpcClient->configure(grpcHost.toStdString(), grpcPort, grpcEnabled, grpcProducer.toStdString());
    qInfo() << "GrpcAlarm 配置: enabled=" << grpcEnabled
            << "host=" << grpcHost << "port=" << grpcPort;

    m_dbInitSuccess = false;
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



