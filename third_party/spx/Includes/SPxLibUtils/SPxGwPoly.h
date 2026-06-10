/*********************************************************************
 *
 * (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxGwPoly.h,v $
 * ID: $Id: SPxGwPoly.h,v 1.4 2014/07/18 13:51:44 rew Exp $
 *
 * Purpose:
 *   SPx graphics widget polyline class.
 *
 * Revision Control:
 *  18/07/14 v1.4    AGC	Support AppendPoint() function.
 *
 * Previous Changes:
 *  07/10/13 1.3    AGC	Move to SPxLibUtils.
 *			Make cross-platform.
 *			Add SPxAreaObj support.
 *  25/01/13 1.2    SP 	Allow group to be set in constructor.
 *  11/01/13 1.1    SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_GW_POLY_H
#define _SPX_GW_POLY_H

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxGwShape.h"

/*********************************************************************
 *
 *  Constants
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Macros
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Type definitions
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Class definitions
 *
 *********************************************************************/

/* Forward declarations. */
class SPxExtRenderer;
class SPxGwWidgetGroup;
struct SPxGwPolyPriv;

class SPxGwPoly : public SPxGwShape
{
public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxGwPoly(SPxGwWidgetGroup *group=NULL);
    virtual ~SPxGwPoly(void);

    /* Configuration. */
    SPxErrorCode SetPoints(const SPxGwPoint_t points[], 
                           unsigned int numPoints);

    SPxErrorCode GetPoints(unsigned int maxNumPoints,
                           SPxGwPoint_t pointsRtn[], 
                           unsigned int *numPointsRtn) const;

    virtual SPxErrorCode SetAutoCloseEnabled(int isEnabled);
    int IsAutoCloseEnabled(void) const { return m_isAutoCloseEnabled; }

    /* Old style pixels interface. */
    SPxErrorCode SetPointsPixels(const SPxGwPoint_t points[], 
                                 unsigned int numPoints);

    SPxErrorCode GetPointsPixels(unsigned int maxNumPoints,
                                 SPxGwPoint_t pointsRtn[], 
                                 unsigned int *numPointsRtn) const;

    /* SPxAreaObj interface. */
    virtual int IsMovable(void) const { return TRUE; }
    virtual SPxErrorCode SetPos(double x, double y);
    virtual SPxErrorCode GetPos(double *x, double *y) const;
    virtual int IsPointWithin(double x, double y, double margin) const;

    virtual unsigned int GetNumPoints(void) const;
    virtual SPxErrorCode SetPointPos(unsigned int idx, double x, double y, int fixedRatio=FALSE);
    virtual SPxErrorCode GetPointPos(unsigned int idx, double *x, double *y) const;

    virtual int IsEditable(void) const { return TRUE; }
    virtual SPxErrorCode AddPoint(double x, double y);
    virtual SPxErrorCode RemovePoint(unsigned int idx);
    virtual SPxErrorCode AppendPoint(double x, double y);

    virtual int IsAutoCloseSupported(void) const { return TRUE; }

    virtual SPxErrorCode LoadCfg(FILE *f);
    virtual SPxErrorCode SaveCfg(FILE *f) const;

private:

    /*
     * Private variables.
     */
    SPxAutoPtr<SPxGwPolyPriv> m_p;

    /* Auto close the poly? */
    int m_isAutoCloseEnabled;

    /*
     * Private functions.
     */
    virtual SPxErrorCode renderAll(SPxExtRenderer *renderer);

}; /* SPxGwPoly */

#endif /* _SPX_GW_POLY_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

