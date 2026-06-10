#ifndef _SPX_MBT_H
#define _SPX_MBT_H

#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxTypes.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibData/SPxRib.h"
#include "SPxLibUtils/SPxWorldMap.h"
#include "./SPxLibTracker/SPxMBTplot.h"
/*********************************************************************
*
*   Constants
*
**********************************************************************/
#define SPX_MBT_NUM_RANGE_INTERVALS         32 
#define SPX_MBT_NUM_SECTORS                 32
#define SPX_MBT_NUM_HALF_SECTORS            16
#define SPX_MBT_MAX_MODELS                  32  // 注意model 和 instance的区别，一个model可以有多个实例instance 

#define SPX_MBT_PARA_GROUP_INNER        0   // 内部  该项不显示
#define SPX_MBT_PARA_GROUP_GENERAL      1   // 显示标签1  General – various general settings
#define SPX_MBT_PARA_GROUP_ACTIVE       2   // 显示标签2 The parameters affecting whether and where the model is active
#define SPX_MBT_PARA_GROUP_MHT          4   // 显示标签7 Parameters that are overridden in the MHT
#define SPX_MBT_PARA_GROUP_FILTER       8   // 显示标签4 Parameters affecting filtering
#define SPX_MBT_PARA_GROUP_DYNAMICS    16   // 显示标签5 Parameters affecting target dynamics
#define SPX_MBT_PARA_GROUP_DETECT      32   // 显示标签6 Parameters affecting the criteria for track inititiation
#define SPX_MBT_PARA_GROUP_MODEL       64   // 显示标签8 Parameters that are specific to this model
#define SPX_MBT_PARA_GROUP_PLOTS      128   // 显示标签3 Parameters affecting plots

#define SPX_PASS_SECTOR_FIRST_FLAG_COUNT0     0
#define SPX_PASS_SECTOR_FIRST_FLAG_COUNT1     1
#define SPX_PASS_SECTOR_FIRST_FLAG_COUNT_BE2  2


int  SPxMBTsectorForAzimuth(double aziDeg);

typedef enum 
{
	SPX_MBT_PERIOD_FROM_FIXED = 0,
	SPX_MBT_PERIOD_FROM_MEASURED = 1,
	SPX_MBT_PERIOD_FROM_MHT = 2,
}SPxMBTPeriodMethod;

class SPxTargetDB;
class SPxNavData;
class SPxActiveRegion;
class SPxMHT;
class SPxMBT;
class SPxMBTparameter;
class SPxMBTmodel;
class SPxMBTmodelParameterGroup;
class SPxMBTtrack;
class SPxWorldMap;

void RemoveActiveRegionFunc(SPxActiveRegion *pActiveRegion, SPxMBT *pMBT);/*实现在SPxMHTdata.cpp 文件*/
int  GetMBTmodelDirectory(char *pMBTpathBuf, unsigned int pMBTpathBufByteSize);/*实现在SPxMHTdata.cpp 文件*/
typedef struct FindPlots 
{
	int nPlots;
	SPxMBTplot *plots[64];
}SPxFindPlots;

/*SPxMBT sizeof=0x4A70 =   19056 字节*/
class SPxMBT : public SPxObj
{
public:
	SPxMBT(SPxTargetDB* pTargetDB, SPxMHT *pMHT, int TargetCapacity, int plotCapacity);
	~SPxMBT();	
	void    Init();
	void    InitPlotsAndTracks();
	void    PushMBTparameter(SPxMBTparameter* para);
	void    PushMBTmodelParameterGroup(SPxMBTmodelParameterGroup*);
	void    PushMBTmodel(SPxMBTmodel *pModel);
	SPxActiveRegion* GetActiveArea() { return this->m_activeArea; }
	SPxMBTmodel *GetFirstModel() { return this->m_firstModel; }
	SPxMBTparameter * GetFirstParameter() { return this->m_firstParameterInChain; }
	SPxMBTmodelParameterGroup * GetMBTmodelParameterGroup(int groupIndexInList);
	SPxMBTmodel * GetModelByModelRef(int modelRef);
	int    GetNumActiveModels();
	double GetSpecifiedModelRangeMeasNoise(SPxMBTmodel *pModel);
	double GetSpecifiedModelAziMeasNoise(SPxMBTmodel *pModel);
	int    GetMBTmodelSpecialisationFileDirectory(char *buffer, unsigned int bufByteSize);
	int    GetBuiltInModelClassType(const char *builtInModelName);/*内置模型的名字 standard  direction tracksupport  static */
	SPxMBTmodel * GetBuiltInModelByType(int modeType);

	double  GetScanPeriod_s();
	double  GetMaxNominalRangeMetres();
	int     GetRangeIndex(double RangeMetres);
	int     IsDebugAvailable(SPxMBTtrack *track);
	void    LookForPlots(SPxMBTtrack *, SPxFindPlots *);
	void    CalcLocalArea(SPxMBTtrack *track, double *psr, double *per, double *psa, double *pea, int *nPlotsUsed, int *nPlotsReject);
	double  GetPfalse(SPxMBTtrack *track);

	void    SetActiveArea(SPxActiveRegion *pActiveRegion);
	void    ATI(signed int iSector);
	SPxMBTplot * NewPlot(double RangeMetres, double azimuthDegrees, double sizeRangeMetres, double sizeAzimuthDegrees,
		char isPlotNonQual, signed int weight, signed int strength, double radialSpeedMps);
	void PushPlotIntoSet(SPxMBTplot *newPlot);
	void PopPlotFromSet(SPxMBTplot *plot);
	void RemovePlot(SPxMBTplot *plot);

	SPxMBTtrack * NewTrack();
	
	SPxMBTtrack *CreateTrack(double range, double azimuth, SPxMBTmodel *model, SPxMBTplot *plot);
	void PushTrackIntoSet(SPxMBTtrack *newTrack);
	void PushTrackIntoSet(int rangeIndex,int sectorIndex,SPxMBTtrack *newTrack);
	void PopTrackFromSet(SPxMBTtrack *pTrack);
	void RemoveTrack(SPxMBTtrack *pTrack);
	void DeleteTrack(SPxMBTtrack *pTrack, int deleteReason);
	void RemoveMBTmodel(SPxMBTmodel *pRemoveModel);
    void NewSector(int isector1);//2023.3.1 original name  is Update
	void SectorUpdate(int sectorIndex, int a4);
	SPxMBTplot * SelectingBestPlotForTrack(SPxMBTtrack *track, SPxFindPlots *a3);
	void DeleteProvisionalInEstablishedTrackGate(SPxMBTtrack *track);
	void ReassignedTrack(SPxMBTtrack *track);

	void ClearSector(int sector);
	SPxMBTmodel * InstanceBuiltInMode(int modeType, const char *objName, int MBTmodelId);
	SPxMBTmodel * InstanceUserDefinedModel(const char *modelClassName, const char *objName, int modelId);
	SPxMBTmodel * InstanceModel(const char *modelClassName, const char *objName, int modelId);
	SPxMBTmodel * InstanceModelFromUserDefinedFile(const char *MBTfileName, const char *objName, int MBTmodelId);

	void    NewReturn(SPxRunProcess *rp, SPxReturn_tag *p_rtn);
	void    CalcScanPeriod();
	void    NewStatistics();
	void    Destroy();
	int     GetEnableModelNum();
	virtual SPxErrorCode SetStateFromConfig(void);
	virtual SPxErrorCode SetConfigFromState(void);
public:
	int Lock() { return this->m_CriticalSection.Enter(); }
	int UnLock() { return this->m_CriticalSection.Leave(); }
	void SetWorldMap(SPxWorldMap *pWorldMap) { this->m_worldMap = pWorldMap; }
public:
	SPxCriticalSection m_CriticalSection;
	SPxMBTparameter *m_firstParameterInChain;
	SPxMBTparameter *m_lastParameterInChain;
	char m_isPlotAndTrackArrayAllocated;
	//<alignment member> (size=3)
	int m_nextFreeTrackIndex;
	int m_recycleTrackIndex;
	int m_numTrackUsed;
	int m_nextFreePlotIndex;
	int m_recyclePlotIndex;
	int m_numPlotUsed;
	int m_trackListSizeInitial;
	int m_plotListSizeInitial;
	SPxTargetDB *m_targetDB;
	int m_nextExpectingProcessSector;
	int m_nDelayScans;/**/
	int m_nSectorsGap;
	int m_flagPassE;
	SPxMBTmodel **m_nonBuiltInModelInstancesPtrArray;
	SPxMBTmodel **m_modelInstancesPtrArray;
	SPxMBTmodel **m_builtInModelInstancesPtrArray;
	int *m_nonBuiltInModelInstancesUsedFlagArray;
	int m_nMaxInstancesPerModel;/*每个模型最多有几个实例*/
	int m_nMaxNoBuiltInModelInstances;
	int m_nBuiltInModel;/*有多少个内置模型*/
	int m_debugFlags;
	char m_mbtModelSpecialisationFileDirectory[256];
	SPxMBTparameter *mbtSetRangeFromMHT_parameter;
	SPxMBTparameter *mbtMaxRangeMetres_parameter;
	SPxMBTtrack *m_trackList;
	SPxMBTplot *m_plotList;
	SPxMBTtrack **m_trackPtrList;
	SPxMBTplot **m_plotPtrList;
	UINT32    m_curTime_CounterElapsedSectors;
	SPxMBTparameter *mbtPeriodMethod_parameter;
	//<alignment member> (size=4)
	REAL64 m_filteredMeasuredScanPeriod_s;
	SPxMBTparameter *mbtFixedPeriod_parameter;
	int m_passSectorFirstFlag;
	int m_errNoNewTrackCounter;
	int m_errNoNewPlotCounter;
	int m_errGE64PlotsInGateCounter;
	UINT32 m_lastTimePassSectorFirst_ms;/*上一次通过扇区1（从1开始）的时间，单位毫秒 */
	SPxMBTtrack *m_trackSetArray[SPX_MBT_NUM_SECTORS *SPX_MBT_NUM_RANGE_INTERVALS];
	int m_nTracksInSetArray[SPX_MBT_NUM_SECTORS *SPX_MBT_NUM_RANGE_INTERVALS];
	SPxMBTplot *m_plotSetArray[SPX_MBT_NUM_SECTORS *SPX_MBT_NUM_RANGE_INTERVALS];
	int m_nPlotsInSetArray[SPX_MBT_NUM_SECTORS *SPX_MBT_NUM_RANGE_INTERVALS];
	int field_4218;
	SPxMHT *m_pMHT;
	int m_debugTrackId;
	SPxMBTparameter *mbtActiveStartAzimuth_parameter;
	SPxMBTparameter *mbtActiveEndAzimuth_parameter;
	SPxMBTparameter *mbtActiveStartRange_parameter;
	SPxMBTparameter *mbtActiveEndRange_parameter;
	SPxMBTparameter *mbtActiveEndRangeSetFromMHT_parameter;
	SPxActiveRegion *m_activeArea;
	UINT32 m_curScanPlotsNonQualCount;
	UINT32 m_curScanPlotsQualCount;
	UINT32 m_preScanPlotsNonQualCount;
	UINT32 m_preScanPlotsQualCount;
	SPxWorldMap *m_worldMap;
	SPxMBTmodel *m_firstModel;
	SPxMBTmodel *m_lastModel;
	double m_log10WeigthArray[256];/*0 到 255 共 256*/
	UINT64 m_preProcessSpending_ms;/**/
	UINT64 m_curProcessSpending_ms;/**/
	FILE *m_debugFileHandle;
	//<alignment member> (size=4)
private:
	/*
	* Static variables for class.
	*/
	static SPxMBTmodelParameterGroup *m_firstInMBTmodelParameterGroupList;	/* First object allocated */
	static SPxMBTmodelParameterGroup *m_lastInMBTmodelParameterGroupList;	/* Last object allocated */
};


/*sizeof=0x5C   92*/
class  SPxMBTmodelParameterGroup :public SPxObj
{
public:
	SPxMBTmodelParameterGroup(SPxMBT* pMBT, const char* groupName,int groupFlag,int isSelected);
	~SPxMBTmodelParameterGroup();
public:
	char* m_groupNameStr;
	int   m_groupFlag;
	SPxMBT *m_pMBT;
	SPxMBTmodelParameterGroup *m_next;
	int    m_isSelected;
};
#endif//_SPX_MBT_H
