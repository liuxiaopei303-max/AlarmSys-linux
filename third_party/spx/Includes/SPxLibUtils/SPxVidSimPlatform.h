/*********************************************************************
 *
 * (c) Copyright 2013, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimPlatform.h,v $
 * ID: $Id: SPxVidSimPlatform.h,v 1.5 2016/10/31 15:37:51 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimPlatform class header.
 *
 * Revision Control:
 *   31/10/16 v1.5   SP 	Support radars.
 *
 * Previous Changes:
 *   07/10/16 1.4   SP 	Derive from SPxVidSimMovable.
 *   20/06/14 1.3   AGC	Move C++ headers to source file.
 *   04/10/13 1.2   AGC	Simplify headers.
 *   15/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_PLATFORM_H
#define _SPX_VID_SIM_PLATFORM_H

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxVidSimMovable.h"

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
class SPxVidSimScene;
class SPxVidSimCameraMount;
class SPxVidSimRadar;

class SPxVidSimPlatform : public SPxVidSimMovable
{
    /* Grant private access to SPxVidSimScene. */
    friend class SPxVidSimScene;

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimPlatform(SPxVidSimScene *scene);
    virtual ~SPxVidSimPlatform(void);

    /* Parent object. */
    virtual SPxVidSimScene *GetScene(void) { return m_scene; }

    /* Add/retrieve a camera mount. */
    virtual SPxErrorCode AddCameraMount(SPxVidSimCameraMount *mount);
    virtual unsigned int GetNumCameraMounts(void);
    virtual SPxVidSimCameraMount *GetCameraMount(unsigned int index);

    /* Add/retrieve a radar. */
    virtual SPxErrorCode AddRadar(SPxVidSimRadar *radar);
    virtual unsigned int GetNumRadars(void);
    virtual SPxVidSimRadar *GetRadar(unsigned int index);

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
    virtual SPxErrorCode SetSceneLink(SPxVidSimScene *scene);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Parent scene. */
    SPxVidSimScene *m_scene;

    /*
     * Private functions.
     */

}; /* SPxVidSimPlatform */

#endif /* _SPX_VID_SIM_PLATFORM_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
