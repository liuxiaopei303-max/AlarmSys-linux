/*********************************************************************
 *
 * (c) Copyright 2013, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxGwArc.h,v $
 * ID: $Id: SPxGwArc.h,v 1.4 2013/11/27 15:28:44 rew Exp $
 *
 * Purpose:
 *   SPx graphics widget arc class.
 *
 * Revision Control:
 *  27/11/13 v1.4    AGC	Fix lat/long arc point adjustment.
 *
 * Previous Changes:
 *  07/10/13 1.3    AGC	Move to SPxLibUtils.
 *			Make cross-platform.
 *			Add SPxAreaObj support.
 *  25/01/13 1.2    SP 	Allow group to be set in constructor.
 *  11/01/13 1.1    SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_GW_ARC_H
#define _SPX_GW_ARC_H

/* Other headers required. */
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

class SPxGwArc : public SPxGwShape
{
public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxGwArc(SPxGwWidgetGroup *group=NULL);
    virtual ~SPxGwArc(void);

    /* Configuration. */
    SPxErrorCode SetCentre(double x, double y);
    double GetCentreX(void) const { return m_centreX; }
    double GetCentreY(void) const { return m_centreY; }

    SPxErrorCode SetSize(double width, double height=0.0);
    double GetWidth(void) const { return m_width; }
    double GetHeight(void) const { return m_height; }

    SPxErrorCode SetStartDegs(double degs);
    double GetStartDegs(void) const { return m_startDegs; }

    SPxErrorCode SetEndDegs(double degs);
    double GetEndDegs(void) const { return m_endDegs; }

    virtual SPxErrorCode SetAutoCloseEnabled(int isEnabled);
    int IsAutoCloseEnabled(void) const { return m_isChordEnabled; }

    /* Old style pixels interface. */
    SPxErrorCode SetCentrePixels(double x, double y);
    double GetCentreXPixels(void) const { return m_centreX; }
    double GetCentreYPixels(void) const { return m_centreY; }

    SPxErrorCode SetSizePixels(double width, double height=0.0);
    double GetWidthPixels(void) const { return m_width; }
    double GetHeightPixels(void) const { return m_height; }

    virtual SPxErrorCode SetChordEnabled(int isEnabled) { return SetAutoCloseEnabled(isEnabled); }
    int IsChordEnabled(void) const { return IsAutoCloseEnabled(); }

    /* SPxAreaObj interface. */
    virtual int IsMovable(void) const { return TRUE; }
    virtual SPxErrorCode SetPos(double x, double y);
    virtual SPxErrorCode GetPos(double *x, double *y) const;
    virtual int IsPointWithin(double x, double y, double margin) const;

    virtual unsigned int GetNumPoints(void) const { return 2; }
    virtual SPxErrorCode SetPointPos(unsigned int idx, double x, double y, int fixedRatio=FALSE);
    virtual SPxErrorCode GetPointPos(unsigned int idx, double *x, double *y) const;

    virtual int IsAutoCloseSupported(void) const { return TRUE; }

    virtual SPxErrorCode LoadCfg(FILE *f);
    virtual SPxErrorCode SaveCfg(FILE *f) const;

private:

    /*
     * Private variables.
     */
    double m_centreX;
    double m_centreY;
    double m_width;
    double m_height;
    double m_startDegs;
    double m_endDegs;
    double m_yOverX; /* Scaling between y and x units. */

    /* Show chord line? */
    int m_isChordEnabled;

    /*
     * Private functions.
     */
    double radius(double angleRads) const;
    virtual SPxErrorCode renderAll(SPxExtRenderer *renderer);

}; /* SPxGwArc */

#endif /* _SPX_GW_ARC_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

