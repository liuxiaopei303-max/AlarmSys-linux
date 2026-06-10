#ifndef _SPX_MHT_MEASUREMENT_H
#define _SPX_MHT_MEASUREMENT_H

/* Supporting headers */
#include "SPxMHTobj.h"
#include "./SPxLibProc/SPxBlobDetect.h"
/*
A component is only considered for association 
if it is provided by the measurement AND the component is selected to be used for association.
测量分量被用于关联，需满足两个条件
1、该分量可以通过测量得到
2、该分类被选择用于关联
*/
typedef enum MeasurementComponent_tag
{
	COMPONENT_RANGEMETRES = 0,/*测量plot的径向距离*/
	COMPONENT_AZIMUTHDEGREES = 1,/*测量plot的方位*/
	COMPONENT_SIZE_RANGEMETRES = 2,/*测量plot的在径向距离维上的尺寸*/
	COMPONENT_SIZE_AZIMUTHDEGREES = 3,/*测量plot的在方位维上的尺寸*/
	COMPONENT_STRENGTH = 4,/*测量plot*/
	COMPONENT_WEIGHT = 5,/*测量plot*/
	COMPONENT_DOPPLER_RADIALSPEEDMPS = 6,/*测量plot的径向多普勒速度*/
	COMPONENT_UNKNOWN_7 = 7,
	COMPONENT_WEIGHT_DB = 8,
	COMPONENT_METRES_FROM_SHORELINE = 9,
	COMPONENT_CENTER_BBOX_RANGEMETRES = 10,
	COMPONENT_CENTER_BBOX_AZIMUTHDEGREES = 11,
	COMPONENT_ALL_NUM=12
}MeasurementComponent;

#define SPX_MEAS_RANGE                       1
#define SPX_MEAS_AZIMUTH                     2
#define SPX_MEAS_SIZERANGE                   4
#define SPX_MEAS_SIZEAZIMUTH                 8

#define SPX_MEAS_STRENGTH                 0x10
#define SPX_MEAS_WEIGHT                   0x20
#define SPX_MEAS_DOPPLER_RADIALSPEED      0x40
#define SPX_MEAS_SENSOR_DATA              0x80

#define SPX_MEAS_WEIGHT_DB               0x100
#define SPX_MEAS_METRES_FROM_SHORELINE   0x200
#define SPX_MEAS_BBOX_RANGEMETRES        0x400
#define SPX_MEAS_BBOX_AZIMUTHDEGREES     0x800

#define SPX_MEAS_QUALITYING_FLAG        0x1000

/*
* internally type for blob information (not the same as that used SPxBlob_t).
* sizeof(SPxBlobInternal_t) = 128
*/

typedef struct SPxPlotInternal_tag {
	/* Position, polar and cartesian. */
	REAL64 rangeMetres;
	REAL64 azimuthDegrees;
	REAL64 radialSpeedMps;	/* Positive towards sensor, negative away */
	/* Convenience values. */
	REAL64 sizeRangeMetres;
	REAL64 sizeAzimuthDegrees;
	REAL64 strength;		/* Sum of values of samples used in blob. */
	REAL64 weight;		/* Number of samples used in blob. */
	/* Quality information. */
	REAL64 weightDB;
	REAL64 metresFromShoreline;	/* Positive out to sea, negative inland */
	REAL64 centerRangeMetres;
	REAL64 centerAzimuthDegrees;
	UINT8  sensorSpecificData[SPX_BLOB_SENSORDATA_SIZE];	/* Sensor-specific */
	/* Flags and reserved field. */
	UINT8 flags;		/* Bitmask of SPX_BLOB_FLAG_NONQUAL ... flags */
} SPxPlotInternal_t;

#define SPX_MEASUREMENT_COMPONENT_NUM  12 

class SPxMHT;
class SPxMHTmeasurementLink;
class SPxMHTcluster;
class SPxMHTtrack;
typedef union 
{
	UINT8   _sensorSpecificData8[SPX_BLOB_SENSORDATA_SIZE];	/* Sensor-specific */
	REAL64  _sensorSpecificData32[SPX_BLOB_SENSORDATA_SIZE/8];	/* Sensor-specific */
}SensorSpecificData;
/*sizeof=0xC0 192*/
class SPxMHTmeasurement : public SPxMHTobj
{
public:
	SPxMHTmeasurement(SPxMHT *m_pMHT, REAL64 rangeMetres, REAL64 azimuthDegrees, SPxTime_t time);
	SPxMHTmeasurement(SPxMHT *m_pMHT, REAL64 rangeMetres, REAL64 azimuthDegrees, REAL64 sizeRangeMetres, REAL64 sizeAzimuthDegrees, SPxTime_t time);
	SPxMHTmeasurement(SPxMHT *m_pMHT, SPxPlotInternal_t *measurementInfo, int plotFlag, SPxTime_t time);
	void Init(SPxMHT *m_pMHT, SPxPlotInternal_t *measurementInfo, int plotFlag, SPxTime_t time);
	virtual ~SPxMHTmeasurement();
	/**/
	REAL64 GetMeasurementComponent(int component_index);
	int GetId() { return this->m_Id; }
	SPxMHT* GetMHT() { return this->m_pMHT; }
	int GetPlotFlag() { return this->m_plotFlag; }
	SensorSpecificData* GetSensorSpecificDataPtr() { return &this->m_sensorSpecificData; }
	SPxBlob_t* GetBobPlotPtr() { return this->m_targetBlobPtr; }
	int GetIndexInAssocMatrix() {return this->m_IndexInAssocMatrix;}
	SPxTime_t * GetBobPlotTime() { return &this->m_epochTime; }
	UINT32 GetCreationTime() { return this->m_creationTime_CounterElapsedSectors; }
	REAL64 GetMeasDetectionProbability() { return this->m_detectionProbability; }
	/**/
	void SetIndexInAssocMatrix(int index) { this->m_IndexInAssocMatrix = index; }	
	void SetAssocTrackPtr(SPxMHTtrack* pTrack) { this->m_assocTrackPtr = pTrack; }
	void SetBolbPlotPtr(SPxBlob_t *blobPtr) { this->m_targetBlobPtr = blobPtr; }
	int SetUnUsedForATIFlag(int);
	/**/
	int IsWithinCluster(SPxMHTcluster *pCluster);
	bool IsAssocatedTrack() { return this->m_assocTrackPtr != NULL; }
	int IsNonQualifyingPlot() { return this->m_isNonQualifyingPlot; }
	bool IsCannotBeUsedForATI(int);
	int IsTimeCanBeUsedForATI(UINT32 timeWait_CounterElapsedSectors);
protected:
	int m_plotFlag;/*标志位，用来指示测量所包含的可用有效分量*/
	REAL64              m_plotComponent[SPX_MEASUREMENT_COMPONENT_NUM];
	SensorSpecificData  m_sensorSpecificData;	/* Sensor-specific */
	REAL64   m_detectionProbability;/*测量可能是目标，也可能是虚警，该变量表示测量是目标的检测概率，一般为0.5*/
	UINT32  m_creationTime_CounterElapsedSectors;
	int m_Id;
	int m_IndexInAssocMatrix;
	/*该标志用来指示该测量只能用于更新航迹信息，不能用于目标起始*/
	int m_isUnUsedForATI;
	/*与该测量关联的航迹，主要用于单假设？？*/
	SPxMHTtrack *m_assocTrackPtr;
	SPxMHT*   m_pMHT;
	SPxTime_t m_epochTime;
	SPxBlob_t *m_targetBlobPtr;
	int m_isNonQualifyingPlot;/*该标志用来指示该plot是否尺寸不满足的plot
							  Plots that do not pass the size tests described above are called non-qualifying plots.*/
	friend class SPxMHTmeasurementSet;
	friend class SPxMHTcluster;
	friend class SPxMHT;
};

class SPxMHTmeasurementLink : public SPxMHTobj
{
public:
	SPxMHTmeasurementLink();
	virtual ~SPxMHTmeasurementLink();
	void Init(SPxMHTmeasurement *pMeas, SPxMHT *pMHT);
	SPxMHTmeasurement* GetMeasurementPtr() { return this->m_measurement; }
	SPxMHTmeasurementLink *GetNext(void) { return this->m_next; }
	SPxMHTmeasurementLink *GetPrev(void) { return this->m_prev; }
	void SetNext(SPxMHTmeasurementLink *next) { this->m_next = next; }
	void SetId(int id) { this->m_id = id; }
private:
	int                     m_id;
	SPxMHTmeasurement*      m_measurement;
	SPxMHTmeasurementLink * m_next;
	SPxMHTmeasurementLink * m_prev;
	SPxMHT*                 m_pMHT;
	friend class SPxMHTmeasurementSet;
	friend class SPxMHT;
};

class SPxMHTmeasurementSet : public SPxMHTobj
{
public:
	SPxMHTmeasurementSet(SPxMHT* m_pMHT);
	virtual ~SPxMHTmeasurementSet();
	SPxMHTmeasurementLink* NewMeasurementLink(SPxMHTmeasurement *pMeas);
	void                   RemoveMeasurementLink(SPxMHTmeasurementLink *pMeasLnk);
	SPxMHTmeasurementLink* GetFirstMeasurementLink() { return this->m_firstMeasurementLink; }
	SPxMHTmeasurementLink* GetLastMeasurementLink() { return this->m_lastMeasurementLink; }
	int  GetNumMeasurements() { return this->m_numMeasurements; }
	void SortMeasurementWithDecreasingOrder(MeasurementComponent componentIndex, int a3);
	SPxMHT* GetMHT() { return this->m_pMHT; }
protected:
	SPxMHTmeasurementLink *m_firstMeasurementLink;
	SPxMHTmeasurementLink *m_lastMeasurementLink;
	int m_numMeasurements;
	SPxMHT* m_pMHT;
};



#endif//_SPX_MHT_MEASUREMENT_H