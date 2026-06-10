/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimMovable.h,v $
 * ID: $Id: SPxVidSimMovable.h,v 1.2 2016/10/07 11:59:37 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimMoveable class header.
 *
 * Revision Control:
 *   07/10/16 v1.2   SP 	Support lat/long position.
 *
 * Previous Changes:
 *   15/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_MOVABLE_H
#define _SPX_VID_SIM_MOVABLE_H

/* Other headers required. */
#include "SPxLibUtils/SPxVidSimObj.h"

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

class SPxVidSimMovable : public SPxVidSimObj
{
public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimMovable(SPxVidSimMovable *parent);
    virtual ~SPxVidSimMovable(void);

    /* Access. */
    virtual SPxVidSimMovable *GetParent(void) { return m_parent; }

    /* Real world position. */
    virtual SPxErrorCode SetLatLong(double latDegs, double lonDegs);
    virtual double GetLatDegs(void);
    virtual double GetLongDegs(void);

    /* Positional adjustment functions. */
    virtual SPxErrorCode SetXMetres(double metres);
    virtual double GetXMetres(int absoluteValue=TRUE);
    virtual SPxErrorCode SetYMetres(double metres);
    virtual double GetYMetres(int absoluteValue=TRUE);
    virtual SPxErrorCode SetZMetres(double metres);
    virtual double GetZMetres(int absoluteValue=TRUE);

    /* Rotational adjustment functions. */
    virtual SPxErrorCode SetPitchDegs(double degs);
    virtual double GetPitchDegs(int absoluteValue=TRUE);
    virtual SPxErrorCode SetRollDegs(double degs);
    virtual double GetRollDegs(int absoluteValue=TRUE);
    virtual SPxErrorCode SetYawDegs(double degs);
    virtual double GetYawDegs(int absoluteValue=TRUE);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    /* Parent object. */
    SPxVidSimMovable *m_parent;

    /* Lat/long position. */
    double m_latDegs;
    double m_lonDegs;
    int m_isLatLongInUse;

    /* Positional adjustment. */
    double m_xMetres;
    double m_yMetres;
    double m_zMetres;

    /* Rotational adjustment. */
    double m_pitchDegs;
    double m_rollDegs;
    double m_yawDegs;

    /*
     * Private functions.
     */

    SPxErrorCode setXMetresNoSync(double metres);
    SPxErrorCode setYMetresNoSync(double metres);
    SPxErrorCode setLatLongNoSync(double latDegs, double lonDegs);
    SPxErrorCode syncXYFromLatLong(void);
    SPxErrorCode syncLatLongFromXY(void);

    /*
     * Private static functions.
     */

    static int eventHandler(void *invokingObj,
                            void *userArg,
                            void *eventInfoPtr);

}; /* SPxVidSimMovable */

#endif /* _SPX_VID_SIM_MOVEABLE_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
