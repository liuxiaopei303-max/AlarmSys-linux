/*
*/
#ifndef _SPX_MHTOBJ_H
#define _SPX_MHTOBJ_H

/* Standard headers */
#include <stdio.h>

/* Include SPxCommon.h for UINT32 type definition */
#include "SPxLibUtils/SPxTypes.h"

 

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Magic number to allow us to sanity-check the base class objects. */
#define SPX_MHTOBJ_MAGIC	0xC0DE1234


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/
double GetMax(double a1, double a2, double a3);
double GetMin(double a1, double a2, double a3);
int     SPxMHTsectorForAzimuth(double azimuth_deg, int m_numSectors);
double  SPxMathsNormaliseDegree(double deg);
/*计算 正北/正南所在轴的夹角
返回值范围 0 到 90
*/
double  SPxMathsDegNormaliseDiffAlong(double a1);
double  SPxMathsCalcDegDiff(double a1, double a2);
double  SPxMathsCalcDegInterval(double deg_start, double deg_end);
int     SPxMathsIsDegInInterval(double deg, double deg_start, double deg_end);
double SPxMathsAbsDegInterval(double deg1, double deg2);
int     SPxMathsNormaliseSector(int isector1, int nstep, int total_sector_count);
int     SPxMathsDiffA2MinusA1(unsigned int a1, unsigned int a2);
int     SPxMathsHalfSectionMinus(int sector1, int sector2, int numSector);
/*
* Base class of all other XXX classes.
*/
class SPxMHTobj
{
public:
	/*
	* Public functions.
	*/
	/* Constructor/destructor. */
	SPxMHTobj(void);
	virtual ~SPxMHTobj(void);

	/* Verify that the magic number is correct */
	bool Verify();
protected:
	/*
	* Private functions.
	*/

	/*
	* Private variables.
	*/

	/* Magic number of verification, to denote start of object. Value is
	* set as SPX_MHTOBJ_MAGIC on object construction.
	*/
	UINT32 m_magicVerify;

	
	/* Prevent use of copy-constructor and assignment operator on SPxObj. */
	SPxMHTobj(const SPxMHTobj&) SPX_FUNCTION_DELETE;
	SPxMHTobj& operator=(const SPxMHTobj&) SPX_FUNCTION_DELETE;

	friend class SPxMHT;
}; /* SPxMHTobj */

typedef struct RangeSegment_tag
{
	double m_startRange;	    /* Start range in metres */
	double m_endRange;		    /* End range in metres */
}RangeSegment;
typedef struct AziSegment_tag
{
	double m_startAzimuth;	    /* Start azimuth in degrees */
	double m_endAzimuth;	    /* End azimuth in degrees */
}AziSegment;

   /*
   * SPxSegment class stores a range/azimuth segment area.
   */
class SPxRangeAziSegment  
{
public:
	/* Constructor/destructor. */
	SPxRangeAziSegment();
	SPxRangeAziSegment(double sr, double er, double sa, double ea);
	~SPxRangeAziSegment(void);
	void Empty();
	void Set(double startRng, double endRng, double startAzi, double endAzi);
	void Enlarge(double range, double azi);
	void GetInterSect(SPxRangeAziSegment *pInterSect, SPxRangeAziSegment *pSegment);
	void MoveCenter(double new_center_range_pos, double new_center_azi_pos);
	void Update(SPxRangeAziSegment *pSegment);

	int IsPointWithin(double range, double azi) const;
	int IsIntersect(SPxRangeAziSegment* pSegment) const;
	int IsInAziInterval(double azi);
	int IsEmpty() { return this->m_isEmpty; }

	double GetAziInterval()const;
	double GetCenterAzi()const;
	double GetCenterRange()const;
	double GetRangeInterval()const;
	double GetStartRange() { return this->m_rangeSeg.m_startRange; }
	double GetEndRange() { return this->m_rangeSeg.m_endRange; }
	double GetStartAzi() { return this->m_aziSeg.m_startAzimuth; }
	double GetEndAzi() { return this->m_aziSeg.m_endAzimuth; }
public:
	int    m_isEmpty;
	RangeSegment  m_rangeSeg;
	AziSegment    m_aziSeg;
}; /* SPxMHTSegment */


#endif /* _SPX_MHTOBJ_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
