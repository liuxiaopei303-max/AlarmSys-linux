#ifndef _SPX_MHT_CLUSTERSET_H
#define _SPX_MHT_CLUSTERSET_H

/* Supporting headers */
#include "SPxMHTobj.h"
#include "SPxMHT.h"

class SPxMHTcluster;
class SPxMHT;
/*
1>  class SPxMHTclusterSet	size(0x18  24):
1>  	+---
1>   0	| +--- (base class SPxMHTobj)
1>   0	| | {vfptr}
1>   4	| | m_magicVerify
1>  	| +---
1>   8	| m_firstCluster
1>  0xc	| m_lastCluster
1>  0x10	| m_numClusters
1>  0x14	| m_pMHT
cluster 的 集合，用链表数据结构实现
*/
class SPxMHTclusterSet : public SPxMHTobj
{

public:
	SPxMHTclusterSet(SPxMHT *m_pMHT);

	/* Destructor. */
	virtual ~SPxMHTclusterSet(void);
	SPxMHTcluster * GetFirstCluster() { return this->m_firstCluster; }
	SPxMHTcluster * GetLastCluster() { return this->m_lastCluster; }
	int  GetNumClusters() { return this->m_numClusters; }
	SPxMHT* GetMHT() { return this->m_pMHT; }
	int  GetNumSectors() { return this->m_pMHT->m_numSectors; }
	/***********************************************************/
	SPxMHTcluster * NewCluster(int clusterId);
	void PushCluster(SPxMHTcluster *pCluster);
	void PopCluster(SPxMHTcluster *pCluster);
	void SortClusterByCoverageAziIntervalWithIncreasingOrder();
	void UpdateCoverage();
	void ClusteringTo(int iSectorThisSetIn);
protected:
	SPxMHTcluster* m_firstCluster;
	SPxMHTcluster* m_lastCluster;
	int            m_numClusters;
	SPxMHT*        m_pMHT;
};


#endif//_SPX_MHT_CLUSTERSET_H