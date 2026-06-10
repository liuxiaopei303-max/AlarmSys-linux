/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamCtrlPelco.h,v $
* ID: $Id: SPxCamCtrlPelco.h,v 1.27 2017/04/24 14:16:29 rew Exp $
*
* Purpose:
*	Header for SPxCamCtrlPelco class - a class
*	to allow controlling a camera with a Pelco-D serial interface.
*
* Revision Control:
*   24/04/17 v1.27   AGC	Virtualise camera address and control type functions.
*
* Previous Changes:
*   06/02/17 1.26   AGC	Fix GetLastTilt().
*   02/02/17 1.25   AGC	Add options for concurrent PTZ.
*   23/01/17 1.24   AGC	Modifications for testing.
*   18/01/17 1.23   AGC	Add priority functions.
*   19/10/16 1.22   AGC	GoToPreset() now virtual.
*   23/09/16 1.21   AGC	Add GetLastPan/Tilt/Zoom() functions.
*   24/03/16 1.20   SP 	Add sleep between zoom and focus commands 
*                       as requested by Silent Sentinel.
*   08/02/16 1.19   AGC	Use speed mode from camera.
*   04/02/16 1.18   AGC	Support for retrieving camera settings.
*   03/11/15 1.17   AGC	Allow retrieval of actual pan/tilt speeds.
*   28/05/15 1.16   AGC	Lock mutex for public GetFOVForZoom() and GetZoomForFOV().
*   26/08/15 1.15   REW	Correct version numbers back to 0.1.
*   26/08/15 1.14   SP 	Make GetFOVForZoom() and GetZoomForFOV() public.
*   26/08/15 1.13   AGC	Add focus LUT support.
*   31/07/15 1.12   AGC	Support absolute focus.
*   25/06/15 1.11   AGC	Add GetZoomLUTFilename() function.
*   23/06/15 1.10   AGC	Add direction specific absolute command in progress checks.
*   12/06/15 1.9    AGC	Stop() function now overrides base class implementation.
*			Add command interval function.
*   22/05/15 1.8    AGC	Change turbo speed value to match recent spec.
*   11/05/15 1.7    AGC	Avoid incorrectly updating absolute support flags.
*   07/11/14 1.6    AGC	Support optional signed tilt.
*   02/06/14 1.5    SP 	Add zoom to FOV LUT.
*   17/02/14 1.4    SP 	Remove zoom calibration.
*   27/01/14 1.3    SP 	Support zoom calibration.
*   04/11/13 1.2    AGC	Cache absolute command support.
*   24/10/13 1.1    AGC	Move to SPx library.
*			Add absolute positioning functions.
*   14/08/13 0.5    AGC	Derive from new SPxAVCamCtrlBase class.
*   17/11/11 0.4    AGC	Rename parameter setting functions.
*   14/11/11 0.3    AGC	Use Up/Down etc for movement functions.
*   22/09/11 0.2    AGC	Allow independent horiz/vert speeds.
*			Add SetSerialName(), SetSerialBaud() functions.
*   13/09/11 0.1    AGC	Initial Version.
**********************************************************************/
#ifndef _SPX_CAM_CTRL_PELCO_H
#define _SPX_CAM_CTRL_PELCO_H

/*
 * Other headers required.
 */

/* For base class. */
#include "SPxLibData/SPxCamCtrlBase.h"

/* Commands that can be sent with SendCommand(). */
static const UINT16 SPX_CAM_PELCO_AUTO_SCAN	= 0x0090;
static const UINT16 SPX_CAM_PELCO_MANUAL_SCAN	= 0x0010;
static const UINT16 SPX_CAM_PELCO_CAMERA_ON	= 0x0088;
static const UINT16 SPX_CAM_PELCO_CAMERA_OFF	= 0x0008;
static const UINT16 SPX_CAM_PELCO_IRIS_CLOSE	= 0x0004;
static const UINT16 SPX_CAM_PELCO_IRIS_OPEN	= 0x0002;
static const UINT16 SPX_CAM_PELCO_FOCUS_NEAR	= 0x0001;
static const UINT16 SPX_CAM_PELCO_FOCUS_FAR	= 0x8000;
static const UINT16 SPX_CAM_PELCO_ZOOM_WIDE	= 0x4000;
static const UINT16 SPX_CAM_PELCO_ZOOM_TELE	= 0x2000;
static const UINT16 SPX_CAM_PELCO_MOVE_DOWN	= 0x1000;
static const UINT16 SPX_CAM_PELCO_MOVE_UP	= 0x0800;
static const UINT16 SPX_CAM_PELCO_MOVE_LEFT	= 0x0400;
static const UINT16 SPX_CAM_PELCO_MOVE_RIGHT	= 0x0200;
static const UINT16 SPX_CAM_PELCO_STOP		= 0x0000;

/* Commands that can be sent with SendRawCommand(). */
static const UINT32 SPX_CAM_PELCO_SET_PRESET		= 0x00000300;
static const UINT32 SPX_CAM_PELCO_CLEAR_PRESET		= 0x00000500;
static const UINT32 SPX_CAM_PELCO_GO_TO_PRESET		= 0x00000700;
static const UINT32 SPX_CAM_PELCO_RESET_PAN		= 0x22000700;
static const UINT32 SPX_CAM_PELCO_SET_ZOOM_SPEED	= 0x00002500;
static const UINT32 SPX_CAM_PELCO_SET_FOCUS_SPEED	= 0x00002700;

/* Camera manager part number. */
static const char SPX_CAMERA_MNGR_PART_NUM[] = "SPX-CAMERA-MNGR";

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Pelco camera control type now defined in base class.
 * This typedef if for backwards compat with old name.
 */
typedef SPxCameraControlType SPxCamCtrlPelcoType;

static const UINT8 SPX_CAM_PELCO_SPEED_MIN	= 0x00;
static const UINT8 SPX_CAM_PELCO_SPEED_MAX	= 0x3F;
static const UINT8 SPX_CAM_PELCO_SPEED_TURBO	= 0x40;
static const UINT8 SPX_CAM_PELCO_SPEED_MAX_255	= 0xFF;
static const float SPX_CAM_PELCO_SPEED_MIN_F	= static_cast<float>(SPX_CAM_PELCO_SPEED_MIN);
static const float SPX_CAM_PELCO_SPEED_MAX_F	= static_cast<float>(SPX_CAM_PELCO_SPEED_MAX);
static const float SPX_CAM_PELCO_SPEED_TURBO_F	= static_cast<float>(SPX_CAM_PELCO_SPEED_TURBO);
static const float SPX_CAM_PELCO_SPEED_MAX_255_F= static_cast<float>(SPX_CAM_PELCO_SPEED_MAX_255);

class SPxSerialRecv;
struct SPxPelcoDMesg;
struct SPxPelcoPMesg;
class SPxNetTCP;

/*
 * Define our class.
 */
class SPxCamCtrlPelco : public SPxCamCtrlBase
{
public:
    /* Construction and destruction. */
    SPxCamCtrlPelco(void);
    virtual ~SPxCamCtrlPelco(void);

    /* Control. */
    virtual SPxErrorCode SetMesgType(SPxMesgIOType_t mesgType);
    virtual SPxErrorCode SetSerialName(const char *name);
    virtual SPxErrorCode SetSerialBaud(unsigned int baud);
    virtual SPxErrorCode SetAddress(const char *address, int port = 0, const char *ifAddr=NULL);
    virtual SPxErrorCode SetAddress(UINT32 address, int port = 0, const char *ifAddr=NULL);

    /* Settings. */
    virtual SPxErrorCode SetCameraControlType(SPxCameraControlType type);
    virtual SPxErrorCode GetCameraControlType(SPxCameraControlType *type) const;
    virtual SPxErrorCode SetCameraAddress(UINT8 address);
    virtual SPxErrorCode GetCameraAddress(UINT8 *address) const;

    /* Camera control functions. */
    SPxErrorCode Move(int horizSpeed, int vertSpeed, int zoomSpeed=0);
    SPxErrorCode MoveUp(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE);
    SPxErrorCode MoveDown(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE);
    SPxErrorCode MoveLeft(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE);
    SPxErrorCode MoveRight(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE);
    SPxErrorCode MoveUpLeft(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE) { return MoveUpLeft(speed, speed, continueZoomFocus); }
    SPxErrorCode MoveUpRight(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE) { return MoveUpRight(speed, speed, continueZoomFocus); }
    SPxErrorCode MoveDownLeft(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE) { return MoveDownLeft(speed, speed, continueZoomFocus); }
    SPxErrorCode MoveDownRight(UINT8 speed=SPX_CAM_PELCO_SPEED_MAX, int continueZoomFocus=FALSE) { return MoveDownRight(speed, speed, continueZoomFocus); }
    SPxErrorCode MoveUpLeft(UINT8 hSpeed, UINT8 vSpeed, int continueZoomFocus=FALSE);
    SPxErrorCode MoveUpRight(UINT8 hSpeed, UINT8 vSpeed, int continueZoomFocus=FALSE);
    SPxErrorCode MoveDownLeft(UINT8 hSpeed, UINT8 vSpeed, int continueZoomFocus=FALSE);
    SPxErrorCode MoveDownRight(UINT8 hSpeed, UINT8 vSpeed, int continueZoomFocus=FALSE);
    SPxErrorCode ResetPan(void);
    SPxErrorCode ZoomIn(void);
    SPxErrorCode ZoomIn(UINT8 speed, int continuePanTilt=FALSE);
    SPxErrorCode ZoomOut(void);
    SPxErrorCode ZoomOut(UINT8 speed, int continuePanTilt=FALSE);
    SPxErrorCode FocusNear(void);
    SPxErrorCode FocusNear(UINT8 speed, int continuePanTilt=FALSE);
    SPxErrorCode FocusFar(void);
    SPxErrorCode FocusFar(UINT8 speed, int continuePanTilt=FALSE);
    virtual SPxErrorCode Stop(void);
    SPxErrorCode ManualScan(void);
    SPxErrorCode AutoScan(void);
    SPxErrorCode SetPreset(UINT8 id);
    SPxErrorCode ClearPreset(UINT8 id);
    virtual SPxErrorCode GoToPreset(UINT8 id);

    SPxErrorCode SendCommand(UINT16 command, UINT8 panSpeed=0, UINT8 tiltSpeed=0) const;
    SPxErrorCode SendRawCommand(UINT32 command) const;

    /* Base class functions. */

    /* Self reference. */
    virtual SPxErrorCode SelfReference(void);

    /* Absolute positioning. */
    virtual int IsAbsCmdInProgress(UINT32 dirMask=SPX_CAMERA_DIR_ALL) const;

    virtual int IsAbsPanSupported(void) const;
    virtual SPxErrorCode SetPan(float panAngleDegs);
    virtual SPxErrorCode GetPan(float *panAngleDegs) const;
    virtual SPxErrorCode GetLastPan(float *panAngleDegs) const;
    SPxErrorCode GetPanPos(UINT16 *panPos) const;

    virtual int IsAbsTiltSupported(void) const;
    virtual SPxErrorCode SetTilt(float tiltAngleDegs);
    virtual SPxErrorCode GetTilt(float *tiltAngleDegs) const;
    virtual SPxErrorCode GetLastTilt(float *tiltAngleDegs) const;
    SPxErrorCode GetTiltPos(UINT16 *tiltPos) const;
    SPxErrorCode SetTiltSigned(int tiltSigned);
    int IsTiltSigned(void) const;

    virtual int IsAbsZoomSupported(void) const;
    virtual SPxErrorCode SetZoom(float fovDegs);
    virtual SPxErrorCode GetZoom(float *fovDegs) const;
    virtual SPxErrorCode GetLastZoom(float *fovDegs) const;
    SPxErrorCode GetZoomPos(UINT16 *zoomPos) const;
    virtual SPxErrorCode SetZoomRange(float minFovDegs, float maxFovDegs);
    virtual SPxErrorCode GetZoomRange(float *minFovDegs, float *maxFovDegs) const;
    virtual SPxErrorCode SetMaxZoomPosition(UINT16 pos);
    virtual SPxErrorCode GetMaxZoomPosition(UINT16 *pos) const;
    virtual SPxErrorCode SetQueriedZoomIsPercentage(int state);
    virtual int IsQueriedZoomPercentage(void) const;
    virtual SPxErrorCode SetZoomLUTFilename(const char *filename);
    virtual SPxErrorCode GetZoomLUTFilename(char *buffer, int bufLen) const;

    virtual int IsAbsFocusSupported(void) const;
    virtual SPxErrorCode SetFocus(float rangeMetres);
    virtual SPxErrorCode GetFocus(float *rangeMetres) const;
    virtual SPxErrorCode GetLastFocus(float *rangeMetres) const;
    SPxErrorCode GetFocusPos(UINT16 *focusPos) const;
    virtual SPxErrorCode SetFocusLUTFilename(const char *filename);
    virtual SPxErrorCode GetFocusLUTFilename(char *buffer, int bufLen) const;

    /* Relative positioning. */
    virtual int IsRelPanSupported(void) const { return TRUE; }
    virtual SPxErrorCode Pan(float panSpeed, float *panSpeedSet=NULL,
			     int continueZoomFocus=FALSE);

    virtual int IsRelTiltSupported(void) const { return TRUE; }
    virtual SPxErrorCode Tilt(float tiltSpeed, float *tiltSpeedSet=NULL,
			      int continueZoomFocus=FALSE);

    virtual int IsRelPanTiltSupported(void) const { return TRUE; }
    virtual SPxErrorCode PanTilt(float panSpeed, float tiltSpeed,
				 float *panSpeedSet=NULL, float *tiltSpeedSet=NULL,
				 int continueZoomFocus=FALSE);

    virtual int IsRelZoomSupported(void) const { return TRUE; }
    virtual SPxErrorCode Zoom(float zoomSpeed, int continuePanTilt=FALSE);

    virtual int IsRelFocusSupported(void) const { return TRUE; }
    virtual SPxErrorCode Focus(float focusSpeed, int continuePanTilt=FALSE);

    virtual int IsAutoFocusSupported(void) const { return TRUE; }
    virtual SPxErrorCode AutoFocus(int autoFocus);
    virtual int IsAutoFocusEnabled(void) const;

    /* Query. */
    virtual int AreQueriesSupported(void) const;
    virtual SPxErrorCode GetFirmwareVersion(UINT16 *version) const;
    virtual SPxErrorCode GetCommsMode(SPxCameraCommsMode *mode) const;
    virtual SPxErrorCode GetPanTiltResponseMode(SPxCameraPanTiltResponseMode *mode) const;
    virtual SPxErrorCode GetZoomSpeedMode(SPxCameraZoomSpeedMode *mode) const;
    virtual SPxErrorCode GetMaxPanSpeedDegsPerSec(double *maxPanSpeedDegsPerSec) const;
    virtual SPxErrorCode GetMaxTiltSpeedDegsPerSec(double *maxTiltSpeedDegsPerSec) const;
    
    /* Priority control. */
    virtual SPxErrorCode GetControlStatus(SPxCameraControlStatus *status) const;
    virtual SPxErrorCode GetControllerAddress(UINT32 *address) const;
    virtual SPxErrorCode SetPriority(UINT8 priority);

    /* Minimum command interval. */
    virtual UINT32 GetMinCmdIntervalMsecs(void) const { return 20; }

    /* Conversion functions. */
    float GetFOVForZoom(UINT16 zoomPos) const;
    UINT16 GetZoomForFOV(float fovDegs) const;

    /* Generic get/set parameter interface. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    /* Protected functions that may be called by derived classes for testing. */
    explicit SPxCamCtrlPelco(SPxNetTCP *netTCP);
    virtual void Shutdown(void);

private:  
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;	/* Private structure - contains STL bits. */

    SPxCameraControlType m_camControlType;	/* Camera control type. */
    UINT8 m_cameraAddress;			/* Address of camera (0-255). */
    mutable SPxAtomic<UINT32> m_opCode;		/* Op-code for extended responses. */
    mutable UINT8 m_opCheck;			/* Checksum for sent data. */
    mutable UINT32 m_data;			/* Received data. */
    mutable SPxAtomic<UINT32> m_recvBytes;	/* Bytes received while waiting for response. */
    mutable int m_isFovSet;			/* Has the field of view been set? */
    mutable float m_minFovDegs;			/* Minimum field of view. */
    mutable float m_maxFovDegs;			/* Maximum field of view. */
    UINT16 m_maxZoomPos;                        /* Zoom position at maximum zoom (min FOV). */
    int m_isQueriedZoomPercentage;              /* Queried zoom value is a percenage of max zoom. */
    mutable int m_absPanSupported;
    mutable UINT32 m_absPanCheckTime;
    mutable int m_absTiltSupported;
    mutable UINT32 m_absTiltCheckTime;
    mutable int m_absZoomSupported;
    mutable UINT32 m_absZoomCheckTime;
    mutable int m_absFocusSupported;
    mutable UINT32 m_absFocusCheckTime;
    mutable int m_queriesSupported;
    mutable UINT32 m_queriesCheckTime;
    mutable UINT32 m_settingsCheckTime;
    mutable UINT32 m_maxSpeedsCheckTime;
    mutable UINT32 m_zoomRangeCheckTime;
    mutable UINT32 m_lastRelCommand;
    mutable UINT16 m_panPos;                    /* Last raw pan position. */
    mutable UINT32 m_panPosCheckTime;           /* Last time pan position was updated. */
    mutable UINT8 m_panSpeed;			/* Current pan speed. */
    mutable UINT16 m_tiltPos;                   /* Last raw tilt position. */
    mutable UINT32 m_tiltPosCheckTime;          /* Last time tilt position was updated. */
    mutable UINT8 m_tiltSpeed;			/* Current tilt speed. */
    mutable UINT16 m_zoomPos;                   /* Last raw zoom position. */
    mutable UINT32 m_zoomPosCheckTime;          /* Last time zoom position was updated. */
    UINT32 m_zoomSpeed;				/* Current zoom speed. */
    mutable UINT16 m_focusPos;			/* Last raw focus position. */
    mutable UINT32 m_focusPosCheckTime;		/* Last time focus positions was updated. */
    UINT32 m_focusSpeed;			/* Current focus speed. */
    mutable int m_isAbsPanCmdInProgress;        /* Absolute command in progress? */
    mutable int m_isAbsTiltCmdInProgress;       /* Absolute command in progress? */
    mutable int m_isAbsZoomCmdInProgress;       /* Absolute command in progress? */
    mutable int m_isAbsFocusCmdInProgress;	/* Absolute command in progress? */
    mutable UINT32 m_absPanCmdInProgressCheckTime; /* Last time this was checked. */
    mutable UINT32 m_absTiltCmdInProgressCheckTime; /* Last time this was checked. */
    mutable UINT32 m_absZoomCmdInProgressCheckTime; /* Last time this was checked. */
    mutable UINT32 m_absFocusCmdInProgressCheckTime;/* Last time this was checked. */
    mutable int m_isAbsPanPosSet;                  /* Are the values below set? */
    mutable int m_isAbsTiltPosSet;                 /* Are the values below set? */
    mutable int m_isAbsZoomPosSet;                 /* Are the values below set? */
    mutable int m_isAbsFocusPosSet;		   /* Are the values below set? */
    mutable UINT16 m_absCmdPanPos;              /* Used to check for pan changes. */
    mutable UINT16 m_absCmdTiltPos;             /* Used to check for tilt changes. */
    mutable UINT16 m_absCmdZoomPos;             /* Used to check for zoom changes. */
    mutable UINT16 m_absCmdFocusPos;		/* Used to check for focus changes. */
    mutable int m_isAutoFocusEnabled;           /* Is auto focus enabled? */
    mutable UINT16 m_firmwareVersion;		/* Firmware version */
    mutable SPxCameraCommsMode m_commsMode;
    mutable SPxCameraPanTiltResponseMode m_panTiltResponseMode;
    mutable SPxCameraZoomSpeedMode m_zoomSpeedMode;
    mutable double m_maxPanSpeedDegsPerSec;
    mutable double m_maxTiltSpeedDegsPerSec;
    UINT8 m_priority;

    /* Private functions. */
    void initialise(void);
    float getTiltDegs(void) const;
    SPxErrorCode getPan(float *panAngleDegs, int genError, int forceUpdate=FALSE) const;
    SPxErrorCode getTilt(float *tiltAngleDegs, int genError, int forceUpdate=FALSE) const;
    SPxErrorCode getZoom(float *fovDegs, int genError, int forceUpdate=FALSE) const;
    SPxErrorCode getFocus(float *focus, int genError, int forceUpdate=FALSE) const;
    SPxErrorCode getFirmwareVersion(UINT16 *firmwareVersion, int genError, int forceUpdate=FALSE) const;
    SPxErrorCode getSettings(void) const;
    SPxErrorCode getMaxSpeeds(void) const;
    SPxErrorCode getZoomRange(void) const;
    SPxErrorCode sendCommand(UINT16 command, UINT8 panSpeed,
	UINT8 tiltSpeed, UINT16 respCode) const;
    SPxErrorCode sendRawCommand(UINT32 command, UINT16 respCode) const;
    SPxErrorCode sendRecvRawCommand(UINT32 command, UINT32 *response,
	UINT16 opCode, int genError=TRUE) const;
    SPxErrorCode sendDCommand(UINT16 command, UINT8 panSpeed=0, UINT8 tiltSpeed=0) const;
    SPxErrorCode sendPCommand(UINT16 command, UINT8 panSpeed=0, UINT8 tiltSpeed=0) const;
    SPxErrorCode calculateCheckSum(SPxPelcoDMesg *msg) const;
    SPxErrorCode calculateCheckSum(SPxPelcoPMesg *msg) const;
    float getFOVForZoom(UINT16 zoomPos) const;
    UINT16 getZoomForFOV(float fovDegs) const;
    UINT8 getPelcoSpeed(float speed, float *speedSet) const;
    UINT32 getCmdSleepMsecs(void) const;
    void checkIfCamManager(void) const;
    virtual unsigned int SerialPortFn(const unsigned char *payload,
	unsigned int payloadBytes);

}; /* SPxCamCtrlPelco */

#endif /* _SPX_CAM_CTRL_PELCO_H */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
