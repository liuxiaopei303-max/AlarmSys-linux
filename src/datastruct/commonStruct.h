#pragma once
#include <QString>
#include <QVector>
#include <QStringList>
#include"QPixmap"
#include <QDateTime>
#include <QUuid>
#include <QHash>
#include <QList>
#include <QSet>
// #include "udpmanage.h"  // 文件不存在，已移除
#ifndef COMMONSTRUCT_H
#define COMMONSTRUCT_H

#endif // COMMONSTRUCT_H

#define SUCCESSINFO "正常"
#define FAILURE "异常"
#define TIPS "提示"
#define REFRESHCHANNEL "已更新最新的信道信息"
#define STARTCOLLECT "MRDS系统已开始采集数据"
#define COLLECTWEATHER "成功采集天气数据"
#define COLLECTWEATHERFAILED "天气传感器采集失败，请手动填写完整"
#define MQTTCONNECTFAILED "mqtt服务器断开连接，正在尝试重连"

//任务的类型
enum COLLECT_TASK_TYPE {
    OVER,
    DOING,
    STOPING,
    UPDATING,
    TIMEOUT,
    UNKNOWN
};

//任务的状态
static const char* COLLECT_TASK_STATE[6] = {
    "任务已结束",
    "正在采集中",
    "采集停止中",
    "正在更新信道列表",
    "任务超时",
    "未知错误"
};

static QStringList alarm_mode_list = {
    "航迹",
    "光电"
};

struct UrlConfig {
    //Uav
    QString LOGIN_DRONE_PLATFORM_URL{};
    QString SPOT_FLIGHT_URL{};
    QString SPOT_FLIGHT_UPDATE_URL{};
    QString ROUTE_FLIGHT_URL{};
    QString FINISH_FLIGHT_URL{};
    QString BACK_FLIGHT_URL{};
    QString GIMBAL_RESET_URL{};
    QString START_LIVE_STREAM{};
    QString STOP_LIVE_STREAM{};
    QString SET_VIDEO_QUALITY{};
    QString SWITCH_VIDEO_CAMERA{};
    QString UAV_CTRL_CONNECT{};
    QString UAV_CTRL_ENTER{};
    QString UAV_CTRL_EXIT{};
    QString UAV_CTRL_PAYLOAD{};
    QString UAV_CTRL_UPLOADAUDIO{};

    //Collect
    QString INIT_URL{};
    QString START_URL{};
    QString STOP_URL{};
    QString MODIFY_URL{};
    QString WEATHER_URL{};
    QString REMOTE_SERVER{};

    //Task
    QString VLM_JUDGE{};
    QString TASK_START{};
    QString TASK_STOP{};

    //UAV Video Detect
    QString UAV_VIDEO_DETECT1{};
    QString UAV_VIDEO_DETECT2{};
    QString UAV_VIDEO_DETECT3{};

    //Audio Trans
    QString AUDIO_TRANS{};
    QString AUDIO_TRANS_WITH_IDENTIFY{};
    QString VIDEO_TRANS{};
    QString ALI_AUDIO_TRANS{};
    QString AUDIO_CHAT{};

    //Ship File
    QString SHIP_FILE{};
    QString SHIP_FILE_STATUS{};
};

/** 航迹告警行为（原 Linux / Windows 硬编码差异统一为 Config.ini [AlarmLogic]） */
struct AlarmLogicConfig {
    /**
     * Config.ini [AlarmLogic]/Mode：与 TrackClassSubscriber 融合主键、鸟情区域 track_type=3 行为联动
     * 0：融合 topic 用 trackId 且仅 id<10000 入库；processAlarms 中 track_type==3 仅处理 reserved1==3
     * 1：融合 topic 用 reserved4 作主键（不做 <10000 过滤）；track_type==3 不做 reserved1 过滤
     */
    int mode = 0;
    int saveAlarmToDb = 0;                 /**< 0=不写 alarm_data，1=写库 */
    int refreshTargetInfoFilterEachLoop = 1; /**< 每轮循环刷新 m_mapTargetInfoFilter，1=原 Linux */
    int alarmMapPruneNonFirstStaleMs = 5000; /**< 内存告警表非首条记录剔除：距 time 超过此毫秒 */
    int dedupSameAlarmWindowMs = 600000;    /**< SaveToDB 同航迹/同规则去重时间窗 */
    int dedupMatchConditionAndTrack = 0;    /**< 0=仅 track_id（原 Linux），1=condition_id+track_id（原 Windows） */
    int originTimeFromTrackMsg = 1;         /**< 1=origintime 用航迹时间（原 Linux），0=用当前时间（原 Windows） */
    int originTrackTimeIsMs = 0;              /**< 与 OriginTimeFromTrackMsg 配合：1=传入 timestamp 为毫秒 epoch，0=秒 epoch（与 msgTimeSecs 一致） */
    int birdRadarSourceId = 9;              /**< 鸟情分支 radarSourceId 判定值 */
    int birdFilterMode = 0;                 /**< 0=传感器文案过滤（birdSelfReportSubstrings），1=fusion.trackID[0] 黑名单 */
    QString birdSelfReportSubstrings = QStringLiteral("自报位"); /**< 逗号分隔，任一子串命中则跳过（birdFilterMode=0） */
    QSet<int> birdSkipTrackIds;             /**< birdFilterMode=1 时跳过这些融合 trackID */
    QSet<int> areaGroupIdAllow;             /**< isTrackInGroupArea 允许的 groupID，空=不限制 */
    QSet<int> noAlarmGroupIds;              /**< 免告警区 group_id 列表，空=关闭免告警区判定 */
    int fuseMapRequireContain = 1;          /**< 1=融合航迹必须在 m_mapFuseTrack 中存在（原 Linux） */
    int trackAlreadyHasAlarmWindowMs = 60000; /**< “持续告警”判定窗口 */
    int alarmFilterTtlMs = 3600000;         /**< 灭告警 filter 存续时间（毫秒），默认 1 小时 */
    int speedDoubleCheckFuseTrackOnly = 1;    /**< 1=仅融合(type==0) 连续两次速度判定（原 Windows） */
    int defaultThreatScore = 90;            /**< 黑名单直告警或未计算威胁度时的默认 threatScore */
};

/** 可疑目标研判与 DDS 发布（Config.ini [SuspiciousTarget]） */
struct SuspiciousTargetConfig {
    int enabled = 0;                       /**< 0=关闭 SuspiciousTargetThread，1=启用 */
    int judgeIntervalMs = 5000;            /**< 研判周期（毫秒），默认 5s */
    double speedLowThresholdMps = 3.0;     /**< 规则1：速度下限（m/s），且与保护区夹角≤ protectAngleMaxDeg */
    double speedHighThresholdMps = 8.0;    /**< 规则2：速度下限（m/s），无需角度条件 */
    double protectAngleMaxDeg = 30.0;      /**< 规则1：航向与指向保护区圆心方位夹角上限（度） */
    int speedCheckClearIntervalMin = 30;   /**< 速度二次确认状态表清空间隔（分钟） */
    QString ddsTopic = QStringLiteral("NewTrackStructSuspicious"); /**< DDS 发布 topic */
};

struct BasicConfig
{
    int m_nStyleMode;  //0-白天 1-夜晚
    int m_nRadarCnt;  //雷达数量，初始默认1
    int m_nCameraCnt;  //光电数量，初始默认1
    int m_nPlatformCnt = 1;
    int m_nRefpCnt;   //参考点数量，初始默认0
    bool m_bEnableWideCam; //环视是否开启 0-关闭，1开启
    bool m_bShipPlatform;//是否是船载平台 0-岸基，1船载
    bool m_bShowFrameRate = false;//是否显示帧率
    bool m_b4K = false;
    int m_nRefreshInterval = 25;//视频刷新速率
    int m_nAutoHandleInterval = 25;//自动处理间隔
    int m_nCameraDecodeType = 1;//0软解码 1硬解码
    int m_nCameraTopRoot = 0;//1相机最高权限
    int m_nUsePtzModel = 0;
    int m_nPPIDDSPort = 103;
    int m_nAlarmEventDDSPort = 199;
    int m_nAlarmEventDDSPortSingle = 146;
    int m_nTrackDDSPort = 141; // 与 track_class 现场域 ID 一致；勿与 MultiTrackDDSPort(相机框 DDS) 混淆，可用 [Basic] TrackDDSPort 覆盖
    int m_nRecognitionDDSPort = 142;
    bool m_bDebugMode = false;
    bool m_bUseRemoteCameraControl = false;//设置是否用远程服务控制相机
    QString m_strControlIP;//远程相机控制服务ip
    int m_nControlPort;//远程相机控制服务端口

    bool m_bUseSingleTrackMeataTask = false;//设置是否使用检测元任务
    QString m_strSingleTrackServerIP = "";//检测http服务ip
    int m_nSingleTrackServerPort = 0;//检测http服务端口
    float m_dCheckOffsetHS = 0.67;//定位后左右转动幅度

    //是否使用dds单播接收
    int m_bUseSingleDDSPort = false;

    int m_nCameraStateDDSPort = 132;
    int m_nCameraMultiTrackRectDDSPort = 122;
    int m_nCameraSingleTrackRectDDSPort = 123;

    //用户id
    QString m_appUserID = "operator";
    //用户权限
    int m_nAppUserpriority = 0;
    bool m_bEnableLaserRadar; //激光雷达是否开启 0-关闭，1开启
    bool m_bEnableMilliwaveRadar; //毫米波雷达是否开启 0-关闭，1开启
    QString m_strVesselName; //船名
    QString m_strAisNum; //AIS编号
    int m_nVesselLength;  //船长度
    int m_nVesselWidth;   //船宽度
    int m_nAlarmCPA;  //告警CPA
    int m_nAlarmTCPA; //告警TCPA
    int m_nAlarmTime; //告警时间
    QString m_strEmailAddr;

    QString m_strADSBIP;//无人机ip
    int m_nADSBPort;//无人机端口

    int m_nGpsInputType; //0-网络 1-串口
    int m_nGpsProto; //定位协议 0-无
    QString m_strGpsIP; //定位IP
    int m_nGpsPort; //定位端口号
    QString m_strGpsSerialPort; //串口号
    int m_nGpsSerialBaud;   //波特率

    int m_nCompInputType; //0-网络 1-串口
    int m_nCompProto; //罗经协议 0-无
    QString m_strCompIP; //罗经IP
    int m_nCompPort; //罗经端口号
    QString m_strCompSerialPort; //串口号
    int m_nCompSerialBaud;   //波特率

    int m_nDeptInputType; //0-网络 1-串口
    int m_nDeptProto; //测深协议 0-无
    QString m_strDeptIP; //测深IP
    int m_nDeptPort; //测深端口号
    QString m_strDeptSerialPort; //串口号
    int m_nDeptSerialBaud;   //波特率

    int m_nWeatherInputType; //0-网络 1-串口
    int m_nWeatherProto; //气象协议 0-无
    QString m_strWeatherIP; //气象IP
    int m_nWeatherPort; //气象端口号
    QString m_strWeatherSerialPort; //串口号
    int m_nWeatherSerialBaud;   //波特率

    int m_nAISInputType; //0-网络 1-串口
    int m_nAISProto; //气象协议 0-无
    QString m_strAISIP; //气象IP
    int m_nAISPort; //气象端口号
    QString m_strAISSerialPort; //串口号
    int m_nAISSerialBaud;   //波特率

    int m_nMergeInputType; //0-网络 1-串口
    int m_nMergeProto; //气象协议 0-无
    QString m_strMergeIP; //气象IP
    int m_nMergePort; //气象端口号
    QString m_strMergeSerialPort; //串口号
    int m_nMergeSerialBaud;   //波特率

    QString m_strSurroundCam0; //环视
    QString m_strSurroundCam1; //船头
    QString m_strSurroundCam2; //左舷
    QString m_strSurroundCam3; //右舷
    QString m_strSurroundCam4; //船尾

    QString m_strDroneIP; //无人机平台IP
    int m_nDronePort; //无人机平台端口号

    //QString m_strUAVVideoDetectIP; //航拍ip
    //int m_nUAVVideoDetectPort; //航拍端口

    QString m_strRecvSelfPoseIP;
    int m_nRecvSelfPosePort;

    QString m_strHostIP;

    float m_uavLat;
    float m_uavlon;

    //启用基准点
    int m_nUseBasePoint = 0; //0-不启用，1-
    double m_dBasePointLon = 0.0;
    double m_dBasePointLat = 0.0;
    double m_dDifferRange;

    QString m_strAudioTransIp; //老的翻译ip
    int m_nAudioTransPort; //老的翻译端口
    int m_nAudioTransWithIdentifyPort; //新的翻译端口
    QString m_strAliAudioTransIp; //新的翻译ip
    int m_nAliAudioTransPort; //新的翻译端口
    bool m_bUseChatRoute; //是否使用chat路由

    QString m_strVideoTransIp; //新的翻译ip
    int m_nVideoTransPort; //新的翻译端口

    QString m_strWeatherServerIp; //气象服务器ip
    int m_nWeatherServerPort; //气象服务器端口

    QString m_strVlmServerIp; //vlm服务器ip
    int m_nVlmServerPort; //vlm服务器端口
    QString m_strQuestConfig; //vlm问题
    QString m_strPromptConfig; //vlm提示词

    int m_nCollectHostPort; //采集主机端口
    QString m_strCollectServerIp; //采集主机ip
    int m_nCollectServerPort; //采集主机端口

    int m_nTaskHostPort; //任务状态本地端口
    int m_nTaskHandleHostPort; //任务执行本地端口
    QString m_strTaskServerIp; //任务主机ip
    int m_nTaskServerPort; //任务服务端口


    QString m_strUavServerIp; //无人机平台ip
    int m_nUavServerPort; //无人机平台端口

    QString m_strUavVideoHostIP; //图传ip
    int m_nUavVideoDetectPort; //图传端口
    QString m_strUavVideoDetectIP; //航拍ip
    int m_nUavVideoDetectPort1; //航拍端口1
    int m_nUavVideoDetectPort2; //航拍端口2
    int m_nUavVideoDetectPort3; //航拍端口3

    QString m_strShipFileIp; //船载文件ip
    int m_nShipFilePort; //船载文件端口

    QString m_strSelfPoseTrackServerIp; //自测位跟踪服务器ip
    int m_nSelfPoseTrackServerPort; //自测位跟踪服务器端口
    int m_nSelfPoseAirTrackServerPort; //自测位跟踪服务器端口

    int m_nLLMMode; //LLM模式 0-光昊标准化 1-光昊翻译 2-阿里模型
    int m_nAutoHandleMode; //自动处理模式 0-截图有框 1-截图没有框
};

struct RadarAttr
{
    int m_nRadarId;  //最多四组雷达，{0，1，2，3}
    QString m_strRadarName;
    bool m_bEnable;

    int m_nResolution; // {0-标准（2k*2k） 1-扩展（4k*2k） 2-扩展方位角（2k*4k）}
    double m_dPRF;
    double m_dAzi;
    double m_dDuraton;

    int m_nPosType;  //使用的位置数据 {0-参考点， 1-经纬度， 2-GPS}

    int m_nRefPoint; // {1-5号参考点}
    double m_dOffsetX; //X偏移量
    double m_dOffsetY; //Y偏移量

    double m_dLon; //经度
    double m_dLat; //纬度
    double m_dAlt; //海拔
    double m_dHeight; //离地高
    double m_dSpiral; //旋转

    int m_nGpsFmt; //GPS/罗经格式 {0-无，1-UDP NMEA-0183无头，2-UDP NMEA-0183 SPx标头}
    QString m_strGpsIP;
    int m_nGpsPort;
    QString m_strGpsInputInterface; //雷达输入接口

    int m_nVideoFmt; //雷达视频格式 {0-无，1-UDP服务协议，2-Cat-240}
    QString m_strVideoInputIP; //视频输入地址
    int m_nVideoInputPort;  //视频输入端口
    QString m_strVideoInputInterface; //视频输入接口

    int m_nTrackFmt; //航迹格式 {0-无，1-UDP服务协议， 2-UDP ASTERIX Cat-10或Cat-48}
    QString m_strTrackInputIP; //航迹输入地址
    int m_nTrackInputPort;  //航迹输入端口
    QString m_strTrackInputInterface; //航迹输入接口

    int m_nRadarType; //雷达类型 {0-无，1-Kelvin Hughes Sharpeye， 2-Simrad 3G/5G/Halo, 3-远程Simrad 3G/4G/Halo}
    int m_nCtrlConf; //连接设置 {仅适用于Kevin Hughes，0-PASER， 1-PCAN}
    //QString m_strCtrlInputIP; //输入地址
   // QString m_strCtrlInputPort;  //输入端口
    double m_nRange; //雷达量程 {0-12NM, 1-6NM, 2-3NM, 4-0.5NM}
    double m_dGain; //雷达增益
    //QString m_strCtrlIP;
    //int m_nCtrlPort;

    //int m_bTestInput; //测试输入 0-不启用 1-启用
    //int m_nNewVideoClr; //新视频颜色
    //int m_nTrackVideoClr; //尾迹视频颜色
    //double m_dVideoGain; //视频增益
};

struct CameraAttr
{
    int m_nCameraId;  //最多十六组光电，{0，1，2，3，..,15}
    QString m_strCameraName;
    QString m_strVideoUrl = "";
    int m_nRectify;
    bool m_bEnable;

    double m_dFieldAngle;   //视场角
    double m_dOrient;      //方位角
    double m_dPitch;       //俯仰角
    double m_dOrientPanoff;
    double m_dPitchRatio1;
    double m_dPitchRatio2;
    double m_dCCD; //ccd
    double m_dFD; //焦距

    int m_nPosType;  //使用的位置数据 {0-参考点， 1-经纬度， 2-GPS}

    int m_nRefPoint; // {1-5号参考点}
    double m_dOffsetX; //X偏移量
    double m_dOffsetY; //Y偏移量

    double m_dLon; //经度
    double m_dLat; //纬度
    double m_dAlt; //海拔
    double m_dHeight; //离地高
    double m_dPosError; //位置误差

    int m_nGpsFmt; //GPS/罗经格式 {0-无，1-UDP NMEA-0183无头，2-UDP NMEA-0183 SPx标头}
    QString m_strGpsIP;
    int m_nGpsPort;
    QString m_strGpsInputInterface;

    int m_nVideoFmt; //光电视频格式 {0-无，1-RTSP H.264，2-UDP}
    QString m_strMediaName;  //媒体名称
    int m_nVideoMode;  //模式 {0-自动，1-UDP单播，2-UDP组播，3-TCP交错，4-HTTP}
    QString m_strVideoUser;
    QString m_strVideoPwd;
    QString m_strVideoInputIP; //视频输入地址
    int m_nVideoInputPort;  //视频输入端口

    int m_nTrackFmt; //航迹格式 {0-无，1-UDP服务协议， 2-UDP ASTERIX Cat-10或Cat-48}
    bool m_bUseTrack = false;
    QString m_strTrackInputIP; //航迹输入地址
    int m_nTrackInputPort;  //航迹输入端口
    bool m_bUseSingleTrack = false;
    QString m_strTrackInputInterface; //航迹输入接口
    QString m_strSingleInputIP;
    int m_nSingleInputPort;

    QString m_strSingleTrackSendIP;
    int m_nSingleTrackSendPort;

    int m_nCtrlType;  //控制器类型 {0-无，1-ONVIF，2-高普乐，3-FLIR,4-11所}
    int m_nTransType;  //传输类型 {0-串口，1-网络}
    QString m_strSerialPort;  //串口号
    int m_nSerialBaud;  //波特率
    QString m_strCtrlInputIP; //视频输入地址
    int m_nCtrlInputPort;  //视频输入端口
    QString m_strCtrlUser;
    QString m_strCtrlPwd;
    QString m_strCtrlIfIP;  //接口
    int m_camDirectionBase;//相机基准 0-正北 1-船艏
    bool m_bTracking = false;

    bool m_isDrone = false;//当前光电是否为无人机
    QString m_strAirportSN;//机场SN码
    QString m_strDroneSN;//无人机SN码
    bool m_isLamp = false;
    float m_dLampAziOffset;
    float m_dLampHight;
    QString m_ControlLampIP;
    int m_ControlLampPort;
};
struct CameraConfig
{
    float m_PanOffset;
    float m_BoatHeight;
    float m_TragetWidth;
    float m_CCDLength;
    float m_TargetScal;
    float m_minJiaoju;
};
struct VideoTargetInfo
{
    int id;
    float lat;
    float lon;
    short row;
    short col;
    long long time;
};
struct TargetImagInfo
{
    VideoTargetInfo m_tartget;
    QPixmap m_img;
};
struct ReferencePointAttr
{
    int m_nId;  //参考点id，{1，2，3，4}
    QString m_strName; //名称
    int m_nInputType; //0-使用GPS 1-手动设置
    int m_nLJFmt; //罗经数据格式，{0-无，1-UDP NMEA-0183无头，2-UDP NMEA-0183 SPx标头}
    QString m_strGpsIP;
    int m_nGpsPort;
    QString m_strSerialPort;
    int m_nSerialBaud;

    double m_dLon; //经度
    double m_dLat; //纬度
    double m_dAlt; //海拔
    double m_dHeight; //高程

    int m_stationSlect;//平台选择 0-固定平台 1-移动平台
    // UDPmanage* m_pUdpGNSS = nullptr;  // UDPmanage类型不存在，已移除
    //bool m_bEnable; //是否启用
};

struct PtzConfAttr
{
    int m_nMainCamID;
    int m_nSideCamEnable;
    int m_nSideCamID;
    int m_nShowAR;
    int m_nShowDetect;
    int m_nShowTrack;
    int m_nShowRadar;
    int m_nShowFuse;
    int m_nShowMiniMap;
    int m_nShowSeaDepth;
    int m_nShowSeaGrid;
    int m_nShowRoute;
    int m_nShowCoastLine;
    int m_nShowWarningArea;
    int m_nShowDistance;
    QString m_nTrackColor;
    QString m_nRadarColor;
    QString m_nFuseColor;

};

enum AreaTargetType {
    MILITARY_SHIP = 1, // 军用舰船
    NON_BATTLE_SHIP = 2, // 非作战船舶
    DREDGING_OR_UNDERWATER_OPERATION = 3, // 疏浚或水下作业
    PASSENGER_SHIP = 4, // 客轮
    CARGO_SHIP = 5, // 货轮
    OIL_SHIP = 6, // 油轮
    FISHING_SHIP = 7, // 渔船   
    HIGH_SPEED_BOAT = 8, // 高速艇
    SEARCH_AND_RESCUE_SHIP = 9, // 搜救船
    PORT_OPERATION_SHIP = 10, // 港口作业船
    LOCAL_SHIP = 11, // 本地船只
    ANTI_POLLUTION_EQUIPMENT = 12, // 防污设备
    DRAFTING_SHIP = 13, // 拖曳船只
    TUG_SHIP = 14, // 拖轮
    SAILING_SHIP = 15, // 帆船
    YACHT = 16, // 游艇
    NAVIGATION_SHIP = 17, // 引航船
    HEALTH_TRANSPORT_SHIP = 18, // 卫生运输船
    COAST_GUARD_SHIP = 19, // 海警船
    BUOY = 20, // 浮标
    FLOATING_OBJECT = 21, // 漂浮物
    SEMISUBMERSIBLE_DEVICE = 22, // 半潜设备
    RESEARCH_VESSEL = 23, // 科考船
    OBSERVATION_PLATFORM = 24 // 观测平台
};


enum CameraTaskTyp
{
    CAMERA_UP = 1,
    CAMERA_DOWN,
    CAMERA_LEFT,
    CAMERA_LEFT_UP,
    CAMERA_LEFT_DOWN,
    CAMERA_RIGHT,
    CAMERA_RIGHT_UP,
    CAMERA_RIGHT_DOWN,
    CAMERA_ZOOMIN,
    CAMERA_ZOOMOUT,
    CAMERA_FOCUSIN,
    CAMERA_FOCUSOUT,
    CAMERA_ABSOLUTE,
    CAMERA_SUPPLEMENT_SEARCH,
    CAMERA_3D,
    CAMERA_DEFAULT,
    CAMERA_AREA_SEARCH,
    CAMERA_LOOK_AT,
    CAMERA_KEY_TRACE,
    CAMERA_TAKE_PIC,
    CAMERA_RECORD,
    CAMERA_STOP_ALL,
    CAMERA_CONTROL_STOP,
    CAMERA_IMPORTANT_TRACK,
    CAMERA_ALARM_CHECK,
    CAMERA_TRACK_CHECK,
    CAMERA_UPDATE_PIDX,
    CAMERA_UPDATE_PIDY,
    CAMERA_UPDATE_PIDXY
};

enum AREA_TYPE
{
    AREA_POLYGON = 1,
    AREA_CIRCLE,
    AREA_RECTANGLE
};

struct DroneSelfPoseTrack {
    float altitude; // 高度（海拔）
    float lat;      // 纬度
    float lon;      // 经度
    float speed;    // 水平速度 (m/s)
    float speed_v;  // 垂直速度 (m/s)
    float course;   // 航向角（度数，正北顺时针）
};

struct DroneCamPose {
    float altitude; // 高度（海拔）
    float lat;      // 纬度
    float lon;      // 经度
};


struct CameraMetaTask
{
    //taskid
    QString m_strTaskID = "";
    int8_t m_priority = -1;
    int8_t m_nHandControl = 0;
    //0紫东可见光 1靖子头可见光 2靖子头红外 3紫东广角 4远遥可见光 5远遥红外 6天测可见光 7天测红外 8天测广角 9紫东红外 13 11所红外
    int8_t m_nCameraIndex = -1;
    int32_t m_nMaxTime = 30000;
    //1上 2下 3左 4右 5zoomIn 6zoomOut 7focusIn 8focusOut 9绝对定位 10默认状态 
    //11区域监视 12航点监视 13区域目标取证 14重点目标连续取证 15重点目标跟踪
    //16所有操作停止 17控制停止 18更新PID参数
    int8_t m_nSendTaskType = 0;

    ////反馈消息类型
    //// 1相机工作中 2开始取证 3相机反馈ptz
    //int8_t m_nResponseType = 0;

    float m_speedP = 0.5;
    float m_speedT = 0.5;
    float m_speedZ = 0.5;

    int m_nCurFocusRectType = 0;

    int8_t m_CheckAbsolute = 0;

    int m_nCheckFindTarget = 0;
    float m_onStateP = 0.0;
    float m_setP = 0.0;
    float m_setT = 0.0;
    float m_setZ = 0.0;

    bool m_bSyncTask = false;
    int m_areaType = 0;
    int m_areaTargetType = 0;
    int32_t m_targetNums = 0;
    float m_targetLon[200] = {};
    float m_targetLat[200] = {};

    float m_targetSize = 2.0f;
    float m_keyTargetLon = 0.0f;
    float m_keyTargetLat = 0.0f;

    int32_t m_nTrackID = -1;
    int m_nRectType = 0;
    int m_nTrackVideoWidth = 0;
    int m_nTrackVideoHeight = 0;
    int m_nTrackXPos = 0;
    int m_nTrackYPos = 0;
    int m_nTrackWidth = 0;
    int m_nTrackHeight = 0;

    int32_t m_nTrackTime = 0;
    int8_t m_nTackAction = 1;

    int32_t m_nRecordTime = 0;
    //0stop 1start
    int8_t m_nRecordAction = 0;

    //执行中
    int m_nExcutionState = 1;
    //正常执行
    int m_nPermisionState = 3;

    bool m_bMoveCenter = true;
    int m_nImportantTrackAction = 0;
    QString m_strImportantAlarmID;
    int m_nImportantTrackID;

    //0所有 1高中 2高  3低 告警处理
    int m_alaramCheckType = 0;
    int m_alarmTrackAction = 0;
    int m_trackCheckAction = 0;

    QDateTime m_taskRunTime;
    int32_t m_nTaskCostTime = 0;
    float px = 0.0f;
    float ix = 0.0f;
    float dx = 0.0f;
    float py = 0.0f;
    float iy = 0.0f;
    float dy = 0.0f;

};

struct CameraExcuteResult
{
    int m_strExcuteState;
    int m_strPermissionState;
    uint32_t m_excuteLeftTime;
    int8_t m_energyCost = 0;
    QString m_strErrorMessage = "";
};

struct PlatformAttr
{
    int m_nType = 0;
    QString m_strName = "new platform";
    int m_nReferenceIndex = 0;
    QStringList m_strRadarList;
    QStringList m_strCameraList;
    int m_nCompassType = 0;
    QString m_strCompassIP = "";
    int m_nCompassPort = 0;
    QString m_strCompassSerialName = "com0";
    int m_nCompassSerialRate = 115200;
    int m_nSoundType = 0;
    QString m_strSoundIP = "";
    int m_nSoundPort = 0;
    QString m_strSoundSerialName = "com1";
    int m_nSoundSerialRate = 115200;

    int m_nWeatherType = 0;
    QString m_strWeatherIP = "";
    int m_nWeatherPort = 0;
    QString m_strWeatherSerialName = "com1";
    int m_nWeatherSerialRate = 115200;
    QByteArray m_strLayout = "";
    PlatformAttr()
    {}
    PlatformAttr(const PlatformAttr& iniCfg)
    {
        m_nType = iniCfg.m_nType;
        m_strName = iniCfg.m_strName;
        m_nReferenceIndex = iniCfg.m_nReferenceIndex;
        m_strRadarList = iniCfg.m_strRadarList;
        m_strCameraList = iniCfg.m_strCameraList;
        m_nReferenceIndex = iniCfg.m_nReferenceIndex;
        m_nCompassType = iniCfg.m_nCompassType;
        m_strCompassIP = iniCfg.m_strCompassIP;
        m_nCompassPort = iniCfg.m_nCompassPort;
        m_strCompassSerialName = iniCfg.m_strCompassSerialName;
        m_nCompassSerialRate = iniCfg.m_nCompassSerialRate;
        m_nSoundType = iniCfg.m_nSoundType;
        m_strSoundIP = iniCfg.m_strSoundIP;
        m_nSoundPort = iniCfg.m_nSoundPort;
        m_strSoundSerialName = iniCfg.m_strSoundSerialName;
        m_nSoundSerialRate = iniCfg.m_nSoundSerialRate;

        m_nWeatherType = iniCfg.m_nWeatherType;
        m_strWeatherIP = iniCfg.m_strWeatherIP;
        m_nWeatherPort = iniCfg.m_nWeatherPort;
        m_strWeatherSerialName = iniCfg.m_strWeatherSerialName;
        m_nWeatherSerialRate = iniCfg.m_nWeatherSerialRate;

        m_strLayout = iniCfg.m_strLayout;
    }
    PlatformAttr& operator=(const PlatformAttr& iniCfg)
    {
        if (this != &iniCfg)
        {
            m_nType = iniCfg.m_nType;
            m_strName = iniCfg.m_strName;
            m_nReferenceIndex = iniCfg.m_nReferenceIndex;
            m_strRadarList = iniCfg.m_strRadarList;
            m_strCameraList = iniCfg.m_strCameraList;
            m_nReferenceIndex = iniCfg.m_nReferenceIndex;
            m_nCompassType = iniCfg.m_nCompassType;
            m_strCompassIP = iniCfg.m_strCompassIP;
            m_nCompassPort = iniCfg.m_nCompassPort;
            m_strCompassSerialName = iniCfg.m_strCompassSerialName;
            m_nCompassSerialRate = iniCfg.m_nCompassSerialRate;
            m_nSoundType = iniCfg.m_nSoundType;
            m_strSoundIP = iniCfg.m_strSoundIP;
            m_nSoundPort = iniCfg.m_nSoundPort;
            m_strSoundSerialName = iniCfg.m_strSoundSerialName;
            m_nSoundSerialRate = iniCfg.m_nSoundSerialRate;
            m_nWeatherType = iniCfg.m_nWeatherType;
            m_strWeatherIP = iniCfg.m_strWeatherIP;
            m_nWeatherPort = iniCfg.m_nWeatherPort;
            m_strWeatherSerialName = iniCfg.m_strWeatherSerialName;
            m_nWeatherSerialRate = iniCfg.m_nWeatherSerialRate;
            m_strLayout = iniCfg.m_strLayout;
        }
        return *this;

    }
};

struct PIDConfig
{
    float px = 0.0;
    float ix = 0.0;
    float dx = 0.0;
    float py = 0.0;
    float iy = 0.0;
    float dy = 0.0;
    float px1 = 0.0;
    float ix1 = 0.0;
    float dx1 = 0.0;
    float py1 = 0.0;
    float iy1 = 0.0;
    float dy1 = 0.0;

};

struct AimConfig
{
    QString aimParamP0 = "";
    QString aimParamT0 = "";
    QString aimParamP1 = "";
    QString aimParamT1 = "";
};

struct OtherConfig
{

    QString m_strDroneIP = "";
    int m_nDronePort = 0;

    QString m_strRadarDetectIP = "";
    int m_nRadarDetectPort = 0;

    QString m_strAirRadarIP = "";
    int m_nAirRadarPort = 0;

    QString m_strElecDetectIP = "";
    int m_nElecDetectPort = 0;

    QString m_strSonarIP = "";
    int m_nSonarPort = 0;

    QString m_strPTZDetectIP = "";
    int m_nPTZDetectPort = 0;
    OtherConfig()
    {}
    OtherConfig(const OtherConfig& iniCfg)
    {
        m_strDroneIP = iniCfg.m_strDroneIP;
        m_nDronePort = iniCfg.m_nDronePort;
        m_strRadarDetectIP = iniCfg.m_strRadarDetectIP;
        m_nRadarDetectPort = iniCfg.m_nRadarDetectPort;
        m_strAirRadarIP = iniCfg.m_strAirRadarIP;
        m_nAirRadarPort = iniCfg.m_nAirRadarPort;
        m_strElecDetectIP = iniCfg.m_strElecDetectIP;
        m_nElecDetectPort = iniCfg.m_nElecDetectPort;
        m_strSonarIP = iniCfg.m_strSonarIP;
        m_nSonarPort = iniCfg.m_nSonarPort;
        m_strPTZDetectIP = iniCfg.m_strPTZDetectIP;
        m_nPTZDetectPort = iniCfg.m_nPTZDetectPort;
    }
    OtherConfig& operator=(const OtherConfig& iniCfg)
    {
        if (this != &iniCfg)
        {
            m_strDroneIP = iniCfg.m_strDroneIP;
            m_nDronePort = iniCfg.m_nDronePort;
            m_strRadarDetectIP = iniCfg.m_strRadarDetectIP;
            m_nRadarDetectPort = iniCfg.m_nRadarDetectPort;
            m_strAirRadarIP = iniCfg.m_strAirRadarIP;
            m_nAirRadarPort = iniCfg.m_nAirRadarPort;
            m_strElecDetectIP = iniCfg.m_strElecDetectIP;
            m_nElecDetectPort = iniCfg.m_nElecDetectPort;
            m_strSonarIP = iniCfg.m_strSonarIP;
            m_nSonarPort = iniCfg.m_nSonarPort;
            m_strPTZDetectIP = iniCfg.m_strPTZDetectIP;
            m_nPTZDetectPort = iniCfg.m_nPTZDetectPort;
        }
        return *this;

    }
};

struct AISTrack
{
    unsigned int MMSI;
    QString name;
    double latDeg;
    double longDeg;
    double speedMps;
    double courseDeg;
    double headingDeg;
    unsigned int IMO;
    double draughtMetres;
    unsigned int shipCargoType;

};

struct VesselCondition
{
    //工况信息
    long long lTime;
    double dLeftRotarySpeed;
    double dLeftShaftPower;
    double dRightRotarySpeed;
    double dRightShaftPower;
    double dDraft;
    //电子倾斜仪
    double dHorizontalDipAngle;
    double dVerticalDipAngle;
    double dHorizontalRollingPeriod;
    double dVerticalRollingPeriod;
    //吃水
    double dLeftHeadDraft;
    double dRightHeadDraft;
    double dLeftBodyDraft;
    double dRightBodyDraft;
    double dLeftTailDraft;
    double dRightTailDraft;
    //罗经
    double dBow;
    double dSteeringAngle;
    //GPS
    int nGpsUTC;
    double dGpsLat;
    double dGpsLon;
    double dGpsAlt;
    double dGpsHeadingAngle;
    double dGpsSpeed;
    //北斗
    int nBeidouUTC;
    double dBeidouLat;
    double dBeidouLon;
    double dBeidouAlt;
    double dBeidouHeadingAngle;
    double dBeidouSpeed;
    //测深
    double dDepth_Meter;
    double dDepth_Foot;
    //风速风向
    double dWindSpeed;
    double dWindDirection;
};

struct Position {
    float x;
    float y;
    float z;
};

struct Orientation {
    float x;
    float y;
    float z;
    float w;
};

struct Scale {
    float x;
    float y;
    float z;
};

typedef struct _BoxPose {

    Position position;
    Orientation orientation;
    _BoxPose& operator=(const _BoxPose& other) {
        if (this != &other) { // 避免自我赋值
            // 使用 memcpy 进行内存拷贝
            memcpy((void*)this, (void*)&other, sizeof(_BoxPose));
        }
        return *this;
    }
}BoxPose;

typedef struct _BoxMarker {
    struct {
        char frame_id[16];
        struct {
            int32_t sec;
            uint32_t nanosec;
        }stamp;
    }header;
    char ns[16];
    int id;
    int marker_cnt; // Assuming you have a variable named marker_cnt to track the marker count
    int type;
    int action;
    BoxPose pose;
    Scale scale;
    struct {
        float r;
        float g;
        float b;
        float a;
    } color;
    char text[16]; // Assuming you also have a text member for text_marker

    _BoxMarker& operator=(const _BoxMarker& other) {
        if (this != &other) { // 避免自我赋值
            // 使用 memcpy 进行内存拷贝
            memcpy((void*)this, (void*)&other, sizeof(_BoxMarker));
        }
        return *this;
    }
}BoxMarker;

typedef struct _MyMarkerArray {
    uint32_t head;//0x5a5a55aa
    uint32_t cnt;
    QVector<BoxMarker> markers;
}MyMarkerArray;

struct RowInfo
{
    int key;
    double lon;
    double lat;
    double size;
    double radian;
    double dis;
    double angle;
    double yy_lon;
    double yy_lat;
    double yy_size;
    double yy_dis;
    double yy_angle;
    double jzt_lon;
    double jzt_lat;
    double jzt_size;
    double jzt_dis;
    double jzt_angle;
    int rect_x;
    int rect_y;
    int rect_width;
    int rect_height;
    std::string p;
    std::string t;
    std::string z;
    QString name;
    QString time;
    //RowInfo(int x1,double x2,double x3,double x4,
    //double x5,double x6,double x7,double x8,double x9,double x10,
    //double x11,double x12,double x13,double x14,double x15,double x16,
    //double x17,int x18,int x19,int x20,int x21,std::string x22,
    //    std::string x23, std::string x24, QString x25, QString x26) :
    //key(x1),lon(x2),lat(x3),size(x4), radian(x5),dis(x6){}
    //RowInfo(const RowInfo& info)
    //{
    //    key = info.key;
    //    lon = info.lon;
    //    lat = info.lat;
    //    size = info.size;
    //    radian = info.radian;
    //    dis = info.dis;
    //    angle = info.angle;
    //    yy_lon = info.yy_lon;
    //    yy_lat = info.yy_lat;
    //    yy_size = info.yy_size;
    //    yy_dis = info.yy_dis;
    //    yy_angle = info.yy_angle;
    //    jzt_lon = info.jzt_lon;
    //    jzt_lat = info.jzt_lat;
    //    jzt_size = info.jzt_size;
    //    jzt_dis = info.jzt_dis;
    //    jzt_angle = info.jzt_angle;
    //    rect_x = info.rect_x;
    //    rect_y = info.rect_y;
    //    rect_width = info.rect_width;
    //    rect_height = info.rect_height;
    //    p = info.p;
    //    t = info.t;
    //    z = info.z;
    //    name = info.name;
    //    time = info.time;
    //}
    //RowInfo& operator=(const RowInfo& info)
    //{
    //    if (this != &info) { // 避免自我赋值
    //        key = info.key;
    //        lon = info.lon;
    //        lat = info.lat;
    //        size = info.size;
    //        radian = info.radian;
    //        dis = info.dis;
    //        angle = info.angle;
    //        yy_lon = info.yy_lon;
    //        yy_lat = info.yy_lat;
    //        yy_size = info.yy_size;
    //        yy_dis = info.yy_dis;
    //        yy_angle = info.yy_angle;
    //        jzt_lon = info.jzt_lon;
    //        jzt_lat = info.jzt_lat;
    //        jzt_size = info.jzt_size;
    //        jzt_dis = info.jzt_dis;
    //        jzt_angle = info.jzt_angle;
    //        rect_x = info.rect_x;
    //        rect_y = info.rect_y;
    //        rect_width = info.rect_width;
    //        rect_height = info.rect_height;
    //        p = info.p;
    //        t = info.t;
    //        z = info.z;
    //        name = info.name;
    //        time = info.time;
    //    }
    //    return *this;

    //}
};

#define MODAL_NAME_TV_VIDEO "光电-可见光"
#define MODAL_NAME_RADAR_VIDEO "雷达回波"
#define MODAL_NAME_MULTIMODAL "通侦"
#define MODAL_NAME_AIS "AIS"
#define MODAL_NAME_TRACKS "航迹"
#define MODAL_NAME_ADSB "ADS-B"
#define MODAL_NAME_SHIPFILE "船舶档案"

enum class ModalType {
    RadarEcho,
    Track,
    ShipFile,//船舶档案
    VisibleLightVideo, //可见光
    InfraredVideo, //红外
    ADSB,
    AIS,
    NaviRadar, //雷侦 空
    Sonar
};

inline uint qHash(const ModalType& key, uint seed = 0)
{
    return ::qHash(static_cast<int>(key), seed);
}

enum TargetType {
    WATER = 1,
    UNDERWATER = 2,
    AIR = 3
};
struct CaptureTargetItem {
    TargetType type; //水下 水上 空中
    qint64 id; //目标ID
    QString name; //目标名称
    QString typeName;
    QDateTime captureTime;
    //海上目标：0-雷达回波  1-可见光 2-红外 3-AIS 4-水声 5-航迹 6-雷侦
    //空中目标：0-雷达回波  1-ADSB  2-航迹
    QVector<bool> isModalCapture; //各模态是否采集到数据
    // QSet<ModalType> capturedModals;  // ModalType类型未定义，已移除
    bool isRealTime;
};

// 目标信息结构体
struct TargetInfo {
    qint64 targetId;    // 目标ID
    int targetType;      // 目标类型
    QString targetName;  // 目标名称
};

// 目标关联结构体
struct TargetAssociation {
    qint64 associationId = 0;
    qint64 targetId = 0;
    QString targetName;
    QString identifier;
    qint64 trackId = 0;
    qint64 fusedTrackId = 0;
    QDateTime associationTime = QDateTime::currentDateTime();
};

struct TargetCollection {
    QUuid collectionId = QUuid();
    qint64 targetId = 0;
    int modalityId = 0;
    QDateTime collectionTime = QDateTime::currentDateTime();
    int collectionMethod = 0;
    double longitude = 0.0;
    double latitude = 0.0;
    int sensorId = 0;
    double relativeBearing = 0.0;
    double relativeDistance = 0.0;
    int behaviorId = 0;
};

// 传感器信息结构体
struct SensorInfo {
    int sensorId;
    QString sensorName;
};

// AIS记录结构体
struct AISRecord {
    qint64 mmsi = 0;
    QDateTime recordTime = QDateTime::currentDateTime();
    int sensorId = 0;
    QString name;
    double longitude = 0.0;
    double latitude = 0.0;
    double speed = 0.0;
    double course = 0.0;
    qint32 heading = 0;
    qint32 imo = 0;
    QString callSign;
    qint32 shipType = 0;
    QString fileName;
    qint32 messageOffset = 0;
    qint32 messageSize = 0;
    QByteArray waveByte;
};

// ADS-B记录结构体
struct ADSBRecord {
    QString icao;
    QDateTime recordTime = QDateTime::currentDateTime();
    int sensorId = 0;
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
};

// 雷达航迹记录结构体
struct TrackRecord {
    qint64 trackId = 0;
    QDateTime recordTime = QDateTime::currentDateTime();
    int sensorId = 0;
    QString name;
    qint32 type = 0;
    qint64 mmsi = 0;
    double distance = 0.0;
    double azimuth = 0.0;
    double speed = 0.0;
    double course = 0.0;
    double size = 0.0;
    double longitude = 0.0;
    double latitude = 0.0;
    double cpa = 0.0;
    double tcpa = 0.0;
};

// 融合航迹记录结构体
struct FusedTrackRecord {
    qint64 fusedTrackId = 0;
    QDateTime recordTime = QDateTime::currentDateTime();
    int sensorId = 0;
    QString name;
    qint32 fusionType = 0;
    QVector<qint64> originalTrackIds;
    double distance = 0.0;
    double azimuth = 0.0;
    double speed = 0.0;
    double course = 0.0;
    double size = 0.0;
    double longitude = 0.0;
    double latitude = 0.0;
    double cpa = 0.0;
    double tcpa = 0.0;
};

// 雷达回波采集结构体
struct RadarEchoCollection {
    QUuid collectionId = QUuid();
    int sequenceNumber = 0;
    int sensorId = 0;
    QString sensorName;
    QDateTime collectionTime = QDateTime::currentDateTime();
    double distance = 0.0;
    double azimuth = 0.0;
    QString imagePath;
    QByteArray imageBlob;
};

// 视频采集结构体
struct VideoCollection {
    QUuid collectionId = QUuid();
    int sequenceNumber = 0;
    int sensorId = 0;
    QString sensorName;
    int cameraType = 0;
    QDateTime collectionTime = QDateTime::currentDateTime();
    double p = 0.0;
    double t = 0.0;
    double z = 0.0;
    QString imagePath;
    QByteArray imageBlob;
};

// 截图采集结构体
struct PicCollection {
    QUuid collectionId = QUuid();
    int sensorId = 1;
    QString sensorName;
    int cameraType = 0;
    QDateTime collectionTime = QDateTime::currentDateTime();
    int weatherId = 1;
    int seaStateId = 1;
    int visibleId = 1;
    double p = 0.0;
    double t = 0.0;
    double z = 0.0;
    QString rectInfo;
    QByteArray imageBlob;
};

// 水声采集结构体
struct SonarCollection {
    QUuid collectionId = QUuid();
    int sequenceNumber = 0;
    int sensorId = 0;
    QString sensorName;
    QDateTime collectionTime = QDateTime::currentDateTime();
    QString imagePath;
    QByteArray imageBlob;
};

// AIS采集记录结构体
struct AISCollectionRecord {
    QUuid collectionId = QUuid();
    int sensorId = 0;
    QString sensorName;
    int sequenceNumber = 0;
    int mmsi = 0;
    QDateTime recordTime = QDateTime::currentDateTime();
    QString name;
    double longitude = 0.0;
    double latitude = 0.0;
    double speed = 0.0;
    double course = 0.0;
    int heading = 0;
    int imo = 0;
    QString callSign;
    int shipType = 0;
    QString fileName;
    int messageOffset = 0;
    int messageSize = 0;
    QByteArray waveByte;
};

// 航迹信息结构体
struct TrackInfo {
    qint64 trackId = 0;
    int sequenceNumber = 0;
    QDateTime recordTime = QDateTime::currentDateTime();
    double distance = 0.0;
    double azimuth = 0.0;
    double speed = 0.0;
    double course = 0.0;
    double size = 0.0;
    double longitude = 0.0;
    double latitude = 0.0;
    double cpa = 0.0;
    double tcpa = 0.0;
};

// 航迹采集记录结构体
struct TrackCollectionRecord {
    QUuid collectionId = QUuid();
    int sensorId = 0;
    QString sensorName;
    QDateTime collectionTime = QDateTime::currentDateTime();
    bool isFused = false;
    QList<TrackInfo> tracks;
};

// ADS-B采集记录结构体
struct ADSBCollectionRecord {
    QUuid collectionId = QUuid();
    int sensorId = 0;
    QString sensorName;
    int sequenceNumber = 0;
    QString icao;
    QDateTime recordTime = QDateTime::currentDateTime();
    double longitude = 0.0;
    double latitude = 0.0;
    double altitude = 0.0;
};

// 目标实时信息结构体
struct TargetRealtimeInfo {
    qint64 targetId = 0;
    QString targetName;
    QString targetType;
    double longitude = 0.0;
    double latitude = 0.0;
    QString strLon;
    QString strLat;
    double distanceNm = 0.0;
    double directionDeg = 0.0;
    double speed = 0.0;
    double course = 0.0;
    QDateTime recordTime = QDateTime::currentDateTime();
};



//信道信息
struct Channel {
    int nID;
    QString strName;
    QString strType;
    int nStatus;

    bool operator==(const Channel& other) const {
        return (nID == other.nID) && (strName == other.strName) && (strType == other.strType) && (nStatus == other.nStatus);
    }
};


struct ChannelTask {
    QString strTaskName;
    QVector<Channel> vChannels;
    int nTaskStatus;//-1不显示，0显示
};

//标注信息格式:Sec_BasicInfo/Sec_Weather
struct SecBasicInfo {
    QString collector;  //采集人
    QString collectorDate;  //采集日期
    QVector<QPair<QString, QString>> labelers; // QPair用于存储标注人和标注日期
    QString version;    //采集版本
    long dataSize;  //数据量，单位：byte
};

struct SecWeather {
    int WeatherID;
    QString WeatherName;
    double WindSpeed; // 风速，单位：m/s
    int WindDirection; // 风向，单位：°
    double Temperature; // 气温，单位：°C
    double AtmosphericPressure; // 气压，单位：Kpa
    double Humidity; // 湿度，单位：%
    int PrecipitationID;
    QString PrecipitationName;
    double Precipitation; // 降水/雪/雹量，单位：mm
    int VisibilityID; // 能见度级别ID，1-6
    QString VisibilityName;
    int SeaStateID; // 海况ID，0-9
    QString SeaStateName;
    double WaveHeight; // 波高，单位：m
    double WavePeriod; // 波周期，单位：s
};

struct RadarInfo {
    int RadarID;    //雷达ID
    QString RadarName;  //雷达名称
    QString RadarProducer;  //雷达厂商
    QString RadarType;  //雷达型号
    QString RadarPosition;  //雷达安装地点
    double RadarLon; // 经度
    double RadarLat; // 纬度
    int Range; // 量程, 单位：m
    QString WaveBand; // 波段
    double BeamHWidth; // 波束水平宽度, 单位：m
    double BeamVWidth; // 波束垂直宽度, 单位：m
    double RotateSpeed; // 转速，单位：m/s（注意：这里假设转速的单位是m/s可能不太准确，通常是度/秒或弧度/秒，但根据给定信息填写）
    double TransitPower; // 发射功率，单位：kw
};

struct CameraInfo {
    int CameraID;   // 相机ID
    QString CameraName; //相机名称
    QString CameraProducer; //相机厂商
    QString CameraType; //相机型号
    QString CameraPosition; //相机安装地点
    double CameraLon; // 经度
    double CameraLat; // 纬度
    int CameraZoom; //焦距
    int CameraWidth; // 水平像素宽度
    int CameraHeight; // 垂直像素高度
    double CameraFovx; // 最大水平视场
    double CameraFovy; // 最大垂直视场
};

struct CollectInformation
{
    SecBasicInfo basicInfo;
    SecWeather weather;
    QVector<RadarInfo> vRadarInfo;
    QVector<CameraInfo> vCameraInfo;
};

//视场角（FOV）在 liveview 中的区域
struct LiveViewWorldRegion
{
    double top{};//左上角的 y 轴起始点
    double bottom{};//右下角的 y 轴起始点
    double left{};//左上角的 x 轴起始点
    double right{};//右下角的 x 轴起始点
    void clear() {
        top = 0;
        bottom = 0;
        left = 0;
        right = 0;
    }
};

//飞行器相机信息
struct Cameras
{
    double zoomFactor{};//变焦倍数
    LiveViewWorldRegion liveViewWorldRegion{};//视场角（FOV）在 liveview 中的区域
    double irZoomFactor{};//红外变焦倍数
    int zoomFocusMode{};//变焦镜头对焦模式
    int zoomFocusValue{};//变焦镜头对焦值
    int zoomFocusState{};//变焦镜头对焦状态
    int zoomMaxFocusValue{};//变焦镜头最大对焦值
    int zoomMinFocusValue{};//变焦镜头最小对焦值
    int zoomCalibrateFarthestFocusValue{};//变焦镜头标定的最远对焦值
    int zoomCalibrateNearestFocusValue{};//变焦镜头标定的最近对焦值
    void clear() {
        zoomFactor = 0;
        liveViewWorldRegion.clear();
        irZoomFactor = 0;
        zoomFocusMode = 0;
        zoomFocusValue = 0;
        zoomFocusState = 0;
        zoomMaxFocusValue = 0;
        zoomMinFocusValue = 0;
        zoomCalibrateFarthestFocusValue = 0;
        zoomCalibrateNearestFocusValue = 0;
    }
};

//负载编号
struct TypeSubtypeGimbalindex
{
    double zoomFactor{};//变焦倍数
    double gimbalPitch{};//云台俯仰轴角度
    double gimbalRoll{};//云台横滚轴角度
    double gimbalYaw{};//云台偏航轴角度
    void clear() {
        zoomFactor = 0;
        gimbalPitch = 0;
        gimbalRoll = 0;
        gimbalYaw = 0;
    }
};

//飞行器电池信息
struct Battery
{
    int capacityPercent{};//电池的总剩余电量
    int remainFlightTime{};//剩余飞行时间
    int returnHomePower{};//返航所需电量百分比
    void clear() {
        capacityPercent = 0;
        remainFlightTime = 0;
        returnHomePower = 0;
    }
};

//飞行器充电状态
struct DroneChargeState {
    int state{};//充电状态
    int capacityPercent{};//电量百分比
    void clear() {
        state = 0;
        capacityPercent = 0;
    }
};

//网关当前整体直播状态推送
struct LiveStatus {
    int videoQuality{};//直播码流的质量
    int status{};//直播状态
    void clear() {
        videoQuality = 0;
        status = 0;
    }
};

//网络状态
struct NetworkState {
    int type{};//网络类型
    int quality{};//网络质量
    double rate{};//网络速率
    void clear() {
        type = 0;
        quality = 0;
        rate = 0;
    }
};

//子设备状态
struct SubDevice {
    int deviceOnlineStatus{};//机场停机坪上的飞行器开机状态
    int devicePaired{};//机场停机坪上的飞行器是否与机场对频
    void clear() {
        deviceOnlineStatus = 0;
        devicePaired = 0;
    }
};

enum DRONE_MODE_CODE {
    STANDBY,    //0,待机
    TAKEOFF_PREPARATION,    //1,起飞准备
    TAKEOFF_PREPARATION_COMPLETED,  //2,起飞准备完毕
    MANUAL_FLIGHT,  //3,手动飞行
    AUTOMATIC_TAKEOFF,  //4,自动起飞
    WAYLINE_FLIGHT, //5,航线飞行
    PANORAMIC_PHOTOGRAPHY,  //6,全景拍照
    INTELLIGENT_TRACKING,   //7,智能跟随
    ADS_B_AVOIDANCE,    //8,ADS-B 躲避
    AUTO_RETURNING_TO_HOME, //9,自动返航
    AUTOMATIC_LANDING,  //10,自动降落
    FORCED_LANDING, //11,强制降落
    THREE_BLADE_LANDING,    //12,三桨叶降落
    UPGRADING,  //13,升级中
    NOT_CONNECTED,  //14,未连接
    APAS,   //15,APAS
    VIRTUAL_STICK_STATE,    //16,虚拟摇杆状态
    LIVE_FLIGHT_CONTROLS,   //17,指令飞行
    AIRBORNE_RTK_FIXING_MODE,   //18,空中 RTK 收敛模式
    DOCK_ADDRESS_SELECTING, //19,机场选址中
    UNKNOWN_CODE
};

//无人机状态
static QStringList droneStatusList = {
    "task_finish",      //一键起飞任务完成
    "task_ready",       //准备起飞
    "wayline_cancel",   //取消飞向目标点
    "wayline_failed",   //执行失败
    "wayline_ok",       //执行成功，已飞向目标点
    "wayline_progress"  //执行中
};

static QList<int> droneModeCodeList = {
    MANUAL_FLIGHT,
    AUTOMATIC_TAKEOFF,
    WAYLINE_FLIGHT,
    PANORAMIC_PHOTOGRAPHY,
    INTELLIGENT_TRACKING,
    ADS_B_AVOIDANCE,
    AUTO_RETURNING_TO_HOME,
    AUTOMATIC_LANDING,
    FORCED_LANDING,
    THREE_BLADE_LANDING,
    APAS,
    VIRTUAL_STICK_STATE,
    LIVE_FLIGHT_CONTROLS,
    AIRBORNE_RTK_FIXING_MODE,
    DOCK_ADDRESS_SELECTING
};

//无人机状态字段
struct DroneInformation
{
    QVector<Cameras> vecCameras{};//飞行器相机信息
    int isNearAreaLimit{};//是否接近限飞区
    int isNearHeightLimit{};//是否接近设定的限制高度
    int heightLimit{};//飞行器限高
    TypeSubtypeGimbalindex typeSubtypeGimbalindex{};//负载编号
    QString trackId{};//航迹 ID
    Battery battery{};//飞行器电池信息
    int seriousLowBatteryWarningThreshold{};//严重低电量告警阈值
    int lowBatteryWarningThreshold{};//低电量告警阈值
    int windDirection{};//当前风向
    double windSpeed{};//风速
    double homeDistance{};//距离 Home 点的距离
    int attitudeHead{};//偏航轴角度
    double attitudeRoll{};//横滚轴角度
    double attitudePitch{};//俯仰轴角度
    double elevation{};//相对起飞点高度
    double height{};//绝对高度
    double latitude{};//纬度
    double longitude{};//经度
    double verticalSpeed{};//垂直速度
    double horizontalSpeed{};//水平速度
    int modeCode{};//飞行器状态
    QString gateway{};//网关设备的 SN
    int rtknum{};
    void clear() {
        vecCameras.clear();
        isNearAreaLimit = 0;
        isNearHeightLimit = 0;
        heightLimit = 0;
        typeSubtypeGimbalindex.clear();
        trackId.clear();
        battery.clear();
        seriousLowBatteryWarningThreshold = 0;
        lowBatteryWarningThreshold = 0;
        windDirection = 0;
        windSpeed = 0;
        homeDistance = 0;
        attitudeHead = 0;
        attitudeRoll = 0;
        attitudePitch = 0;
        elevation = 0;
        height = 0;
        latitude = 0;
        longitude = 0;
        verticalSpeed = 0;
        horizontalSpeed = 0;
        modeCode = -1;
        gateway.clear();
        rtknum = 0;
    }
};

struct UavWarningInfomation
{
    int level{};
    int module{};
    int in_the_sky{};
    QString code{};
    QString device_type{};
    int imminent{};
};

struct UavWarningMsg
{
    // 添加告警消息相关字段
    QString method;         // 方法类型，例如"joystick_invalid_notify"
    QString bid;            // 消息bid字段
    QString tid;            // 消息tid字段
    qint64 timestamp = 0;   // 时间戳
    int needReply = 0;      // 是否需要回复，1表示需要回复
    int reason = 0;         // 原因代码，存储在data.reason中
    bool isValid = false;   // 标记该告警是否有效

    // 清除方法
    void clear() {
        method.clear();
        bid.clear();
        tid.clear();
        timestamp = 0;
        needReply = 0;
        reason = 0;
        isValid = false;
    }
};


//机场状态字段
struct AirportInformation
{
    DroneChargeState droneChargeState{};//飞行器充电状态
    QVector<LiveStatus> vecLiveStatus{};//网关当前整体直播状态推送
    int droneInDock{};//飞行器是否在舱
    NetworkState networkState{};//网络状态
    SubDevice subDevice{};//子设备状态
    int flighttaskStepCode{};//机场任务状态
    double latitude{};//纬度
    double longitude{};//经度
    QString status{};//飞行器的状态
    int coverState{};//机舱的状态：0关闭、1开盖、2半开盖
    int modecode{};//机场状态
    int flighttaskstepcode{};//机场任务状态
    int rainfall{}; //机场降雨
    double windspeed{}; //机场风速
    double environment_temperature; //温度
    QString gateway{};//网关设备的 SN

    void clear() {
        droneChargeState.clear();
        vecLiveStatus.clear();
        droneInDock = 0;
        networkState.clear();
        subDevice.clear();
        flighttaskStepCode = 0;
        latitude = 0;
        longitude = 0;
        modecode = 0;
        flighttaskstepcode = 0;
        status.clear();
        gateway.clear();
    }
};

//无人机管理平台登录信息
struct DroneManagementPlatformLoginInfo {
    QString userName{};
    QString userId{};
    QString workspaceId{};
    int userType{};
    QString mqttUserName{};
    QString mqttPassword{};
    QString accessToken{};
    QString mqttAddr{};
    QString ip{};
    int port{};
};


struct DroneCtrlInfo {
    QString address{};
    QString username{};
    QString password{};
    QString client_id{};
    QString expire_time{};
    QString enable_tls{};
};

struct CollectionTaskHistoryInfo {
    QString taskName{};
    QString collector{};
    QString collectionTime{};
    int collectionDuration{};
    QString collectionStatus{};
};

struct TrackName {
    int trackId;
    QString trackName;
    int trackType;
};

// 船舶档案结构体
struct ShipFileInfo {
    qint64 targetId = 0;
    QUuid collectionId = QUuid();
    QString sensorName;
    QDateTime collectionTime;
    QString CallSign;
    int mmsi;
    QString targetType;
    QString shipLength;
    QString shipWidth;
    QString shipDraft;
    QString Condition;
    QString Destination;
    QString ArrTime;
    unsigned int SearchTime;
    QString Updatetime;
    mutable QList<QByteArray> imageBlob;
};

//屏幕信息
struct ScreenInfo {
    QPoint topLeft; // 屏幕左上角坐标  
    QSize size;     // 屏幕大小  
};

//检测框标注信息
struct VideoRectLabel {
    int behaviorID = 0;
};

//7天气象预报结构体
struct DailyWeatherInfo {
    QVector <QString> DayWeather;//白天天气情况
    QVector <QString> NightWeather;//晚上天气情况
    QVector <QString> lowestTemp;//最低温度
    QVector <QString> highestTemp;//最高温度
};

//每3小时气象信息结构体
struct HourWeatherInfo {
    QVector<QString> Time;//时间
    QVector<QString> Weather;//天气情况
    QVector<QString> Temp;//温度
    QVector<QString> Rain;//降水
    QVector<QString> WindSpeed;//风速
    QVector<QString> WindDirection;//风向
    QVector<QString> Pressure;//气压
    QVector<QString> Humidity;//湿度
    QVector<QString> RainNum;//云量
};

//船舶档案结构体
struct BoatFileInfo {
    QString BoatFileCallNum;//呼号
    QString BoatFileHDG;//船首向
    QString BoatFileMMSI;//MMSI
    QString BoatFileCourseDeg;//航向
    QString BoatFileIMO;//IMO
    QString BoatFileSpeed;//航速
    QString BoatFileType;//类型
    QString BoatFileLat;//纬度
    QString BoatFileLon;//经度
    QString BoatFileState;//状态
    QString BoatFileShipLength;//船长
    QString BoatFileDestination;//目的地
    QString BoatFileShipwidth;//船宽
    QString BoatFileArrTime;//预到时间
    QString BoatFileDraft;//吃水
    QString BoatFileUpdateTime;//更新时间
    QString BoatFileSearchTime;//查询时间
};


//告警设置结构体
struct AlarmSettingInfo {
    QString alramName; //告警名称
    int alarmState; //告警状态 0-未启用 1-启用
    QString area_name;//告警区域名称
    int filterState;//过滤状态 0-未启用 1-启用
    int area_function;//区域功能：1-禁航 2-限速
    int track_type;//航迹类型：1-雷达航迹 2-AIS航迹 3-融合航迹
    int speed_condition;//速度判断：1:<  2:<= 3: =  4: >=  5: >
    int speed;//速度值
    int area_judge;//区域判断：1-进入区域 2-离开区域 3-靠近区域
    int direction;//航向： 1-远离 2-靠近  （只针对gate区域类型）
    int group_id;//组id
    int area_id;//区域id
    int ID;//告警条件id
};

//气象描述数据库结果
struct WeatherInfoFromDB {
    QList<QString> descriptionList;
    QList<QString> infoList;
};

//告警区域信息结构体
struct AlarmAreaInfo {
    QString area_name;//区域名称
    int area_type;//区域类型：1-矩形；2-圆形；3-多边形；4-线；5-文字
    int area_id;//区域id
    int group_id;//组id
    int warning_type;
};

//指纹识别数据结构体
struct FingerRecognitionInfo {
    unsigned int m_MMSI_Decode;  //个体ID解码结果 默认"####"
    unsigned int m_MMSI_Recognition;//个体ID识别结果 默认"####"
    unsigned int m_newTargetFlag; //新个体标志 0：已注册 1：新个体
    unsigned int m_warningFlag;    //告警标志 0：正常 1：告警
    float m_confRecognition;  //识别置信度
    float m_confMatch;   //匹配置信度 未进行匹配处理 -1
    qint64 time; //更新时间
    QString position; //位置
    QString rec_result; //判别结果
};


//告警规则结构体
struct AlarmRule {
    QString condition_id; //规则id
    QString name; //规则名称
    int alarmstate; //告警状态
    int filterstate = 0;
    QString area_name;
    int area_function = 0; //区域功能
    int track_type = 0;
    int speed_condition;
    int speed;
    int area_judge;
    int direction;
    int group_id;
    int area_id;
    int alarm_level;
    int distance_condition;
    int distance;
    QString area_st;
    QString area_et;
    int area_cycle;
    QString targetattr_st;
    QString targetattr_et;
    int targetattr_cycle;
    int targetattr_side;
    int targetattr_ifmil;
    int targetattr_type;
    QString targetbh_st;
    QString targetbh_et;
    int targetbh_cycle;
    int targetbh_type;
    int height_min = 0;
    int height_max = 100;
    int course_min = 0;
    int course_max = 360;
    int blacklist_judge = 0;
    int whitelist_judge = 0;
    int affiliation_judge = 0;
    int corp_judge = 0;
    int area_type = 1;
    int isBirdTrack = -1;

    // 新增AIS相关字段
    int has_ais = 1; // 是否有AIS信息：0-无，1-有
    int ais_type = 0; // AIS目标类型：0-全部，1-客船，2-货船，3-油轮，4-渔船，5-军用船舶，99-其他
    int ais_behavior = 0; // AIS行为类型：0-全部，1-正常航行，2-抛锚，3-搁浅，4-失控，5-拖拽，6-捕鱼，99-其他

    // 新增其他检测相关字段
    int camera_detect_type = 0; // 相机检测目标类型位掩码
    int llm_detect_type = 0; // 大模型研判目标类型位掩码
    int uav_detect_type = 0; // 无人机检测目标类型位掩码

    // 威胁度相关字段
    int threat_level1 = 0; // 查证阈值
    int threat_level2 = 0; // 告警阈值
    int dist_to_protect_area = -1; // 到保护区距离：-1为不考虑
    int entry_time = 0;
};

/** 告警识别规则子表 alarm_identification_rules_sub（主表 alarm_identification_rules.rule_id，一对多 criteria） */
struct AlarmIdentificationRuleSub {
    int id = 0;
    QString rule_id;
    QString criteria_id;
    int criteria_type = 0;
    int criteria_order = 1;
    QString detection_rules_json; /**< jsonb 原文，可再用 QJsonDocument::fromJson 解析 */
    QString logic_operator = QStringLiteral("AND");
    bool enabled = true;
    QDateTime created_at;
    QDateTime updated_at;
};

/** 区域处置规则绑定 area_handle_rules（激活 scheme 下 group_id + area_id 关联的处置方案） */
struct AreaHandleRuleBinding {
    int id = 0;
    QString handle_scheme_id;
    QString handle_scheme_name;
    int priority = 0;
    bool enabled = true;
    QDateTime created_at;
};

//告警数据结构体
struct AlarmData {
    QString alarm_id;//唯一ID，
    QString condition_id;//告警规则,
    int mode_id;//告警类型1
    int track_id;
    long unique_id = 0;
    int targettype;
    int targetbehavior;
    double targetdist;
    double targetlon;
    double targetlat;
    double targetspeed;
    double targetdir;
    QString time;
    QString origintime;
    int alarm_status;//告警状态  0新生成，1已读，2删除。
    int alarm_count = 0;
    int group_id;
    int area_id;
    int threatScore = 0;
    int task_status = 0;
    /** system_evaluation_data.threat_time（毫秒）：新威胁落库时写入，与 TrackAlarmThread 日志 timeOrigin/timeNow 语义一致 */
    qint64 threat_time_ms = 0;
};

/** 规则告警有效处置状态（与 DDS taskStatus / gRPC disposition 对齐） */
struct AlarmEffectiveDisposition {
    int task_status = 0;
    QString resolved_time_str;
};

struct AreaInfo {
    int groupID;
    int areaID;
    QString groupName;
    QString areaName;
    int areaType;
    QString stratPoint;
    QString endPoint;
    QString areaRect;
    QString areaPoints;
    int lineWidth;
    QString lineColor;
    int checkState;
    int waringType;
    int waringTime;
    int protectGroupID = -1; // 保护区组ID
    int protectAreaID = -1;  // 保护区区域ID

};
struct AlarmArea {
    int groupID;
    int areaID;
    QString groupName;
    QString areaName;
    int m_alertAreaType;//1矩形；2圆形；3多边形；4直线
    QPointF m_startP;
    QPointF m_endP;
    QRectF m_alertAreaRect;
    QPolygonF m_alertAreaPolygon;
    QVector<QPointF> m_road;
    int lineWidth;
    QString lineColor;
    int checkState;
    int waringType;
    int waringTime;

};

struct TargetInfoFilter {
    int reid;
    int black_white_attr; // 黑白名单属性：1-白名单，2-黑名单，0-普通
    int affiliation_attr; // 敌我识别属性：1-敌方，2-我方，3-友方，4-中立，5-不明
    int corp_attr; // 合作目标属性：1-合作，0-非合作

    // 注意：检测类型和威胁度信息现在从cognitive_results_comprehensive表中实时获取
    // 不再存储在TargetInfoFilter中
};

struct UavSpeakerText {
    QString id;
    int mode;
    QString name;
    QString content;
    QString time;
    QString md5;
    QString file_id;
};

struct AlarmPicInfo
{
    QString alarm_id;
    QDateTime pic_time;
    QByteArray imageBlob;
};
struct SelfPoseTrack
{
    int id;
    int type;//0海面，1空中
    int side;//0红方，1蓝方，2白方
    float altitude;
    float lat;
    float lon;
    float speed;
    float speed_v;
    float course;
    long timesec;
    long timeusec;
    float attitude_head;        //飞机偏航轴角度
    float attitude_pitch;       //飞机俯仰轴角度
    float attitude_roll;        //飞机横滚轴角度
    double gimbal_pitch;        //云台俯仰轴角度 范围-180到180
    double gimbal_roll;         //云台横滚轴角度 范围-180到180
    double gimbal_yaw;          //云台偏航轴角度 范围-180到180
    char sn[32];                //无人机sn
    char device_name[32];
};


// 敌我属性枚举
enum AffiliationEnum {
    AFFILIATION_ENEMY = 0,      // 敌方
    AFFILIATION_OWNERSHIP = 1,  // 己方
    AFFILIATION_FRIENDLY = 2,   // 友方
    AFFILIATION_NEUTRAL = 3,    // 中性
    AFFILIATION_UNKNOWN = 4     // 未知
};

// 数据源枚举
enum SourceEnum {
    SOURCE_AIS = 0,             // AIS系统
    SOURCE_LLM = 1,             // 大语言模型
    SOURCE_FUSION_ENGINE = 2,   // 融合引擎
    SOURCE_RADAR = 3,           // 雷达
    SOURCE_MANUAL = 4,          // 人工
    SOURCE_ARCHIVE = 5,         // 档案库
    SOURCE_UNKNOWN = 6          // 未知来源
};

// 目标威胁度评估相关定义
enum TargetTypeForThreat {
    TARGET_AIR = 1,     // 对空目标
    TARGET_SEA = 2      // 对海目标
};

// 具体目标类型枚举
enum SeaTargetType {
    SEA_SPEEDBOAT = 1,      // 快艇 - 10分
    SEA_WARSHIP = 2,        // 军艇 - 10分
    SEA_MOTORBOAT = 3,      // 摩托艇 - 8分
    SEA_FISHING_BOAT = 4,   // 渔船 - 4分
    SEA_SHIP = 5,           // 舰艇 - 4分
    SEA_CARGO_SHIP = 6,     // 货船 - 2分
    SEA_UNKNOWN = 7,        // 未知 - 5分
    SEA_BUOY = 8            // 浮标 - 0分
};

enum AirTargetType {
    AIR_DRONE = 1,          // 无人机 - 10分
    AIR_DRONE_SWARM = 2,    // 无人机群 - 10分
    AIR_COMPOUND_WING = 3,  // 复合翼 - 10分
    AIR_ROTORCRAFT = 4,     // 旋翼 - 10分
    AIR_AIRCRAFT = 5,       // 飞机 - 0分
    AIR_BIRD = 6,           // 飞鸟 - 0分
    AIR_BIRD_FLOCK = 7      // 鸟群 - 0分
};

enum CombatIntention {
    INTENTION_ATTACK = 1,           // 攻击 - 10分
    INTENTION_RECONNAISSANCE = 2,   // 侦察 - 8分  
    INTENTION_PATROL = 3,          // 巡逻 - 6分
    INTENTION_TRAINING = 4,        // 训练 - 4分
    INTENTION_TRANSIT_OPERATION = 5, // 过境作业 - 0分
    INTENTION_TRANSIT_RETURN = 6,   // 过境返港 - 0分
    INTENTION_CARGO_ROUTE = 7,      // 货船航线 - 0分
    INTENTION_OTHER = 8             // 其他 - 2分
};

struct ThreatAssessmentInput {
    int payloadScore;           // 所带载荷评分（默认0分）
    int stealthScore;           // 隐身性能评分（默认0分）
    TargetTypeForThreat targetType;  // 目标类型（对空/对海）
    int specificTargetType;     // 具体目标类型（对应SeaTargetType或AirTargetType的值）
    CombatIntention intention;  // 作战意图
    double entryAngle;          // 进入角（度）
    double speed;               // 速度
    double distance;            // 距离

    ThreatAssessmentInput()
        : payloadScore(0), stealthScore(0), targetType(TARGET_AIR), specificTargetType(AIR_DRONE),
        intention(INTENTION_OTHER), entryAngle(0.0), speed(0.0), distance(0.0) {}
};

struct ThreatAssessmentResult {
    double targetTypeScore;     // 目标类型评分 s(type)
    double capabilityScore;     // 目标能力评分 s(θ)
    double intentionScore;      // 目标意图评分 s(φ)  
    double opportunityScore;    // 目标机会评分 s(ω)
    double speedScore;          // 速度评分 s(v)
    double distanceScore;       // 距离评分 s(l)
    double totalThreatLevel;    // 总威胁度
    QString threatDescription;  // 威胁等级描述

    ThreatAssessmentResult()
        : capabilityScore(0.0), intentionScore(0.0), opportunityScore(0.0),
        totalThreatLevel(0.0), threatDescription("无威胁") {}
};

// 全面认知结果结构体
struct CognitiveResultsComprehensive {
    // =================================================================
    // 1. 核心标识符与时间戳 (Core Identifiers & Timestamps)
    // =================================================================
    qint64 trackId;                     // 航迹ID
    qint64 uniqueId;                    // 唯一ID
    qint64 reId;                        // 重识别ID (用于解决航迹断续问题)
    QDateTime observedAt;               // 观测时间

    // =================================================================
    // 2. 最终一致性认知结果 (Final & Consistent Cognitive Results)
    // =================================================================
    QString finalTargetType;            // 最终目标类型
    SourceEnum finalTypeSource;         // 最终类型的决策来源
    QString finalTargetStatus;          // 最终目标状态
    SourceEnum finalStatusSource;       // 最终状态的决策来源
    AffiliationEnum finalAffiliation;   // 最终敌我属性
    SourceEnum finalAffiliationSource;  // 最终属性的决策来源
    int finalThreatLevel;               // 最终威胁度 (0-100)
    QString finalIntent;                // 最终综合意图

    // =================================================================
    // 3. 来源: 融合航迹引擎 (Source: Fused Track Engine)
    // =================================================================
    double fusedLongitude;              // 融合后的经度
    double fusedLatitude;               // 融合后的纬度
    float fusedAltitudeM;               // 融合后的高程 (米)
    float fusedSpeedKnots;              // 融合后的速度 (节)
    float fusedCourseDeg;               // 融合后的航向 (度)

    // =================================================================
    // 4. 来源: AIS系统 (Source: AIS)
    // =================================================================
    qint64 aisMmsi;                     // MMSI号
    qint64 aisImo;                      // IMO号
    QString aisCallsign;                // 呼号
    QString aisTargetTypeRaw;           // AIS上报的原始类型描述
    int aisNavStatusCode;               // AIS导航状态码
    double aisLengthM;                  // AIS上报的长度 (米)
    double aisWidthM;                   // AIS上报的宽度 (米)

    // =================================================================
    // 5. 来源: 船舶档案库 (Source: Ship Profile Archive)
    // =================================================================
    QString archiveTargetType;          // 档案库中的船舶类型
    double archiveLengthM;              // 档案库中的船长 (米)
    double archiveWidthM;               // 档案库中的船宽 (米)
    QString archiveDestination;         // 档案库中的目的地
    QString archiveStatus;              // 档案库中的状态

    // =================================================================
    // 6. 来源: 大语言模型/视觉模型 (Source: LLM/Vision Model)
    // =================================================================
    QString llmTargetType;              // LLM识别的目标类型
    float llmTargetTypeConfidence;      // LLM类型识别置信度
    QString llmIntentRaw;               // LLM推理的原始意图
    float llmDetectionConfidence;       // LLM检测置信度

    // =================================================================
    // 7. 记录元数据 (Record Metadata)
    // =================================================================
    QDateTime createdAt;                // 记录创建时间

    // =================================================================
    // 8. 时间目标类型 (Time Target Type)
    // =================================================================
    QString timeTargetType;              // 时间目标类型
    float timeTargetConfidence;          // 时间目标类型置信度

    // 构造函数
    CognitiveResultsComprehensive()
        : trackId(0), uniqueId(0), reId(0),
        finalTypeSource(SOURCE_UNKNOWN), finalStatusSource(SOURCE_UNKNOWN),
        finalAffiliation(AFFILIATION_UNKNOWN), finalAffiliationSource(SOURCE_UNKNOWN),
        finalThreatLevel(0),
        fusedLongitude(0.0), fusedLatitude(0.0), fusedAltitudeM(0.0f),
        fusedSpeedKnots(0.0f), fusedCourseDeg(0.0f),
        aisMmsi(0), aisImo(0), aisNavStatusCode(0),
        aisLengthM(0.0), aisWidthM(0.0),
        archiveLengthM(0.0), archiveWidthM(0.0),
        llmTargetTypeConfidence(0.0f), llmDetectionConfidence(0.0f),
        observedAt(QDateTime::currentDateTime()),
        createdAt(QDateTime::currentDateTime()),
        timeTargetType(""), timeTargetConfidence(0.0f) {}

    void clear() {
        trackId = 0;
        uniqueId = 0;
        reId = 0;
        observedAt = QDateTime::currentDateTime();
        createdAt = QDateTime::currentDateTime();
    }
};

// =================================================================
// 威胁度研判参数结构体 (对应数据库area_params表)
// =================================================================
struct ThreatAssessmentParams {
    int groupId;                    // 区域组ID
    int areaId;                     // 区域ID

    // 对海目标评分配置
    double seaSpeedboatScore;       // 快艇评分 (默认10.0)
    double seaWarshipScore;         // 军艇评分 (默认10.0)
    double seaMotorboatScore;       // 摩托艇评分 (默认8.0)
    double seaFishingBoatScore;     // 渔船评分 (默认4.0)
    double seaShipScore;            // 舰艇评分 (默认4.0)
    double seaCargoShipScore;       // 货船评分 (默认2.0)
    double seaUnknownScore;         // 未知海上目标评分 (默认5.0)
    double seaBuoyScore;            // 浮标评分 (默认0.0)

    // 对空目标评分配置
    double airDroneScore;           // 无人机评分 (默认10.0)
    double airDroneSwarmScore;      // 无人机群评分 (默认10.0)
    double airCompoundWingScore;    // 复合翼评分 (默认10.0)
    double airRotorcraftScore;      // 旋翼评分 (默认10.0)
    double airAircraftScore;        // 飞机评分 (默认0.0)
    double airBirdScore;            // 飞鸟评分 (默认0.0)
    double airBirdFlockScore;       // 鸟群评分 (默认0.0)

    // 意图评分配置
    double intentionAttackScore;          // 攻击意图评分 (默认10.0)
    double intentionReconnaissanceScore;  // 侦察意图评分 (默认8.0)
    double intentionPatrolScore;          // 巡逻意图评分 (默认6.0)
    double intentionTrainingScore;        // 训练意图评分 (默认4.0)
    double intentionTransitOperationScore; // 过境作业意图评分 (默认3.0)
    double intentionTransitReturnScore;   // 过境返港意图评分 (默认2.0)
    double intentionCargoRouteScore;      // 货船航线意图评分 (默认2.5)
    double intentionOtherScore;           // 其他意图评分 (默认5.0)

    // 权重配置
    double typeWeight;              // 类型判据权重 (默认0.420)
    double angleWeight;             // 进入角判据权重 (默认0.180)
    double speedWeight;             // 速度判据权重 (默认0.160)
    double distanceWeight;          // 距离判据权重 (默认0.240)

    // 速度判据参数
    double speedThresholdLow;       // 速度低阈值(m/s) (默认5.0)
    double speedThresholdHigh;      // 速度高阈值(m/s) (默认30.0)

    // 距离判据参数
    double maxEffectiveDistance;    // 最大有效距离(米) (默认50.0)

    // 威胁度阈值
    double highThreatThreshold;     // 高威胁阈值 (默认70.0)
    double mediumThreatThreshold;   // 中等威胁阈值 (默认30.0)

    // 元数据
    QString createdAt;              // 创建时间
    QString updatedAt;              // 更新时间

    // 构造函数 - 设置默认值
    ThreatAssessmentParams()
        : groupId(0), areaId(0),
        // 对海目标评分默认值
        seaSpeedboatScore(10.0), seaWarshipScore(10.0), seaMotorboatScore(8.0),
        seaFishingBoatScore(4.0), seaShipScore(4.0), seaCargoShipScore(2.0),
        seaUnknownScore(5.0), seaBuoyScore(0.0),
        // 对空目标评分默认值
        airDroneScore(10.0), airDroneSwarmScore(10.0), airCompoundWingScore(10.0),
        airRotorcraftScore(10.0), airAircraftScore(0.0), airBirdScore(0.0),
        airBirdFlockScore(0.0),
        // 意图评分默认值
        intentionAttackScore(10.0), intentionReconnaissanceScore(8.0),
        intentionPatrolScore(6.0), intentionTrainingScore(4.0),
        intentionTransitOperationScore(3.0), intentionTransitReturnScore(2.0),
        intentionCargoRouteScore(2.5), intentionOtherScore(5.0),
        // 权重默认值
        typeWeight(0.420), angleWeight(0.180), speedWeight(0.160), distanceWeight(0.240),
        // 速度和距离参数默认值
        speedThresholdLow(5.0), speedThresholdHigh(30.0), maxEffectiveDistance(50.0),
        // 威胁度阈值默认值
        highThreatThreshold(70.0), mediumThreatThreshold(30.0),
        // 元数据
        createdAt(""), updatedAt("") {}

    // 带参数的构造函数
    ThreatAssessmentParams(int gId, int aId)
        : ThreatAssessmentParams() {
        groupId = gId;
        areaId = aId;
    }

    // 拷贝构造函数
    ThreatAssessmentParams(const ThreatAssessmentParams& other) {
        *this = other;
    }

    // 赋值操作符
    ThreatAssessmentParams& operator=(const ThreatAssessmentParams& other) {
        if (this != &other) {
            groupId = other.groupId;
            areaId = other.areaId;

            // 对海目标评分
            seaSpeedboatScore = other.seaSpeedboatScore;
            seaWarshipScore = other.seaWarshipScore;
            seaMotorboatScore = other.seaMotorboatScore;
            seaFishingBoatScore = other.seaFishingBoatScore;
            seaShipScore = other.seaShipScore;
            seaCargoShipScore = other.seaCargoShipScore;
            seaUnknownScore = other.seaUnknownScore;
            seaBuoyScore = other.seaBuoyScore;

            // 对空目标评分
            airDroneScore = other.airDroneScore;
            airDroneSwarmScore = other.airDroneSwarmScore;
            airCompoundWingScore = other.airCompoundWingScore;
            airRotorcraftScore = other.airRotorcraftScore;
            airAircraftScore = other.airAircraftScore;
            airBirdScore = other.airBirdScore;
            airBirdFlockScore = other.airBirdFlockScore;

            // 意图评分
            intentionAttackScore = other.intentionAttackScore;
            intentionReconnaissanceScore = other.intentionReconnaissanceScore;
            intentionPatrolScore = other.intentionPatrolScore;
            intentionTrainingScore = other.intentionTrainingScore;
            intentionTransitOperationScore = other.intentionTransitOperationScore;
            intentionTransitReturnScore = other.intentionTransitReturnScore;
            intentionCargoRouteScore = other.intentionCargoRouteScore;
            intentionOtherScore = other.intentionOtherScore;

            // 权重配置
            typeWeight = other.typeWeight;
            angleWeight = other.angleWeight;
            speedWeight = other.speedWeight;
            distanceWeight = other.distanceWeight;

            // 速度和距离参数
            speedThresholdLow = other.speedThresholdLow;
            speedThresholdHigh = other.speedThresholdHigh;
            maxEffectiveDistance = other.maxEffectiveDistance;

            // 威胁度阈值
            highThreatThreshold = other.highThreatThreshold;
            mediumThreatThreshold = other.mediumThreatThreshold;

            // 元数据
            createdAt = other.createdAt;
            updatedAt = other.updatedAt;
        }
        return *this;
    }

    // 清除方法
    void clear() {
        groupId = 0;
        areaId = 0;

        // 重置为默认值
        seaSpeedboatScore = 10.0;
        seaWarshipScore = 10.0;
        seaMotorboatScore = 8.0;
        seaFishingBoatScore = 4.0;
        seaShipScore = 4.0;
        seaCargoShipScore = 2.0;
        seaUnknownScore = 5.0;
        seaBuoyScore = 0.0;

        airDroneScore = 10.0;
        airDroneSwarmScore = 10.0;
        airCompoundWingScore = 10.0;
        airRotorcraftScore = 10.0;
        airAircraftScore = 0.0;
        airBirdScore = 0.0;
        airBirdFlockScore = 0.0;

        intentionAttackScore = 10.0;
        intentionReconnaissanceScore = 8.0;
        intentionPatrolScore = 6.0;
        intentionTrainingScore = 4.0;
        intentionTransitOperationScore = 3.0;
        intentionTransitReturnScore = 2.0;
        intentionCargoRouteScore = 2.5;
        intentionOtherScore = 5.0;

        typeWeight = 0.420;
        angleWeight = 0.180;
        speedWeight = 0.160;
        distanceWeight = 0.240;

        speedThresholdLow = 5.0;
        speedThresholdHigh = 30.0;
        maxEffectiveDistance = 50.0;

        highThreatThreshold = 70.0;
        mediumThreatThreshold = 30.0;

        createdAt.clear();
        updatedAt.clear();
    }

    // 验证参数有效性
    bool isValid() const {
        // 检查权重总和是否接近1.0
        double weightSum = typeWeight + angleWeight + speedWeight + distanceWeight;
        if (abs(weightSum - 1.0) > 0.001) {
            return false;
        }

        // 检查速度阈值
        if (speedThresholdHigh <= speedThresholdLow) {
            return false;
        }

        // 检查威胁度阈值
        if (highThreatThreshold <= mediumThreatThreshold) {
            return false;
        }

        // 检查评分范围 (0-100)
        QList<double> scores = {
            seaSpeedboatScore, seaWarshipScore, seaMotorboatScore, seaFishingBoatScore,
            seaShipScore, seaCargoShipScore, seaUnknownScore, seaBuoyScore,
            airDroneScore, airDroneSwarmScore, airCompoundWingScore, airRotorcraftScore,
            airAircraftScore, airBirdScore, airBirdFlockScore,
            intentionAttackScore, intentionReconnaissanceScore, intentionPatrolScore,
            intentionTrainingScore, intentionTransitOperationScore, intentionTransitReturnScore,
            intentionCargoRouteScore, intentionOtherScore,
            highThreatThreshold, mediumThreatThreshold
        };

        for (double score : scores) {
            if (score < 0.0 || score > 100.0) {
                return false;
            }
        }

        // 检查权重范围 (0-1)
        QList<double> weights = { typeWeight, angleWeight, speedWeight, distanceWeight };
        for (double weight : weights) {
            if (weight < 0.0 || weight > 1.0) {
                return false;
            }
        }

        return true;
    }

    // 获取目标类型评分
    double getSeaTargetScore(SeaTargetType targetType) const {
        switch (targetType) {
        case SEA_SPEEDBOAT: return seaSpeedboatScore;
        case SEA_WARSHIP: return seaWarshipScore;
        case SEA_MOTORBOAT: return seaMotorboatScore;
        case SEA_FISHING_BOAT: return seaFishingBoatScore;
        case SEA_SHIP: return seaShipScore;
        case SEA_CARGO_SHIP: return seaCargoShipScore;
        case SEA_UNKNOWN: return seaUnknownScore;
        case SEA_BUOY: return seaBuoyScore;
        default: return seaUnknownScore;
        }
    }

    double getAirTargetScore(AirTargetType targetType) const {
        switch (targetType) {
        case AIR_DRONE: return airDroneScore;
        case AIR_DRONE_SWARM: return airDroneSwarmScore;
        case AIR_COMPOUND_WING: return airCompoundWingScore;
        case AIR_ROTORCRAFT: return airRotorcraftScore;
        case AIR_AIRCRAFT: return airAircraftScore;
        case AIR_BIRD: return airBirdScore;
        case AIR_BIRD_FLOCK: return airBirdFlockScore;
        default: return 0.0;
        }
    }

    double getIntentionScore(CombatIntention intention) const {
        switch (intention) {
        case INTENTION_ATTACK: return intentionAttackScore;
        case INTENTION_RECONNAISSANCE: return intentionReconnaissanceScore;
        case INTENTION_PATROL: return intentionPatrolScore;
        case INTENTION_TRAINING: return intentionTrainingScore;
        case INTENTION_TRANSIT_OPERATION: return intentionTransitOperationScore;
        case INTENTION_TRANSIT_RETURN: return intentionTransitReturnScore;
        case INTENTION_CARGO_ROUTE: return intentionCargoRouteScore;
        case INTENTION_OTHER: return intentionOtherScore;
        default: return intentionOtherScore;
        }
    }
};

Q_DECLARE_METATYPE(CaptureTargetItem)
Q_DECLARE_METATYPE(RadarEchoCollection)
Q_DECLARE_METATYPE(VideoCollection)
Q_DECLARE_METATYPE(SonarCollection)
Q_DECLARE_METATYPE(AISCollectionRecord)
Q_DECLARE_METATYPE(TrackInfo)
Q_DECLARE_METATYPE(TrackCollectionRecord)
Q_DECLARE_METATYPE(ADSBCollectionRecord)
Q_DECLARE_METATYPE(ShipFileInfo)
Q_DECLARE_METATYPE(ModalType)
Q_DECLARE_METATYPE(ThreatAssessmentParams)
