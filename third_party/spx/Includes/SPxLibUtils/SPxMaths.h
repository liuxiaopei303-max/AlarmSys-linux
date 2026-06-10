/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxMaths.h,v $
* ID: $Id: SPxMaths.h,v 1.37 2017/08/31 15:35:58 rew Exp $
*
* Purpose:
*   Mathematical utility functions.
*
* Revision Control:
*   31/08/17 v1.37   AGC	Add SPxMathsCalcRangeBearingIntersect().
*
* Previous Changes:
*   16/05/17 1.36   AGC	Add SPxMathsRotateXYNorthToBearing().
*   30/09/16 1.35   REW	Add SPxMathsCalcLatLongCPA().
*   23/12/15 1.34   AGC	Support arguments in either order for SPxMathsClamp.
*   10/12/15 1.33   AGC	Add range/azimuth to XY conversion functions.
*   26/11/15 1.32   AGC	Enhance slant range functions.
*			Add SPxMathsEarthCurvature().
*   22/04/15 1.31   AGC	Add SPxMathsModInRange().
*   26/03/15 1.30   AGC	Add SPxMathsSign().
*			Add SPxMathsModFilter().
*   17/03/15 1.29   AGC	Support all types for SPxMathsClamp().
*			Fix cppcheck warning.
*   26/01/15 1.28   AGC	Add SPxMathsRound().
*   07/11/14 1.27   AGC	Add SPxMathsModDiff().
*   22/10/14 1.26   AGC	Add SPxMathsMin/Max() taking four arguments.
*			Add SPxMathsHypot().
*   02/10/14 1.25   REW	Add isOnSegmentA/BRet to SPxMathsIntersect().
*   24/09/14 1.24   AGC	Fix icc remark.
*   24/09/14 1.23   AGC	Add UINT64 version of SPxMathsOrderedDiff().
*			Add SPxMathsMulDiv().
*   01/09/14 1.22   AGC	Add new SPxArea overload for SPxMathsClamp().
*   22/08/14 1.21   SP 	Add SPxMathsSlantToGroundRange().
*			Add SPxMathsGroundToSlantRange().
*   27/08/14 1.20   AGC	Add SPxArea overloads for SPxMathsClamp().
*			Add SPxMathsMin/SPxMathsMax.
*			Add SPxMathsNextPower().
*   01/04/14 1.19   SP 	Remove SPxMathsGetStabilisationError().
*			Replace with SPxMathsWorldToPlatform() and 
*			SPxMathsPlatformToWorld().
*   26/03/14 1.18   AGC	Add SPxMathsOrderedDiff().
*   26/03/14 1.17   SP 	Add SPxMathsGetStabilisationError().
*   15/01/14 1.16   AGC	Add SPxMathsIntersect().
*   06/01/14 1.15   REW	Add Intercept functions.
*   26/11/13 1.14   AGC	Add SPxPos overload for SPxMathsClamp.
*   22/11/13 1.13   AGC	Add SPxSize overload for SPxMathsClamp.
*   18/11/13 1.12   REW	Do last change differently.
*   18/11/13 1.11   REW	Avoid icc warning for SPxMathsFilter().
*   18/11/13 1.10   AGC	Add SPxMathsFilter().
*   21/10/13 1.9    AGC	Add SPxMathsClamp().
*   07/10/13 1.8    AGC	Add SPxMathsDistance().
*   17/09/13 1.7    SP 	Add SPxMathsRotateXY().
*   18/03/13 1.6    AGC	Add SPxMathsFastLog().
*   25/01/13 1.5    AGC	Add SPxMathsNormalise().
*   06/06/12 1.4    SP 	Add SPxMathsIsNan().
*   12/03/12 1.3    REW	Need SPxError.h for SPxErrorCode.
*   08/03/12 1.2    REW	Add CPA functions.
*   22/09/11 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_MATHS_H
#define _SPX_MATHS_H

/*********************************************************************
*
*   Headers 
*
**********************************************************************/

/* Need error types. */
#include "SPxLibUtils/SPxError.h"

/* Need SPxArea/SPxPos/SPxSize. */
#include "SPxLibUtils/SPxArea.h"
#include "SPxLibUtils/SPxPos.h"
#include "SPxLibUtils/SPxSize.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

/*
 * Extern the functions we provide.
 */
/* Comparison functions. */
template<typename T>
extern int SPxMathsCompRel(T v1, T v2, T maxRelError);

template<typename T>
extern int SPxMathsCompAbs(T v1, T v2, T maxError);

template<typename T>
inline T SPxMathsMulDiv(T v, T mul, T div)
{
    return (div == T()) ? T(-1) : ((v / div) * mul) + ((v % div) * mul / div);
}

template<typename T>
extern T SPxMathsRound(T val);

extern float SPxMathsNormalise(float val, float min, float max);
extern double SPxMathsNormalise(double val, double min, double max);
extern int SPxMathsNormalise(int val, int min, int max);

template<typename T>
inline T SPxMathsClamp(T val, T m1, T m2)
{
    const T& min = (m1 < m2) ? m1 : m2;
    const T& max = (m2 > m1) ? m2 : m1;
    return (val < min) ? min : ((max < val) ? max : val);
}

template <typename T>
inline SPxPos<T> SPxMathsClampPos(const SPxPos<T>& val, const SPxPos<T>& min, const SPxPos<T>& max)
{
    return SPxPos<T>(SPxMathsClamp(val.X, min.X, max.X),
		     SPxMathsClamp(val.Y, min.Y, max.Y));
}

template <typename T, typename T2a, typename T2b, typename T2c>
inline SPxSize<T, T2a> SPxMathsClampSize(const SPxSize<T, T2a>& val, const SPxSize<T, T2b>& min, const SPxSize<T, T2c>& max)
{
    return SPxSize<T, T2a>(SPxMathsClamp(val.Width, min.Width, max.Width),
			   SPxMathsClamp(val.Height, min.Height, max.Height));
}

template <typename T, typename T2>
inline SPxArea<T, T2> SPxMathsClampArea(const SPxArea<T, T2>& val, const SPxPos<T>& min, const SPxPos<T>& max)
{
    return SPxArea<T, T2>(SPxMathsClampPos(val.tl(), min, max),
			  SPxMathsClampPos(val.br(), min, max));
}

template <typename T, typename T2a, typename T2b, typename T2c>
inline SPxArea<T, T2a> SPxMathsClampArea(const SPxArea<T, T2a>& val, const SPxSize<T, T2b>& min, const SPxSize<T, T2c>& max)
{
    SPxArea<T, T2a> clampArea(SPxPos<T>(), SPxMathsClampSize(val.Size, min, max));
    clampArea.setCentre(val.centre());
    return clampArea;
}

template<typename T, typename U>
inline T SPxMathsFilter(const T& val1, const T& val2, U filter)
{
    return val1 * filter + val2 * (static_cast<U>(1) - filter);
}

template<typename T>
inline int SPxMathsSign(T val) {
    return (T() < val) - (val < T());
}

template<typename T>
inline const T& SPxMathsMin(const T& a, const T& b)
{
    return (b < a) ? b : a;
}

template<typename T>
inline const T& SPxMathsMin(const T& a, const T& b, const T& c)
{
    return (b < a) ? ((c < b) ? c : b) : ((c < a) ? c : a);
}

template<typename T>
inline const T& SPxMathsMin(const T& a, const T& b, const T& c, const T& d)
{
    const T& maxAB = SPxMathsMin(a, b);
    const T& maxCD = SPxMathsMin(c, d);
    return SPxMathsMin(maxAB, maxCD);
}

template<typename T>
inline const T& SPxMathsMax(const T& a, const T& b)
{
    return (b > a) ? b : a;
}

template<typename T>
inline const T& SPxMathsMax(const T& a, const T& b, const T& c)
{
    return (b > a) ? ((c > b) ? c : b) : ((c > a) ? c : a);
}

template<typename T>
inline const T& SPxMathsMax(const T& a, const T& b, const T& c, const T& d)
{
    const T& maxAB = SPxMathsMax(a, b);
    const T& maxCD = SPxMathsMax(c, d);
    return SPxMathsMax(maxAB, maxCD);
}

template<typename T>
extern T SPxMathsHypot(T a, T b);

extern UINT8 SPxMathsOrderedDiff(UINT8 val1, UINT8 val2, UINT8 wrapVal=0xFF);
extern UINT16 SPxMathsOrderedDiff(UINT16 val1, UINT16 val2, UINT16 wrapVal=0xFFFF);
extern UINT32 SPxMathsOrderedDiff(UINT32 val1, UINT32 val2, UINT32 wrapVal=0xFFFFFFFF);
extern UINT64 SPxMathsOrderedDiff(UINT64 val1, UINT64 val2, UINT64 wrapVal=0xFFFFFFFFFFFFFFFFULL);

template<typename T>
extern T SPxMathsModDiff(T val1, T val2, T range);

template<typename T>
extern int SPxMathsModInRange(T val, T start, T end, T range);

template<typename T, typename U>
inline T SPxMathsModFilter(T val1, T val2, T range, U filter)
{
    const T diff = SPxMathsModDiff(val1, val2, range);
    return SPxMathsNormalise(val1 + diff * (static_cast<U>(1) - filter), T(), range);
}

extern int SPxMathsNextPower(int val, int power);

extern int SPxMathsIsNaN(double fval);

/* Closest Point of Approach (CPA/TCPA) functions. */
extern SPxErrorCode SPxMathsCalcPolarCPA(
				    double rangeMetres1, double azimuthDegs1,
				    double speedMps1, double courseDegs1,
				    double rangeMetres2, double azimuthDegs2,
				    double speedMps2, double courseDegs2,
				    double *closestMetresPtr,
				    double *closestSecondsPtr);
extern SPxErrorCode SPxMathsCalcCartesianCPA(
				    double xMetres1, double yMetres1,
				    double speedMps1, double courseDegs1,
				    double xMetres2, double yMetres2,
				    double speedMps2, double courseDegs2,
				    double *closestMetresPtr,
				    double *closestSecondsPtr);
extern SPxErrorCode SPxMathsCalcLatLongCPA(
				    double latDegs1, double longDegs1,
				    double speedMps1, double courseDegs1,
				    double latDegs2, double longDegs2,
				    double speedMps2, double courseDegs2,
				    double *closestMetresPtr,
				    double *closestSecondsPtr);

/* Intercept functions. */
extern SPxErrorCode SPxMathsCalcPolarIntercept(
				    double interceptorRangeMetres,
				    double interceptorAziDegs,
				    double interceptorSpeedMps,
				    double targetRangeMetres,
				    double targetAziDegs,
				    double targetSpeedMps,
				    double targetCourseDegs,
				    double *timeSecsPtr,
				    double *courseDegsPtr,
				    double *rangeMetresPtr = NULL,
				    double *aziDegsPtr = NULL);
extern SPxErrorCode SPxMathsCalcCartesianIntercept(
				    double interceptorXMetres,
				    double interceptorYMetres,
				    double interceptorSpeedMps,
				    double targetXMetres,
				    double targetYMetres,
				    double targetSpeedMps,
				    double targetCourseDegs,
				    double *timeSecsPtr,
				    double *courseDegsPtr,
				    double *xMetresPtr = NULL,
				    double *yMetresPtr = NULL);

/* Fast approximate natural logarithm. */
extern double SPxMathsFastLog(double val, unsigned int numTerms=2);

/* Rotate an X/Y point. */
extern void SPxMathsRotateXY(double x, double y, double angleDegs, 
                             double *pXRtn, double *pYRtn);
void SPxMathsRotateXYNorthToBearing(double x, double y, 
				    double bearingDegs, 
				    double *pXRtn, double *pYRtn);

/* Distance between two cartesian points. */
extern double SPxMathsDistance(double x1, double y1,
			       double x2, double y2);

/* Distance from point (x, y) to line segment. */
extern double SPxMathsDistance(double x, double y,
			       double x1, double y1,
			       double x2, double y2);

/* Find intersection point (xRet, yRet)
 * between two lines [(x1, y1)->(x2, y2) and (x3, y3)->(x4,y4)].
 */
extern SPxErrorCode SPxMathsIntersect(double x1, double y1,
				      double x2, double y2,
				      double x3, double y3,
				      double x4, double y4,
				      double *xRet, double *yRet,
				      int *isOnSegmentARet=NULL,
				      int *isOnSegmentBRet=NULL);

/* Intersection between range from lat/long and bearing from lat/long. */
extern SPxErrorCode SPxMathsCalcRangeBearingIntersect(double latDegs1, double longDegs1,
						      double rangeMetres1, double bearingDegs1,
						      double latDegs2, double longDegs2,
						      double bearingDegs2,
						      double *latDegsRtn, double *longDegsRtn);

/* Convert between range/azi and XY. */
extern double SPxMathsRAtoX(double range, double aziDegs);
extern double SPxMathsRAtoY(double range, double aziDegs, int northPositive=TRUE);
extern double SPxMathsXYtoRange(double x, double y);
extern double SPxMathsXYtoAziDegs(double x, double y, int northPositive=TRUE);

/* Convert world relative bearing/elevation to platform relative. */
extern SPxErrorCode SPxMathsWorldToPlatform(double worldBearingDegs,
                                            double worldElevationDegs,
                                            double yawDegs,
                                            double pitchDegs,
                                            double rollDegs,
                                            double *platformBearingDegs,
                                            double *platformElevationDegs);

/* Convert platform relative bearing/elevation to world relative. */
extern SPxErrorCode SPxMathsPlatformToWorld(double platformBearingDegs,
                                            double platformElevationDegs,
                                            double yawDegs,
                                            double pitchDegs,
                                            double rollDegs,
                                            double *worldBearingDegs,
                                            double *worldElevationDegs);

/* Slant range conversion. */
extern double SPxMathsSlantToGroundRange(double radarHeightMetres,
					 double pointHeightMetres,
					 double slantRangeMetres,
					 int useEarthCurvature=FALSE);
extern double SPxMathsGroundToSlantRange(double radarHeightMetres,
					 double pointHeightMetres,
					 double groundRangeMetres,
					 int useEarthCurvature=FALSE);

/* Earth curvature. */
extern double SPxMathsEarthCurvature(double rangeMetres);

#endif /* _SPX_MATHS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
