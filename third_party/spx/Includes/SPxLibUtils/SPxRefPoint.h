/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxRefPoint.h,v $
 * ID: $Id: SPxRefPoint.h,v 1.1 2017/08/22 14:24:01 rew Exp $
 *
 * Purpose:
 *   Header for SPxRefPoint class.
 *
 * Revision Control:
 *   22/08/17 v1.1   SP 	Initial version.
 *
 * Previous Changes:
 *
 *********************************************************************/

#ifndef _SPX_REF_POINT_H
#define _SPX_REF_POINT_H

/*
 * Other headers required.
 */

#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/*********************************************************************
 *
 *   Constants
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

/* Forward declare other classes we use. */
struct SPxRefPointPriv;

/*
 * Define our class.
 */
class SPxRefPoint : public SPxObj
{
public:
    /*
     * Public types.
     */

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxRefPoint(void);
    virtual ~SPxRefPoint(void);

    /* General. */
    virtual UINT32 GetID(void) const;
    virtual SPxErrorCode SetDisplayName(const char *name);
    virtual const char *GetDisplayName(void) const;
    virtual SPxErrorCode SetLatLong(double latDegs, 
                                    double longDegs);
    virtual SPxErrorCode GetLatLong(double *latDegsRtn, 
                                    double *longDegsRtn) const;
    virtual SPxErrorCode GetDistanceFromPoint(double latDegs, 
                                              double longDegs,
                                              double *distMetresRtn) const;

    /* User data. */
    virtual void SetUserData(void *ptr) { m_userData = ptr; }
    virtual void *GetUserData(void) const { return(m_userData); }

    /*
     * Public static functions.
     */

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

private:

    /*
     * Private variables.
     */
    SPxAutoPtr<SPxRefPointPriv> m_p;
    mutable SPxCriticalSection m_mutex;      /* Object mutex. */
    UINT32 m_id;                             /* Unique ID. */
    void *m_userData;                        /* User data. */
    double m_latDegs;                        /* Latitude in degrees. */
    double m_longDegs;                       /* Longitude in degrees. */

    /*
     * Private functions.
     */

    /* General. */

    /*
     * Private static functions.
     */

}; /* SPxRefPoint */

#endif /* SPX_REF_POINT_H */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
