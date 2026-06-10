#ifndef _SPXMHTTRACK_H
#define _SPXMHTTRACK_H

/* Supporting headers */
#include "SPxMHTobj.h"
#include "SPxMHTmeasurement.h"
#include "SPxMHThypothesis.h"

class SPxMHTtrack;
class SPxMHThypothesis;
class SPxMHTmeasurement;
class SPxMHTpublicId;
class SPxMBTmodel;
class SPxMHTtrack;
typedef enum TRACK_STATE
{
	SPX_TRACK_UNKNOWN_STATE     = 0,
	SPX_TRACK_PROVISIONAL_STATE = 1,
	SPX_TRACK_ESTABLISHED_STATE = 2,
	SPX_TRACK_LOST_STATE        = 3
}TrackState;

/* 点迹起始方式 */
typedef enum TRACK_INITIATION_METHOD
{
	SPX_TRACK_INITIATION_UNSET  = 0,
	SPX_TRACK_INITIATION_MHT    = 1,
	SPX_TRACK_INITIATION_MANUAL = 2,
	SPX_TRACK_INITIATION_MBT    = 3
}TrackInitiationMethod;

typedef enum TRACK_FILTER_MODE
{
	FILTER_MODE_FIXED_GAIN = 0,
	FILTER_MODE_SPEED_SENSITIVE = 1,
	FILTER_MODE_MANOEUVRE_DETECTION = 2
}TrackFilterMode;

typedef enum TRACK_MOTION_STATUS
{
	/*Target status.
	This is 
	0 for stationary targets,
	1 For moving targets, 
	2 for manoeuvering targets*/
	SPX_TRACK_STATIONARY   = 0,
	SPX_TRACK_MOVING       = 1,
	SPX_TRACK_MANOEUVERING = 2
}TrackMotionStatus;

#define  MN_FOR_NOT_ESTABLISHEDED                          1 
#define  SPEED_OUT_RANGE_FOR_NOT_ESTABLISHED               2 
#define  CLOSE_TO_EXISTING_TRACK_FOR_NOT_ESTABLISHED       3 
#define  WAKE_TO_EXISTING_TRACK_FOR_NOT_ESTABLISHED        4 
#define  SHADOW_TO_EXISTING_TRACK_FOR_NOT_ESTABLISHED      5 
#define  DOPPLERSPEED_OUT_RANGE_FOR_NOT_ESTABLISHED        6 
#define TRACK_REJECTED_FOR_NOT_ESTABLISHED                 7 
#define SPRT_FOR_NOT_ESTABLISHED                           8 
#define NOT_PRINCIPAL_HYPOTHESIS_FOR_NOT_ESTABLISHED       9
#define SPEED_CONSISTENCY_FOR_NOT_ESTABLISHED              10 
#define CLUTTER_EFFECT_DISABLED_ATI_FOR_NOT_ESTABLISHED    11 
#define COURSE_STABILITY_FOR_NOT_ESTABLISHED               12
#define RANGE_OUT_ATIRANGE_FOR_NOT_ESTABLISHED             13 


#define SPX_TRACK_CLOSE_NOT_CHECK                  0x00000000
#define SPX_TRACK_FIXED_MARGIN_PROXIMITY_CHECK     0x00000001
#define SPX_TRACK_WAKE_CHECK                       0x00000002
#define SPX_TRACK_SHADOW_CHECK                     0x00000004
#define SPX_TRACK_VARIED_MARGIN_PROXIMITY_CHECK    0x00000008


extern const char*  ReasonForTrackNotEstablished[];

#include <stdio.h>

class SPxMHTtrackLink : public SPxMHTobj
{
public:
	SPxMHTtrackLink(SPxMHThypothesis *_parent=NULL, SPxMHTtrack *_track=NULL);
	~SPxMHTtrackLink() {};
	void Init(SPxMHThypothesis *_parent, SPxMHTtrack *_track);	
	SPxMHTtrackLink* GetPrev() { return this->m_prev; }
	SPxMHTtrackLink* GetNext() { return this->m_next; }
	/*返回下一个有效的节点，如果在末端就返回头*/
	SPxMHTtrackLink* GetCircleNext() {
		SPxMHTtrackLink* pNext = this->m_next;
		if (!pNext)
			pNext = this->m_parent->GetFirstTrackLink();
		return pNext;
	}
	SPxMHThypothesis* GetParent() { return this->m_parent; }
	SPxMHTtrack* GetTrack() { return this->m_track; }
	int    GetTrackId();/*注意与 m_id 的区别*/
	void SetId(int id) { this->m_id = id; }
protected:
	int               m_id;
	SPxMHTtrack*      m_track;
	SPxMHTtrackLink * m_next;
	SPxMHTtrackLink * m_prev;
	SPxMHThypothesis* m_parent;

	friend class  SPxMHThypothesis;
	friend class  SPxMHT;
};

void SetIntersectFlag(SPxMHTtrackLink *pTrackLnkFirst, SPxMHTtrackLink *pTrackLnk2);

class SPxMHTtrack : public SPxMHTobj
{
public:
	SPxMHTtrack() {}
	~SPxMHTtrack() {}
	void   Init(SPxMHThypothesis *_parent, SPxMHTtrack *_track);
	int GetTrackId();
	int GetTrackState() { return this->m_state; }
	/*返回1表示这次有测量plot关联到track，可用于更新updated
	  返回0表示这次没有测量plot关联到track，航迹自己外推coasted
	*/
	bool GetTrackIsUpdated() {return this->m_bitHitCount & 1;}
	const char* GetTrackStateStr();
	double GetCourse(){ return this->m_courseDeg; }
	double GetSpeed() { return this->m_speed; }
	double GetWeight() { return this->m_meanWeight; }
	int    GetIndexInAssocMatrix() { return this->m_IndexInAssocMatrix; }
	SPxMHTmeasurement* GetAssociatedMeasurement() { return this->m_associatedMeasurement; }
	SPxMBTmodel * GetAttachedMBTmodel() { return this->m_attachedMBTmodel; }
	SPxMHTtrackLink* GetParent() { return m_parent; }
	SPxMHTpublicId*  GetPublicIdPtr() { return this->m_publicId; }
	SPxRangeAziSegment * GetNextSearchGate() { return &this->m_nextSearchGate; }
	SPxRangeAziSegment * GetLastSearchGate() { return &this->m_lastSearchGate; }
	double   GetRadSpeed() {return this->m_rangeChangeRateMps;}
	UINT32 GetMotionStatus(){return this->m_motionStatus;}

	int    GetIFFAltitude_ft();
	UINT32    GetTrackAgeScansAfterEstablished();
	int    GetTrackAgeScansAfterCreation();
	int    GetSensorSpecificData(char *sensorSpecificData, int BufferSizeInByte);
	double GetErrorSD(unsigned int iComponent);
	void GetHits(int *numDetectionsP, int *numNotDetectionsQ_P, int *numDetectionsQ, int HitCountWindowSize);
	int  GetTTMIdValue(int notNewFlag = TRUE);
	double GetNextUpdateTimeSecond(double curAziDeg, double nextAziDeg);
	/*******/
	int  IsDebugAvailable();
	bool IsInPrincipalHypothesis();
	int  IsPointMeasurementInNextSearchGate(SPxMHTmeasurement *pointMeasurement);
	int  IsMeasurementInNextSearchGate(SPxMHTmeasurement *pMeasurement);
	int  IsCloseToOtherEstablishedTracksInHypothesis(double rangeMargin, double aziMargin);
	int  IsCloseToOtherTrack(SPxMHTtrack* pOtherTrack, double rangeMargin, double aziMargin);
	int  TestCloseToExistingTracks(SPxMHTcluster *pCluster, int CheckATIcloseToTrack);
	int  TestNearToOtherTrack();
	int  TestSlowToNearOtherTrack();
	int  IsSimilar(SPxMHTtrack *pOtherTrack);
	int  IsExtreme();
	int  IsIntersectWithOtherTrack(SPxMHTtrack* pOtherTrack);
	int  CheckCourseStability();/* 检查航向的稳定性返回值1 表示稳定  返回0 表示不稳定*/
	int  CheckSpeedConsistency();
	int  IsInCoastExtendedActiveState() { return this->m_isCoastExtendedActiveState == TRUE; }
	int  CheckTrackInitiationIfSpecificDirection();
	int  CheckTimeToDeleteLostTrack();
	/*****************************************************************/
	void Init(SPxMHT *m_pMHT);
	void Create(SPxMHThypothesis *pHypo, SPxMHTmeasurement *pMeasurement, double rangeGain, double aziGain, SPxMHTpublicId *publicId);
	void Clone(SPxMHTtrack *pTrack);
	SPxMHTtrack * Clone();
	void AccumTrackLinksCountShareThis();
	void CalcCourseVarinceAndSD(int useHistroyCounter);
	void CalcSpeedVarinceAndSD(int useHistroyCounter);
	
	void EmptySpeedAndCourseHistroyArray();
	void UpdateHistroySpeedAndCourseArray();
	void UpdateError(unsigned int component_index, double newError);
	/*检查航迹是否由临时状态变为建立状态*/
	void TestTrackForPromotion();
	int  UpdateTrackWithoutMeasurement();
	void UpdateSecondarySensor();
	int  UpdateTrackWithMeasurement();
 
 
	void UpdateTagetMotionStatus();
	void PredictedTargetXYForTargetMotion(double nextUpdateTime);
	void PerdictedTargetXYForPlatformShift(double nextUpdateTime);
	int  Update(SPxMHThypothesis *pThypothesis);
	int  CanUseNonQualifyingPlot();
	void FilterAssocStength(double assocStength);
	double CalcAssocStrength(SPxMHTmeasurement* pMeas, int iMeas, double *wPtr);
	void  SetClassShortformName(const char *NAME3) {
		sprintf(this->m_classShortformName3, "%s", NAME3);
	}
	void SetAssociatedMeasurement(SPxMHTmeasurement* pMeasurement) { this->m_associatedMeasurement = pMeasurement; }
	void SetAttachedMBTmodel(SPxMBTmodel * model) { this->m_attachedMBTmodel = model; }
	void UpdateTrackClassInfo();
	SPxRangeAziSegment * GetPredictedEnlargeBoundingBox(SPxRangeAziSegment *a2, double range=0.0, double azi=0.0);
	SPxRangeAziSegment * GetBoundingBox(SPxRangeAziSegment *a2);
	void   EmptyHistroyMeasurementsId();
	/*计算增益*/
	void CalcFilterGains();
	void CalcSpeedSensitiveRangeAndAziGain();
	void CalcFixedRangeAndAziGain();
	void CalcManoeuvreDetectionRangeAndAziGain();
	void CalcRangeAndAziChangeRateGain();
	void ConsiderSpeedChangingForManoeuvre();
	void ConsiderCourseChangingForManoeuvre();
	void ConsiderAzimuthErrorForManoeuvre();
	void ConsiderRangeErrorForManoeuvre();
	void ConsiderTargetManoeuvre();
	void ReduceFilterGiansBasedOnSignalStrength(double rangeGain, double aziGain);
	/*计算搜索波门，参见J:\SPxLibSrc\SPxLibSrc\src\SPxMHTtrack_Gate.cpp*/
    void CalcGateDueToSensorNoise(SPxRangeAziSegment* pGate);
    void UpdateGateForMeasError(int isErrSerious, double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForTrackUncertainty(double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForPlatformUncertainty(int isErrSerious, double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForAccelAndDecel(double timeElapseSecond, SPxRangeAziSegment* pGate);
    //void UpdateGateForDecel(double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForTurn(double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForAziMetres(double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForSpeedGainWeight(double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForRangeLimit(double timeElapseSecond, SPxRangeAziSegment* pGate);
    void UpdateGateForAzimuthLimit(double timeElapseSecond, SPxRangeAziSegment* pGate);
	void CalcNextSearchGate(double elapseTimeNextUpdate);/*根据下一次更新时间，计算目标可能出现的波门*/
	void SetSizeUsedForSPRT(SPxMHTmeasurement *pMeasurement);
	int  GetMatchMeasCount(SPxMHTmeasurementSet *pMeasSet, SPxMHTmeasurement **firstMatchMeas);
	void OutputTrackDetails(FILE *file_handle, const char *preOutputStr);
	void DeleteTrack();
public:
	void GetHits_MN_ATI(int *pnumHits, int *pnumScans);
protected:
	
	void ShowntHistoryCourse(int maxShownCount = 8);
	void ShowntHistorySpeed(int maxShownCount = 8);
	
	
	void OutputTrackDebugFileIfMode3IfInPrincipalHypothesis();
	/*该函数的作用是检查航迹的合法性，有效性，比如是否在不在存在的
	返回 1 表示有效 ，返回0 表示无效 可以删除该航迹了*/
	int  TestTrackValidity();

	double GetFilterValue(unsigned int component_index);
	void  CalcMeasComAssocStrength(SPxMHTmeasurement* pMeas, int iComponent, double* wPtr, double* hPtr);
	double CalcMeasAssocStrength(SPxMHTmeasurement* pMeas);
public:
	/*获取航迹初始化即创建的方法*/
	const char* GetTrackCreationMethodStr()
	{
		switch (this->m_trackInitiationMethod){
		case SPX_TRACK_INITIATION_UNSET:
			return "unset?";
		case SPX_TRACK_INITIATION_MHT:
			return "MHT";
		case SPX_TRACK_INITIATION_MANUAL:
			return "Manual";
		case SPX_TRACK_INITIATION_MBT:
			return "MBT";
		default:
			return "unset?";
		}
	}
public:
	SPxMHTpublicId *m_publicId;
	SPxMHTtrackLink *m_parent;
	UINT16  m_nTrackLinksShareThis;
	UINT32 m_creationTime_CounterElapsedSectors;/*计数时间单位为扇区数，计数累加  航迹建立时间*/
	UINT32 m_lastUpdateTime_CounterElapsedSectors;/*计数时间单位为扇区数，计数累加  航迹最新更新时间*/
	UINT32 m_lastUpdateTime2_CounterElapsedSectors;/*计数时间单位为扇区数，计数累加  航迹最新更新时间*/
	UINT32 m_establishedTime_CounterElapsedSectors;/*计数时间单位为扇区数，计数累加 航迹确认时间*/
	int m_nUpdateCounter;/*更新次数计数*/
	int m_nPastScansSinceManoeuvred;/*计数器，记录从目标停止机动到现在过去了多少圈*/
	SPxTime_t m_epochTime;
public:
	/*滤波后的位置*/
	REAL64 m_xMetres;		/* Tracked cartesian position */
	REAL64 m_yMetres;		/* Tracked cartesian position */
	/*预测下一次出现的位置*/
	REAL64 m_xMetresPredicted;		/* expected  or pedicted position */
	REAL64 m_yMetresPredicted;		/* expected  or pedicted position */
	
     /*测量给出的位置*/
	REAL64 m_xMetresMeas;		/* 测量给出的 x position */
	REAL64 m_yMetresMeas;		/* 测量给出的 y  cartesian position */

	/*滤波后的位置*/
	REAL64 m_rangeMetres;   /* Tracked polar position */
	REAL64 m_azimuthDegrees;/* Tracked polar position */
	REAL64 m_rangeChangeRateMps;/*注意这不是速度，这是极坐标情况下 距离R分量的变化率*/
	REAL64 m_aziChangeRateDps;/*这是极坐标情况下 方位A分量的变化率*/
	REAL64 m_rangeChangeRateGain;
	REAL64 m_aziChangeRateGain;

	REAL64 m_sizeRangeMetres; 
	REAL64 m_sizeAzimuthDegrees;
	/*Last Error R = %.1lfm, Last Error A = %.2lf deg*/
	REAL64 m_lastRangeError;
	REAL64 m_lastAzimuthError;
	REAL64 m_rangeMetresPredicted;
	REAL64 m_azimuthDegreesPredicted;

	REAL64 m_speed;/*速度 单位m/s*//* The target's speed in m/s */
	REAL64 m_speedVariance;/*速度 方差 单位 （m/s）^2*/
	REAL64 m_speedSD;/*速度 标准差 单位 m/s*/

	REAL64 m_courseDeg;/*目标航向 单位为度*//* The course in degrees */
	REAL64 m_courseChangeRateDps;/*航向改变率，单位 度每秒 Course change rate = %.3lf deg/sec.*/
	REAL64 m_courseChangeRateDpsPrevious;
	REAL64 m_speedChangeRate;/*速度改变率 ，单位  m/s/s*/
	REAL64 m_courseVariance;
	REAL64 m_courseSD;
	REAL64 m_radialDopplerMps;/*Doppler range rate (+ve in)*/
	int    m_nLastScansSincePlatformManoeuvred;/*从船平台停止机动需要过去多少圈,才算船平台停止机动*/
	REAL64 m_errorVarianceArray[COMPONENT_ALL_NUM];/*误差的方差*/
	REAL64 m_errorSDArray[COMPONENT_ALL_NUM];/*误差的标准差*/
	REAL64 m_errorMeanArray[COMPONENT_ALL_NUM];/*利用当前和新的误差平滑后的误差*/
	REAL64 m_errorLastArray[COMPONENT_ALL_NUM];/*当前新的误差*/

	REAL64 m_rangeMetresMeas;   /* Measured polar position */
	REAL64 m_azimuthDegreesMeas;/* Measured polar position */

	REAL64 m_sizeRangeMetresMeas;	/* Measured size in range */
	REAL64 m_sizeAzimuthDegreesMeas;	/* Measured size in azimuth */
	REAL64 m_measWeight100;/*航迹所关联测量的样本数，归一化到0.0 到 100.0 */
	SPxRangeAziSegment m_lastSearchGate;
	SPxRangeAziSegment m_nextSearchGate;
	UINT8 m_state;
	INT8 m_rangeErrorFlag;
	INT8 m_aziErrorFlag;
	UINT8 m_aziChange;
	UINT8 m_speedChange;
	UINT8 m_nNeedLastScansSinceManoeuvred;/*从目标停止机动需要过去多少圈,才算停止机动*/
	UINT8 m_trackInitiationMethod;
	REAL64 m_rangeGain;
	REAL64 m_aziGain;
	REAL64 m_rangeGainPrevious;
	REAL64 m_aziGainPrevious;
	REAL64 m_xMetresChangeRatePrevious;
	REAL64 m_yMetresChangeRatePrevious;
	UINT8 m_scansConsecutiveForSpeedOutOfRange;/*速度超出范围 的连续圈数*/
	UINT8 m_scansCount;/*扫描计数*/
	int m_isDebugSet;
	UINT32 m_hitsIntegrationCounter;
	UINT8  m_codeReasonForNotEstablished;
	UINT32 m_lastUpdateTimeWithValidMeas_CounterElapsedSectors;
	UINT32 m_bitHitCount;
	UINT8 m_numScansCoast;/*航迹外推多少圈*/
	/*下次该航迹的最小更新的扇区数（累加），
	一般设置为当前更新扇区数加上半圈里包含的扇区数*/
	UINT32 m_minNextUpdateTime_CounterBySectors; /*计数时间单位为扇区数，计数累加  
												 下次航迹更新的最近时间，更新时间必须大于该时间，否则不予更新*/
	UINT8   field_370;
	SPxMHTmeasurement *m_associatedMeasurement;
	UINT16 m_nConsecutiveValidMeasurements;
	double m_strengthDividedWeight;
	UINT8 m_IndexInAssocMatrix;
	SPxMHT* m_pMHT;
	UINT32 m_curMeasurementId;
	UINT32 m_preMeasurementId;
	UINT32 m_pre2MeasurementId;
	UINT32 m_pre3MeasurementId;
	UINT32 m_pre4MeasurementId;
	REAL64 m_strength;
	UINT32 m_lastWeight;
	REAL64 m_meanWeight;
	REAL64 m_xMetresChangeRate; /*Xdot(m / s)*/
	REAL64 m_yMetresChangeRate; /*Ydot(m / s)*/
	REAL64 m_rangeMetresPrevious;		    /* Tracked polar position */
	REAL64 m_azimuthDegreesPrevious;/* Tracked polar position */
	REAL64 m_Pd;
	REAL64 m_confidenceSPRT;/*The SPRT detector a confidence level for a candidate track */
	REAL64 m_tresholdDeletionSPRT;/*The thresholds for deletion,, Td, */
	REAL64 m_tresholdPromotionSPRT;/*The thresholds for promotion, Ta,*/
	REAL64 m_confidencePercentSPRT;/*信任度归一化到 0 到 1 即百分比，显示比例的时候×100*/
	int    m_measurementFlag;
	REAL64 m_sizeUsedForSPRT;
	UINT8  m_extremeFlag;
	UINT8  field_411;
	UINT16 m_isManoeuvering;
	SensorSpecificData  m_sensorSpecificData;
	REAL64 m_speedHistroyArray[32];
	REAL64 m_courseHistroyArray[32];
	int m_histroyCounter;
	int m_wakedTrackId;/*被跟随的航迹id，也就是说当前航迹很可能紧紧跟随一个已经存在的航迹 */
	UINT8  m_IsExtendCoastSetting;/*设置是否允许进行扩展外推，这是允许不允许的变量*/
	UINT8 m_isCoastExtendedActiveState;/*当前航迹是否处于扩展外推状态，这是航迹是否处于扩展外推的标志*/
	UINT8 m_isCoastMatureExtendedActive;
	UINT8 m_countInWake;/*该航迹跟随的次数*/
	int   m_nMeasPlotInSearchGate;/*当前来的测量集中有多少个在该航迹的预测波门内*/
	int   m_nNonQualifyingMeasPlotInSearchGate;/*有多少个不符合要求的测量在该航迹的预测波门内，一般 */
	int   m_ageScansFromCreationToEatablished;/*从航迹起始到航迹确立所经历的扇区数*/
	/* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG). */
	REAL64 m_latDegs;		/* Tracked Latitude, positive north */
	REAL64 m_longDegs;		/* Tracked Longitude, positive east */

							/* Lat/long (extMask & SPX_PACKET_TRACK_EXT_LATLONG_MEAS). */
	REAL64 m_latDegsMeas;		/* Measured latitude, positive north */
	REAL64 m_longDegsMeas;	/* Measured longitude, positive east */
	int    m_isLatLonAvailable;/*目标的经纬度位置信息是否可用*/
	double m_xMetresInitFirst;/*目标起始最初的X坐标*/
	double m_yMetresInitFirst;/*目标起始最初的Y坐标*/
	double m_recentAssocStrength;
	UINT8  m_isTrackSizeChange;
	UINT8  m_IsUnallowedAssociation;/*是否允许关联，比如航迹多，测量集少，那么就有一部分航迹不允许关联航迹*/
	double m_centerBBoxRangeMetres;
	double m_centerBBoxAzimuthDegrees;
	int    m_lastReportUpdateTime_CounterElapsedSectors;/*最新一次报告航迹更新的时间*/

	REAL64       field_6B0;
	REAL64       field_6B8;
	SPxMBTmodel *m_attachedMBTmodel;
	int     m_ageOnceAsMBTtrack;
	int     m_IFFId;
	UINT8   m_IFFCounter;
	int    m_ADSBmodeC;
	UINT8   m_ADSBCounter;
	int   m_mbtToBeMHTtrackTime_CounterElapsedSectors;/*从MBT航迹转为MHT航迹的时间*/
	/*If non-qualifying plots are generated by the plot extractor they can be used by manually initiated tracks. 
	When it is first created, a manually initiated track can use non-qualifying plots.*/
	UINT8  m_canUseNonQualifyingPlot;
	UINT8  m_associatedNonQualifyingPlotsCount;
	/* Target motion status (extMask & SPX_PACKET_TRACK_EXT_MOTION_STATUS) */
	UINT32 m_motionStatus;
	UINT32 m_movingCounter;
	UINT32 m_stationaryCounter;
	SPxBlob_t *m_targetBlobPtr;
	int m_isIntersect;/*#该航迹是否与其它航迹的跟踪波门相交*/
	UINT32 m_classCode;		/* Track class code*/
	/*This is a shortform (3 character) name which is used to show the class in
	the SPx Server PPI window.*/
	char  m_classShortformName3[4];
	/*计数器，用来记录满足需求 meets the requirements of a class for lockAfter consecutive
    scans 的连续圈数*/
	int   m_nConsecutiveScansForClass;
	/*标志，用来指示it will be permanently locked into that class for the duration of the
  // track’s life. This can be used to latch a class once a sufficient number of
  // scans have built confidence in a class*/
	UINT8 m_isLatchToClass;
	/*  // If the parameter holdFor is set, a track that meets the requirements for a
  // class will remain in that class for at least holdFor scans, even if it does
  // not otherwise meet the membership criteria. This parameter implements
  // hysteresis on the track class membership.*/
	int   m_nScansHoldFor;
	/*This parameter controls the behaviour of the processing when a track is
	currently classified, but on the latest update there is no match with any
	class and there is no holdFor set. The normal behaviour (noDefault=0) is
	to change the track’s code to 0. If nodefault = 1, the track’s class
	remains unchanged.*/
	int   m_noDefaultForClass;
	friend class SPxMHThypothesis;
	friend class SPxMHTtrackClass;
	friend class SPxMHTnetReport;
};

#endif//_SPXMHTTRACK_H

