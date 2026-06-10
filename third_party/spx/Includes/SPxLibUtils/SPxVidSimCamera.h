/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimCamera.h,v $
 * ID: $Id: SPxVidSimCamera.h,v 1.7 2016/10/07 11:59:37 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimCamera class header.
 *
 * Revision Control:
 *   07/10/16 v1.7   SP 	Move events to base class.
 *
 * Previous Changes:
 *   15/10/15 1.6   SP  Remove unused functions and types.
 *   14/10/15 1.5   SP  Support colour mode and brightness.  
 *   04/09/15 1.4   SP 	Support FOV LUT.
 *   26/08/15 1.3   SP 	Improve FOV speed control.
 *   06/08/15 1.2   SP 	Support remote control.
 *   15/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_CAMERA_H
#define _SPX_VID_SIM_CAMERA_H

/* Other headers required. */
#include "SPxLibUtils/SPxVidSimObj.h"
#include "SPxLibData/SPxCamCtrlPelco.h"

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
class SPxVidSimCameraMount;
class SPxThread;

class SPxVidSimCamera : public SPxVidSimObj
{
    /* Grant SPxVidSimCameraMount private access. */
    friend class SPxVidSimCameraMount;

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
    SPxVidSimCamera(SPxVidSimCameraMount *mount);
    virtual ~SPxVidSimCamera(void);
    virtual SPxErrorCode Initialise(void);

    /* Parent object. */
    virtual SPxVidSimCameraMount *GetCameraMount(void) 
    { 
        return m_cameraMount; 
    }

    /* Camera controller. */
    virtual SPxCamCtrlPelco *GetCamCtrl(void) { return m_camCtrl; }

    /* Configuration. */
    virtual SPxErrorCode SetFrameSizePixels(unsigned int width, 
                                            unsigned int height);
    virtual unsigned int GetFrameWidthPixels(void);
    virtual unsigned int GetFrameHeightPixels(void);
    virtual SPxErrorCode SetFramesPerSec(unsigned int fps);
    virtual unsigned int GetFramesPerSec(void) { return m_fps; }
    virtual SPxErrorCode SetFieldOfViewDegs(double degs);
    virtual double GetFieldOfViewDegs(void) { return m_fieldOfViewDegs; }
    virtual double GetMinFieldOfViewDegs(void);
    virtual double GetMaxFieldOfViewDegs(void);
    virtual SPxErrorCode SetZoomSpeed(double speed);
    virtual SPxErrorCode SetColourMode(ColourMode_t mode);
    virtual ColourMode_t GetColourMode(void);
    virtual SPxErrorCode SetBackgroundBrightness(double brightness);
    virtual double GetBackgroundBrightness(void);
    virtual SPxErrorCode SetTargetBrightness(double brightness);
    virtual double GetTargetBrightness(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Set link to parent object. */
    virtual SPxErrorCode SetCameraMountLink(SPxVidSimCameraMount *mount);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    /* Parent camera mount. */
    SPxVidSimCameraMount *m_cameraMount;

    /* Camera settings. */
    unsigned int m_frameWidthPixels;    /* Frame width in pixels. */
    unsigned int m_frameHeightPixels;   /* Frame height in pixels. */
    unsigned int m_fps;                 /* Frames per second. */
    double m_fieldOfViewDegs;           /* Actual field of view in degrees. */
    double m_reqFieldOfViewDegs;        /* Requested field of view in degrees. */
    double m_relZoomSpeed;              /* Relative zoom speed. */
    double m_maxFovDegsPerSec;          /* Maximum zoom speed. */
    UINT32 m_lastFovUpdateMsecs;        /* Last update time. */
    SPxCamCtrlPelco *m_camCtrl;         /* Controller used for FOV lookup. */
    ColourMode_t m_colourMode;          /* Colour, greyscale, etc. */
    double m_backgroundBrightness;      /* Background brightness. */
    double m_targetBrightness;          /* Target brightness. */

    /* Thread used to update camera zoom. */
    SPxThread *m_thread;

    /* Requested zoom position. */


    /*
     * Private static variables.
     */

    static unsigned int m_nextCameraNum; /* Used to name cameras. */

    /*
     * Private functions.
     */

    void updateCamera(void);

    /*
     * Private static functions.
     */

    static void *threadHandler(SPxThread *thread);

}; /* SPxVidSimCamera */

#endif /* _SPX_VID_SIM_CAMERA_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
