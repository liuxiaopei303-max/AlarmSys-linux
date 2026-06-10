/*********************************************************************
* (c) Copyright 2007 - 2010, 2015 - 2016 Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTargetDB.h,v $
* ID: $Id: SPxTargetDB.h,v 1.9 2016/09/23 13:52:44 rew Exp $
*
* Purpose:
*   Header for     class
*
* Revision Control:
*   .
*
* Previous Changes:
 
*   08/10/07 1.1    DGJ	First version
**********************************************************************/

#ifndef _SPX_MHTCLUSTER_H
#define _SPX_MHTCLUSTER_H

/* Supporting headers */
#include "SPxMHTobj.h"

typedef enum ASSOCIATION_MODE
{
	SPX_ASSOCIATION_MODE_UNKNOWN  = 0,
	SPX_ASSOCIATION_MODE_SINGLE_H = 1,
	SPX_ASSOCIATION_MODE_MULTI_H  = 2
}AssociationMode;

/*在一个簇中，最多能有多少个假设*/

#define  MAX_NUM_HYPOTHESIS_ALLOWED_IN_CLUSTER  12 

class SPxMHThypothesis;
class SPxMHTclusterSet;
class SPxMHTmeasurementSet;
class SPxMHTtrackLink;

/*sizeof=0x70 112*/
class SPxMHTcluster : public SPxMHTobj
{

public:
	SPxMHTcluster(int numSectors=0, int dataSize=0){}
	virtual ~SPxMHTcluster(void);
	void Init(SPxMHTclusterSet *parent, int cluster_label_id);
	/* Destructor. */
	SPxMHTclusterSet* GetParent() { return m_parent; }
	int GetClusterId() { return this->m_id; }
	int GetClusterInternalId() { return this->m_idInternal; }
	SPxRangeAziSegment* GetCoverage() { return &this->m_coverageRangeAzi; }
	int IsCoverageEmpty() { return this->m_coverageRangeAzi.IsEmpty(); }
	UINT32 GetNumHypothesis() { return this->m_numberHypothesis; }	/*获取假设的个数*/
	int GetNumberAssociationMeasurements() { return this->m_curAssociationMeasurementsCount;}
	SPxMHTcluster* GetNext() { return this->m_next; }
	SPxMHThypothesis * GetFirstHypothesis() { return this->m_firstHypothesis; }
	SPxMHThypothesis * GetLastHypothesis() { return this->m_lastHypothesis; }
	UINT32 GetDebug(void) const { return(m_debug); }
	UINT32 GetMinNextUpdateTime() { return this->m_minNextUpdateTime_CounterBySectors; }
	/*得到主假设，如果有就返回，没有就重新获取*/
	SPxMHThypothesis * GetPrincipalHypothesis();
	double BuildPrincipalHypothesis();
	SPxMHThypothesis * GetMinConfidenceHypothesis();

	/*返回当前cluster占据的扇区号，一个cluster 可能占据多个扇区，
	得到的扇区号是覆盖区域最尾端角度所在扇区*/
	int GetSectorForAzimuth();
	double GetStartRange() {return this->GetCoverage()->GetStartRange(); }
	double GetEndRange() {return this->GetCoverage()->GetEndRange(); }
	double GetStartAzi() {return this->GetCoverage()->GetStartAzi(); }
	double GetEndAzi() {return this->GetCoverage()->GetEndAzi(); }
	/***************************************************************************/
	void SetPrincipalHypothesis(SPxMHThypothesis *pHypothesis) { this->m_principalHypothesis = pHypothesis; }
	void SetSectorIndex(int index1) { this->m_sectorIndex = index1; }
	void SetDebug(UINT32 debug);
	void SetMinNextUpdateTime(int time) { this->m_minNextUpdateTime_CounterBySectors = time; }
	/***************************************************************************/
	int IsDebugAvailable();
	int  IsUsingSingleHypothesisAssociation(SPxMHTmeasurementSet *pMeasSet);
	/*在该假设簇中是否包含已经确立的航迹*/
	int IsContainEstablishTrack();
	int CanUseNonQualifyingPlot() { return this->m_canUseNonQualifyingPlot; }
	/***************************************************************************/
	/*计算假设类的距离增益和，以最大的距离增益和作为主假设，注意与最大信任度为主假设的区别
	该函数应该用于单假设关联，即只对主假设进行关联，其它假设不关联*/
	void ReGenPrincipalHypothesisByRangeGain();

	void UpdateCoverage();
	SPxMHThypothesis* NewHypothesis(double);
	void PushHypothesis(SPxMHThypothesis *pHypothesis);
	void PopHypothesis(SPxMHThypothesis *pHypothesis, int is_normalise_cluster);
	void PruneHypothesis(double hypothesisThresholdR= -1.0);	
	void NormaliseCluster();

	void RemoveEmptyHypothesis();
	int  RemoveSimilarHypotheses();
	/*该函数的作用是从簇中移除指定ID的tack*/
	void RemoveSpecifiedTrack(int trackId);
	void RemoveCoastExtendedActiveTracks();
	void RemoveSpecifiedTrackLink(SPxMHTtrackLink *pRemoveTrackLnk, SPxMHThypothesis *pExcludeHypoInThis);
	int RemoveOtherHyposForNonIntersectWithPrincipalHypo();
	int GetTrackCanbeSplitted(SPxMHTtrackLink **splitTrack, SPxMHThypothesis** splitTrackInHypo);
	int  Split();
	int  SplitByTracks(int isOnlyUsePrincipalHypo);
	void  OutputTrackInfoAfterAssociation();	
	void AssociateMeasurements(SPxMHTmeasurementSet *pMeasSet, SPxMHTcluster *pNewCluster);
	void DeRatingHypothesis();
	void DoExtremeAssociations(SPxMHTcluster *pCluster2);
	void CheckCloseTracksForExtendCoasting();
	void Update();
	SPxMHTcluster * ClusteringBy(SPxMHTclusterSet *pClusterSet);
	int  Clustering(SPxMHTcluster *pCluster2);
	void MergeAndRemove(SPxMHTcluster *pCluster);
	void ReassignmentToSector();
public:
	/*可用于外部显示*/
	const char* GetAssociationModeStr()
	{
		if (this->m_usedAssociationMode == SPX_ASSOCIATION_MODE_MULTI_H)
			return "MH";
		else if (this->m_usedAssociationMode == SPX_ASSOCIATION_MODE_SINGLE_H)
			return "SH";
		else
			return "--";
	}
protected:
	/*注意下面两个id的区别，
	m_idInternal 在初始化的时候 已经分配好，从始至终不变
	m_id 随着创建，移除，不停的变
	*/
	int                m_idInternal;
	int                m_id;
	SPxMHThypothesis * m_firstHypothesis;
	SPxMHThypothesis * m_lastHypothesis;
	int                m_debug;
	UINT32             m_numberHypothesis;//该簇内假设的数量
	SPxMHTcluster *    m_next;
	SPxMHTcluster *    m_prev;
	SPxMHTclusterSet * m_parent;
	SPxRangeAziSegment m_coverageRangeAzi;
	SPxMHThypothesis*  m_principalHypothesis;
	/*下次该簇的最小更新的扇区数（累加）*/
	UINT32 m_minNextUpdateTime_CounterBySectors; /*计数时间单位为扇区数，计数累加
												 下次航迹更新的最近时间，更新时间必须大于该时间，否则不予更新*/
	int                m_sectorIndex;/*!!! 从1 开始 form 1 start*/
	int                m_usedAssociationMode;/*簇内航迹与测量的关联模型，注意系统设置多假设关联，在簇内也不一定使用多假设关联*/
	int                m_curAssociationMeasurementsCount;//number association  measurements
	int                m_canUseNonQualifyingPlot;

	friend class SPxMHTclusterSet;
	friend class SPxMHT;
	friend class SPxMHThypothesis;
};

#endif /* _SPX_MHTCLUSTER_H */
