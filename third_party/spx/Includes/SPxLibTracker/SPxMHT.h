#ifndef _SPX_MHT_H
#define _SPX_MHT_H

#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxTypes.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibData/SPxRib.h"
#include "SPxLibData/SPxProcessRegion.h"

#include "SPxMHTobj.h"
#include "SPxLibUtils/SPxWorldMap.h"
#include "SPxMHTmeasurement.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/
/* Debug flags. */

#define SPX_MHT_DEBUG_TRACK_UPDATE                             0x00000001
#define SPX_MHT_DEBUG_TRACK_ESTABLISHED                        0x00000002
#define SPX_MHT_DEBUG_TRACK_DELETED                            0x00000004
#define SPX_MHT_DEBUG_SCAN_PERIOD_TIMING                       0x00000008
#define SPX_MHT_DEBUG_SECTOR_UPDATE_EVENTS                     0x00000020
#define SPX_MHT_DEBUG_MEASUREMENTS_EXTRACTED_FOR_EACH_SECTOR   0x00000040
#define SPX_MHT_ECHO_DEBUG_TO_STDOUT                           0x40000000
#define	SPX_MHT_DEBUG_VERBOSE	                               0x80000000	/* Verbose */

/*  
bit 0 : 1表示该参数可以用于全部显示
bit 1 ：1表示该参数可以用于BASIC显示
bit 2 ：1表示该参数可以用于Custom显示
bit 3 ：1表示该参数不在GUI上显示
*/
#define SPX_MHT_PARAMETER_SHOW_MODE_FULL      0x00000001
#define SPX_MHT_PARAMETER_SHOW_MODE_BASIC     0x00000002
#define SPX_MHT_PARAMETER_SHOW_MODE_CUSTOM    0x00000004
#define SPX_MHT_PARAMETER_NOT_SHOW            0x00000008

/*不同处理量过载标识：簇 假设 航迹链接  航迹 */
#define SPX_MHT_CLUSTERS_OVERLOAD    0x00000001
#define SPX_MHT_HYPOTHESES_OVERLOAD  0x00000002
#define SPX_MHT_TRACKLINKS_OVERLOAD  0x00000004
#define SPX_MHT_TRACKS_OVERLOAD      0x00000008

/*航迹提升模式，也就是航迹由暂态到稳态或成熟状态的判决准则*/
typedef enum 
{
	SPX_ATI_MN_DETECTOR   = 0,
	SPX_ATI_SPRT_DETECTOR = 1
}TrackerATImode;

#define PARAMETER_SETTING_ASSOCIATION_MH 0  /*参数配置文件设置 多假设关联*/
#define PARAMETER_SETTING_ASSOCIATION_SH 1  /*参数配置文件设置 单假设关联*/

typedef enum
{
	SPX_MHT_ATI_DIRECTION_NORMAL = 0,
	SPX_MHT_ATI_DIRECTION_UNIDIRECTIONAL = 1,
	SPX_MHT_ATI_DIRECTION_BIDIRECTIONAL  = 2
}MHT_ATIdirectionalMode;

typedef enum
{
	SPX_MHT_DEBUG_MODE_NULL = 0,
	SPX_MHT_DEBUG_MODE_AllHypo_AllNewTracks = 1,
	SPX_MHT_DEBUG_MODE_OnlyPriHypo_AllNewTracks = 2,
	SPX_MHT_DEBUG_MODE_AllHypo_AllNewTracks_SeparateFilePerTrack = 3
}MHTDebugMode;

/*p87 Table 6 – mhtClutterControl parameters*/
typedef enum 
{
	CLUTTER_CONTROL_DISABLED   = 0,/*Clutter control is disabled (this is the default).*/
	CLUTTER_CONTROL_LOW        = 1,/*ATI integration time is extended according to local clutter*/
	CLUTTER_CONTROL_MIDDLE     = 2,/*ATI is extended (more) according to local clutter*/
	CLUTTER_CONTROL_HIGH       = 3,/*ATI is extended (more still) according to local clutter*/
	CLUTTER_CONTROL_MAX_NO_ATI = 4,/*ATI is turned off if there is any local clutter*/
	CLUTTER_CONTROL_LEVEL_NUM
}ClutterControlLevel;

typedef enum
{
	CLUTTER_LEVEL_NO   = 0,
	CLUTTER_LEVEL_LOW  = 1,
	CLUTTER_LEVEL_HIGH = 2,
	CLUTTER_LEVEL_MAX  = 3,
	CLUTTER_LEVEL_NUM
}ClutterLevel;

typedef enum {
	MHT_GUI_PARA_TAB_ITEM_TRACK    = 0,
	MHT_GUI_PARA_TAB_ITEM_ATI      = 1,
	MHT_GUI_PARA_TAB_ITEM_PLOTS    = 2,
	MHT_GUI_PARA_TAB_ITEM_GENERAL  = 3,
	MHT_GUI_PARA_TAB_ITEM_RADAR    = 4,
	MHT_GUI_PARA_TAB_ITEM_ASSOC    = 5,
	MHT_GUI_PARA_TAB_ITEM_FILTER   = 6,
	MHT_GUI_PARA_TAB_ITEM_ADVANCED = 7
}GuiParaTabItem;
/*
关联矩阵是关联算法的前置条件
航迹与测量的关联矩阵，关联强度大，即测量与航迹的匹配度就越高 
                        列数  127+1 = 128
	     | M0| M1| M2| M3| ........| M127| M128(无测量关联，即对航迹外推，也就是说当前需要关联的航迹，没有)|
-----------------------------------------------------------------------------------------------
行     T0|
数     T1|
128    T2|
+      T3|
2	    .|
=	    .|
130	    .|
     T127|
      Tnew（当前测量关联到新航迹的，即产生新的航迹）|
      TFalse（当前测量为噪声，即关联到虚假航迹）|
*/
#define SPX_ASSOCIATION_MATRIX_MAX_TRACK_COUNT        128 /* 航迹与测量的关联矩阵，所允许的航迹最大个数 */
#define SPX_ASSOCIATION_MATRIX_MAX_MEASUREMENT_COUNT  128  /* 航迹与测量的关联矩阵，所允许的测量最大个数 */
#define SPX_ASSOCIATION_MATRIX_MAX_COLUMN_NUM         (SPX_ASSOCIATION_MATRIX_MAX_MEASUREMENT_COUNT + 1)   /*129*/  
#define SPX_ASSOCIATION_MATRIX_MAX_ROW_NUM         (SPX_ASSOCIATION_MATRIX_MAX_TRACK_COUNT + 1 + 1)        /*130*/
typedef enum
{
	CLUTTER_EFFECT_NO     = 0,
	CLUTTER_EFFECT_LOW    = 1,
	CLUTTER_EFFECT_MIDDLE = 2,
	CLUTTER_EFFECT_HIGH   = 3,
	CLUTTER_EFFECT_MAX    = 4,
	CLUTTER_EFFECT_NUM
}ClutterEffect;

#define CLUTTER_ANALYSIS_AREA_SQUARES_X_NUM  101
#define CLUTTER_ANALYSIS_AREA_SQUARES_Y_NUM  101
/*按列存储  那么访问的时候为  x×101 + y */
#define CLUTTER_ANALYSIS_AREA_SQUARES_NUM   (CLUTTER_ANALYSIS_AREA_SQUARES_X_NUM*CLUTTER_ANALYSIS_AREA_SQUARES_Y_NUM)

class SPxTargetDB;
class SPxMHTclusterSet;
class SPxMHTmeasurementSet;
class SPxMHTtrackerParameter;
class SPxNavData;
class SPxActiveRegion;
class SPxRunProcess;

class SPxMHTmeasurementLink;
class SPxMHTmeasurement;
class SPxMHTtrackLink;
class SPxMBT;
class SPxMHTtrack;
class SPxMHTTtmId;
class SPxMHThypothesis;
class SPxMHTpublicId;
class SPxMHTtpControlRule;
class SPxMBTmodel;

typedef struct SPxTrackerStatus_tag
{
	UINT16 numClusters; /* Number of clusters */
	UINT16 numHypotheses; /* Number of hypotheses */
	UINT16 numTrackLinks; /* Number of track links */
	UINT16 numTracks;/*注意这里的tracks数目包含非确认航迹*/
	UINT32 scanProcessingtime_ms;
	UINT32 numMeasAccepted; /* Number of measurements accepted */
	UINT32 numMeasRejected; /* Number of measurements rejected */
	UINT16 overloadFlags; /* Bitmask of SPX_MHT_STATUS_OVERLOAD... */
}SPxTrackerStatus_t;

typedef void(*ReportMsgProc)(const char*);
typedef void(*TrackReportProc)(int, SPxMHTtrack *, SPxObj *receiver);
typedef void(*TrackStatusProc)(SPxTrackerStatus_t *, SPxObj *receiver);
typedef void(*SectorReportProc)(SPxMHT *, int, int, SPxObj *receiver);

typedef struct SPxTrackReport_tag
{
	SPxObj *receiver/*pNetReport*/;//接收回调者
	int reportTrackId;
	TrackReportProc callbackFunc;
	SPxTrackReport_tag *pNext;
}SPxTrackReport;




typedef struct SPxSectorMsgReport_tag
{
	SPxSectorMsgReport_tag *pNext;
	int field_4;
	SectorReportProc callbackFunc;
	//SPxMHTnetReport *pNetReport;
	SPxObj *receiver;
	int isEnable;
}SPxSectorMsgReport_t;

typedef struct MeasInSectorForDebug_tag {
	int nMeas;/*在这个扇区内有多少个测量*/
	SPxBlob_t* pArray;/*测量首地址,连续数据区*/
}MeasInSectorForDebug_t;
/*sizeof=0x53168 340328*/
class SPxMHT : public SPxObj
{
public:
	SPxMHT(SPxTargetDB* pTargetDB,int TargetCapacity);
	~SPxMHT();
	/* Generic parameter assignment. */
	virtual int SetParameter(char *paraName, char *paraValue);
	virtual int GetParameter(char *paraName, char *valueBuf, int bufLen);
	virtual SPxErrorCode SetStateFromConfig(void);
	void  OutputMessage(const char *msgBuf = NULL);
	void  OutputDebug(const char *debugBuf = NULL);
	void  SetParameterFromConfig(char* a2);
	SPxErrorCode LoadCustomViewTrackParameterFile(char *custom_param_file_name);
	/*****************************************************/
	SPxMHTtrackerParameter * GetParameterByID(int parameterId, int silentt = FALSE);
	SPxMHTtrackerParameter * GetParamaterByName(const char *paraName, int silent = FALSE);
	void SetEnableFlagBasedOnParaValue();
	int QueryExists(const char *paraName, const char *activeRegionName);
	/*得到航迹与测量之间的关联强度，*/
	double GetAssocStrength(int trackAssocIndex, int measAssocIndex)
	{
		return this->m_assocStrengthMatrix[SPX_ASSOCIATION_MATRIX_MAX_COLUMN_NUM * trackAssocIndex + measAssocIndex];
	}
	double GetAverageAgeScansFromCreationToEatablished()
	{
		if (this->m_averageAgeScansFromCreationToEatablished == -1.0)
			return 0.0;
		else
			return this->m_averageAgeScansFromCreationToEatablished;
	}
	int  GetDRCLevel(){return this->m_nSegmentsDRCcount;}
	SPxSegment * GetDRCactiveRegionFirstSegment()
	{
		if (this->m_areaRegionDRC)
			return this->m_areaRegionDRC->GetFirstSegment();
		else
			return NULL;
	}
	double GetAveragePd(){return this->m_averagePD;}
	// The average probability of false detection. This is
	// a measure of the number of plots that are not
	// used to update a track.
	double GetAveragePf(){return this->m_averagePF;}
	void   GetClutterEffect(double range, double azimuth, ClutterLevel clutterLevel, int *clutterEffectPtr);
	int    GetClutterEffect(double range, double azimuth, double rangeX, double rangeY);
	double GetMeasurementVariance(MeasurementComponent componentIndex);
	double GetMeasurementComponentErrorWeigth(unsigned int componentIndex, SPxMHTtrack *ptrack);

	double GetFalseTrackCountArea(int gridX, int gridY);

	/*该函数的作用是获取满足显示模式设置的第一个参数*/
	SPxMHTtrackerParameter * GetFirstParameter(int paraShowMode);
	void GetReviseMNAndClutterEffectOnMN_ATI(double range, double azimuth, ClutterLevel clutterLevel, int M, int N,
		int *p_revisedM, int *p_revisedN, int *p_clutterEffect);
	/*航迹处于临时状态时*/
	double GetMaxInitiationSpeed(double target_range_m, double target_azimuth_deg);
	double GetMaxNominalRangeMetres()
	{
		this->m_curMaxNominalRangeMetres = this->m_maxNominalRangeMetres;
		return this->m_maxNominalRangeMetres;
	}
	double GetMinInitiationSpeed(double target_range_m, double target_azimuth_deg);
	double GetMeasWeight_100(SPxMHTmeasurement *pMeas);
    SPxMBT * GetMBT(){return this->m_pMBT;}
	/**************************************************/
	void Init(int TotalClusters, int TotalHypotheses, int TotalTracks, int TotalTrackLinks);
	void InitClusterSetList();
	void InitClusters();
	void InitThypothesis();
	void InitMeasurementLink();
	void InitPublicId();
	void InitTrackLinks();
	void InitTracks();
	void InitTTMId();
	SPxErrorCode LoadDRCControlFile();
	void DumpParameterToFile(FILE *DumpFileHandle);
	//void RemoveCluster(SPxMHTcluster *pCluster);
	//void  RemoveHypothesis(SPxMHThypothesis *pHypothesis);
	//void RemoveMeasurementLink(SPxMHTmeasurementLink *pMeasLink);
	//void RemovePublicId(SPxMHTpublicId *a2);
	//void  RemoveTrackLink(SPxMHTtrackLink *pTrackLink);
	/***********   与执行命令有关********************/
	SPxErrorCode SwapLabels(int track1Id, int track2Id);
	int LabelTrack(int trackId, const char *labelName);
	int DesignateTrack(int trackId, int isDesignate);
	void UndesignateAllTracks();
	void OnRadarModeChange(char modechange);
	void MarkingTracksSizeChange();/*指示目标航迹大小发生了变化*/
	void ClearDebugFileContents();
	void DeleteAllTracks();
	int  DeleteTrack(int trackId);
	/*该函数会被外部调用，比如鼠标选择目标的时候需要调用该函数根据鼠标的位置返回航迹类
	鼠标点 Point.x, Point.y, 转为屏幕中对应雷达中心位置的 xMetres 和 yMetres
	然后转为距离雷达的距离和方位，然后调用下面的函数得到航迹类
	// 定时器刷新
	//
	int __thiscall CSPxWinCtrlMHTtrack::OnUpdateTargetInfo(CSPxWinCtrlMHTtrack *this)

	*/
	SPxMHTtrack *FindTrack(double rngMetre, double aziDeg, double distanceThreshold, double *pdistance);
	SPxErrorCode MoveTrack(int moveTrackId, double new_range, double new_azimuth);
	SPxErrorCode ReferenceTrack(int refTrackId);
	/*****************************************************/
	int  InterpretCommand(char *commandStr);
	void SetAreaDependentParametersValue(const char* suffix);
	void SetRunProcess(SPxRunProcess* rp) { this->m_runProcess = rp; }
	int  NewReturn(SPxRunProcess *rp, SPxReturn *p_rtn);
	int TestNewReturn(int azimuthDim, SPxReturn *pRtn, MeasInSectorForDebug_t * measInSector);
    int  NewSector(int isector);//2023.3.1 original name is Update
	int TestUpdate(int curProcessSector, MeasInSectorForDebug_t  measInSector);
	int  TestValidityOfMeasurement(SPxMHTmeasurementSet *pMeasSet, SPxMHTmeasurement *pTestMeas);
	int  SectorUpdate(int isector, SPxMHTmeasurementSet *pMeasSet);
	SPxMHTtrack * ManualInitiationTrack(double range, double azimuth);
	void CalcScanPeriod();
	void GetTrackStatusInfo(SPxTrackerStatus_t *pStatus);
	void DynamicLoadControl();
	void  DataRateControl();
	void FilterLostTrackRate();
	void GetGridSquareXY(double rangeX, double rangeY, int *pgridX, int *pgridY);
	ClutterLevel GetClutterLevel(double range, double azimuth, double rangeX, double rangeY);
	void GetRevisePfOnSPRT_ATI(ClutterLevel clutterLevel, double* revisedPfPtr);
	int  IsMaximumClutterEffect_DisabledATI(int clutterControl, ClutterLevel clutterLevel);
	SPxErrorCode GetMotion(double secs, double *xMetresPtr, double *yMetresPtr);
	void OnTrackEstablishedReport(SPxMHTtrack *pTrack);
	void OnTrackUpdateReport(SPxMHTtrack *pTrack);
	void OnTrackDeleteReport(SPxMHTtrack *pTrack);
	void OnInterpolatedReport();
	/**************************************************/
	void RemoveCluster(SPxMHTcluster *pCluster);
	void RemoveHypothesis(SPxMHThypothesis *pHypothesis);
	void RemoveMeasurementLink(SPxMHTmeasurementLink *pMeasurement_link);
	void RemovePublicId(SPxMHTpublicId *);
	void RemoveTrack(SPxMHTtrack* );
	void RemoveTrackLink(SPxMHTtrackLink *);
	void RemoveTtmId(SPxMHTTtmId *);
	SPxMHTcluster*    NewCluster(SPxMHTclusterSet *pClusterSet,int clusterId);
	SPxMHThypothesis* NewHypothesis(SPxMHTcluster *);
	SPxMHTmeasurementLink* NewMeasurementLink(SPxMHTmeasurement *pMeas);
	SPxMHTpublicId *  NewPublicId();
	SPxMHTtrack *     NewTrack();
	SPxMHTtrackLink * NewTrackLink(SPxMHThypothesis *, SPxMHTtrack *);
	SPxMHTTtmId *     NewTtmId();
	SPxMHTtrack * CreateTrack(SPxMHThypothesis *a2, SPxMHTmeasurement *a3, double rangeGain, double aziGain, SPxMHTpublicId *a6);
	/*used for sector update*/
	void DisperseInputClusterSetToClusterSetList(SPxMHTclusterSet *pInputClusterSet);
	/*从输入的测量集pInputMeasSet中挑选出在指定簇（输入pCluster）里的测量形成新的测量集*/
	SPxMHTmeasurementSet *CreateNewMeasSetForClusterFromInputMeasSet(SPxMHTcluster *pCluster, SPxMHTmeasurementSet * pInputMeasSet);
	void PickMeasForClusterFromAuxMeasSet(SPxMHTcluster *pCluster, SPxMHTmeasurementSet * pMeasSetForClusterUpdate);
	void PendingMeasToAuxMeasSetFromInputMeasSet(SPxMHTmeasurementSet * pInputMeasSet);
	void ClusterUpdate(int curSector, SPxMHTcluster *pCluster, SPxMHTmeasurementSet * pMeasSet);
	void ATI(SPxMHTmeasurementLink *pMeasLink, int *countptr);
	void ATI(SPxMHTmeasurementSet *pMeasSet);
	int ProcessATISidelobe(SPxMHTmeasurement *pMeas);//add spx v1.87 
	void LoadReductionByReducingMeasurement(SPxMHTmeasurementSet *,SPxMHTcluster *);
	void StatisticsMeasurements(SPxMHTmeasurementSet *pMeasSet);
	void LogMeasurementToFile(SPxMHTmeasurementSet *pMeasSet);
	void SetNavData(SPxNavData *pNavData){this->m_navData = pNavData;}
	void SetWorldMap(SPxWorldMap *pWorldMap) {this->m_worldMap = pWorldMap;}
	void SetAssocStrength(int trackAssocIndex, int measAssocIndex,double val)
	{
		this->m_assocStrengthMatrix[SPX_ASSOCIATION_MATRIX_MAX_COLUMN_NUM * trackAssocIndex + measAssocIndex] = val;
	}
	void  StatisticsTracksSize();
	int CheckWhetherTimeElapse500ms();
	void DisableAreaDebug();
	void EnableAreaDebug(double startRng, double endRng, double startAzi, double endAzi);
	void TrackerStatistics();
	void ResetStats();
	void StartStatistics();
	virtual SPxTrackReport *NewTrackPacketReport(TrackReportProc fun_ptr, int TrackId, SPxObj *receiver);
	virtual void RemoveTrackPacketReport(SPxTrackReport *needDelReport);
	virtual int  GetNumTrackReports(){return this->m_numTrackReports;}
	virtual void SetTrackStatusProcFunc(TrackStatusProc func, SPxObj *receiver);
	virtual SPxSectorMsgReport_t * NewSectorReport(SectorReportProc func, SPxObj *receiver, int isEnable);
	int     UnattachedSpecifiedMBTmodel(SPxMBTmodel *pMBTmodel);
	SPxMHTtrack * EstablishedAsMHTtrack(double rangeMetres, double azimuthDegrees, double speed, double courseDegs,
		double sizeRangeMetres, double sizeAzimuthDegrees, double rangeGain, double aziGain, double weight,
		SPxMBTmodel *pMBTModel, SPxBlob_t *blob, int *isEstablished, int age, char canUseNonQualifyingPlot, int isSupportOnly);
	int CheckATIcloseToExistTrack(double rangeMetres, double azimuthDegrees, double xMetres, double yMetres);
public:
	int Lock() { return this->m_criticalSection.Enter(); }
	int UnLock() { return this->m_criticalSection.Leave(); }
private:
	int  GetParameterValue(char *paraName, char *valueBuf, int bufLen, SPxActiveRegion *pActiveRegion);
public:
	/*called by 外部接口，定时获取状态信息？？,这里需要锁*/
	SPxMHTclusterSet * GetClusterSet_Lock(int isector1)
	{
		this->Lock();
		return this->m_clusterSetList[isector1 - 1];
	}
    /*在遍历完一个SPxMHTclusterSet 需要解锁*/
public:
	SPxMHTclusterSet **m_clusterSetList;
	SPxMHTclusterSet *m_auxClusterSet;/*辅助簇集，起过渡作用，用于存放更新后的簇，更新完的簇根据你所处扇区，m_auxClusterSet 弹出元素，分配到不同的clusterSetList，*/
	SPxMHTmeasurementSet *m_auxMeasSet;/*辅助测量集，用于存放1 暂时未被用于cluster更新的测量集等*/
	SPxAtomic<UINT32>     m_curTime_CounterElapsedSectors;
	UINT32 m_lastClutterControlTime_CounterElapsedSectors;
	SPxCriticalSection m_criticalSection;
	SPxRangeAziSegment m_debugArea;
	int m_isAreaDebug;
	SPxMHTtrackerParameter *m_firstParameterInChain;
	SPxMHTtrackerParameter *m_lastParameterInChain;
	SPxSectorMsgReport_t *m_firstReport_SectorMsg;
	SPxSectorMsgReport_t *m_lastReport_SectorMsg;
	UINT32 m_overloadFlags;
	UINT32 m_debugFlags;
	int m_numTrackReports;
	SPxActiveRegion *m_areaRegionDRC;
	int m_nScansToDelayDropDRCcount;
	int m_nSegmentsDRCcount;
	ReportMsgProc m_reportMsgToWinMessageCtrlFunc;/*向GUI输出信息的函数*/
	int field_E0;
	/********************************/
	SPxAtomic<int> m_numMeasLinkFree;
	SPxAtomic<int> m_numMeasLinkUsed;
	int m_nextFreeMeasLinkIndex;
	int m_recycleMeasLinkIndex;
	int m_measLinkListSizeInitial;
	int m_usedForSortMeasLinkArrayNum;
	int  m_countMeans;/*对每一个来的测量进行计数，用以给测量分配ID，从1 开始*/	
	SPxMHTmeasurementLink ** m_measLinkPtrList;
	SPxMHTmeasurementLink * m_measLinkList;
	SPxMHTmeasurementLink **m_usedForSortMeasLinkArray;
	/********************************/
	SPxAtomic<int> m_numPublicIdFree;
	SPxAtomic<int> m_numPublicIdUsed;
	int m_nextFreePublicIdIndex;
	int m_recyclePublicIdIndex;
	int m_publicIdListSizeInitial;
	SPxMHTpublicId **m_publicIdPtrList;
	SPxMHTpublicId *m_publicIdList;
	/********************************/
	SPxAtomic<int> m_numTTMIdFree;
	SPxAtomic<int> m_numTTMIdUsed;
	int m_nextFreeTTMIdIndex;
	int m_recycleTTmIdIndex;
	int m_ttmIdSizeInitial;
	SPxMHTTtmId **m_ttmIdPtrList;
	SPxMHTTtmId *m_ttmIdList;
	int m_isDebugAllNewTracks;
	SPxMHTcluster ** m_usedForSortclusterArray;
	int m_usedForSortclusterArrayNum;
	int m_numMeasAccepted;
	int m_numMeasRejected;
	SPxActiveRegion *m_inActiveArea;
	SPxActiveRegion *m_activeArea;	
	/*用于存放航迹与测量之间的关联强度的矩阵*/
	double m_assocStrengthMatrix[SPX_ASSOCIATION_MATRIX_MAX_ROW_NUM * SPX_ASSOCIATION_MATRIX_MAX_COLUMN_NUM];/*16641+129 = 16770  在地址 0x160*/
	int m_assocTracksCount;
	int m_assocMeasurementsCount;
	SPxMHTtpControlRule *m_firstTpControlRule;
	SPxMHTtpControlRule *m_lastTpControlRule;
	int m_TargetCapacity;
	/********************************/
	SPxAtomic<int>  m_numTrackFree;
	SPxAtomic<int>  m_numTrackUsed;
	SPxMHTtrack **m_trackPtrList;
	SPxMHTtrack *m_trackList;
	int m_trackListSizeInitial;
	int m_nextFreeTrackIndex;
	int m_recycleTrackIndex;
	int m_totalTracksConfigure;
	/********************************/
	SPxAtomic<int>  m_numClusterFree;
	SPxAtomic<int>  m_numClusterUsed;
	int m_nextFreeClusterIndex;
	int m_recycleClusterIndex;
	int m_clusterListSizeInitial;
	/* 该变量是一个计数器，从1开始就自动累加，没有重新赋值操作,用于分配cluster id */
	int m_idCountAllocForCluster;
	int m_totalClustersConfigure;
	SPxMHTcluster **m_clusterPtrList;
	SPxMHTcluster *m_clusterList;
	/********************************/
	SPxAtomic<int>  m_numTrackLinkFree;
	SPxAtomic<int>  m_numTrackLinkUsed;
	int m_nextFreeTrackLinkIndex;
	int m_recycleTrackLinkIndex;
	SPxMHTtrackLink **m_trackLinkPtrList;
	SPxMHTtrackLink *m_trackLinkList;
	int m_trackLinkListSizeInitial;
	int m_totalTrackLinksConfigure;
	/********************************/
	int m_nextFreeHypothesisIndex;
	int m_recycleHypothesisIndex;
	SPxAtomic<int>  m_numHypothesisFree;
	SPxAtomic<int>  m_numHypothesisUsed;
	int m_hypothesisListSizeInitial;
	int m_countHypos;/*假设 ID 表示*/
	SPxMHThypothesis **m_hypothesisPtrList;
	SPxMHThypothesis *m_hypothesisList;
	int m_totalHypothesisConfigure;
	/********************************/
	SPxTargetDB *m_targetDB;
	SPxTrackReport *m_firstTrackReport;
	SPxTrackReport *m_lastTrackReport;
	TrackStatusProc m_onTrackStatusReportCallbackFunc;
	SPxObj *m_trackStatusReceiver;//
	SPxMHTmeasurementSet *m_measurementSetTemp;
	FILE* m_msgFileHandle;
	FILE* m_debugFileHandle;
	unsigned long m_lastOutputTime;
	int  m_nextExpectingProcessSector;
	char m_infoBuffer[256];
	char m_spxmht_debugFileName[256];
	SPxNavData* m_navData;
	double m_degIntervalHeadingAndCourse;/* 船首与航向的偏离角度*/
	int m_initialDelaySectors;/* m_numSectors * mhtInitialDelayScans */
	SPxWorldMap *m_worldMap;/* World map, if any */
	int m_antiClockwise;
	SPxActiveRegion *m_activeRegionArray[50];
	int	m_numActiveRegions;
	int m_debugTrackId;
	int m_IsEnableDebugOnSpecifiedTrack;
	UINT8 m_isClutterPlotReductionActive;

	int m_debugClusterId;
	int m_IsEnableDebugOnSpecifiedCluster;
	int m_loadReductionState;
	int m_countLoadReductionStateHolding;/*计数器，表示负载抑制状态m_loadReductionState 保持的次数*/
	UINT32 m_lastStasOrResetStasTime_ms;
	/*以下为统计变量，用于统计 航迹确认的平均圈数，检测概率，虚警概率等*/
	int field_21138;
	int m_countEstablishedTracks;
	int m_sumTracksAgeScansFromCreationToEatablished;
	/*航迹确认的平均圈数*/
	double m_averageAgeScansFromCreationToEatablished;/*平均等于总和除以个数m_countEstablishedTracks*/
	int    m_minAgeScansFromCreationToEatablished;/*最短航迹确认圈数*/
	int    m_maxAgeScansFromCreationToEatablished;/*最长航迹确认圈数*/

	int m_countEstablishedTrackUpdateWithMeas;
	int m_countEstablishedTrackUpdateWithoutMeas;
	double m_averagePD;
	int m_countMeasurements;
	int m_countEstablishedTrackAssociaedWithMeas;
	REAL64 m_averagePF;/*统计当前的虚惊概率，使用未成功关联的测量和总测量的比*/
	REAL64 m_maxTargetSizeUsedForSPRT;
	REAL64 m_minTargetSizeUsedForSPRT;
	REAL64 m_preMinMeasWeight;
	REAL64 m_preMaxMeasWeight;
	REAL64 m_averagerMeasWeight;
	char   field_211A0;
	REAL64 m_curMinMeasWeight;
	REAL64 m_curMaxMeasWeight;
	REAL64 m_sumCountMeasurements;
	REAL64 m_sumWeightMeasureents;
	int m_numEstablishTracks;

	/*滤波后的起批的错误率，用于评估当前的杂波环境的恶劣程度*/
	REAL64  m_falseTrackCountAreaArray[CLUTTER_ANALYSIS_AREA_SQUARES_NUM];/*10201*/
	/*统计划定区域每一圈错误的航迹数*/
	REAL64  m_numLostTrackPerScanArray[CLUTTER_ANALYSIS_AREA_SQUARES_NUM];
	/*统计划定区域每一圈禁止航迹起始数*/
	int  m_countDisableATIperScanArray[CLUTTER_ANALYSIS_AREA_SQUARES_NUM];
	/*用于测量比较的分量索引*/
	MeasurementComponent m_componentIndexUsedForMeasCompare;
	/*When tracks are being debugged, this flag affects whether all hypotheses are shown(state = 1)
	or just the principal hypothesis (state = 0).*/
	int m_isDebugAllHypo1orOnlyPriHypo0;/**/
	int m_lastCommandReturned;
	REAL64 m_curMaxClutterAnalysisRangeMetres;/*用于杂波分析的最大距离*/
	SPxMBT *m_pMBT;
	SPxAtomic<int>     m_numSectors;
	UINT32 m_lastTimePassSectorFirst_ms;
	int m_isOwnPlatformManoeuvring;/*本船或平台是否机动*/
	int m_nPastScansSinceOwnPlatformManoeuvred;/*计数器，记录从平台或本船停止机动到现在过去了多少圈*/
	UINT32 m_scanProcessingtime_ms;
	UINT32 m_filteredScanProcessingtime_ms;
	unsigned int m_radarIndex;		/* Radar index to use. */
	int field_52EF8;
public:
	/*spx1.87 163个参数  后面加XX表示在spx1.87中新增*/
	SPxMHTtrackerParameter *mhtOutputParameterList_parameter;
	SPxMHTtrackerParameter *mhtLeavePlotsBeforeATI_parameter;
	SPxMHTtrackerParameter *mhtMinTrackingRange_parameter;
	SPxMHTtrackerParameter *mhtMaxTrackingRange_parameter;
	SPxMHTtrackerParameter *mhtMeasThreshRange_parameter;
	SPxMHTtrackerParameter *mhtMeasThreshAzimuth_parameter;
	SPxMHTtrackerParameter *mhtReportAsLost_parameter;
	SPxMHTtrackerParameter *mhtModelRefForManualTrack_parameter;
	SPxMHTtrackerParameter *mhtInitialDelayScans_parameter;
	SPxMHTtrackerParameter *mhtTrackCanBeCreated_parameter;
	SPxMHTtrackerParameter *mhtRangeMeasurementVariance_parameter;
	SPxMHTtrackerParameter *mhtAzimuthMeasurementVariance_parameter;
	SPxMHTtrackerParameter *mhtAutoCalculatePeriod_parameter;
	SPxMHTtrackerParameter *mhtUpdatePeriod_parameter;
	SPxMHTtrackerParameter *mhtAllowATIinMHTcluster_parameter;
	SPxMHTtrackerParameter *mhtCheckATIcloseToTrack_parameter;
	SPxMHTtrackerParameter *mhtATItrackMargin_parameter;
	SPxMHTtrackerParameter *mhtATIwakeDistance_parameter;
	SPxMHTtrackerParameter *mhtATIwakeRange_parameter;
	SPxMHTtrackerParameter *mhtATIwakeTime_parameter;
	SPxMHTtrackerParameter *mhtATIwakeAngle_parameter;
	SPxMHTtrackerParameter *mhtATIshadowDistance_parameter;
	SPxMHTtrackerParameter *mhtATIshadowAngle_parameter;
	SPxMHTtrackerParameter *mhtATItrackMarginTime_parameter;
	SPxMHTtrackerParameter *mhtATIdirectionalMode_parameter;
	SPxMHTtrackerParameter *mhtATIdirection_parameter;
	SPxMHTtrackerParameter *mhtATIdirectionError_parameter;
	SPxMHTtrackerParameter *mhtPointTargetAziDimDegrees_parameter;
	SPxMHTtrackerParameter *mhtPointTargetRanDimMetres_parameter;
	SPxMHTtrackerParameter *mhtAssociationMode_parameter;
	SPxMHTtrackerParameter *mhtMaximumClusterAzimuthSizeDegrees_parameter;
	SPxMHTtrackerParameter *mhtHypothesisThresholdR_parameter;
	SPxMHTtrackerParameter *mhtTrackCanExist_parameter;
	SPxMHTtrackerParameter *mhtReducedSignalGain_parameter;
	SPxMHTtrackerParameter *mhtCheckStablePeriodPercent_parameter;
    SPxMHTtrackerParameter *mhtGenerateReport_parameter;//
    SPxMHTtrackerParameter *mhtGenerateCreatedReport_parameter;//add for report first create target
	SPxMHTtrackerParameter *mhtGenerateProvisionalReport_parameter;
	SPxMHTtrackerParameter *mhtMinATIRange_parameter;
	SPxMHTtrackerParameter *mhtMaxATIRange_parameter;
	//add spx v1.87
	SPxMHTtrackerParameter *mhtATIminAge_parameterXX;
	SPxMHTtrackerParameter *mhtATIdefaultNoATIdistance_parameterXX;
	SPxMHTtrackerParameter *mhtSaveNoATIpos_parameterXX;
	//
	SPxMHTtrackerParameter *mhtMinPlotRangeSize_parameter;
	SPxMHTtrackerParameter *mhtMaxPlotRangeSize_parameter;
	SPxMHTtrackerParameter *mhtMinPlotAzimuthSize_parameter;
	SPxMHTtrackerParameter *mhtMaxPlotAzimuthSize_parameter;
	SPxMHTtrackerParameter *mhtMinPlotAmplitudeTrack_parameter;
	SPxMHTtrackerParameter *mhtMinPlotAmplitudeATI_parameter;
	SPxMHTtrackerParameter *mhtMinPlotWeightATI_parameter;
	SPxMHTtrackerParameter *mhtMaxPlotWeightATI_parameter;
	SPxMHTtrackerParameter *mhtMinPlotWeightTrack_parameter;
	//add spx v1.87
	SPxMHTtrackerParameter *mhtMinPlotAziSzATI_parameterXX;
	SPxMHTtrackerParameter *mhtMinPlotRanSzATI_parameterXX;
	SPxMHTtrackerParameter *mhtMaxPlotDopplerVarATI_parameterXX;
	SPxMHTtrackerParameter *mhtMinPlotDopplerATI_parameterXX;
	//
	SPxMHTtrackerParameter *mhtFilterMode_parameter;
	SPxMHTtrackerParameter *mhtTargetFixedGainRange_parameter;
	SPxMHTtrackerParameter *mhtTargetFixedGainAzimuth_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedGainMaxGainRange_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedGainMinGainRange_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedGainMaxGainAzimuth_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedGainMinGainAzimuth_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedGainMaxSpeed_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedGainMinSpeed_parameter;
	SPxMHTtrackerParameter *mhtInitialRangeSizeVariance_parameter;
	SPxMHTtrackerParameter *mhtInitialAzimuthSizeVariance_parameter;
	SPxMHTtrackerParameter *mhtDynamicLoadControl_parameter;
	SPxMHTtrackerParameter *mhtDynamicLoadControlCPUmax_parameter;
	SPxMHTtrackerParameter *mhtClutterControl_parameter;
	SPxMHTtrackerParameter *mhtClutterTrackAgeLimit_parameter;
	SPxMHTtrackerParameter *mhtNoTrackScanPeriod_parameter;
	SPxMHTtrackerParameter *mhtWeightRangeError_parameter;
	SPxMHTtrackerParameter *mhtWeightAzimuthError_parameter;
	SPxMHTtrackerParameter *mhtWeightRangeSizeError_parameter;
	SPxMHTtrackerParameter *mhtWeightAzimuthSizeError_parameter;
	SPxMHTtrackerParameter *mhtWeightRangeRateError_parameter;
	SPxMHTtrackerParameter *mhtWeightStrengthError_parameter;
	SPxMHTtrackerParameter *mhtWeightWeightError_parameter;
	SPxMHTtrackerParameter *mhtTargetTimeGainMaxGainRange_parameter;
	SPxMHTtrackerParameter *mhtTargetTimeGainMinGainRange_parameter;
	SPxMHTtrackerParameter *mhtTargetTimeGainMaxGainAzimuth_parameter;
	SPxMHTtrackerParameter *mhtTargetTimeGainMinGainAzimuth_parameter;
	SPxMHTtrackerParameter *mhtSpeedGainWeight_parameter;
	SPxMHTtrackerParameter *mhtAziGainRangeFactor_parameter;
	//add spx v1.87
	SPxMHTtrackerParameter *mhtManoeuvreSensitivity_parameterXX;
	//
	SPxMHTtrackerParameter *mhtMinAziErrForManoeuvre_parameter;
	SPxMHTtrackerParameter *mhtMinRanErrForManoeuvre_parameter;
	SPxMHTtrackerParameter *mhtTargetTurnRate_parameter;
	SPxMHTtrackerParameter *mhtTargetTurnAccel_parameter;
	SPxMHTtrackerParameter *mhtMinTrackGateRange_parameter;
	SPxMHTtrackerParameter *mhtMinTrackGateAzimuth_parameter;
	SPxMHTtrackerParameter *mhtMinTrackGateAzimuthMetres_parameter;
	SPxMHTtrackerParameter *mhtMaxTrackGateAzimuthMetres_parameter;
	SPxMHTtrackerParameter *mhtMaxTrackGateRange_parameter;
	SPxMHTtrackerParameter *mhtMaxTrackGateAzimuth_parameter;
	SPxMHTtrackerParameter *mhtCoastScans_parameter;
	SPxMHTtrackerParameter *mhtCoastAgeMin_parameter;
	SPxMHTtrackerParameter *mhtCoastAgeMax_parameter;
	SPxMHTtrackerParameter *mhtCoastExtendedTime_parameter;
	SPxMHTtrackerParameter *mhtCoastMatureExtension_parameter;
	SPxMHTtrackerParameter *mhtTargetMaxDeceleration_parameter;
	SPxMHTtrackerParameter *mhtTargetMaxAcceleration_parameter;
	SPxMHTtrackerParameter *mhtTargetMaxTrackingSpeed_parameter;
	SPxMHTtrackerParameter *mhtTargetMinTrackingSpeed_parameter;
	SPxMHTtrackerParameter *mhtTargetSpeedScans_parameter;
	SPxMHTtrackerParameter *mhtAssocMask_parameter;
	SPxMHTtrackerParameter *mhtTrackLimitForDRChigh_parameter;
	SPxMHTtrackerParameter *mhtTrackLimitForDRClow_parameter;
	SPxMHTtrackerParameter *mhtDRCreductionDelay_parameter;
	SPxMHTtrackerParameter *mhtClutterHoldMultiplier_parameter;
	SPxMHTtrackerParameter *mhtATImode_parameter;
	SPxMHTtrackerParameter *mhtATImodeSPRTdesiredFalseAlarm_parameter;
	SPxMHTtrackerParameter *mhtATImodeSPRTdesiredFalseReject_parameter;
	SPxMHTtrackerParameter *mhtATImodeSPRTpd_small_parameter;
	SPxMHTtrackerParameter *mhtATImodeSPRTpd_large_parameter;
	SPxMHTtrackerParameter *mhtATImodeSPRTpf_parameter;
	SPxMHTtrackerParameter *mhtATIinhibitLandMetres_parameter;
	SPxMHTtrackerParameter *mhtATIinhibitSeaMetres_parameter;
	SPxMHTtrackerParameter *mhtATIintegrationM_parameter;
	SPxMHTtrackerParameter *mhtATIintegrationN_parameter;
	SPxMHTtrackerParameter *mhtATIuseRangeRate_parameter;
	SPxMHTtrackerParameter *mhtATIrangeRateThreshold_parameter;
	SPxMHTtrackerParameter *mhtMeasuredRangeRateLimit_parameter;
	SPxMHTtrackerParameter *mhtATIinitialAzimuthGain_parameter;
	SPxMHTtrackerParameter *mhtATIinitialRangeGain_parameter;
	SPxMHTtrackerParameter *mhtATInoConsistencyTests_parameter;
	SPxMHTtrackerParameter *mhtATImaxIntegrationMultiplier_parameter;
	SPxMHTtrackerParameter *mhtTargetMaxInitiationSpeed_parameter;
	SPxMHTtrackerParameter *mhtTargetMaxInitiationSpeedRev_parameter;
	SPxMHTtrackerParameter *mhtTargetMinInitiationSpeed_parameter;
	SPxMHTtrackerParameter *mhtATIrangeRateNotchCentre_parameter;
	SPxMHTtrackerParameter *mhtATIrangeRateNotchWidth_parameter;
	//add spx v1.87
	SPxMHTtrackerParameter * mhtSquelchSpeed_parameterXX;
	SPxMHTtrackerParameter * mhtSquelchScans_parameterXX;
	SPxMHTtrackerParameter * mhtSquelchCourseSD_parameterXX;
	SPxMHTtrackerParameter * mhtSquelchOnlyManual_parameterXX;
	SPxMHTtrackerParameter * mhtGateFromSizeFactor_parameterXX;
	//
	SPxMHTtrackerParameter *mhtATIrequireCourseStability_parameter;
	//add spx v1.87
	SPxMHTtrackerParameter *mhtATIminSpeedForCourseStability_parameterXX;
	//
	SPxMHTtrackerParameter *mhtTestCloseTracksAfterCoast_parameter;
	SPxMHTtrackerParameter *mhtAlwaysCreateTtmIds_parameter;
	SPxMHTtrackerParameter *mhtMaxTtmId_parameter;
	SPxMHTtrackerParameter *mhtInterpolatedReportsMode_parameter;
	SPxMHTtrackerParameter *mhtInterpolatedReportInterval_parameter;
	SPxMHTtrackerParameter *mhtInterpolatedReportTrack_parameter;
	SPxMHTtrackerParameter *mhtMbtTrackGateMarginRange_parameter;
	SPxMHTtrackerParameter *mhtMbtTrackGateMarginAzimuth_parameter;
	SPxMHTtrackerParameter *mhtDebugMode_parameter;
	//
	SPxMHTtrackerParameter *mhtPlatformPositionMode_parameterXX;
	SPxMHTtrackerParameter *mhtModelPriorityDerate_parameterXX;
	SPxMHTtrackerParameter *mhtManDerateATI_parameterXX;
	SPxMHTtrackerParameter *mhtEnableVideoTrack_parameterXX;
	SPxMHTtrackerParameter *mhtMinDensityVideoTrack_parameterXX;
	SPxMHTtrackerParameter *mhtMinRangeVideoTrack_parameterXX;
	SPxMHTtrackerParameter *mhtMinHitsVideoTrack_parameterXX;
	SPxMHTtrackerParameter *mhtEnableAISassoc_parameterXX;
	SPxMHTtrackerParameter *mhtAISrangeMargin_parameterXX;
	SPxMHTtrackerParameter *mhtAISazimuthMargin_parameterXX;
	SPxMHTtrackerParameter *mhtAIStimeoutSecs_parameterXX;
	SPxMHTtrackerParameter *mhtATIenableSidelobeTests_parameterXX;
	SPxMHTtrackerParameter *mhtATIsidelobeMaxRange_parameterXX;
	SPxMHTtrackerParameter *mhtATIsidelobeAngle_parameterXX;
	SPxMHTtrackerParameter *mhtATIsidelobeRangeDelta_parameterXX;
	//
	int    m_parameterShowMode;
	REAL64 m_ScanPeriodFilteredOrParameterSet_s;/* Update period in use (seconds) */
	UINT8 m_isChangingScanPeriod;
	REAL64 m_filteredScanPeriod_s;
	REAL64 m_preMeasuredScanPeriod_s;
	double m_rangeResolutionMetres;
	int m_isRangeResolutionSet;
	double m_aziResolutionDegrees;
	int m_isAziResolutionSet;
	SPxRunProcess *m_runProcess;
	REAL64 m_maxNominalRangeMetres;
	REAL64 m_curMaxNominalRangeMetres;
private:
	static FILE* m_gMeasurementFileHandle;
	static int   m_isLogMeasurementToFile;
	static UINT32   m_gMHTObjectCount;
	static int   m_isCreate_spxmht_message_file;
	static int  m_isCreate_spxmht_debug_file;

};
#endif//_SPX_MHT_H
