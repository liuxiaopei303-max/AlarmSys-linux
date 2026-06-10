#ifndef _SPX_MHTPUBLICID_H
#define _SPX_MHTPUBLICID_H

#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxTypes.h"


#include "SPxMHTobj.h"

class SPxMHT;


class SPxMHTTtmId : public SPxMHTobj
{
public:
	SPxMHTTtmId() {}
	~SPxMHTTtmId() {}
public:
	int m_id;
};

/*SPxMHTpublicId 大小  0x48  72 */
class SPxMHTpublicId : public SPxMHTobj
{
public:
	SPxMHTpublicId();
	~SPxMHTpublicId() {}

	FILE * GetTrackDebugFileHandle() { return this->m_trackDebugFileHandle; }
	int    GetTTMIdValue(SPxMHT *pMHT);
	char * GetTrackLabelName(){ return this->m_trackLabelName; }
	int    GetTrackId(){return this->m_trackId;}
	/*多少个track 共享该 SPxMHTpublicId*/
	int    GetTracksCountShareThis() { return this->m_nTracksShareThis; }
	/******************************************************/
	FILE * CreateTrackDebugFileHandle(const char *TrackDebugFileName);
	void   SetTackLabelName(const char *label_name);
	void   SetEstablishedReportFlag(int flag) { this->m_isHasReportEstablished = flag; }
	/*当创建航迹Track或克隆航迹Track时，调用该函数*/
	void   AccumTracksCountShareThis(){ ++this->m_nTracksShareThis; }
	int    IsDesignated() { return this->m_isDesignated; }
	int    IsReferenceSet() { return this->m_isReferenceSet; }
	void   Init();
private:
	int m_trackId;
	int m_nTracksShareThis;/*多少个track 共享该 SPxMHTpublicId*/
	int m_isHasReportEstablished;/*该标志用来在指示航迹确认时，是否report了，如果没有report，如果已经report过了，就不用report 确认了*/
	int m_isDeletionMarkedSet;
	char m_trackLabelName[32];
	SPxMHTTtmId *m_ttmId;
	int m_isDesignated;
	int m_isReferenceSet;
	FILE*m_trackDebugFileHandle;

	friend class SPxMHTtrack;
	friend class SPxMHT;
};


#endif//_SPX_MHTPUBLICID_H