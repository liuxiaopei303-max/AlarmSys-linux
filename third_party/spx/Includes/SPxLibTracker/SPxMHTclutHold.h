#ifndef _SPX_MHTCLUTHOLD_H
#define _SPX_MHTCLUTHOLD_H

#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxTypes.h"

#include "./SPxLibTracker/SPxMHT.h"


class SPxRunProcess;
class SPxMHTtrack;
 

class SPxMHTclutHold : public SPxObj
{
public:
	SPxMHTclutHold(SPxMHT* pMHT, SPxRunProcess* rp);
	~SPxMHTclutHold();
	void ClutterHold(int _NULL, SPxMHTtrack *track);

private:
	SPxMHT *m_pMHT;
	SPxRunProcess *m_rp;
	SPxTrackReport *m_trackReport;
};

#endif//_SPX_MHTCLUTHOLD_H