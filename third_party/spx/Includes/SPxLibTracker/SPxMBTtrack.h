#ifndef _SPXMBTTRACK_H
#define _SPXMBTTRACK_H

/* Supporting headers */
#include "SPxMHTobj.h"
#include "./SPxLibProc/SPxBlobDetect.h"
#include "./SPxLibTracker/SPxMBTplot.h"

class SPxMBT;
class SPxMBTmodel;

/*!!! sizeof=0x210  528  compare ida*/
class SPxMBTtrack
{
public:
	SPxMBTtrack();
	~SPxMBTtrack() {}
	void  GetGateIndex(SPxRangeAziSegment *area, int *sa_index, int *ea_index, int *sr_index, int *er_index);
	SPxMBTtrack * CloneTrack();
	void  Init();
	void  Init(SPxMBTplot *plot);/*目标起始*/
	void  InitTrackGate(SPxMBTplot *plot);
	void  UpdateWithoutPlot();
	void  UpdateWithPlot(SPxMBTplot *plot);
	void  Update(SPxMBTplot *plot);
	int   TestTrackForPromotion();
	void  Predict();
	void  CalcNextSearchGate();
	int   CheckCanBeDelete();/*检验航迹是否满足被删除的条件，返回0表示 不满足删除条件，返回其它值对应的删除原因*/
	void  PromotionToMHTtrackEstablished();
public:
	int  m_trackId;
	int  m_establishedAsMHTtrackId;
	char m_allocated;
	char m_isLandTrack;/*该航迹的位置是否处于陆地上，是1 不是 -1*/
	char m_isSeaTrackCloseToCoast;/*该航迹的位置是否处于海上，且很接近海岸边atiCoastTrackDistance*/
	//<alignment member> (size = 5)
	/*平滑后的值 m_rangeMetres
	当前目标的距离和方位 m_rangeMetres 通过
	上一次预测目标的距离和方位 m_rangeMetresPredicted 与
	当前测量得到的距离和方位  rangeMetres
	进行滤波（不同的加权系数）m_rangeGain 后的值 
	this->m_rangeMetres = （plot->rangeMetres - this->m_rangeMetresPredicted） * this->m_rangeGain + this->m_rangeMetresPredicted;
	*/
	double m_rangeMetres;
	double m_azimuthDegrees;
	double m_sizeRangeMetres;
	double m_sizeAzimuthDegrees;
	double m_speed;
	double m_courseDeg;
	int    m_age;
	//<alignment member> (size=4)
	double m_rangeGain;
	double m_aziGain;
	double m_rangeChangeRateGain;
	double m_aziChangeRateGain;
	UINT32 m_bitHitCountLow;
	UINT32 m_bitHitCountHigh;
	double m_detectionRate;
	SPxMBTplot *m_associatedPlot;
	//<alignment member> (size=4)
	SPxRangeAziSegment m_nextSearchGate;	
	SPxMBTtrack *m_next;
	SPxMBTtrack *m_prev;
	unsigned char m_sectorIndex;
	unsigned char m_rangeIndex;
	//<alignment member> (size=2)
	SPxMBT *m_pMBT;
	UINT32 m_minNextUpdateTime_CounterBySectors;
	unsigned short m_hitsCount;
	unsigned short m_missHitsCount;
	double m_xMetres;
	double m_yMetres;
	double m_xMetresPredicted;
	double m_yMetresPredicted;
	double m_rangeMetresPredicted;
	double m_azimuthDegreesPredicted;
	double m_xMetresChangeRate;
	double m_yMetresChangeRate;
	double m_courseSD;
	double m_speedSD;
	double m_rangeChangeRateMps;
	double m_aziChangeRateDps;
	char m_isEstablished;
	//<alignment member> (size=7)
	double m_pFalseAccumulation;
	double m_pFalse;
	char m_isExistingPlotsInNewGate;
	//<alignment member> (size = 1)
	unsigned short m_associatedNonQualifyingPlotsCount;
	unsigned short m_associatedQualifyingPlotsCount;
	unsigned short m_curNumPlotsSeen;
	SPxMBTmodel *m_attachedMBTmodel;
	int m_nCountLeftSinceCourseError;
	unsigned short m_countForPfHighThanPd;
	unsigned short m_countForPfHighThanThreshold;
	//<alignment member> (size = 4)
	double m_lastRangeError;/*当前plot的测量值rangeMetres 与 track的m_rangeMetresPredicted 预测值的差 为当前误差*/
	double m_lastAzimuthError;
	unsigned char m_countRangeErrorAscending;
	unsigned char m_countAziErrorAscending;
	//<alignment member> (size=6)
	double m_filteredRangeError;
	double m_filteredAziError;
	double m_meanNumPlotsSeen;
	unsigned char m_errNotAssociatedPlotCounter;
	unsigned char m_scansConsecutiveForSpeedOutOfRange;
	//<alignment member> (size=6)
	double m_weight;
	double m_radialDopplerMps;
	double m_rangeErrorVariance;
	double m_aziErrorVariance;
	double m_courseHistroyArray[6];
	double m_speedHistroyArray[6];
	unsigned char m_histroyCounter;
	unsigned char m_curHisArrayIndex;
	char m_isAssociatedPlotNonQual;
	//<alignment member> (size=1)
	SPxBlob_t *m_targetBlobPtr;
};

#endif//_SPXMBTTRACK_H

