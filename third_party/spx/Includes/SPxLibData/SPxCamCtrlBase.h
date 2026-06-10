/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamCtrlBase.h,v $
* ID: $Id: SPxCamCtrlBase.h,v 1.36 2017/07/31 12:58:07 rew Exp $
*
* Purpose:
*	Header for SPxAVCamCtrlBase class - an AV library base
*	class for camera contorl.
*
* Revision Control:
*   31/07/17 v1.36   AGC	Support SPxCamera setting max pan/tilt speeds.
*
* Previous Changes:
*   26/07/17 1.35   AJH	Add zoom/focus LUT functions.
*   13/07/17 1.34   AJH	Add Nexus support.
*			Support action on TCP connect.
*   22/06/17 1.33   AGC	Add Vision4ce support.
*   06/06/17 1.32   AGC	Support measured range.
*   26/05/17 1.31   AGC	Support primary/secondary zoom/focus.
*   24/04/17 1.30   AGC	Add virtual camera address and control type functions.
*   20/04/17 1.29   AGC	Add ZEOSys support.
*   02/02/17 1.28   AGC	Add options for concurrent PTZ.
*   31/01/17 1.27   AGC	Add IsConnected() function.
*   23/01/17 1.26   AGC	Modifications for testing.
*   18/01/17 1.25   AGC	Add priority functions.
*   31/10/16 1.24   AGC	Support for marking controller as slave.
*   19/10/16 1.23   AGC	Add GoToPreset().
*   23/09/16 1.22   AGC	Add GetLastPan/Tilt/Zoom() functions.
*   23/08/16 1.21   AGC	Make Shutdown() function virtual.
*   08/04/16 1.20   AGC	Support network control.
*   22/02/16 1.19   AGC	Add user override for camera command interval.
*   04/02/16 1.18   AGC	Support for retrieving camera settings.
*   10/12/15 1.17   AGC	Add absolute direction disabling.
*   03/11/15 1.16   AGC	Packet decoder callback data now const.
*			Add SPX_CAMERA_TYPE_REPLAY.
*			Allow retrieval of actual pan/tilt speeds.
*   10/09/15 1.15   AGC	Support per object debug to log file.
*   26/08/15 1.14   REW	Correct version numbers back to 0.1.
*   26/08/15 1.13   AGC	Add focus LUT support.
*   17/07/15 1.12   AGC	Add Shutdown() function.
*   25/06/15 1.11   AGC	Add GetZoomLUTFilename() function.
*   23/06/15 1.10   AGC	Add direction specific absolute command in progress checks.
*   12/06/15 1.9    AGC	Add Stop() function.
*			Add command time/interval functions.
*   06/06/14 1.8    SP 	Make WriteDebug() protected.
*   02/06/14 1.7    SP 	Add SetZoomLUTFilename().
*   14/04/14 1.6    SP 	Make serial port functions virtual.
*   27/03/14 1.5    SP 	Add SPX_CAMERA_TYPE_TEST.
*   17/02/14 1.4    AGC	Support setting serial port in Create().
*   17/02/14 1.3    SP	 Add new zoom configuration functions.
*   24/10/13 1.2    REW	Avoid compiler warning.
*   24/10/13 1.1    AGC	Move to SPx library.
*   23/08/13 0.2    SP 	Add IsAutoFocusEnabled().
*   14/08/13 0.1    AGC	Initial Version.
**********************************************************************/
#ifndef _SPX_CAM_CTRL_BASE_H
#define _SPX_CAM_CTRL_BASE_H

/*
 * Other headers required.
 */

/* For FILE. */
#include <cstdio>

/* For SPxAutoPtr class. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPxCriticalSection class. */
#include "SPxLibUtils/SPxCriticalSection.h"

/* For SPxMesgIOType_h enum. */
#include "SPxLibNet/SPxDecoderRepeater.h"

/* For SPxNetTCPAction enum. */
#include "SPxLibNet/SPxNetTCP.h"

/* For base class. */
#include "SPxLibUtils/SPxObj.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Available types of camera - update if new derived classes added. */
enum SPxCameraType
{
    SPX_CAMERA_TYPE_NONE = 0,
    SPX_CAMERA_TYPE_CHESS_ASP = 1,
    SPX_CAMERA_TYPE_CHESS_PIRANHA_36 = 2,
    SPX_CAMERA_TYPE_PELCO = 3,
    SPX_CAMERA_TYPE_ZEO_SYS = 4,
    SPX_CAMERA_TYPE_VISION4CE = 5,
    SPX_CAMERA_TYPE_NEXUS = 6,
    SPX_CAMERA_TYPE_REPLAY = 50,
    SPX_CAMERA_TYPE_TEST = 100
};

enum SPxCameraCommsMode
{
    SPX_CAMERA_COMMS_MODE_TRADITIONAL = 0,
    SPX_CAMERA_COMMS_MODE_STRICT = 1,
    SPX_CAMERA_COMMS_MODE_REV_TILT = 2
};

enum SPxCameraPanTiltResponseMode
{
    SPX_CAMERA_PAN_TILT_RESPONSE_MODE_NORMAL = 0,
    SPX_CAMERA_PAN_TILT_RESPONSE_MODE_LINEAR255 = 1,
    SPX_CAMERA_PAN_TILT_RESPONSE_MODE_QUAD = 2,
    SPX_CAMERA_PAN_TILT_RESPONSE_MODE_QUAD255 = 3
};

enum SPxCameraZoomSpeedMode
{
    SPX_CAMERA_ZOOM_SPEED_MODE_NORMAL = 0,
    SPX_CAMERA_ZOOM_SPEED_MODE_DEPEND_CAM1 = 1,
    SPX_CMAREA_ZOOM_SPEED_MODE_DEPEND_FOLLOW_VIEW = 2,
    SPX_CAMERA_ZOOM_SPEED_MODE_DEPEND_FOLLOW_CTRL = 3
};

enum SPxCameraControlStatus
{
    SPX_CAMERA_CONTROL_UNKNOWN = 0,
    SPX_CAMERA_CONTROL_IN_USE = 1,
    SPX_CAMERA_CONTROL_AVAILABLE = 2,
    SPX_CAMERA_CONTROL_BLOCKED = 3,
    SPX_CAMERA_CONTROL_NOT_CONNECTED = 4
};

enum SPxCameraControlType
{
    SPX_CAMERA_CTRL_TYPE_PELCO_D = 0,
    SPX_CAMERA_CTRL_TYPE_PELCO_P = 1,

    /* Old enum values for previous incarnation in SPxCamCtrlPelco. */
    SPX_CAM_CTRL_TYPE_PELCO_D = 0,
    SPX_CAM_CTRL_TYPE_PELCO_P = 1,

    /* Old AV enum values for previous incarnation of SPxCamCtrlPelco class. */
    SPX_AV_CAMERA_CONTROL_PELCO_D = 0,
    SPX_AV_CAMERA_CONTROL_PELCO_P = 1
};

enum SPxCameraLUTType
{
    SPX_CAMERA_LUT_ZOOM = 0,
    SPX_CAMERA_LUT_FOCUS = 1
};

static const UINT32 SPX_CAMERA_DIR_NONE  = 0x00;
static const UINT32 SPX_CAMERA_DIR_PAN   = 0x01;
static const UINT32 SPX_CAMERA_DIR_TILT  = 0x02;
static const UINT32 SPX_CAMERA_DIR_ZOOM  = 0x04;
static const UINT32 SPX_CAMERA_DIR_FOCUS = 0x08;
static const UINT32 SPX_CAMERA_DIR_QUERY = 0x10;
static const UINT32 SPX_CAMERA_DIR_ALL   = 0xFF;

/* Forward declarations. */
class SPxSerialPort;
struct SPxTime_tag;

/*
 * Define our class.
 */
class SPxCamCtrlBase : public SPxObj, public SPxSerial, public SPxNetAddr
{
public:
    /* Construction and destruction. */
    explicit SPxCamCtrlBase(SPxCameraType camType,
			    const unsigned char *syncBytes, int numSyncBytes,
			    int installHandler, unsigned int msgSize=0,
			    SPxNetTCP *netTCP=NULL);
    virtual ~SPxCamCtrlBase(void);
    virtual int IsSupported(void) const { return TRUE; }
    virtual SPxErrorCode Create(const char *serialName=NULL,
				unsigned int serialBaud=2400);
    SPxCameraType GetType(void) const;

    /* Settings. */
    virtual SPxErrorCode SetMesgType(SPxMesgIOType_t mesgType);
    virtual SPxMesgIOType_t GetMesgType(void) const;

    /* Serial port control. */
    virtual int IsSerialSupported(void) const { return TRUE; }
    virtual SPxErrorCode SetSerialName(const char *name);
    virtual SPxErrorCode GetSerialName(char *name, unsigned int bufLen) const;
    virtual const char *GetSerialName(void) const;
    virtual SPxErrorCode SetSerialBaud(unsigned int baud);
    virtual SPxErrorCode GetSerialBaud(unsigned int *baud) const;
    virtual unsigned int GetSerialBaud(void) const;

    /* Network control. */
    virtual int IsNetworkSupported(void) const { return TRUE; }
    virtual SPxErrorCode SetAddress(const char *address, int port = 0, const char *ifAddr = NULL);
    virtual SPxErrorCode SetAddress(UINT32 address, int port = 0, const char *ifAddr = NULL);
    virtual UINT32 GetAddress(void) const;
    virtual const char *GetAddressString(void) const;
    virtual int GetPort(void) const;
    virtual UINT32 GetIfAddress(void) const;
    virtual const char *GetIfAddressString(void) const;

    virtual SPxErrorCode SetCameraControlType(SPxCameraControlType /*type*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetCameraControlType(SPxCameraControlType * /*type*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetCameraAddress(UINT8 /*address*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetCameraAddress(UINT8 * /*address*/) const { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsConnected(void) const;

    SPxErrorCode SetAbsDirMask(UINT32 absDirMask);
    UINT32 GetAbsDirMask(void) const;

    /* Self reference. */
    virtual SPxErrorCode SelfReference(void) { return SPX_ERR_NOT_SUPPORTED; }

    /* Absolute positioning. */
    virtual int IsAbsCmdInProgress(UINT32 /*dirMask*/=SPX_CAMERA_DIR_ALL) const { return FALSE; }

    virtual int IsAbsPanSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetPan(float /*panAngleDegs*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetPan(float * /*panAngleDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetLastPan(float * /*panAngleDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetPanRange(float * /*minPanAngleDegs*/, float * /*maxPanAngleDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsAbsTiltSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetTilt(float /*tiltAngleDegs*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetTilt(float * /*tiltAngleDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetLastTilt(float * /*tiltAngleDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetTiltRange(float * /*minTiltAngleDegs*/, float * /*maxTiltAngleDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsAbsZoomSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetZoom(float /*fovDegs*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetZoom(float * /*fovDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetLastZoom(float * /*fovDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetZoomRange(float /*minFovDegs*/, float /*maxFovDegs*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetZoomRange(float * /*minFovDegs*/, float * /*maxFovDegs*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetMaxZoomPosition(UINT16 /*pos*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetMaxZoomPosition(UINT16 * /*pos*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetQueriedZoomIsPercentage(int /*state*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual int IsQueriedZoomPercentage(void) const { return FALSE; }
    virtual SPxErrorCode SetZoomLUTFilename(const char * /*filename*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetZoomLUTFilename(char * /*buffer*/, int /*bufLen*/) const { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsAbsFocusSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetFocus(float /*rangeMetres*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetFocus(float * /*rangeMetres*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetLastFocus(float * /*rangeMetres*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetFocusRange(float * /*minFocusLevel*/, float * /*maxFocusLevel*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetFocusLUTFilename(const char * /*filename*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetFocusLUTFilename(char * /*buffer*/, int /*bufLen*/) const { return SPX_ERR_NOT_SUPPORTED; }

    /* Absolute zoom/focus functions that support primary/secondary. */
    virtual int IsAbsCmdInProgress(UINT32 dirMask, int /*isPrimary*/) const { return IsAbsCmdInProgress(dirMask); }
    virtual int IsAbsZoomSupported(int /*isPrimary*/) const { return IsAbsZoomSupported(); }
    virtual SPxErrorCode SetZoom(float fovDegs, int /*isPrimary*/) { return SetZoom(fovDegs); }
    virtual SPxErrorCode GetZoom(float *fovDegs, int /*isPrimary*/) const { return GetZoom(fovDegs); }
    virtual SPxErrorCode GetLastZoom(float *fovDegs, int /*isPrimary*/) const { return GetLastZoom(fovDegs); }
    virtual SPxErrorCode SetZoomRange(float minFovDegs, float maxFovDegs, int /*isPrimary*/) { return SetZoomRange(minFovDegs, maxFovDegs); }
    virtual SPxErrorCode GetZoomRange(float *minFovDegs, float *maxFovDegs, int /*isPrimary*/) const { return GetZoomRange(minFovDegs, maxFovDegs); }

    virtual int IsAbsFocusSupported(int /*isPrimary*/) const { return IsAbsFocusSupported(); }
    virtual SPxErrorCode SetFocus(float rangeMetres, int /*isPrimary*/) { return SetFocus(rangeMetres); }
    virtual SPxErrorCode GetFocus(float *rangeMetres, int /*isPrimary*/) const { return GetFocus(rangeMetres); }
    virtual SPxErrorCode GetLastFocus(float *rangeMetres, int /*isPrimary*/) const { return GetLastFocus(rangeMetres); }

    /* Relative positioning. */
    virtual int IsRelPanSupported(void) const { return FALSE; }
    virtual SPxErrorCode Pan(float /*panSpeed*/, float * /*panSpeedSet*/=NULL,
			     int /*continueZoomFocus*/=FALSE)
    {
	return SPX_ERR_NOT_SUPPORTED;
    }

    virtual int IsRelTiltSupported(void) const { return FALSE; }
    virtual SPxErrorCode Tilt(float /*tiltSpeed*/, float * /*tiltSpeedSet*/=NULL,
			      int /*continueZoomFocus*/=FALSE)
    {
	return SPX_ERR_NOT_SUPPORTED;
    }

    virtual int IsRelPanTiltSupported(void) const { return FALSE; }
    virtual SPxErrorCode PanTilt(float /*panSpeed*/, float /*tiltSpeed*/,
				 float * /*panSpeedSet*/=NULL, float * /*tiltSpeed*/=NULL,
				 int /*continueZoomFocus*/=FALSE)
    {
	return SPX_ERR_NOT_SUPPORTED;
    }

    virtual int IsRelZoomSupported(void) const { return FALSE; }
    virtual SPxErrorCode Zoom(float /*zoomSpeed*/, int /*continuePanTilt*/=FALSE) { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsRelFocusSupported(void) const { return FALSE; }
    virtual SPxErrorCode Focus(float /*focusSpeed*/, int /*continuePanTilt*/=FALSE) { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsAutoFocusSupported(void) const { return FALSE; }
    virtual SPxErrorCode AutoFocus(int /*autoFocus*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual int IsAutoFocusEnabled(void) const { return FALSE; }

    /* Relative zoom/focus functions that support primary/secondary. */
    virtual int IsRelZoomSupported(int /*isPrimary*/) const { return IsRelZoomSupported(); }
    virtual SPxErrorCode Zoom(float zoomSpeed, int continuePanTilt, int /*isPrimary*/){ return Zoom(zoomSpeed, continuePanTilt); }
    virtual int IsRelFocusSupported(int /*isPrimary*/) const { return IsRelFocusSupported(); }
    virtual SPxErrorCode Focus(float focusSpeed, int continuePanTilt, int /*isPrimary*/) { return Focus(focusSpeed, continuePanTilt); }
    virtual int IsAutoFocusSupported(int /*isPrimary*/) const { return IsAutoFocusSupported(); }
    virtual SPxErrorCode AutoFocus(int autoFocus, int /*isPrimary*/) { return AutoFocus(autoFocus); }
    virtual int IsAutoFocusEnabled(int /*isPrimary*/) const { return IsAutoFocusEnabled(); }

    /* Presets. */
    virtual SPxErrorCode GoToPreset(UINT8 /*id*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GoToPreset(UINT8 id, int /*isPrimary*/) { return GoToPreset(id); }

    /* Queries. */
    virtual int AreQueriesSupported(void) const { return FALSE; }
    virtual SPxErrorCode GetFirmwareVersion(UINT16 * /*version*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetCommsMode(SPxCameraCommsMode * /*mode*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetPanTiltResponseMode(SPxCameraPanTiltResponseMode * /*mode*/) const
    {
	return SPX_ERR_NOT_SUPPORTED;
    }
    virtual SPxErrorCode GetZoomSpeedMode(SPxCameraZoomSpeedMode * /*mode*/) const
    {
	return SPX_ERR_NOT_SUPPORTED;
    }

    /* These two functions may be implemented/called regardless of AreMaxSpeedsSupported(). */
    virtual SPxErrorCode SetMaxPanSpeedDegsPerSec(double /*maxPanSpeedDegsPerSec*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetMaxTiltSpeedDegsPerSec(double /*maxTiltSpeedDegsPerSec*/) { return SPX_ERR_NOT_SUPPORTED; }

    virtual int AreMaxSpeedsSupported(void) const { return FALSE; }
    virtual SPxErrorCode GetMaxPanSpeedDegsPerSec(double * /*maxPanSpeedDegsPerSec*/) const
    {
	return SPX_ERR_NOT_SUPPORTED;
    }
    virtual SPxErrorCode GetMaxTiltSpeedDegsPerSec(double * /*maxTiltSpeedDegsPerSec*/) const
    {
	return SPX_ERR_NOT_SUPPORTED;
    }

    /* Priority control. */
    virtual SPxErrorCode GetControlStatus(SPxCameraControlStatus * /*status*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetControllerAddress(UINT32 * /*address*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetPriority(UINT8 /*priority*/) { return SPX_ERR_NOT_SUPPORTED; }

    /* Range measurement. */
    virtual int IsMeasuredRangeSupported(void) const { return FALSE; }
    virtual SPxErrorCode GetMeasuredRange(double * /*rangeMetres*/) const { return SPX_ERR_NOT_SUPPORTED; }

    virtual SPxErrorCode Stop(void);
    virtual SPxErrorCode Stop(int /*isPrimary*/) { return Stop(); }
    virtual UINT32 GetLastCmdTime(void) const;
    virtual UINT32 GetMinCmdIntervalMsecs(void) const { return 0; }
    SPxErrorCode SetMinCmdIntervalMsecsOverride(int minCmdIntervalMsecs);
    int GetMinCmdIntervalMsecsOverride(void) const;
    UINT32 GetCmdIntervalMsecs(void) const;

    /* Retrieve internal serial port and TCP objects. */
    SPxErrorCode GetSerialPort(SPxSerialPort **serialPort);
    SPxErrorCode GetNetTCP(SPxNetTCP **tcp) const;

    /* Mark as slave. */
    void SetSlave(int slave);
    int IsSlave(void) const;

    /* Generic get/set parameter interface. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    void SetLogFile(std::FILE *logFile);
    void SetLogFile(const char *logFileName);
    const char *GetLogFile(void) const;
    void SetVerbosity(UINT32 verbosity);
    UINT32 GetVerbosity(void) const;

protected:
    /* Protected functions. */
    SPxErrorCode SendMessage(const void *command, unsigned int numBytes) const;
    SPxErrorCode RecvMessage(void *command, unsigned int bufLen, UINT32 timeoutMS) const;
    void WriteDebug(UINT32 verbosity, const char *format, ...) const;
    virtual void Shutdown(void);
    SPxErrorCode readLUTFile(const char *filename, SPxCameraLUTType lut);
    const char *getLUTFilename(SPxCameraLUTType lut) const;
    unsigned int getLUTSize(SPxCameraLUTType lut) const;
    float getLUTValueForCamPos(UINT16 camPos, SPxCameraLUTType lut) const;
    UINT16 getLUTCamPosForValue(float value, SPxCameraLUTType lut) const;
    void getLUTBounds(UINT16 *firstPos, UINT16 *lastPos, float *firstVal,
	float *lastVal, SPxCameraLUTType lut) const;

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;	/* Private class structure. */

    /* Private functions. */
    static unsigned int serialPortFnWrapper(SPxSerialPort *serial, void *userArg,
					    SPxTime_tag *time,
					    const unsigned char *payload, unsigned int payloadBytes);
    static void netFnWrapper(SPxNetTCP *tcp,
			     SPxErrorCode errorCode,
			     SPxNetTCPAction action,
			     void* userArg,
			     const unsigned char* buffer,
			     unsigned int bufLen,
			     UINT32 fromAddr,
			     SOCKET_TYPE sock);
    static void netConnectWrapper(SPxNetTCP *tcp,
	SPxErrorCode errorCode,
	SPxNetTCPAction action,
	void* userArg,
	const unsigned char* buffer,
	unsigned int bufLen,
	UINT32 fromAddr,
	SOCKET_TYPE sock);
    unsigned int receiveData(const unsigned char* payload,
			     unsigned int payloadBytes);

    virtual unsigned int SerialPortFn(const unsigned char * /*payload*/,
				      unsigned int /*payloadBytes*/) { return 0; }

    virtual SPxErrorCode Setup(void) { return SPX_NO_ERROR; };


}; /* SPxCamCtrlBase */

/* For backwards compatibility. */
typedef SPxCamCtrlBase SPxAVCamCtrlBase;

#endif /* _SPX_CAM_CTRL_BASE_H */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
