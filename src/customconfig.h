#pragma once

#include <QMap>
#include <QMutex>
#include <QSet>
#include <QReadWriteLock>
#include <QVector>
#include <string>
#include <iostream>
#include <fstream>
#include <QQueue>
#include <QNetworkReply>
#include <QSystemSemaphore>
#include <QObject>
#include <memory>

#include "SPxLibData/SPxPackets.h"

#include "db/DataAccessLayer.h"
#include "datastruct/commonStruct.h"

#include "fastdds_alarmevent/AlarmEvent.hpp"
#include "fastdds_alarmevent/AlarmEventPublisherApp.hpp"
#include "fastdds_alarmevent/AlarmEventSubscriberApp.hpp"
#include "fastdds_track/TrackClassSubscriberApp.hpp"

class AlarmGrpcSnapshotClient;

class FastddsMsgPublisherApp;
class FastddsRecognitionPublisherApp;
class NewTrackStructAlarmPublisherApp;
class NewTrackStructSubscriberApp;

using namespace std;
using namespace casia::event;
using namespace casia::event::alarm;

#define CHANNEL_TASK_CNT 15
#define TIMEOUT_MSEC 5000
const QString LOGIN_DRONE_PLATFORM_URL_FMT = "http://%1:%2/manage/api/v1/login";
const QString SPOT_FLIGHT_URL_FMT = "http://%1:%2/api4third/control/api/v1/devices/jobs/fly-to-point";
const QString SPOT_FLIGHT_UPDATE_URL_FMT = "http://%1:%2/api4third/control/api/v1/devices/jobs/fly-to-point";
const QString ROUTE_FLIGHT_URL_FMT = "http://%1:%2/api4third/control/api/v1/devices/jobs/fly-to-points-update";
const QString FINISH_FLIGHT_URL_FMT = "http://%1:%2/api4third/control/api/v1/devices/jobs/fly-to-point-stop";
const QString BACK_FLIGHT_URL_FMT = "http://%1:%2/api4third/control/api/v1/devices/jobs/return-home";
const QString GIMBAL_RESET_URL_FMT = "http://%1:%2/api4third/control/api/v1/devices/payload/commands";
const QString START_LIVE_STREAM_FMT = "http://%1:%2/api4third/manage/api/v1/live/streams/start";
const QString STOP_LIVE_STREAM_FMT = "http://%1:%2/api4third/manage/api/v1/live/streams/stop";
const QString SET_VIDEO_QUALITY_FMT = "http://%1:%2/api4third/manage/api/v1/live/streams/update";
const QString SWITCH_VIDEO_CAMERA_FMT = "http://%1:%2/api4third/manage/api/v1/live/streams/switch";
const QString UAV_CTRL_CONNECT_FMT = "http://%1:%2/api4third/control/api/v1/workspaces/drc/connect";
const QString UAV_CTRL_ENTER_FMT = "http://%1:%2/api4third/control/api/v1/workspaces/drc/enter";
const QString UAV_CTRL_EXIT_FMT = "http://%1:%2/api4third/control/api/v1/workspaces/drc/exit";
const QString UAV_CTRL_LIGHT_FMT = "http://%1:%2/api4third/control/api/v1/payload/psdk/commands";
const QString UAV_CTRL_PAYLOAD_FMT = "http://%1:%2/api4third/control/api/v1/payload/psdk/commands";
const QString UAV_CTRL_UPLOADAUDIO_FMT = "http://%1:%2/api4third/control/api/v1/payload/psdk/upload-audio";

const QString VLM_JUDGE_FMT = "http://%1:%2/v1";
const QString TASK_START_FMT = "http://%1:%2/api/v1/tasks";
const QString TASK_STOP_FMT = "http://%1:%2/api/v1/tasks/stop";

const QString COLLECT_INIT_FMT = "http://%1:%2/api/channels";
const QString COLLECT_START_FMT = "http://%1:%2/api/collection/start";
const QString COLLECT_STOP_FMT = "http://%1:%2/api/collection/stop";
const QString COLLECT_MODIFY_FMT = "http://%1:%2/api/eo/detection/update";
const QString COLLECT_WEATHER_FMT = "http://%1:%2/api/weather";

const QString UAV_VIDEO_DETECT1_FMT = "http://%1:%2/api/v1/tasks";
const QString UAV_VIDEO_DETECT2_FMT = "http://%1:%2/api/v1/tasks";
const QString UAV_VIDEO_DETECT3_FMT = "http://%1:%2/api/v1/tasks";

const QString AUDIO_TRANS_FMT = "http://%1:%2/asr_interface_llm";
const QString AUDIO_TRANS_WITH_IDENTIFY_FMT= "http://%1:%2/asr_interface_with_identify";
const QString VIDEO_TRANS_FMT = "http://%1:%2/video_synthesis";
const QString ALI_AUDIO_TRANS_FMT = "http://%1:%2/asr";
const QString AUDIO_CHAT_FMT = "http://%1:%2/api/v1/smart-chat/stream";

const QString SHIP_FILE_FMT = "http://%1:%2/start-collection";
const QString SHIP_FILE_STATUS_FMT = "http://%1:%2/task-status";

enum CAMERA_COMMAND {
    GIMBAL_RESET,
    CAMERA_MODE_SWITCH,
    CAMERA_PHOTO_TAKE,
    CAMERA_RECORDING_START,
    CAMERA_RECORDING_STOP,
    CAMERA_FOCAL_LENGTH_SET,
    CAMERA_AIM,
    CAMERA_LOOKAT,
    CAMERA_SCREEN_SPLIT
};

static QStringList camera_command_list = {
    "gimbal_reset",
    "camera_mode_switch",
    "camera_photo_take",
    "camera_recording_start",
    "camera_recording_stop",
    "camera_focal_length_set",
    "camera_aim",
    "camera_look_at",
    "camera_screen_split"
};

enum RESET_MODE {
    RESET_MODE0,
    RECENTER,//回中
    DOWN,//向下
    RECENTER_PAN,//横轴不动，纵轴回中
    PITCH_DOWN//横轴不动，纵轴向下
};

enum CAMERA_MODE {
    CAMERA_MODE0,
    PHOTO,//拍照
    VIDEO//录像
};

enum CAMERA_TYPE {
    CAMERA_TYPE0,
    ZOOM,//变焦
    WIDE,//广角
    IR//红外
};


static QStringList reset_mode_list = {
    "",
    "回中",
    "向下",
    "仅纵轴回中",
    "仅纵轴向下"
};

static QStringList camera_mode_list = {
    "",
    "拍照",
    "录像"
};

static QStringList camera_type_list_cn = {
    "",
    "变焦",
    "广角",
    "红外"
};

static QStringList camera_type_list_en = {
    "zoom",
    "wide",
    "ir"
};

enum TASK_TYPE {
    SEND,
    PREPARE,
    EXECUTE,
    BACK,
    BACKOVER,
    FAILED,
    OVERTIME
};

enum CMD_TYPE {
    SEND_SUCCESS,
    SEND_FAILED,
    SEND_TIMEOUT,
    DONE
};

static const char* task_type_list[7] = {
    "任务已发送",
    "任务准备中",
    "任务执行中",
    "正在返航",
    "在舱",
    "任务发送失败",
    "任务发送超时"
};

static const char* cmd_type_list[4] = {
    "指令发送成功",
    "指令发送失败",
    "指令发送超时",
    "指令已执行"
};

enum UAV_TASK_INDEX {
    SPOT_INDEX,
    WAYLINE_INDEX,
    UNKNOWN_INDEX
};

static const char* taskName[3] = {
    "定点飞行",
    "航线飞行",
    "未知"
};

static QStringList uavMode = {
    "待机",
    "起飞准备",
    "起飞准备完毕",
    "手动飞行",
    "自动起飞",
    "航线飞行",
    "全景拍照",
    "智能跟随",
    "ADS-B 躲避",
    "自动返航",
    "自动降落",
    "强制降落",
    "三桨叶降落",
    "升级中",
    "未连接",
    "APAS",
    "虚拟摇杆状态",
    "指令飞行",
    "空中 RTK 收敛模式",
    "机场选址中"
};

static QStringList airportMode = {
    "作业准备中",
    "飞行作业中",
    "作业后状态恢复",
    "自定义飞行区更新中",
    "地形障碍物更新中",
    "任务空闲"
};

static QStringList rainfallCode = {
    "无雨",
    "小雨",
    "中雨",
    "大雨"
};

enum CONTROL_TYPE {
    FLIGHT_CTL,//飞行控制
    CAMERA_CTL//相机控制
};

//无人机位置信息标志位
enum DRONE_POSITION_TYPE {
    DRONE_POSITION_TYPE0,
    START_POS,//第一次发送位置信息
    CONTINUOUS_POS,//第二次及连续发送位置信息
    FINISH_POS//最后一次发送位置信息
};

class CustomConfig:public QObject
{
    Q_OBJECT
private:
    CustomConfig();
    static CustomConfig* instance;
    static QMutex mutex;

public:
    BasicConfig m_struBasicConfig;   //配置-基础配置
    AlarmLogicConfig m_alarmLogic;   //配置-航迹告警逻辑（Config.ini [AlarmLogic]）
    OtherConfig m_struOtherConfig;//配置-其他配置
    UrlConfig m_struUrlConfig{};
    int m_nRatio; //0-1080；1-2160
    QVector<RadarAttr> m_vRadarConfig;    //配置-雷达
    QVector<CameraAttr> m_vCameraConfig;   //配置-PTZ相机
    QVector<ReferencePointAttr> m_vRefPoint;   //配置-餐点点
    QVector<PtzConfAttr> m_vPtzConfig;   //配置-PTZ视频
    PtzConfAttr WildCamConfAttr;   //配置-广角相机
    QVector<PlatformAttr> m_vPlatfromConfig;
    QVector<PIDConfig> m_vPIDConfig;
    QVector<AimConfig> m_vAimConfig;

    QMap<int, int> m_mapShowView;
    int m_nUavCurMainIndex;
    QNetworkAccessManager* m_gNetworkManager;

    const int m_nBirdType = 1;
    const int m_nPlaneType = 2;
    QStringList m_listRange;    //量程
    QStringList m_listBowOrient;  //船艏向
    QStringList m_listTrackCommon;  //航迹-通用
    QStringList m_listTrackMerge;   //航迹-融合
    QStringList m_listTrackAIS;  //航迹-AIS
    QStringList m_listAlarmTbl;  //航迹-告警
    QStringList m_listEquipTbl;  //采集-设备
    QStringList m_listChannelTbl;  //采集-设备
    QStringList m_listTemplateTbl;  //采集-模板
    QStringList m_listHistoryTbl;  //采集-历史任务
    QMap<int, SPxPacketTrackExtended> m_mapRadarTrack;
    QMap<int, SPxPacketTrackExtended> m_mapMillRadarTrack;
    QMap<int, SPxPacketTrackExtended> m_mapFuseTrack;
    QMap<int, SPxPacketTrackExtended> m_mapBirdFuseTrack;
    QMap<int, SPxPacketTrackExtended> m_mapBirdRadarTrack;
    QMap<int, TrackName> m_mapTrackName;
    bool m_isSelect = false;
    QVector<TargetImagInfo> m_vTarget;
    int m_nCurTrackType = 0; //0-融合，1-雷达
    int m_nSeaShipCnt = 0;
    int m_nFlyingCnt = 0;
    int m_nBirdsCnt = 0;
    int m_nUavCnt = 0;


    //告警数据
    QMap<QString, AlarmRule> m_mapAlarmRule; //告警规则
    /** 激活 scheme 下（经 area_identification_rules 过滤）rule_id -> 子表条件（alarm_identification_rules_sub） */
    QMap<QString, QList<AlarmIdentificationRuleSub>> m_mapAlarmIdentificationRulesSub;
    /** "group_id_area_id" -> 该区域绑定的识别 rule_id 列表（不同 rule 鉴定为并集） */
    QMap<QString, QStringList> m_mapAreaKeyToIdentificationRuleIds;
    /** detection_rules 鉴定首次 VERIFY_SUCCESS 的 resolvedTime（按 alarm_id，仅写一次） */
    QMap<QString, QString> m_mapAlarmIdentificationResolvedTime;
    /** HTTP 确认告警：unique_id 集合，SendAllAlarmEventMsg 直接置 VERIFY_SUCCESS */
    QSet<qint64> m_setManualConfirmedUniqueIds;
    /** 人工确认 resolvedTime（按 unique_id；目标尚未进 m_mapAlarmData 时预置） */
    QMap<qint64, QString> m_mapManualConfirmResolvedTimeByUniqueId;
    /** 前端手动确认告警（不在规则 m_mapAlarmData 中），SendAllAlarmEventMsg 发布 */
    QMap<QString, AlarmData> m_mapManualAlarmData;
    /** manual 告警 unique_id -> 是否对空航迹（决定 TrackType::BIRD / FUSE） */
    QMap<qint64, bool> m_mapManualAlarmAirTrack;
    QMap<QString, AlarmData> m_mapAlarmData; //告警数据
    /** condition_id + "_" + track_id -> 最近一次触发告警的毫秒时间戳；持续告警判定用，避免 m_mapAlarmData 被提前清理后查不到 */
    QMap<QString, qint64> m_mapConditionTrackLastAlarmMs;
    QList<ThreatAssessmentParams> m_listThreatAssessmentParams; //威胁度研判参数
    QMap<QString, QPair<int, int>> m_mapSchemeProtectAreas; //方案保护区信息 <"groupId_areaId", <protectGroupId, protectAreaId>>
    QMap<int, TargetInfoFilter> m_mapTargetInfoFilter; //目标信息过滤
    QString m_nCurCheckAlarmID = ""; //重点关注告警id
    QString m_nCurAlarmID = ""; //当前告警id
    QMap<QString, QString> m_mapAlarmDescription; //告警详细信息
    QMutex m_alarmDataMutex;
    QMutex m_alarmFilterMutex;
    QList<QPair<int, int>> m_listAlarmFilter; // (type, trackid) type: 0-对海 1-对空，收到 api/alarm_filter 后不发送该 trackid 的对应类型告警
    QMutex m_vTargetPicMutex;
    bool m_useCheckAlarmCamera = false;
    bool m_useCameraCheckTrack = false;
    bool m_bDoImportantTrack = false;
    int m_nAutoCheckMode = 1;
    int m_nAutoWarningCheckMode = 1;
    QMap<QString, QList<AlarmData>>m_mapAreaAlarmData;//区域告警数据
    void* m_pViewControl = nullptr; /* 无 UI：航迹告警使用 alarm_geoproj 平面投影，不依赖 SPxViewControl */

    QVector<QMap<int, SPxPacketTrackExtended>> m_vecFuseTrack;
    MyMarkerArray m_markerArray;  //激光雷达结构体
    QMap<int, AISTrack> m_mapAISTrack;

    QMap<int, QList<QPointF>>m_mapFuseTrail;
    QMap<int, QList<QPointF>>m_mapBirdFuseTrail;
    QMap<int, QList<QPointF>>m_mapRadarTrail;
    QMap<int, QList<QPointF>>m_mapAISTrail;
    QMap<int, QList<QPointF>>m_mapBirdRadarTrail;
    // 航迹/轨迹容器跨线程共享：订阅线程写、告警线程读
    mutable QReadWriteLock m_trackDataLock;

    QMap<int, int>m_mapRadarTrackClass;
    QMap<int, int>m_mapFuseTrackClass;
    QMap<int, int>m_mapSelfPoseTrackClass;
    QMap<int, int>m_mapSelfPoseAirTrackClass;
    int m_selectTrackId = 0;//选中航迹ID
    int m_trackType;//1-雷达航迹，2-AIS航迹，3-融合航迹,4-船只自保位航迹，5-无人机自保位
    int m_nSytleMode; //0-日间 ，1-夜间
    QString sourceName;

    string valS(string paramName);
    int valI(string paramName);
    float valF(string paramName);

    //UDPmanage* m_rectUDP[16] = { nullptr };
    //UDPmanage* m_singleRectUDP[16] = { nullptr };

    int m_nLastMMSI = 0; //最近一次查询的mmsi号
    bool m_nUavInit = false;
    QString m_strWeatherInfo;

    QMap<int, void*> g_mapPtzWidget{};

    //数据库接口
    DataAccessLayer& dbHelper = DataAccessLayer::getInstance();
    bool m_dbInitSuccess = false;
    bool m_bFastDDSInitialized = false; // 标记FastDDS是否成功初始化
    QList<TargetInfo> m_listTargetInfo;
    QMap<int, QString> m_sensorInfoMap;

    FastddsMsgPublisherApp* m_fastddsPublisher = nullptr;
    //告警信息发送
    AlarmEventPublisherApp* m_alarmevnetPublisherMulti = nullptr;
    AlarmEventPublisherApp* m_alarmevnetPublisherSingle = nullptr;
    AlarmEventSubscriberApp* m_alarmeventSubscriber = nullptr;
    NewTrackStructAlarmPublisherApp* m_newTrackStructAlarmPublisherMulti = nullptr;
    NewTrackStructAlarmPublisherApp* m_newTrackStructAlarmPublisherSingle = nullptr;
    NewTrackStructSubscriberApp* m_newTrackStructSubscriber = nullptr;
    TrackClassSubscriberApp* m_trackSubscriber = nullptr;
    FastddsRecognitionPublisherApp* m_recognitionPublisher = nullptr;

    std::unique_ptr<AlarmGrpcSnapshotClient> m_alarmGrpcClient;
    qint64 m_lastAlarmGrpcPushMs = 0;

    //告警条件列表
    QList<AlarmSettingInfo> m_alarmCondition;
    QList<AreaInfo> m_alarmArea;

    /** 目标类型（HTTP 等写入），用于 SendAllAlarmEventMsg 补发 */
    QMap<QString, int> m_mapTargetType;

public:
    static CustomConfig* getInstance() {

        mutex.lock();
        if (instance == nullptr) {
            instance = new CustomConfig();
        }
        mutex.unlock();
        return instance;
    }

    ~CustomConfig();

    void LoadConfig();
    void SaveConfig();

    void setConfigPath(string path);
    void SetLog(string logMes);
    void setConfig(string name, string value);

    //fastdds
    void InitFastdds();
    void CleanupDDSResources(int domainId);
    void DestoryFastdds();
    bool IsFastDDSInitialized() const { return m_bFastDDSInitialized; }
    /**
     * 通过 Fast DDS 字符串 topic 向其它模块发控制消息（原 Windows 工程依赖 FastddsMsgPublisherApp）。
     * 当前 Linux 无头版未链接该发布者：此函数为刻意空操作，仅打日志。
     * 若需启用：在 CMake 编入 fastdds/FastddsMsg*.cxx、在 InitFastdds 中创建 m_fastddsPublisher，并在此调用 send()。
     */
    void SendMsg(QString msg);
    void SendNewAlarm(QString alarmid);
    void SendAllAlarmEventMsg();
    /** 有告警时每 1s 向 TrackManager 推送 gRPC 告警快照（UpdateAlarmSnapshot） */
    void tickAlarmGrpcSnapshot();
    void SendNewTrackStructAlarmMsg(const AlarmEvent* alarmEvent = nullptr);
    /** true：完整组装并发布（SendAllAlarmEventMsg 全逻辑）；false：仅向 DDS 发空 AlarmEvent，不做全量告警组装 */
    bool alarmEventPushFullEnabled() const;
    void setAlarmEventPushFullEnabled(bool on);
    void addAlarmFilter(int type, int trackid); // type: 0-对海 1-对空
    /** 人工确认告警：按 unique_id 标记，后续 SendAllAlarmEventMsg 直接 VERIFY_SUCCESS */
    bool confirmAlarmByUniqueId(qint64 uniqueId, QString* outMessage = nullptr);
    /** 规则告警：task_status + 鉴定/人工确认升级（DDS 与 gRPC 共用） */
    AlarmEffectiveDisposition resolveRuleAlarmDisposition(const AlarmData& alarmData);
    void setTargetType(const QString& id, int type);

    // 威胁度参数相关方法
    ThreatAssessmentParams getThreatAssessmentParams(int groupId, int areaId);

signals:
    void sig_initfastddsFinished();
private:
    mutable QMutex m_alarmEventPushModeMutex;
    bool m_bAlarmEventPushFullEnabled = true;
    string m_configPath;
    QMap<string, string> m_configMap;
};

