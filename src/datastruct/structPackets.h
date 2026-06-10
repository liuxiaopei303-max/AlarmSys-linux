#pragma once

#ifndef STRUCTHELP_H
#define STRUCTHELP_H
#ifdef _MSC_BUILD
#pragma execution_character_set("utf-8")
#endif
#include "QVector"
#include "QDateTime"
#include <QBuffer>
#include <QImage>
#include <QPointF>
//#include <NMEAParser/NMEAParserData.h>

//#define SAMPLE 4096
#define	 SPX_PACKET_TYPEB_RADAR_CONFIG	0x101//config类型
#define	 SPX_PACKET_TYPEB_TOC			0x140 //table TOC类型
#define	SPX_PACKET_TYPEB_NMEA			0x131	/* NULL-terminated $ string */
#define	SPX_PACKET_TYPEB_SERIAL			0x132	/* Raw (binary?) data */
#define	SPX_PACKET_TYPEB_ADSB			0x133	/* Binary message */
#define	SPX_PACKET_TYPEB_AIS			0x134	/* Like NMEA, but !. */
#define	SPX_PACKET_TYPEB_RADAR_RETURN	0x102  //回波类型
#define	SPX_PACKET_TYPEB_PLOT           0x113  //回波类型
#define	SPX_PACKET_TYPEB_TRACK          0x111  //回波类型



#define	SPX_RIB_PACKING_RAW8		0   /* 8 bit samples uncompressed */
#define	SPX_RIB_PACKING_RAW4		1   /* 4 bit samples tightly packed */
#define	SPX_RIB_PACKING_RAW2		2   /* 2 bit samples tightly packed */
#define	SPX_RIB_PACKING_RAW1		3   /* 1 bit samples tightly packed */
#define	SPX_RIB_PACKING_RAW4_1111	4   /* 4+1+1+1+1 bits per byte */
#define	SPX_RIB_PACKING_RAW5_111	5   /* 5+1+1+1 bits per byte */
#define	SPX_RIB_PACKING_RAW6_11		6   /* 6+1+1 bits per byte */
#define	SPX_RIB_PACKING_RAW7_1		7   /* 7+1 bits per byte */
#define	SPX_RIB_PACKING_RAW16		8   /* 16-bits, LS-byte first */
#define	SPX_RIB_PACKING_RAW4_4		9   /* 4-bits A (MSB), 4-bits B (LSB)*/
#define	SPX_RIB_PACKING_ORC		10  /* RAW8 ORC compressed */
#define	SPX_RIB_PACKING_ZLIB		20  /* RAW8 ZLib compressed */
#define SPX_RIB_PACKING_RAW12           30  /* 12-bit sample in MSBs */
#define SPX_RIB_PACKING_RAW12_1111      31  /* 12+1+1+1+1 */
#define SPX_RIB_PACKING_RAW8_8		40  /* 8-bits A plus 8-bits B */
#define SPX_RIB_PACKING_RAW8_7_1	41  /* 8-bits + 7-bits + 1-bit */
#define SPX_RIB_PACKING_RAW8_6_11	42  /* 8-bits + 6-bits + 1+1-bit */
#define SPX_RIB_PACKING_RAW8_5_111	43  /* 8-bits + 5-bits + 1+1+1-bit */
#define SPX_RIB_PACKING_RAW8_4_1111	44  /* 8-bits + 4-bits + 1+1+1+1-bit */
/* See note above regarding the next two values. */
#define	SPX_RIB_PACKING_OPTIMAL		254 /* Never put in headers */
#define	SPX_RIB_PACKING_INVALID		255 /* Invalid */

/* Used in compression functions to mean uncompressed data. */
#define	SPX_RIB_PACKING_RAW SPX_RIB_PACKING_RAW8

/* Maximum number of bytes per sample that is supported. */
#define SPX_RIB_PACKING_SAMPLE_BYTES_MAX 2


typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef long long INT64;
typedef short INT16;
typedef signed char INT8;	/* Needs "signed" for windows */
typedef unsigned char UCHAR;
typedef float REAL32;
typedef double REAL64;

struct HEHDT_Struct{
    int mmsi=-1;
    UINT32 time = QDateTime::currentDateTime().toTime_t();
    float heading=0.0f;
};

//struct RMC_Struct:public CNMEAParserData::_RMC_DATA_T{
//    RMC_Struct(_RMC_DATA_T& t){
//        m_timeGGA = t.m_timeGGA;
//        m_nHour = t.m_nHour;
//        m_nMinute = t.m_nMinute;
//        m_nSecond = t.m_nSecond;
//        m_dSecond = t.m_dSecond;
//        m_dLatitude = t.m_dLatitude;
//        m_dLongitude = t.m_dLongitude;
//        m_dAltitudeMSL = t.m_dAltitudeMSL;
//        m_nStatus = t.m_nStatus;
//        m_dSpeedKnots = t.m_dSpeedKnots;
//        m_dTrackAngle = t.m_dTrackAngle;
//        m_nMonth = t.m_nMonth;
//        m_nDay = t.m_nDay;
//        m_nYear = t.m_nYear;
//        m_dMagneticVariation = t.m_dMagneticVariation;
//    }
//    RMC_Struct(){}
//    int mmsi=-1;
//    UINT32 time = QDateTime::currentDateTime().toTime_t();
//};

//更新海康摄像头范围
struct HCNSection{
    int ID=1;
    float m_fHorizontalValue=0.;
    float m_fVerticalValue=0.;
    double m_range=6000;
    double m_Azimuth;
};

struct VideoConfig{
    float panOffset=0;
    float boatheight =0;
    float targetWidth=0;
    float ccdLength=0;
    float minJiaoju=0;
    int targetSreenScale=0;
};

struct PlotPacket;
struct TrackPacket;
struct RadarPacket;

struct DB_Data{
    int mmsi=-1;
    UINT32 time=-1;
    QByteArray data;
    DB_Data(const  QByteArray& data){
        time = QDateTime::currentDateTime().toTime_t();
        this->data=data;
    }
    DB_Data(){
        time = QDateTime::currentDateTime().toTime_t();
    }
};
//struct AIS_ServerJPG:public DB_Data{
//    AIS_ServerJPG(const  QByteArray& data){
//        time = QDateTime::currentDateTime().toTime_t();
//        this->data=data;
//    }
//    AIS_ServerJPG(){
//        time = QDateTime::currentDateTime().toTime_t();
//    }
//};

struct AIS_ClientJPG:public DB_Data{
    AIS_ClientJPG(const  QByteArray& data){
        time = QDateTime::currentDateTime().toTime_t();
        this->data=data;
    }
    AIS_ClientJPG(){
        time = QDateTime::currentDateTime().toTime_t();
    }
};

struct SHIP_INFO{
    QString MMSI ;
    QString HeadingDirection ;
    QString CallSign ;
    QString TrackDirection ;
    QString IMO ;
    QString SailingSpeed ;
    QString Type ;
    QString Latitude ;
    QString State ;
    QString Longitude ;
    QString LOA ;
    QString Destination ;
    QString Breadth ;
    QString EstimatedArrivalTime ;
    QString Draft ;
    QString AISUpdateTime ;
    QString ShipName;
};
struct SHIP_IMAGE{
    int ID;
    QString MMSI;
    QByteArray Image;
    void loadImage(const QImage &img){
        {
            QBuffer buffer(&Image);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer,"PNG");
            buffer.close();
        }
        if(Image.isEmpty()){
            QBuffer buffer(&Image);
            buffer.open(QIODevice::WriteOnly);
            img.save(&buffer,"JPG");
            buffer.close();
        }
    }
};

struct WARSHIP_INFO{
    QString ShipNumber;
    QString ShipName;
    QString Type;
    QString Drain;
    QString LOA;
    QString Breadth;
    QString Remark;
};

struct WARSHIP_IMAGE{
    int ID=0;
    QString ShipNumber;
    QByteArray Image;
};
//struct StaticAIS_info{
//    QString MMSI;
//    QString head;
//    QString CallSign;
//    QString TrackDirection;
//    QString IMO;
//    QString SailingSpeed;
//    QString Type;
//    QString Lat;
//    QString State;
//    QString Lon;
//    QString LOA;
//    QString Destination;
//    QString Breadth;
//    QString EstimatedArrivalTime;
//    QString Draft;
//    QString AISUpdateTime;
//    StaticAIS_info(){

//    }
//    StaticAIS_info(QStringList& list){
//        if(list.length()==16){
//            MMSI = list[0];
//            head = list[1];
//            CallSign = list[2];
//            TrackDirection = list[3];
//            IMO = list[4];
//            SailingSpeed = list[5];
//            Type = list[6];
//            Lat = list[7];
//            State = list[8];
//            Lon = list[9];
//            LOA = list[10];
//            Destination = list[11];
//            Breadth = list[12];
//            EstimatedArrivalTime = list[13];
//            Draft = list[14];
//            AISUpdateTime = list[15];
//        }
//    }
//    StaticAIS_info(QString& str){
//        QStringList list = str.split('&');
//        if(list.length()==16){
//            MMSI = list[0];
//            head = list[1];
//            CallSign = list[2];
//            TrackDirection = list[3];
//            IMO = list[4];
//            SailingSpeed = list[5];
//            Type = list[6];
//            Lat = list[7];
//            State = list[8];
//            Lon = list[9];
//            LOA = list[10];
//            Destination = list[11];
//            Breadth = list[12];
//            EstimatedArrivalTime = list[13];
//            Draft = list[14];
//            AISUpdateTime = list[15];
//        }
//    }
//};

typedef struct
{
    UINT16 magic16;//直接写0x4342
    UINT16 packetType;//类型  0x0110代表MinimalPacket,0x0111代表NormalPacket,0x0112 代表ExtendedPacket
    UINT32 totalSize;//总共大小

    UINT32 timeSec;//时间从系统获取
    UINT32 timeUsec;
}SPxHeader;


/* Plot reports. */
//typedef struct SPxPacketPlot_tag {
//    UINT8 senderID; /* Sender identification */
//    UINT8 reserved01; /* Reserved, set to zero */
//    UINT8 reserved02; /* Reserved, set to zero */
//    UINT8 reserved03; /* Reserved, set to zero */
//    UINT32 reserved04; /* Reserved, set to zero */
//    /* Position */
//    REAL32 rangeMetres; /* Polar position */
//    REAL32 azimuthDegrees; /* Polar position */
//    UINT32 reserved16; /* Reserved, set to zero */
//    UINT32 reserved20; /* Reserved, set to zero */
//    /* Bounding box. */
//    REAL32 startRangeMetres;
//    REAL32 endRangeMetres;
//    REAL32 startAzimuthDegrees;
//    REAL32 endAzimuthDegrees;
//    /* Quality information. */
//    UINT32 weight; /* Typically number of samples used in plot */
//    UINT32 strength; /* Typically sum of values of samples used */
//    UINT32 numComponents; /* How many small plots merged to make this */
//    UINT32 reserved52; /* Reserved, set to zero */
//    UINT32 reserved56; /* Reserved, set to zero */
//    UINT32 reserved60; /* Reserved, set to zero */
//    /* Enhanced information if available from sensor. */
//    UINT32 enhancedFlags; /* Which of the following fields are valid */
//    REAL32 radialSpeedMps; /* Positive towards sensor, negative away */
//    UINT16 iffMode3A; /* Mode 3/A code, or zero if not known */
//    UINT16 iffModeC; /* Mode C code, or zero if not known */
//    UINT16 iffMode2; /* Mode 2 code, or zero if not known. */
//    UINT16 iff12BitMode1; /* 12-bit Mode 1 code, or zero if not known. */
//    UINT8 iffMode1; /* 6-bit Mode 1 code, or zero if not known. */
//    UINT8 iffFlags; /* SPX_PACKET_TRACK_SECONDARY_IFF_FLAGS_... */
//    UINT16 reserved80; /* Reserved, set to zero */
//    UINT32 reserved84; /* Reserved, set to zero */
//    UINT32 reserved88; /* Reserved, set to zero */
//    UINT32 reserved92; /* Reserved, set to zero */
//    UINT32 reserved96; /* Reserved, set to zero */
//} SPxPacketPlot ;

///* Minimal track reports - Java and .NET may require update if this structure
// * is changed.
// */
//typedef struct SPxPacketTrackMinimal_tag {
//    UINT32 id;			/* Track ID (public) */
//    UINT8 senderID;		/* Sender identification */
//    UINT8 status;		/* Track status (SPX_PACKET_ALARM_STATUS_...) */
//    UINT8 numCoasts;		/* Number of consecutive coasts */
//    UINT8 id_ttm;		/* TTM ID */
//    REAL32 rangeMetres;		/* Tracked Range */
//    REAL32 azimuthDegrees;	/* Tracked Azimuth */
//    REAL32 speedMps;		/* Speed */
//    REAL32 courseDegrees;	/* Course */
//    REAL32 sizeMetres;		/* Smoothed size in metres */
//    REAL32 sizeDegrees;		/* Smoothed size in degrees */
//    UINT32 weight;		/* Weight of target (number of samples) */
//    UINT32 strength;		/* Strength of target (sum of values) */
//    UINT8 flags;		/* Target flags (SPX_PACKET_TRACK_FLAG_...) */
//    UINT8 reserved1;		/* Reserved, set to zero. */
//    UINT16 reserved2;		/* Reserved, set to zero. */
//    UINT32 reserved3;		/* Reserved, set to zero */
//    UINT32 reserved4;		/* Reserved, set to zero */
//    UINT32 reserved5;		/* Reserved, set to zero */
//} SPxPacketTrackMinimal;

///* Normal track reports - Java and .NET may require update if this structure
// * is changed.
// */
//typedef struct SPxPacketTrackNormal_tag {
//    SPxPacketTrackMinimal min;	/* Everything in the minimal report first */
//    REAL32 xMetres;		/* Tracked cartesian position */
//    REAL32 yMetres;		/* Tracked cartesian position */
//    REAL32 measRange;		/* Measured polar position */
//    REAL32 measAzimuth;		/* Measured polar position */
//    REAL32 measSizeMetres;	/* Measured size in range */
//    REAL32 measSizeDegrees;	/* Measured size in azimuth */
//    REAL32 sdRange;		/* SD of range measurements */
//    REAL32 sdAzimuth;		/* SD of azimuth measurements */
//    REAL32 sdRangeSize;		/* SD of size-in-range measurements */
//    REAL32 sdAzimuthSize;	/* SD of size-in-azimuth measurements */
//    UINT16 numDetectionsPQ;	/* Recent hit count, P (LSB) out of Q (MSB) */
//    UINT16 trackClass;		/* Track class */
//    UINT32 reserved1;		/* Reserved, set to zero */
//    UINT32 reserved2;		/* Reserved, set to zero */
//    UINT32 reserved3;		/* Reserved, set to zero */
//} SPxPacketTrackNormal;

//struct RadarConfig {
//    UINT32 ConfigTime = 0;
//    UINT64 ConfigOffset = 0;
//};




//struct PlotPacket {
//    /* Position */
//    REAL32 rangeMetres;  /* Polar position */
//    REAL32 azimuthDegrees; /* Polar position */
//    /* Bounding box. */
//    REAL32 startRangeMetres;
//    REAL32 endRangeMetres;
//    REAL32 startAzimuthDegrees;
//    REAL32 endAzimuthDegrees;
//    UINT32 reserved04; /*置信度*/
//    int ID;
//    PlotPacket(){

//    }
//    PlotPacket(SPxPacketPlot_tag& cpPlot){
//        rangeMetres=cpPlot.rangeMetres;
//        azimuthDegrees = cpPlot.azimuthDegrees;
//        startRangeMetres = cpPlot.startRangeMetres;
//        endRangeMetres = cpPlot.endRangeMetres;
//        startAzimuthDegrees = cpPlot.startAzimuthDegrees;
//        endAzimuthDegrees = cpPlot.endAzimuthDegrees;
//        reserved04 = cpPlot.reserved04;
//        ID = cpPlot.senderID;
//    }
//};


//struct TrackPacketBase{
//    UINT32 id; /* Track ID (public) 航迹号*/
//    UINT32 aisFusionMmsi;/*关联AIS的MMSI，如果没有AIS关联则为0*/
//    UINT32 createTime;//航迹起批时间
//    //    UINT8 status; /* Track status (enum TrackState {Tentative = 1, Confirmed = 2, Deleted = 3})*/
//    //    UINT8 numCoasts; /* Number of consecutive coasts */
//    REAL32 rangeMetres; /* Tracked Range 目标中心点距离 米*/
//    REAL32 azimuthDegrees; /* Tracked Azimuth  目标中心点方位角 度*/
//    REAL32 speedMps; /* Speed 航速*/
//    REAL32 courseDegrees; /* Course 航向*/
//    REAL32 sizeMetres; /* Smoothed size in metres 目标距离向长度 米*/
//    REAL32 sizeDegrees; /* Smoothed size in degrees 目标方位向长度  度*/

//    TrackPacketBase(){

//    }

//    TrackPacketBase(SPxPacketTrackNormal_tag& cptrack){
//        id = cptrack.min.id;
//        //        status = cptrack.min.status;
//        //        numCoasts = cptrack.min.numCoasts;
//        aisFusionMmsi=0;
//        rangeMetres = cptrack.min.rangeMetres;
//        azimuthDegrees = cptrack.min.azimuthDegrees;
//        speedMps = cptrack.min.speedMps;
//        courseDegrees = cptrack.min.courseDegrees;
//        sizeMetres = cptrack.min.sizeMetres;
//        sizeDegrees = cptrack.min.sizeDegrees;
//    }
//};

//struct TrackPacket:public TrackPacketBase {
//    //新增字段
//    UINT32 time;//接收（入库）时间
//    UINT32 detectionTime;//检测时间
//    UINT32 circleNum;//圈号
//    UINT32 radarMmsi;//所属雷达mmsi，若为固定站则为0或空
//    UINT32 radarCreateTime;//所属雷达安装时间
//    QString radarName;//所属雷达名

//    REAL64 lon=-190 ;//经度
//    REAL64 lat=-190 ;//纬度
//    double tcpa=-1;
//    double cpa=-1;
//    TrackPacket(){
//        time = QDateTime::currentDateTime().toTime_t();
//    }

//    TrackPacket(TrackPacketBase base){
//        time = QDateTime::currentDateTime().toTime_t();
//        id = base.id;
//        aisFusionMmsi=base.aisFusionMmsi;
//        //        status = cptrack.min.status;
//        //        numCoasts = cptrack.min.numCoasts;
//        rangeMetres = base.rangeMetres;
//        azimuthDegrees = base.azimuthDegrees;
//        speedMps = base.speedMps;
//        courseDegrees = base.courseDegrees;
//        sizeMetres = base.sizeMetres;
//        sizeDegrees = base.sizeDegrees;
//    }
//};

//struct UserTarget{
//    QString name; /* 名*/
//    QString shipNum; /*弦号*/
//    UINT32 type=-1; /*类型 0其他 1捕捞船 2拖船  3潜艇 4帆船 5客轮 6游艇 7军舰 8快艇 9货船 10充气船 11浮标 12潜器 13礁石  14人员 15危险船只*/
//    UINT32 trackid=-1; /* Track ID (public) 航迹号*/
//    UINT32 mmsi=-1;/*关联AIS的MMSI，如果没有AIS关联则为0*/
//    QString info;//信息
//    QString userName;//录入人员

//    UINT32 createTime=QDateTime::currentDateTime().toTime_t();//录入时间
//    REAL32 rangeMetres=-1; /* Tracked Range 目标中心点距离 米*/
//    REAL32 azimuthDegrees=-1; /* Tracked Azimuth  目标中心点方位角 度*/
//    REAL32 speedMps=-1; /* Speed 航速*/
//    REAL32 courseDegrees=-1; /* Course 航向*/
//    REAL32 sizeMetres=-1; /* Smoothed size in metres 目标距离向长度 米*/
//    REAL32 sizeDegrees=-1; /* Smoothed size in degrees 目标方位向长度  度*/
//    UINT32 radarMmsi=-1;//所属雷达mmsi，若为固定站则为0或空
//    UINT32 radarCreateTime=-1;//所属雷达安装时间
//    QString radarName;//所属雷达名
//    REAL64 lon=-190 ;//经度
//    REAL64 lat=-190 ;//纬度
//};
//struct UserTargetIMG{
//    UINT32 trackid=-1; /* Track ID (public) 航迹号*/
//    UINT32 createTime;//录入时间
//    QByteArray image;//图片

//    void loadImage(const QImage &img){
//        QBuffer buffer(&image);
//        buffer.open(QIODevice::WriteOnly);
//        img.save(&buffer, "PNG");
//        buffer.close();
//    }
//};

//#pragma pack(1)
typedef struct
{
    unsigned short vis_x;//目标在可见光中的x坐标(0~1920-1)
    unsigned short vis_y;//目标在可见光中的y坐标(0~1080-1)
    unsigned short id;//航迹ID(0~999)
    //    unsigned short reserve;//航迹ID(0~999)
    float distance_m;//航迹距离(m)
    float azi_deg; //航迹方位角(度)
}RadarToVis;//14字节
#pragma pack()


//struct PicHistory {
//    int boat, left, top, right, bottom;//图片框大小
//    double conf;//置信度

//    QVector<QPoint> history;  //（x,y）
//    QTime time;
//};


///*
// * Fusion information, sometimes included in SPxPacketTrackExtended rpts.
// * Update SPxRemoteServer and SPxFuseNetReport byte-swapping if this changes.
//*/

//#define SPX_MAX_NUM_TRACK_IDS	(8)
//typedef struct SPxPacketTrackFusion_tag {
//    /* Bitmask of which sensor types are supporting the track. */
//    UINT32 sensorTypes;		/* Combination of SPX_PACKET_TRACK_SENSOR_.. */

//    /* Bitmap of which sensors are supporting the track. LSB is
//     * sensor 0, etc.  Up to 32 sensors can be included.
//     */
//    UINT32 sensors;

//    /* Array of track IDs used for this track/report, to allow a receiver to
//     * interact with the originating server to get or set information for,
//     * or delete, tracks.  The number of entries in this array
//     * corresponds to the number of bits set in 'sensors'; trackId[0]
//     * corresponds to the least significant bit set, and so on.
//     */
//    UINT32 trackID[SPX_MAX_NUM_TRACK_IDS];

//    /* The maximum separation in metres between any two observations
//     * contributing to this track.
//     */
//    REAL32 maxRangeDiffMetres;

//    /* Reserved for future use. Set to zero. */
//    UINT32 reserved[21];
//} SPxPacketTrackFusion;

///*
// * Secondary track info, sometimes included in SPxPacketTrackExtended rpts.
// * Update SPxRemoteServer and SPxFuseNetReport byte-swapping if this changes.
// */
//typedef struct SPxPacketTrackSecondary_tag {
//    /* Indication of which sensor type is giving this information. */
//    UINT32 sensorType;		/* One of SPX_PACKET_TRACK_SENSOR_... */

//    /* Unique ID. */
//    UINT32 uniqueID;		/* E.g. MMSI, AA, or 0 if not available */

//    /* Name/label (up to 20 characters plus terminating NULL). */
//    char name[21];		/* Callsign, vessel name etc. */

//    /* Target information. */
//    UINT8 targetType;		/* E.g. Ship cargo type, or ADS-B cat+value */
//    UINT8 targetStatus;		/* E.g. AIS NavStatus, ADS-B capabilities */
//    UINT8 targetFlags;		/* Misc flags, e.g. IFF validities */

//    /* IFF codes, stored as octal values with three bits per digit, so the
//     * 4-digit codes use the least significant 12-bits and the 2-digit code
//     * uses the least significant 6-bits (even though bit 2 is never set).
//     * They should be printed as octal values in order to look correct to
//     * operators.
//     */
//    UINT16 iffMode3A;		/* Mode 3/A code, or zero if not known */
//    UINT16 iffMode2;		/* Mode 2 code, or zero if not known. */
//    UINT8 iffMode1;		/* 6-bit Mode 1 code, or zero if not known. */
//    UINT8 iffFlags;		/* SPX_PACKET_TRACK_SECONDARY_IFF_FLAGS_... */
//    UINT16 iff12BitMode1;	/* 12-bit Mode 1 code, or zero if not known. */

//    /* Target dimensions, if available. */
//    UINT16 targetWidth;		/* Target width in metres */
//    UINT16 targetLength;	/* Target length in metres */
//    REAL32 targetDraught;	/* Target draught in metres */
//    REAL32 targetHeading;	/* Target heading in degrees */

//    /* Reserved for future use. Set to zero. */
//    UINT32 reserved[19];
//} SPxPacketTrackSecondary;

///*
// * Threat info, sometimes included in SPxPacketTrackExtended rpts.
// * Update SPxRemoteServer and SPxFuseNetReport byte-swapping if this changes.
// */

//#define SPX_MAX_THREAT_NAME_LENGTH (31)
//typedef struct SPxPacketTrackThreat_tag
//{
//    /* Threat name ending with '\0'.  32-bytes, so 8 words. */
//    char name[SPX_MAX_THREAT_NAME_LENGTH + 1];

//    /* Type and level. */
//    UINT8 type;			/* One of SPX_PACKET_THREAT_TYPE... */
//    UINT8 level;		/* One of SPX_PACKET_THREAT_LEVEL... */
//    UINT8 flags;		/* Any of SPX_PACKET_THREAT_FLAG... */
//    UINT8 reserved11;

//    /* ID of app/operator that classified the threat. */
//    UINT32 setterID;

//    /* Timestamp of when app/operator changed the threat values. */
//    UINT32 lastChangedSecs;	/* Seconds since unix epoch */

//    /* Reserved fields for future use. */
//    UINT32 reserved12;		/* Reserved, set to zero */
//    UINT32 reserved13;		/* Reserved, set to zero */
//    UINT32 reserved14;		/* Reserved, set to zero */
//    UINT32 reserved15;		/* Reserved, set to zero */
//    UINT32 reserved16;		/* Reserved, set to zero */
//} SPxPacketTrackThreat;


///* Extended track reports.
// *
// * - Java and .NET may require update if this structure changed.
// *
// * NB: This is a variable-sized structure on the network, since most of
// * the fields are optional.  The 'extMask' field contains bit flags to
// * indicate which of the subsequent fields are present on the network.
// * If present, they will be in the order given here.
// *
// * Therefore, only use sizeof() this structure with extreme care.
// *
// * Furthermore, note that this structure is subject to change without
// * notice, including the size of the sensorSpecificData[] array.  Use
// * the sensorDataBytes field to determine how many bytes are actually
// * valid in the array, with sizeof(sensorSpecificData) indicating the
// * size of the array itself.
// *
// * SPxRemoteServer::TrackPackNet() and TrackUnpackNet() should be maintained
// * in conjunction with this structure.
// */
//typedef struct SPxPacketTrackExtended_tag {
//    /* Everything in the normal report first */
//    SPxPacketTrackNormal norm;

//    /* Size (in bytes) of the entire message on the net (including the
//     * 'norm' component, this size, the mask and the variable fields
//     * that are present, but excluding those that are missing on the net
//     * (and excluding the SPxPacketHeader).
//     */
//    UINT32 netSize;

//    /* Bitmask of which of the following fields are present. */
//    UINT32 extMask;

//    /* Optional fields, presence on network and validity in client structure
//     * controlled by bit flags in 'extMask' field above.
//     */
//    /* Radial velocity (extMask & SPX_PACKET_TRACK_EXT_RADIALSPEED). */
//    REAL32 radialSpeedMps;	/* Positive towards sensor, negative away */
//    REAL32 radialSpeedSD;	/* SD of radialSpeedMps */

//    /* Age of track (extMask & SPX_PACKET_TRACK_EXT_AGE). */
//    UINT32 age;			/* Age in scans (revolutions, or updates) */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG). */
//    REAL32 latDegs;		/* Tracked Latitude, positive north */
//    REAL32 longDegs;		/* Tracked Longitude, positive east */

//    /* Message time (extMask & SPX_PACKET_TRACK_EXT_MSGTIME).
//     * This is the time the message was sent, not the time of the report.
//     */
//    UINT32 msgTimeSecs;		/* Whole seconds since unix epoch */
//    UINT32 msgTimeUsecs;	/* Microseconds to be added to msgTimeSecs */

//    /* Sensor-specific data (extMask & SPX_PACKET_TRACK_EXT_SENSORDATA). */
//    UINT32 sensorDataBytes;	/* Number of bytes in sensorSpecificData */
//    UINT8 sensorSpecificData[32]; /* Sensor-specific data */

//    /* Altitude (extMask & SPX_PACKET_TRACK_EXT_ALTITUDE). */
//    REAL32 altitudeMetres;	/* Altitude in metres */

//    /* Fusion information (extMask & SPX_PACKET_TRACK_EXT_FUSION). */
//    SPxPacketTrackFusion fusion;	/* Fusion information */

//    /* Secondary information (extMask & SPX_PACKET_TRACK_EXT_SECONDARY). */
//    SPxPacketTrackSecondary secondary;	/* IFF/AIS/ADSB information etc. */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG_MEAS). */
//    REAL32 latDegsMeas;		/* Measured latitude, positive north */
//    REAL32 longDegsMeas;	/* Measured longitude, positive east */

//    /* Search gate (extMask & SPX_PACKET_TRACK_EXT_GATE). */
//    REAL32 gateStartRangeMetres;	/* Start range in metres */
//    REAL32 gateEndRangeMetres;		/* End range in metres */
//    REAL32 gateStartAziDegs;		/* Start azimuth in degrees */
//    REAL32 gateEndAziDegs;		/* End azimuth in degrees */

//    /* CPA/TCPA (extMask & SPX_PACKET_TRACK_EXT_CPA). */
//    REAL32 cpaMetres;		/* Cloest point of approach. */
//    REAL32 tcpaSecs;		/* Time to closest point of approach. */

//    /* Threat info (extMask & SPX_PACKET_TRACK_EXT_THREAT). */
//    SPxPacketTrackThreat threat;	/* Threat information. */

//    /* Description (extMask & SPX_PACKET_TRACK_EXT_DESCRIPTION). */
//    UINT16 descNetSize;			/* Byte length of description on net. */
//    char description[62];		/* Track description. */

//    /* Target accel and turn rate (extMask & SPX_PACKET_TRACK_EXT_ACCEL) */
//    REAL32 accelMps;
//    REAL32 turnRateDps;

//    /* Target motion status (extMask & SPX_PACKET_TRACK_EXT_MOTION_STATUS) */
//    UINT16 motionStatus;

//    /* More fields may be added here in the future. */
//    UINT16 padding; /* Not used. */
//    /* Event time (extMask & SPX_PACKET_TRACK_EXT_EVENT_TIME) */
//    UINT32 eventTimeSecs; /* Whole seconds since unix epoch */
//    UINT32 eventTimeUsecs; /* Microseconds to be added to eventTimeSecs */
//    /* More fields may be added here in the future. */
//} SPxPacketTrackExtended;
//typedef struct SPxPacketTrackExtended_tag1 {
//    /* Everything in the normal report first */
//    SPxPacketTrackNormal norm;

//    /* Size (in bytes) of the entire message on the net (including the
//    * 'norm' component, this size, the mask and the variable fields
//    * that are present, but excluding those that are missing on the net
//    * (and excluding the SPxPacketHeader).
//    */
//    UINT32 netSize;

//    /* Bitmask of which of the following fields are present. */
//    UINT32 extMask;

//    /* Age of track (extMask & SPX_PACKET_TRACK_EXT_AGE). */
//    UINT32 age;			/* Age in scans (revolutions, or updates) */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG). */
//    REAL32 latDegs;		/* Tracked Latitude, positive north */
//    REAL32 longDegs;		/* Tracked Longitude, positive east */

//    /* Fusion information (extMask & SPX_PACKET_TRACK_EXT_FUSION). */
//    SPxPacketTrackFusion fusion;	/* Fusion information */

//    /* CPA/TCPA (extMask & SPX_PACKET_TRACK_EXT_CPA). */
//    REAL32 cpaMetres;		/* Cloest point of approach. */
//    REAL32 tcpaSecs;		/* Time to closest point of approach. */


//    /* More fields may be added here in the future. */
//} SPxPacketTrackExtended1;
//typedef struct SPxPacketTrackExtended_tag11 {
//    /* Everything in the normal report first */
//    SPxPacketTrackNormal norm;

//    /* Size (in bytes) of the entire message on the net (including the
//    * 'norm' component, this size, the mask and the variable fields
//    * that are present, but excluding those that are missing on the net
//    * (and excluding the SPxPacketHeader).
//    */
//    UINT32 netSize;

//    /* Bitmask of which of the following fields are present. */
//    UINT32 extMask;

//    /* Age of track (extMask & SPX_PACKET_TRACK_EXT_AGE). */
//    UINT32 age;			/* Age in scans (revolutions, or updates) */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG). */
//    REAL32 latDegs;		/* Tracked Latitude, positive north */
//    REAL32 longDegs;		/* Tracked Longitude, positive east */

//    UINT32 sensorDataBytes; /* Number of bytes in sensorSpecificData */
//    //UINT8 sensorSpecificData[12]; /* Sensor-sp                        /* Fusion information (extMask & SPX_PACKET_TRACK_EXT_FUSION). */
//    SPxPacketTrackFusion fusion;	/* Fusion information */

//    /* CPA/TCPA (extMask & SPX_PACKET_TRACK_EXT_CPA). */
//    REAL32 cpaMetres;		/* Cloest point of approach. */
//    REAL32 tcpaSecs;		/* Time to closest point of approach. */


//    /* More fields may be added here in the future. */
//} SPxPacketTrackExtended11;
//typedef struct SPxPacketTrackExtended_tag2 {
//    /* Everything in the normal report first */
//    SPxPacketTrackNormal norm;

//    /* Size (in bytes) of the entire message on the net (including the
//    * 'norm' component, this size, the mask and the variable fields
//    * that are present, but excluding those that are missing on the net
//    * (and excluding the SPxPacketHeader).
//    */
//    UINT32 netSize;

//    /* Bitmask of which of the following fields are present. */
//    UINT32 extMask;

//    /* Age of track (extMask & SPX_PACKET_TRACK_EXT_AGE). */
//    UINT32 age;			/* Age in scans (revolutions, or updates) */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG). */
//    REAL32 latDegs;		/* Tracked Latitude, positive north */
//    REAL32 longDegs;		/* Tracked Longitude, positive east */

//    /* Fusion information (extMask & SPX_PACKET_TRACK_EXT_FUSION). */
//    SPxPacketTrackFusion fusion;	/* Fusion information */

//    /* Secondary information (extMask & SPX_PACKET_TRACK_EXT_SECONDARY). */
//    SPxPacketTrackSecondary secondary;	/* IFF/AIS/ADSB information etc. */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG_MEAS). */

//    /* CPA/TCPA (extMask & SPX_PACKET_TRACK_EXT_CPA). */
//    REAL32 cpaMetres;		/* Cloest point of approach. */
//    REAL32 tcpaSecs;		/* Time to closest point of approach. */


//    /* More fields may be added here in the future. */
//} SPxPacketTrackExtended2;
//typedef struct SPxPacketTrackExtended_tag22 {
//    /* Everything in the normal report first */
//    SPxPacketTrackNormal norm;

//    /* Size (in bytes) of the entire message on the net (including the
//    * 'norm' component, this size, the mask and the variable fields
//    * that are present, but excluding those that are missing on the net
//    * (and excluding the SPxPacketHeader).
//    */
//    UINT32 netSize;

//    /* Bitmask of which of the following fields are present. */
//    UINT32 extMask;

//    /* Age of track (extMask & SPX_PACKET_TRACK_EXT_AGE). */
//    UINT32 age;			/* Age in scans (revolutions, or updates) */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG). */
//    REAL32 latDegs;		/* Tracked Latitude, positive north */
//    REAL32 longDegs;		/* Tracked Longitude, positive east */

//    UINT32 sensorDataBytes; /* Number of bytes in sensorSpecificData */
//    //UINT8 sensorSpecificData[12]; /* Sensor-sp                       /* Fusion information (extMask & SPX_PACKET_TRACK_EXT_FUSION). */
//    SPxPacketTrackFusion fusion;	/* Fusion information */

//    /* Secondary information (extMask & SPX_PACKET_TRACK_EXT_SECONDARY). */
//    SPxPacketTrackSecondary secondary;	/* IFF/AIS/ADSB information etc. */

//    /* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG_MEAS). */

//    /* CPA/TCPA (extMask & SPX_PACKET_TRACK_EXT_CPA). */
//    REAL32 cpaMetres;		/* Cloest point of approach. */
//    REAL32 tcpaSecs;		/* Time to closest point of approach. */


//    /* More fields may be added here in the future. */
//} SPxPacketTrackExtended22;

//struct PlotList
//{
//    UINT32 Time=0;
//    UINT32 UTime=0;
//    SPxPacketPlot plot;
//};

//struct TrackList_S
//{
//    UINT32 Time=0;
//    UINT32 UTime=0;
//    SPxPacketTrackNormal Track;
//};

//struct AisList
//{
//    UINT32 Time=0;
//    UINT32 UTime=0;
//    std::string ais;
//};

//struct HDTList
//{
//    UINT32 Time=0;
//    UINT32 UTime=0;
//    std::string HDT;
//};

//struct RMCList
//{
//    UINT32 Time=0;
//    UINT32 UTime=0;
//    std::string RMC;
//};
//struct CPRConfig
//{
//    UINT32 TotalSize;
//    UINT32 ConfigTimesec;
//    UINT32 ConfigTimemsec;
//    UCHAR  ChannelIndex;
//    UINT16 NominalLength;
//    REAL32 StartRange;
//    REAL32 EndRange;
//};
////中文对应英文，用于数据字段英文名和其中文名对应
//struct AttrName
//{
//public:
//    AttrName(){
//        cnName="";
//        enName="";
//    }
//    AttrName(QString cn,QString en)
//    {
//        cnName=cn;
//        enName=en;
//    }

//    QString cnName;//中文名
//    QString enName;//英文名
//};


//struct RadarData   //雷达数据结构
//{
//    unsigned short quanhao=0;
//    UINT32 azi;
//    std::string data ="";
//};

//struct TrackOperation  //航迹操作结构
//{
//    UINT32 trackID=-1;//航迹id
//    UINT32 detecteTime=-1;//检测时间
//    UINT32 time=-1;//操作时间
//    UINT32 type=-1;//操作类型
//    REAL64 lon=-1 ;//原始经度
//    REAL64 lat=-1 ;//原始纬度
//    REAL64 newlon=-1 ;//修改后经度
//    REAL64 newlat=-1 ;//修改后纬度
//    QString userName="-1";//用户名

//    TrackOperation(){
//        time = QDateTime::currentDateTime().toTime_t();
//    }
//    TrackOperation(TrackPacket track){
//        trackID=track.id;
//        detecteTime=track.time;
//        time = QDateTime::currentDateTime().toTime_t();
//        lon=track.lon;
//        lat=track.lat;
//    }
//};


typedef struct
{
    unsigned short x;//左上角横坐标0~448
    unsigned short y;//左上角纵坐标0~256
    unsigned short w;//宽度0~448
    unsigned short h;//高度0~256
    unsigned short class1;//类别
    unsigned short score;//置信度,0~100
}RectInfo;

struct RectTrackInfo
{
    int x = 0;//左上角横坐标0~448
    int y = 0;//左上角纵坐标0~256
    int w = 0;//宽度0~448
    int h = 0;//高度0~256
    int ID;//类别
    int type = 0;
    float speed = 0.0;
    RectTrackInfo()
    {}
    RectTrackInfo(const RectTrackInfo& iniCfg)
    {
        x = iniCfg.x;
        y = iniCfg.y;
        w = iniCfg.w;
        h = iniCfg.h;
        ID = iniCfg.ID;
        type = iniCfg.type;
        speed = iniCfg.speed;
    }
    RectTrackInfo& operator=(const RectTrackInfo& iniCfg)
    {
        if (this != &iniCfg)
        {
            x = iniCfg.x;
            y = iniCfg.y;
            w = iniCfg.w;
            h = iniCfg.h;
            ID = iniCfg.ID;
            type = iniCfg.type;
            speed = iniCfg.speed;

        }
        return *this;

    }
};

#endif // STRUCTHELP_H
