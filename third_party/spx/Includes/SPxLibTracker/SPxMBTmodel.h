#ifndef _SPX_MBT_MODEL_H
#define _SPX_MBT_MODEL_H

/* Supporting headers */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxCriticalSection.h"
/* Include SPxCommon.h for UINT32 type definition */
#include "SPxLibUtils/SPxTypes.h"
#include "./SPxLibUtils/SPxStrings.h"
#include "SPxLibData/SPxProcParam.h"
#include "SPxMBTParameter.h"

#define SPX_MBT_MODEL_UNKNOWN  0
#define SPX_MBTMODEL_Abstract      0
#define SPX_MBTMODEL_Standard     1
#define SPX_MBTMODEL_Direction    2
#define SPX_MBTMODEL_TrackSupport 3
#define SPX_MBTMODEL_Static       4

#define SPX_MBT_MODEL_LAST_IN_LIST_INSERT_BEFORE_HERE  5

#define SPX_MBT_MAX_NUM_MHT_PARAMETERS 1024

/*mbt override mht parameter 的 状态
0 表示 SPxMBTparameterMHT 还未创建，处于待创建阶段
1 表示 SPxMBTparameterMHT 不需要被外部进行 override 
2 表示 SPxMBTparameterMHT 已经被重写
注意：模型中包含了MHT的参数，
*/



typedef enum
{
	SPX_MBT_PARAMETER_MHT_NOT_CREATED = 0,
	SPX_MBT_PARAMETER_MHT_OVERRIDED_BY_INNER_BASE_MODELS = 1,
	SPX_MBT_PARAMETER_MHT_OVERRIDED_BY_PUBLIC = 2,
}SPxMBToverrideMHTparameterFlag;


class SPxMBTparameter;
class SPxMBT;
struct SPxMBTplot;
class SPxActiveRegion;
class SPxMBTtrack;

class SPxMBTmodel : public SPxObj
{

public:
	SPxMBTmodel(SPxMBT* pMBT);
	virtual ~SPxMBTmodel();
public:
	SPxMBTparameter * CreateParameter(const char* name, int defaultI, int minI, int maxI, const char* desc, int groupFlag);
	SPxMBTparameter * CreateParameter(const char* name, double defaultR, double minR, double maxR, const char* desc, int groupFlag);
	SPxMBTparameter * GetParameter(const char *paraName);
	int IsEnable() { return this->m_isEnable; }
	SPxActiveRegion* GetActiveArea() {return this->m_activeArea;}
	SPxMBTmodel*GetNext() { return this->m_next; }
	SPxMBT*     GetMBT() { return this->m_pMBT; }
	SPxMBTparameter * GetFirstParameter() { return this->m_firstParameterInChain; }
	char* GetModeClassName() {return this->m_modelClassName;}
	int GetModeClassType() { return this->m_modeClassType; }
	int GetModeId() {return this->m_modelId;}
	void PushParameter(SPxMBTparameter* para);
	void PopParameter(SPxMBTparameter *para);

	int   SetModelClassName(const char *modelClassName) { return SPxSnprintf(this->m_modelClassName, "%s", modelClassName); }
	void  SetActiveArea(SPxActiveRegion *pActiveRegion);
	void  SetInActiveArea(SPxActiveRegion *pActiveRegion);
	int  SetDescription(char *descriptionStr);
	void SetParasBelongToGroupShowFlag(int groupFlag, char isShow);

	virtual SPxErrorCode SetStateFromConfig(void);//6
	virtual SPxErrorCode SetConfigFromState(void);//7
	virtual void         SetActive(int isActive);//8
	/*virtual int        GetActive(void) const {return m_active;}  9*/
	virtual void         SetPfalseThreshold();/*虚函数 10 从0开始*/
	virtual int          TestPlotCanBeAttached(SPxMBTplot *plot);//11
	virtual void         VirtuaFunc12(SPxMBTtrack *) {};//---12
	virtual void         VirtualFunc13(SPxMBTtrack *, SPxMBTplot *) {};//----13
	virtual double       GetATIRangeGain(int );//14
	virtual double       GetATIAzimuthGain(int );//15
	virtual double       CalcMaxRangeChange(SPxMBTtrack *track);//16
	virtual double       CalcMaxAziChange(SPxMBTtrack *track);//17
	virtual void         CalcGateDueToSpeed(SPxMBTtrack *mbtTrack) {};//---18
	virtual void         CalcGateDueToModel(SPxMBTtrack *mbtTrack, SPxMBTplot *plot) {};//19
	virtual int          CheckCourseChange(SPxMBTtrack *mbtTrack) { return 0; };//20
	virtual int          TestModelSpecificError(SPxMBTtrack *) { return 0; };// 21 返回1表示没有航迹通过测试，参数没有通过模型参数的检验
	//----
	int IsActive(double rangeMetres, double azimuthDegrees);
	SPxMBTparameterMHT * OverrideMHTparameterValue(const char *paraName, double paraValue, int isPublicOverride);
	int    GetOverrideMHTparameterValueI(int MHTparameterId);
	double GetOverrideMHTparameterValueR(int MHTparameterId);
	int    GetOverrideMHTparameterFlag(int MHTparameterId);
	void   RemoveOverrideMHTparameter(const char *MHTparaName);
	void  Init(SPxMBTmodel *modePtr);
	SPxErrorCode SaveMBTModelToFile(const char *modelFileName, const char *modelName);
protected:
	int  m_modeClassType;/*unknow standard direction tracksupport static   or user defined  */
	int  m_modelId;
	int m_isAtiAllowed;/*是否允许航迹自动起始，即创建新的航迹，如果允许可以创建新的航迹，如果不允许，点迹只能用于已有航迹更新*/
	char field_54;
	char field_55;
	char m_descriptionStr[80];/*对模型的描述*/
	char m_basedOn[80];/*该模型是依据的内置模型的名称*/
	int  m_isEnable;
	char m_modelClassName[80];/*可能是内置类型，也可能是其它派生类型*/
	SPxMBTparameter *m_firstParameterInChain;
	SPxMBTparameter *m_lastParameterInChain;
	SPxActiveRegion *m_activeArea;/*模型适用的区域*/
	SPxActiveRegion *m_inActiveArea;/*模型不适用的区域*/
	unsigned char     m_MBTparameterMHToverrideFlag[SPX_MBT_MAX_NUM_MHT_PARAMETERS];
	SPxMBTparameterMHT * m_MBTparameterMHTarray[SPX_MBT_MAX_NUM_MHT_PARAMETERS];
    /*37个参数*/
	SPxMBTparameter * mbtModelRef_parameter;
	SPxMBTparameter * mbtModelPriority_parameter;
	SPxMBTparameter * mbtCanUseNQplots_parameter;
	SPxMBTparameter * mbtAtiActiveStartAzimuth_parameter;
	SPxMBTparameter * mbtAtiActiveEndAzimuth_parameter;
	SPxMBTparameter * mbtAtiActiveStartRange_parameter;
	SPxMBTparameter * mbtAtiActiveEndRange_parameter;
	SPxMBTparameter * mbtAtiRangeGain_parameter;
	SPxMBTparameter * mbtAtiAzimuthGain_parameter;
	SPxMBTparameter * mbtAtiInitialSpeedGain_parameter;
	SPxMBTparameter * mbtAtiOnlyEstimatePosition_parameter;
	SPxMBTparameter * mbtAtiMinSpeed_parameter;
	SPxMBTparameter * mbtAtiMaxSpeed_parameter;
	double field_1590;
	double field_1598;
	SPxMBTparameter * mbtAtiRemoveLandTracks_parameter;
	SPxMBTparameter * mbtAtiRemoveCoastTracks_parameter;
	SPxMBTparameter * mbtAtiCoastTracksDistance_parameter;
	SPxMBTparameter * mbtAtiMinDetectionRate_parameter;
	SPxMBTparameter * mbtMhtAlwaysUsesNQplots_parameter;
	SPxMBTparameter * mbtAtiMinPlotWeight_parameter;
	SPxMBTparameter * mbtAtiMaxPlotWeight_parameter;
	SPxMBTparameter * mbtAtiMinPlotLevel_parameter;
	SPxMBTparameter * mbtAtiMinPlotSizeRange_parameter;
	SPxMBTparameter * mbtAtiMinPlotSizeAzimuth_parameter;
	SPxMBTparameter * mbtAtiMaxPlotSizeRange_parameter;
	SPxMBTparameter * mbtAtiMaxPlotSizeAzimuth_parameter;
	double m_rangeGain;
	double m_aziGain;
	SPxMBTparameter * mbtAtiMaxCoasts_parameter;
	SPxMBTparameter * mbtAtiMinHits_parameter;
	SPxMBTparameter * mbtAtiMinRdot_parameter;
	SPxMBTparameter * mbtAtiMaxRdot_parameter;
	SPxMBTparameter * mbtAtiMaxSDofCourse_parameter;
	SPxMBTparameter * mbtAtiMaxSDofSpeedPc_parameter;
	SPxMBTparameter * mbtAtiMinConfidence_parameter;

	double m_pFalseThreshold;/*模型d*/
	SPxMBTmodel *m_next;
	SPxMBTparameter * mbtAtiIsActive_parameter;
	SPxMBTparameter * mbtMhtCanTakeOver_parameter;
	SPxMBTparameter * mbtSupportOnly_parameter;
	SPxMBTparameter * mbtRangeMeasNoise_parameter;
	SPxMBTparameter * mbtAzimuthMeasNoise_parameter;
	int field_1620;
	SPxMBT *m_pMBT;
	int field_1628;
	friend class SPxMBT;
	friend class SPxMBTtrack;
	friend class SPxMHTtrack;
	friend class SPxMHTtrackClass;
	friend class SPxMHTcluster;
};

class SPxMBTmodelStandard :public SPxMBTmodel
{
public:
	SPxMBTmodelStandard(SPxMBT* pMBT);
	virtual void CalcGateDueToModel(SPxMBTtrack *mbtTrack, SPxMBTplot *plot);

};

/**********************************************/

#define SPX_MBTMODEL_Direction_ATIMode_ApproachingRadar    0
#define SPX_MBTMODEL_Direction_ATIMode_ApproachingSpecificLocation    1
#define SPX_MBTMODEL_Direction_ATIMode_FixedCourse    2

class SPxMBTmodelDirection :public SPxMBTmodel
{
public:
	SPxMBTmodelDirection(SPxMBT* pMBT);
	virtual ~SPxMBTmodelDirection() {};
	virtual void CalcGateDueToSpeed(SPxMBTtrack *track);
	virtual void CalcGateDueToModel(SPxMBTtrack *mbtTrack, SPxMBTplot *plot);
	virtual int  CheckCourseChange(SPxMBTtrack *track);
	virtual int  TestModelSpecificError(SPxMBTtrack *track);
	SPxMBTparameter* mbtAtiDirectionError_parameter;
	SPxMBTparameter* mbtAtiMode_parameter;
	SPxMBTparameter* mbtAtiPointX_parameter;
	SPxMBTparameter* mbtAtiPointY_parameter;
	SPxMBTparameter* mbtAtiFixedCourse_parameter;
	double  CalcCourseDeg(SPxMBTtrack *track);
};
/*该模型不提供航迹自动起始能力。 This model does not provide ATI capabilities.

*/
class SPxMBTmodelTrackSupport :public SPxMBTmodelStandard
{
public:
	SPxMBTmodelTrackSupport(SPxMBT* pMBT);


};
/*
The model looks for stationary or very slow moving targets. 
It sets appropriate values for filter gains for the acquisition process and in the subsequent MHT processing. 
This model can be used to find very weak targets which appear in the video only infrequently.
*/
class SPxMBTmodelStatic :public SPxMBTmodelStandard
{
public:
	SPxMBTmodelStatic(SPxMBT* pMBT);


};

#endif//_SPX_MBT_MODEL_H