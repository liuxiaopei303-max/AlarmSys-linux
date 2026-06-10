/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimTargetRendererGL.h,v $
 * ID: $Id: SPxVidSimTargetRendererGL.h,v 1.2 2016/10/31 15:37:51 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimTargetRendererGL class header.
 *
 * Revision Control:
 *   31/10/16 v1.2   SP 	Move headers to source files.
 *
 * Previous Changes:
 *   19/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_TARGET_RENDERER_GL_H
#define _SPX_VID_SIM_TARGET_RENDERER_GL_H

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

/* Forward declare any classes required. */
class SPxVidSimDispGL;
class SPxVidSimScene;
class SPxRadarTrack;
class SPxVidSimModelDatabase;
class SPxVidSimModel;
class SPxTrackDatabase;

class SPxVidSimTargetRendererGL : public SPxVidSimObj
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
    SPxVidSimTargetRendererGL(SPxVidSimDispGL *disp);
    virtual ~SPxVidSimTargetRendererGL(void);

    /* Configuration. */
    virtual SPxErrorCode RenderGL(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Set link to parent object. */

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    SPxVidSimDispGL *m_disp;            /* Display to render to. */
    SPxVidSimScene *m_scene;            /* Scene for this display. */
    SPxVidSimModelDatabase *m_modelDB;  /* Model database. */
    SPxTrackDatabase *m_trackDB;        /* Track database. */    

    /*
     * Private static variables.
     */

    static int staticRenderTrackGL(void *database, void *arg, void *track);

    /*
     * Private functions.
     */

    SPxErrorCode RenderTrackGL(SPxRadarTrack *track);
    SPxVidSimModel *SelectModel(SPxRadarTrack *track);

}; /* SPxVidSimTargetRendererGL */

#endif /* _SPX_VID_SIM_TARGET_RENDERER_GL_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
