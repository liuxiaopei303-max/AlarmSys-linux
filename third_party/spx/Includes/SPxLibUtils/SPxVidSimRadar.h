/*********************************************************************
 *
 * (c) Copyright 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimRadar.h,v $
 * ID: $Id: SPxVidSimRadar.h,v 1.1 2016/10/31 15:37:51 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimRadar class header.
 *
 * Revision Control:
 *   31/10/16 v1.1   SP 	Initial version.
 *
 * Previous Changes:
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_RADAR_H
#define _SPX_VID_SIM_RADAR_H

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxVidSimMovable.h"
#include "SPxLibUtils/SPxTime.h"

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

/* Forward declare any classes required. */
class SPxVidSimPlatform;
class SPxRIB;
class SPxPIM;
class SPxNetworkReceive;
class SPxRunProcess;

class SPxVidSimRadar : public SPxVidSimMovable
{
    /* Grant SPxVidSimPlatform private access. */
    friend class SPxVidSimPlatform;

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimRadar(SPxVidSimPlatform *platform);
    virtual ~SPxVidSimRadar(void);
    virtual SPxErrorCode Initialise(void);

    /* Parent object. */
    virtual SPxVidSimPlatform *GetPlatform(void) { return m_platform; }

    /* Object access. */
    virtual SPxPIM *GetPIM(void) { return m_pim; }
    virtual SPxRunProcess *GetPIMPro(void) { return m_pimPro; }

    /* Configuration save and restore. */
    virtual SPxErrorCode SetStateFromConfig(void);
    virtual SPxErrorCode SetConfigFromState(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Set link to parent object. */
    virtual SPxErrorCode SetPlatformLink(SPxVidSimPlatform *platform);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Parent platform. */
    SPxVidSimPlatform *m_platform;

    /* Radar video. */
    SPxRIB *m_rib;                /* RIB. */
    SPxPIM *m_pim;                /* PIM. */
    SPxRunProcess *m_pimPro;      /* Process connected to PIM. */
    SPxNetworkReceive *m_netSrc;  /* Network source. */
   
    /*
     * Private functions.
     */


    /*
     * Private static functions.
     */

}; /* SPxVidSimRadar */

#endif /* _SPX_VID_SIM_RADAR_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
