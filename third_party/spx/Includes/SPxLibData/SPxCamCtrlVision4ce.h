/*********************************************************************
*
* (c) Copyright 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamCtrlVision4ce.h,v $
* ID: $Id: SPxCamCtrlVision4ce.h,v 1.8 2017/09/26 15:43:45 rew Exp $
*
* Purpose:
*	Header for SPxCamCtrlVision4ce class - a class
*	to allow controlling cameras via Vision4ce video tracker.
*
* Revision Control:
*   26/09/17 v1.8    AGC	Separate primary/secondary zoom range.
*
* Previous Changes:
*   31/08/17 1.7    AGC	Fix line endings.
*   29/08/17 1.6    AGC	Support Set/GetZoomRange().
*   31/07/17 1.5    AGC	Support SPxCamera setting max pan/tilt speeds.
*   28/07/17 1.4    AGC	Support presets.
*			Support absolute focus.
*   07/07/17 1.3    AGC	Implement functions to retrieve net address.
*   26/06/17 1.2    AGC	Avoid unused parameter warning.
*   22/06/17 1.1    AGC	Initial Version.
**********************************************************************/
#ifndef SPX_CAM_CTRL_VISION4CE_H_
#define SPX_CAM_CTRL_VISION4CE_H_

/* For base class. */
#include "SPxLibData/SPxCamCtrlBase.h"

/* Forward declarations. */
class SPxThread;
class SPxVision4ce;

class SPxCamCtrlVision4ce : public SPxCamCtrlBase
{
public:
    /* Construction and destruction. */
    explicit SPxCamCtrlVision4ce(SPxVision4ce *vision4ce);
    virtual ~SPxCamCtrlVision4ce(void);

    /* Control. */
    virtual int IsConnected(void) const;

    /* Connection set through SPxVision4ce object. */
    virtual int IsSerialSupported(void) const { return FALSE; }
    virtual int IsNetworkSupported(void) const { return FALSE; }

    /* Base class functions. */
    virtual UINT32 GetAddress(void) const;
    virtual const char *GetAddressString(void) const;
    virtual int GetPort(void) const;
    virtual UINT32 GetIfAddress(void) const;
    virtual const char *GetIfAddressString(void) const;

    /* General */
    virtual SPxErrorCode Stop(int isPrimary);

    /* Absolute positioning. */
    virtual int IsAbsCmdInProgress(UINT32 dirMask, int isPrimary) const;

    virtual int IsAbsPanSupported(void) const;
    virtual SPxErrorCode SetPan(float panAngleDegs);
    virtual SPxErrorCode GetPan(float *panAngleDegs) const;
    virtual SPxErrorCode GetLastPan(float *panAngleDegs) const;

    virtual int IsAbsTiltSupported(void) const;
    virtual SPxErrorCode SetTilt(float tiltAngleDegs);
    virtual SPxErrorCode GetTilt(float *tiltAngleDegs) const;
    virtual SPxErrorCode GetLastTilt(float *tiltAngleDegs) const;

    virtual int IsAbsZoomSupported(int isPrimary) const;
    virtual SPxErrorCode SetZoom(float fovDegs, int isPrimary);
    virtual SPxErrorCode GetZoom(float *fovDegs, int isPrimary) const;
    virtual SPxErrorCode GetLastZoom(float *fovDegs, int isPrimary) const;
    virtual SPxErrorCode SetZoomRange(float minFovDegs, float maxFovDegs, int isPrimary);
    virtual SPxErrorCode GetZoomRange(float *minFovDegs, float *maxFovDegs, int isPrimary) const;

    virtual int IsAbsFocusSupported(int isPrimary) const;
    virtual SPxErrorCode SetFocus(float rangeMetres, int isPrimary);
    virtual SPxErrorCode GetFocus(float *rangeMetres, int isPrimary) const;
    virtual SPxErrorCode GetLastFocus(float *rangeMetres, int isPrimary) const;
    virtual SPxErrorCode SetFocusLUTFilename(const char *filename);
    virtual SPxErrorCode GetFocusLUTFilename(char *buffer, int bufLen) const;

    /* Relative positioning. */
    virtual int IsRelPanSupported(void) const;
    virtual SPxErrorCode Pan(float panSpeed, float *panSpeedSet=NULL,
			     int continueZoomFocus=FALSE);

    virtual int IsRelTiltSupported(void) const;
    virtual SPxErrorCode Tilt(float tiltSpeed, float *tiltSpeedSet=NULL,
			      int continueZoomFocus=FALSE);

    virtual int IsRelPanTiltSupported(void) const;
    virtual SPxErrorCode PanTilt(float panSpeed, float tiltSpeed,
				 float *panSpeedSet=NULL, float *tiltSpeedSet=NULL,
				 int continueZoomFocus=FALSE);

    virtual int IsRelZoomSupported(int isPrimary) const;
    virtual SPxErrorCode Zoom(float zoomSpeed, int continuePanTilt, int isPrimary);

    virtual int IsRelFocusSupported(int isPrimary) const;
    virtual SPxErrorCode Focus(float focusSpeed, int continuePanTilt, int isPrimary);

    virtual int IsAutoFocusSupported(int isPrimary) const;
    virtual SPxErrorCode AutoFocus(int autoFocus, int isPrimary);
    virtual int IsAutoFocusEnabled(int isPrimary) const;

    virtual SPxErrorCode GoToPreset(UINT8 id);

    virtual SPxErrorCode SetMaxPanSpeedDegsPerSec(double maxPanSpeedDegsPerSec);
    virtual SPxErrorCode SetMaxTiltSpeedDegsPerSec(double maxTiltSpeedDegsPerSec);

    virtual UINT32 GetMinCmdIntervalMsecs(void) const { return 20; }

protected:
    virtual void Shutdown(void);

private:  
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Private functions. */
    void checkAbsCmdInProgress(void) const;
    int isDirConnected(void) const;
    int isCamConnected(UINT8 index) const;
    
}; /* SPxCamCtrlVision4ce */

#endif /* SPX_CAM_CTRL_VISION4CE_H_ */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
