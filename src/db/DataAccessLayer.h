// DataAccessLayer.h

#ifndef DATAACCESSLAYER_H
#define DATAACCESSLAYER_H

#include <QObject>
#include <QList>
#include <QCryptographicHash>
#include <QDateTime>
#include "DatabaseManager.h"
#include "datastruct/commonStruct.h"

/*
 *
 * 1、同步使用
 * DataAccessLayer& dal = DataAccessLayer::getInstance();
 * bool success = dal.deleteTarget(123);
 *
 * 2、异步使用
 * QFuture<bool> future = dal.deleteTargetAsync(123);
 * // 做其他工作...
 * bool success = future.result(); // 在需要结果时等待
 *
 * 3、使用 QFutureWatcher 来处理异步结果
 * QFutureWatcher<bool>* watcher = new QFutureWatcher<bool>();
 * QObject::connect(watcher, &QFutureWatcher<bool>::finished, [=]() {
 *   bool success = watcher->result();
 *   // 处理结果
 *   watcher->deleteLater();
 * });
 * watcher->setFuture(dal.deleteTargetAsync(123));
 * */

class DataAccessLayer : public QObject
{
    Q_OBJECT
public:
    static DataAccessLayer& getInstance();
    bool initDatabase();
    DbInfo loadDbParams();

    // 1. 目标基本信息接口
    QFuture<QList<TargetInfo>> getAllTargetInfoAsync();
    QList<TargetInfo> getAllTargetInfo();

    QFuture<bool> addTargetInfoAsync(const TargetInfo& info);
    bool addTargetInfo(const TargetInfo& info);

    QFuture<TargetInfo> getTargetInfoAsync(qint64 targetId);
    TargetInfo getTargetInfo(qint64 targetId);

    QFuture<bool> updateTargetInfoAsync(const TargetInfo& info);
    bool updateTargetInfo(const TargetInfo& info);

    bool updateCameraRoot(int camera_id, int root_pos);

    QFuture<bool> deleteTargetAsync(qint64 targetId);
    bool deleteTarget(qint64 targetId);

    // QFuture<bool> mergeTargetsAsync(const QList<qint64>& targetIdsToMerge, const TargetInfo& newTargetInfo);
    bool mergeTargets(const QList<qint64>& targetIdsToMerge, const TargetInfo& newTargetInfo);

    static qint64 generateUniqueTargetId();//生成目标唯一ID

    // 2. 目标关联信息接口
    QFuture<bool> addTargetAssociationAsync(const TargetAssociation& association);
    bool addTargetAssociation(const TargetAssociation& association);

    QFuture<QList<TargetAssociation>> getTargetAssociationsAsync(qint64 targetId);
    QList<TargetAssociation> getTargetAssociations(qint64 targetId);

    QFuture<TargetAssociation> getLatestTargetAssociationAsync(qint64 targetId);
    TargetAssociation getLatestTargetAssociation(qint64 targetId);

    QFuture<QMap<qint64, TargetAssociation>> getAllLatestTargetAssociationsAsync();
    QMap<qint64, TargetAssociation> getAllLatestTargetAssociations();

    // 3. 目标采集信息接口
    QFuture<bool> addTargetCollectionAsync(const TargetCollection& collection);
    bool addTargetCollection(const TargetCollection& collection);

    QFuture<QList<TargetCollection>> getTargetCollectionsAsync(qint64 targetId);
    QList<TargetCollection> getTargetCollections(qint64 targetId);

    // 4. 传感器信息接口
    QFuture<bool> addSensorInfoAsync(const SensorInfo& sensor);
    bool addSensorInfo(const SensorInfo& sensor);

    QFuture<SensorInfo> getSensorInfoAsync(int sensorId);
    SensorInfo getSensorInfo(int sensorId);

    int getCameraRootPos(int camera_id);

    QFuture<bool> updateSensorInfoAsync(const SensorInfo& sensor);
    bool updateSensorInfo(const SensorInfo& sensor);

    QFuture<QMap<int, QString>> getAllSensorInfoAsync();
    QMap<int, QString> getAllSensorInfo();
    QFuture<void> loadSensorInfoAsync();

    QMap<int, int> getAllCameraRootPos();

    void loadSensorInfo();
    // 5. 记录信息接口 异步
    QFuture<int> addAISRecordsAsync(const QList<AISRecord>& records);
    QFuture<int> addADSBRecordsAsync(const QList<ADSBRecord>& records);
    QFuture<int> addTrackRecordsAsync(const QList<TrackRecord>& records);
    QFuture<int> addFusedTrackRecordsAsync(const QList<FusedTrackRecord>& records);

    // 6. 雷达回波和光电采集接口
    QFuture<bool> addRadarEchoCollectionsAsync(const QList<RadarEchoCollection>& collections);
    bool addRadarEchoCollections(const QList<RadarEchoCollection>& collections);

    QFuture<bool> addVideoCollectionsAsync(const QList<VideoCollection>& collections);
    bool addVideoCollections(const QList<VideoCollection>& collections);

    QFuture<bool> addSonarCollectionsAsync(const QList<SonarCollection>& collections);
    bool addSonarCollections(const QList<SonarCollection>& collections);

    QFuture<bool> addPicDictAsync(const QList<PicCollection>& collections);
    bool addPicDict(const QList<PicCollection>& collections);

    //7.获取采集记录接口
    QList<RadarEchoCollection> getRadarEchoCollections(qint64 targetId);
    QList<VideoCollection> getVisibleLightVideoCollections(qint64 targetId);
    QList<VideoCollection> getInfraredVideoCollections(qint64 targetId);
    QList<VideoCollection> getVideoCollectionsByCameraType(qint64 targetId, int cameraType);
    QList<SonarCollection> getSonarCollections(qint64 targetId);
    QList<AISCollectionRecord> getAISCollections(qint64 targetId);
    QList<TrackCollectionRecord> getTrackCollections(qint64 targetId);
    QList<ADSBCollectionRecord> getADSBCollections(qint64 targetId);
    QList<ShipFileInfo> getShipFileCollections(qint64 targetId);
    //8.删除采集记录接口
    bool deleteRadarEchoRecord(const QUuid& collectionId, int sequenceNumber);// 删除雷达回波记录
    bool deleteVideoRecord(const QUuid& collectionId, int sequenceNumber, int cameraType); // 删除光电记录（可见光和红外）
    bool deleteSonarRecord(const QUuid& collectionId, int sequenceNumber);  // 删除水声记录
    bool deleteAISRecord(const QUuid& collectionId, int sequenceNumber);// 删除AIS记录
    bool deleteTrackRecords(const QUuid& collectionId);// 删除航迹记录（删除所有相关数据）
    bool deleteADSBRecord(const QUuid& collectionId, int sequenceNumber);// 删除ADSB记录


    // 9. 获取目标采集统计信息
    CaptureTargetItem getTargetCaptureInfo(qint64 targetId);
    QMap<qint64, CaptureTargetItem> getAllTargetCaptureInfo();

    //10 获取目标最近一次记录的航迹信息
    TargetRealtimeInfo getTargetRealtimeInfo(qint64 targetId);//从数据库获取的实际是历史信息
    //11 字典表获取接口
    QMap<int, QString> getTargetCategories();
    QMap<int, QString> getModalityTypes();
    QMap<int, QString> getCollectionMethods();
    QMap<int, QString> getBehaviorTypes();
    QStringList getBehaviorTypes2();
    QStringList getDictTargetTypes();
    QMap<QString, int> getDictSeaStateTypes();
    QMap<QString, int> getDictWeatherTypes();
    QMap<QString, int> getDictVisible();
    QMap<QString, int> getDictPrecipitation();
    QMap<QString, QString>getDictRouteFlight();
    QMap<QString, int> getDictRouteFlightID();

    QString getSensorName(const int& sensorId);

    QFuture<void> loadDictionariesAsync();
    void loadDictionaries();

    QString convertToDegreeMinute(double coordinate);

    bool addCollectionTaskHistoryInfo(const CollectionTaskHistoryInfo& info);
    QList<CollectionTaskHistoryInfo> getAllCollectionTaskHistoryInfo();

    ShipFileInfo getShipFileInfo(QString mmsi);

    //告警相关
    QList<AlarmSettingInfo> getAlarmSetting();
    QList<AlarmAreaInfo> getAlarmAreaInfo();
    QMap<QString, AlarmRule> getAlarmRule();
    /**
     * 子表 alarm_identification_rules_sub：通过 area_identification_rules + 当前激活 scheme 过滤 rule_id；
     * key 为 rule_id，value 为该规则下全部条件（已按 criteria_order、id 排序）。
     */
    QMap<QString, QList<AlarmIdentificationRuleSub>> getAlarmIdentificationRulesSub();
    /** minio_multi_metadata：是否存在 unique_id + file_name LIKE 的记录（用于 detection_rules.optic） */
    bool hasMinioMultiMetadataForUniqueId(qint64 uniqueId, const QString& fileNameLikePattern);
    /** 当前激活 scheme 下 area_identification_rules：key 为 "group_id_area_id"，value 为去重后的 rule_id 列表（用于规则并集） */
    QMap<QString, QStringList> getIdentificationRuleIdsByAreaForActiveScheme();
    /** 当前激活 scheme 下 area_verification_rules：key 为 "group_id_area_id"，value 为查证 rule_id 列表 */
    QMap<QString, QStringList> getVerificationRuleIdsByAreaForActiveScheme();
    /** 当前激活 scheme 下 area_handle_rules：key 为 "group_id_area_id" */
    QMap<QString, QList<AreaHandleRuleBinding>> getAreaHandleRulesByAreaForActiveScheme();
    QMap<int, TargetInfoFilter> getTargetInfoFilter();
    QMap<QString, AlarmData> getAlarmData();
    QMap<QString, AlarmData> getAlarmData(const QDateTime& starttime, int level);
    bool updateAlarmStatus(QString id, int status);
    bool updateAlarmData(QString id, AlarmData data);
    void addAlarmData(AlarmData data);
    /** system_evaluation_data：新威胁时写入 threat_time（与 addAlarmData 解耦，可单独调用） */
    void saveSystemEvaluationThreatOnce(const AlarmData& data);
    /** system_evaluation_data：首次查证成功时写入 alarm_time（毫秒）；无行则插入，已有 alarm_time 则不覆盖 */
    bool setSystemEvaluationAlarmTimeOnce(const QString& alarmId, qint64 uniqueId, qint64 threatTimeMs, qint64 alarmTimeMs);
    QString getTargetType(int id);

    // 检测类型结果结构体
    struct DetectionTypeResult {
        QString finalTargetType;  //最终目标类型
        QString llmTargetType;    // LLM目标类型
        QString camTargetType;    // 相机目标类型 
        QString uavTargetType;    // 无人机目标类型
        int finalThreatLevel;     // 最终威胁度
        bool found;               // 是否找到数据

        DetectionTypeResult() : finalThreatLevel(0), found(false) {}
    };

    // 根据reid获取检测类型信息
    DetectionTypeResult getDetectionTypesByReId(qint64 reId);

    QList<ThreatAssessmentParams> getThreatAssessmentParams();
    ThreatAssessmentParams getThreatAssessmentParams(int groupId, int areaId);
    ThreatAssessmentParams getThreatAssessmentParams(int groupId, int areaId, const QString& schemeId);
    QString getActiveSchemeId(); // 获取当前活跃的方案ID
    QMap<QString, QPair<int, int>> getActiveSchemeProtectAreas(); // 获取当前活跃方案的保护区信息 <areaKey, <protectGroupID, protectAreaID>>
    bool saveThreatAssessmentParams(const ThreatAssessmentParams& params);

    //QFuture<bool> addAlarmPicAsync(const QList<AlarmPicInfo>& collections);
    //bool addAlarmPic(const QList<AlarmPicInfo>& collections);

    //无人机喊话
    QList<UavSpeakerText> ReadAllUavTextMsg();
    bool insertUavTextMsg(UavSpeakerText text);
    bool updateUavTextMsg(UavSpeakerText text);
    bool deleteUavTextMsg(UavSpeakerText text);

    WeatherInfoFromDB getWeatherInfo();

    // 获取当天的天气信息
    struct TodayWeatherInfo {
        QString description;
        QString info;
    };
    TodayWeatherInfo getTodayWeatherInfo();

    QVector<QByteArray> getShipPicture(unsigned int mmsi);
    void addAlarmSetting(AlarmRule info);
    void deleteAlarmSetting(QString contidionid);
    void updateAlarmSetting(AlarmRule collection);
    void saveAlarmSetting(QList< AlarmRule> info);
    void deleteAlarmSetting();
    AreaInfo getAlarmArea(int groupID, int areaID);
    QList<AreaInfo> getGroupArea(int groupID);
    QList<AreaInfo> getAreaInfo();

    QList<AlarmPicInfo> getAlarmPicCollections(QString alarmId);
    QFuture<bool> addAlarmPicAsync(const QList<AlarmPicInfo>& collections);

    bool addAlarmPic(const QList<AlarmPicInfo>& collections);

    bool addCognitiveResult(const CognitiveResultsComprehensive& result);
    bool updateCognitiveResult(const CognitiveResultsComprehensive& result);
    bool saveCognitiveResult(CognitiveResultsComprehensive result);
    QString sourceEnumToString(SourceEnum source);
    QString affiliationEnumToString(AffiliationEnum affiliation);
    SourceEnum stringToSourceEnum(const QString& source);
    AffiliationEnum stringToAffiliationEnum(const QString& affiliation);
private:
    DataAccessLayer(QObject* parent = nullptr);
    ~DataAccessLayer();

    DatabaseManager& m_dbManager;

    // 缓存字典数据
    QMap<int, QString> m_targetCategories;
    QMap<int, QString> m_modalityTypes;
    QMap<int, QString> m_collectionMethods;
    QMap<int, QString> m_behaviorTypes;
    QMap<int, QString> m_sensorInfo;

    QVector<QString> m_behaviorTypes2;
    QMap<QString, int> m_dictSeaState;
    QVector<QString> m_dictTargetTypes;
    QMap<QString, int> m_dictWeatherTypes;
    QMap<QString, int> m_dictVisible;
    QMap<QString, int> m_dictPrecipitation;
    QMap<QString, QString> m_dictRouteFlight;/*航线名称--航线包含的所有轨迹点*/
    QMap<QString, int> m_dictRouteFlightID;/*航线名称--航线ID*/

    QMutex m_dictionariesMutex;
    QMutex m_sensorInfoMutex;

    // 删除拷贝构造函数和赋值运算符
    DataAccessLayer(const DataAccessLayer&) = delete;
    DataAccessLayer& operator=(const DataAccessLayer&) = delete;
};

#endif // DATAACCESSLAYER_H
