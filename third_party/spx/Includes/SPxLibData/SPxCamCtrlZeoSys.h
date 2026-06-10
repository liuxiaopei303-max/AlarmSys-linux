/*********************************************************************
*
* (c) Copyright 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamCtrlZeoSys.h,v $
* ID: $Id: SPxCamCtrlZeoSys.h,v 1.8 2017/06/20 15:30:57 rew Exp $
*
* Purpose:
*	Header for SPxCamCtrlZeoSys class - a class
*	to allow controlling ZeoSys cameras.
*
* Revision Control:
*   20/06/17 v1.8    AGC	Tweak default max speeds.
*
* Previous Changes:
*   07/06/17 1.7    AGC	Support automatic NUC.
*			Support measured range timeout.
*   06/06/17 1.6    AGC	Support laser range finder.
*   26/05/17 1.5    AGC	Support primary/secondary zoom/focus.
*   16/05/17 1.4    AGC	Change line endings.
*   28/04/17 1.3    AGC	Add preset support.
*   24/04/17 1.2    AGC	Support daylight/thermal cameras using camera address.
*   20/04/17 1.1    AGC	Initial Version.
**********************************************************************/
#ifndef SPX_CAM_CTRL_ZEO_SYS_H_
#define SPX_CAM_CTRL_ZEO_SYS_H_

/* For base class. */
#include "SPxLibData/SPxCamCtrlBase.h"

/* Forward declarations. */
class SPxThread;

class SPxCamCtrlZeoSys : public SPxCamCtrlBase
{
public:
    /* Types. */
    enum Presets
    {
	PRESET_POWER_CAM = 1,
	PRESET_POWER_LRF = 2,
	PRESET_OP_STATUS_PTZ = 3,
	PRESET_OP_STATUS_CAM = 4,
	PRESET_OP_STATUS_LRF = 5,
	PRESET_AUTO_FOCUS = 6,
	PRESET_LENS_CAP = 7,
	PRESET_BW_HOT = 8,
	PRESET_FREEZE = 9,
	PRESET_NUC = 10,
	PRESET_NUC_QUICK = 11,
	PRESET_LRF_MEASURE = 12
    };

    /* Construction and destruction. */
    SPxCamCtrlZeoSys(void);
    virtual ~SPxCamCtrlZeoSys(void);
    virtual int IsSupported(void) const;

    /* Control. */
    virtual int IsSerialSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetAddress(const char *address, int port = 0, const char *ifAddr=NULL);
    virtual SPxErrorCode SetAddress(UINT32 address, int port = 0, const char *ifAddr=NULL);
    
    /* Base class functions. */
    
    /* General */
    virtual int IsConnected(void) const;
    virtual SPxErrorCode Stop(int isPrimary);
    virtual SPxErrorCode GoToPreset(UINT8 id, int isPrimary);

    /* Self reference. */
    virtual SPxErrorCode SelfReference(void);

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

    virtual int IsAbsFocusSupported(int isPrimary) const;
    virtual SPxErrorCode SetFocus(float rangeMetres, int isPrimary);
    virtual SPxErrorCode GetFocus(float *rangeMetres, int isPrimary) const;
    virtual SPxErrorCode GetLastFocus(float *rangeMetres, int isPrimary) const;

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
    
    /* Range measurement. */
    virtual int IsMeasuredRangeSupported(void) const;
    virtual SPxErrorCode GetMeasuredRange(double *rangeMetres) const;

    /* Timeouts/intervals. */
    SPxErrorCode SetNUCIntervalMsecs(unsigned int msecs);
    unsigned int GetNUCIntervalMsecs(void) const;
    SPxErrorCode SetMeasuredRangeTimeoutMsecs(unsigned int msecs);
    unsigned int GetMeasuredRangeTimeoutMsecs(void) const;

    /* Generic get/set parameter interface. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    virtual void Shutdown(void);

private:  
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    static void *updateWrapper(SPxThread *thread);
    void *update(SPxThread *thread);
    void stopStack(void);
    void startPanTilt(void);
    void stopPanTilt(void);
    void startCam(int isPrimary);
    void stopCam(int isPrimary);
    void startLRF(void);
    void stopLRF(void);
    void checkAbsCmdStatus(void);
    void checkNuc(void);

    template<typename T, typename FS, typename FC, typename FCA,
	typename StatusT, typename Control>
    SPxErrorCode toggle(const char *desc, T& dev,
			FS& statusFn, FC& controlFn, FCA& controlAsyncFn,
			StatusT statusOn, StatusT statusOff,
			Control controlOn, Control controlOff);

}; /* SPxCamCtrlZeoSys */

#endif /* SPX_CAM_CTRL_ZEO_SYS_H_ */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
