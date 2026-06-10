#ifndef _SPX_MHT_TRACKCLASS_H
#define _SPX_MHT_TRACKCLASS_H

#include "SPxLibUtils/SPxTypes.h"
#include "SPxLibUtils/SPxObj.h"


class SPxMHTtrack;/*前向声明*/
class SPxMHTtrackClass : public SPxObj
{
public:
	SPxMHTtrackClass();
	~SPxMHTtrackClass();
	void Init();
	int ParseBuffer(char *paraBuf);
	/*该函数用来检查输入的track是否属于当前类，通过检查航迹的属性来实现*/
	int TestTrackBelongTo(SPxMHTtrack *pMHTtrack);
	SPxMHTtrackClass *GetNext() { return this->m_next; }
	static void LockList(void);
	static void UnLockList(void);
	static void LoadMHTTrackClassRulesFile(const char *fileName, int isFullPath = FALSE, int silent = FALSE);
	static void RemoveAllMHTtrackClassLists();
private:
	REAL64 m_minSpeed;
	mutable int    m_isMinSpeedSet;/* Is the m_minSpeed valid? */

	REAL64 m_maxSpeed;
	mutable int    m_isMaxSpeedSet;/* Is the m_maxSpeed valid? */

	REAL64 m_minWeight;
	mutable int    m_isMinWeightSet;/* Is the m_minWeight valid? */

	REAL64 m_maxWeight;
	mutable int    m_isMaxWeightSet;/* Is the m_maxWeight valid? */

	REAL64 m_minRadSpeed;
	mutable int    m_isMinRadSpeedSet;/* Is the m_minRadSpeed valid? */

	REAL64 m_maxRadSpeed;
	mutable int    m_isMaxRadSpeedSet;/* Is the m_maxRadSpeed valid? */
	int m_minAge;
	mutable int    m_isMinAgeSet;/* Is the m_minAge valid? */
	int m_isNoDefaultSet;//!!!!!!
	int m_noDefault;///!!!!!!!!!!!!
	char *m_activeAreaName;
	int m_isActiveAreaNameSet;

	REAL64 m_minRange;
	mutable int    m_isMinRangeSet;

	REAL64 m_maxRange;
	mutable int     m_isMaxRangeSet;

	REAL64 m_minAzi;
	mutable int    m_isMinAziSet  ;

	REAL64 m_maxAzi;
	mutable int     m_isMaxAziSet  ;

	REAL64 m_minCourse;
	mutable int     m_isMinCourseSet;

	REAL64 m_maxCourse;
	mutable int     m_isMaxCourseSet;
	int m_overSea;
	int  m_isOverSeaSet ;

	REAL64 m_overSeaMargin;
	mutable int   m_isOverSeaMarginSet;
	int m_overLand;
	mutable int  m_isOverLandSet;
 
	REAL64 m_overLandMargin;
	mutable int  m_isOverLandMarginSet;
	int m_lockAfter ;
	mutable int  m_isLockAfterSet;
	int m_holdFor;
	mutable int  m_isHoldForSet ;
	int m_modelRef;
	mutable int  m_isModelRefSet;
 
	REAL64 m_minAltitude;
	mutable int  m_isMinAltitudeSet;
 
	REAL64 m_maxAltitude;
	mutable int  m_isMaxAltitudeSet;
	int m_setModel;
	mutable int  m_isSetModelSet;

	int m_code;
	char* m_className;
	char m_shortformName[4];
	SPxMHTtrackClass *m_next;

	static SPxCriticalSection m_mutex;	/* List protection mutex */
	static SPxMHTtrackClass *m_firstInChain;	/* First object allocated */
	static SPxMHTtrackClass *m_lastInChain;	/* Last object allocated */
	static SPxAtomic<int> m_numberInstances;	/* Number of instances */
public:
	/* Access to linked list of allocated objects. */
	static SPxMHTtrackClass *GetFirstObject();
	static SPxMHTtrackClass *GetLastObject();

	friend class SPxMHTtrack;

};


#endif//_SPX_MHT_TRACKCLASS_H