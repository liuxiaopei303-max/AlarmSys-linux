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

#ifndef _SPX_MHTHYPOTHESIS_H
#define _SPX_MHTHYPOTHESIS_H

/* Supporting headers */
#include "SPxMHTobj.h"



class SPxMHTcluster;
class SPxMHTtrackLink;
class SPxMHTtrack;
class SPxMHTmeasurement;
class SPxMHTmeasurementSet;
class SPxMHTmeasurementLink;

typedef struct TrackMeasAssoc_tag
{
	SPxMHTtrack*       pAssocTrack;
	SPxMHTmeasurement* pAssocMeas;
	double              assocStrength;/*测量与 航迹的关联强度*/
	void Empty() {
		this->pAssocMeas = NULL;
		this->pAssocTrack = NULL;
		this->assocStrength = 0.0;
	}
	void Set(SPxMHTtrack* pAssocTrack,SPxMHTmeasurement* pAssocMeas,double assocStrength) {
		this->pAssocMeas = pAssocMeas;
		this->pAssocTrack = pAssocTrack;
		this->assocStrength = assocStrength;
	}
}TrackMeasAssoc;
/* 在假设类里，所包含的每个SPxMHTtrackLink 的 id 都不一样？？
在一个cluster里，所包含的不同假设，可能会包含同样id的SPxMHTtrackLink
*/
class SPxMHThypothesis : public SPxMHTobj
{
public:
	SPxMHThypothesis(SPxMHTcluster* pCluster=NULL);
	/* Destructor. */
	virtual ~SPxMHThypothesis() {};
	void Init(SPxMHTcluster *parent);
	SPxMHThypothesis * GetNext() { return this->m_next; }
	SPxMHThypothesis * GetPrev() { return this->m_prev; }
	SPxMHTcluster* GetParent() { return this->m_parent; }
	int  GetHypothesisId() { return this->m_id; }
	SPxRangeAziSegment* GetCoverage() { return &this->m_coverageRangeAzi; }
	SPxMHTtrackLink * GetFirstTrackLink() { return this->m_firstTrackLink; }
	SPxMHTtrackLink * GetLastTrackLink() {return this->m_lastTrackLink;}
	int GetNumTracks() {return this->m_numberTrackLinks;}
	UINT32 GetDebug(void) const { return(m_debug); }
	REAL64 GetConfidence() { return this->m_confidence; }
	int GetEstablishTracksNum();
	/*获取假设中最老的那个航迹，单位为过去的扇区数*/
	UINT32 GetOldestTrack_CounterElapsedSectors();
	SPxMHTtrackLink *GetNonIntersectTrackLink();
	/**************************************************************/
	void PushTrackLink(SPxMHTtrackLink *);
	void PopTrackLink(SPxMHTtrackLink *);
	SPxMHTtrackLink * NewTrackLink(SPxMHTtrack *pTrack);
	void SetDebug(UINT32 debug) { m_debug = debug; }
	void SetConfidence(double confidence) { this->m_confidence = confidence; }
	SPxMHThypothesis * CloneHypoToSpecifiedCluster(SPxMHTcluster*pCloneHypoInCluster, double cloneHypoConf);
	SPxMHThypothesis * Clone();
	void LossyMergeNoMeasAssocHypo(SPxMHThypothesis* pOtherHypo);
	void LosslessMerge(SPxMHThypothesis* pOtherHypo);
	void MultiHypothesisAssociation(SPxMHTmeasurementLink *pMeasLnk, SPxMHThypothesis *pTempHypothesis,
		SPxMHTmeasurementSet *pTempMeasSet, SPxMHTcluster *a6, double a5);
	int  AssocMeasForNewTrack(SPxMHTmeasurement *pMeas, SPxMHTtrackLink **pNewTrackLnk);
	void AssocToFormHypos(SPxMHTmeasurementLink *pMeasLnk, SPxMHThypothesis *pTempHypo, SPxMHTcluster *pFormHyposInCluster);
	SPxMHTcluster * AssocToFormCluster(SPxMHTmeasurementSet *pMeasSet, SPxMHTcluster *pFormHyposInCluster, double conf);
	/**************************************************************/
	bool IsPrincipalHypothesis();
	int IsDebugAvailable();
	int IsValid();
	int IsContainEstablishTrack();/*在该假设中是否包含已经确立的航迹*/
	int IsFullEstablishTracks();	
	int IsContainExtendCoastActiveTrack();/*在该假设中是否包含处于扩展外推的航迹*/
	int IsContainSimilarTrack(SPxMHTtrack *pTrack);
	int IsSimilar(SPxMHThypothesis *pOtherHypo);
	int IsAllTracksIntersect();
	/**************************************************************/
	void InsertTrackLinkAt(SPxMHTtrackLink *pNode, SPxMHTtrackLink *pInsertTrackLnk);
	void MoveSpecifiedTrackToOtherHypo(int trackId, SPxMHThypothesis*pNewHypoTo);
	void UpdateTrack();
	void ForceNoAssociationOnShared(SPxMHTmeasurementSet *pMeasSet);
	SPxRangeAziSegment * UpdateCoverage();
	/*返回true 表示删除自身，返回false 表示自身不符合删除条件，不删除*/
	int RemoveSelf(double principalHypothesisConfidence, double ThresholdR);
	/*进行关联的步骤*/
	void RemoveSomeProvisionalTracks();
	/*计算航迹与测量的关联矩阵，与多假设和单假设无关 */
	int  CalcAssocMatrix(SPxMHTmeasurementSet *pMeasSet);
	/*对假设内的所有航迹，复位关联的测量，即设置关联的测量为空*/
	void NoAssociatedAnyMeas();
	/*单假设关联，也就是说一个航迹至多与一个测量进行关联*/
	void SH_Association(SPxMHTmeasurementSet *pMeasSet);
	/*多假设关联*/
	void MH_Association(SPxMHTmeasurementSet *pMeasSet, SPxMHTcluster* pCluster);
	void MH_Association(SPxMHTmeasurementLink *a2, SPxMHThypothesis *a3, SPxMHTmeasurementSet *a4, SPxMHTcluster *a6, double a5);

protected:
	SPxMHTtrackLink *m_firstTrackLink;
	SPxMHTtrackLink *m_lastTrackLink;
	int m_numberTrackLinks;
	SPxMHThypothesis* m_next;
	SPxMHThypothesis* m_prev;
	REAL64 m_confidence;/*信任度值，该值受m_confidenceDeRatingRatio 影响，*/
	SPxMHTcluster *m_parent;
	SPxRangeAziSegment m_coverageRangeAzi;
	int m_debug;
	/*models. If a track is created for two different models, the models will be
de-rated (reduced confidence in track association) according to their
priority. The highest priority model for a track has de-rating 1.0. The next
highest priority has de-rating 0.5, then 0.25 etc.*/
	REAL64 m_confidenceDeRatingRatio;/*信任度下降系数  值×下降系数 = 最终下降后的值 1.0 表示不下降 0.25表示将为1/4的原值*/
	int m_id;
	REAL64 m_confidenceWithoutDeRating;/*信任度值，该值不受m_confidenceDeRatingRatio 影响*/
	int m_canUseNonQualifyingPlot;
	friend class SPxMHTcluster;
	friend class SPxMHT;
};

#endif /* _SPX_MHTHYPOTHESIS_H */
