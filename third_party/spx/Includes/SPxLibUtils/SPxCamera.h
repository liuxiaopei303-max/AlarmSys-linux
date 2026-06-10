/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamera.h,v $
* ID: $Id: SPxCamera.h,v 1.60 2017/09/26 15:44:24 rew Exp $
*
* Purpose:
*   Header for SPxCamera class.
*
* Revision Control:
*   26/09/17 v1.60   AGC	Add no-speed extrapolation option.
*
* Previous Changes:
*   31/07/17 1.59   AGC	Support setting controller max pan/tilt speeds.
*   22/06/17 1.58   AGC	Add max speed query support.
*   06/06/17 1.57   AGC	Support measured range.
*   02/02/17 1.56   AGC	Add options for concurrent PTZ.
*   23/01/17 1.55   AGC	Add WaitForRequest() function for testing.
*   18/01/17 1.54   AGC	Support priority control.
*   19/10/16 1.53   AGC	Add GoToPreset().
*   23/09/16 1.52   AGC	Add SetUseDevice().
*   09/09/16 1.51   AGC	Allow elevation to retrieved at a bearing.
*   23/08/16 1.50   AGC	Support for disabling use of corrections.
*   03/03/16 1.49   AGC	Set/get zoom LUT filename on background thread.
*   04/02/16 1.48   AGC	Add support for retrieving camera settings.
*   22/01/16 1.47   AGC	Support high priority stop.
*   23/12/15 1.46   AGC	Allow high priority commands.
*			Allow retrieval of actual camera speed.
*   10/12/15 1.45   AGC	Add optional user supplied time for error correction.
*   26/11/15 1.44   AGC	Support record/replay of camera speeds.
*   09/11/15 1.43   AGC	Support recording of camera commands.
*			Number camera request enums.
*   03/11/15 1.42   AGC	Support recording of camera positions.
*   22/10/15 1.41   AGC	Support lean correction.
*   14/10/15 1.40   AGC	Improve extrapolation of bearing/elevation.
*   10/09/15 1.39   AGC	Do not pass LUTs to camera control when slave.
*   26/08/15 1.38   AGC	Add focus LUT support.
*			Set focus as part of LookAtLatLong().
*   27/07/15 1.37   REW	Add pixelsPerMetre to GetMetresFromLatLong().
*   24/07/15 1.36   AGC	Support extrapolation for look-at.
*			Support extrapolation for GetBearing/Elevation.
*			Improve mutex locking for zoom range.
*   24/07/15 1.35   REW	Add pixel offset support.
*   02/07/15 1.34   AGC	Add speed mode.
*			Add elevation correction.
*   25/06/15 1.33   AGC	Remove one of the LookAtLatLong() overloads.
*   25/06/15 1.32   AGC	Indicate when LookAtLatLong() has no new data.
**			Add zoom LUT configuration.
*			Add bounding box to LookAtLatLong() interface.
*   24/06/15 1.31   AGC	Add ZoomChange() function for incremental zoom.
*			Handle deletion of camera controllers.
*   23/06/15 1.30   AGC	Add direction specific absolute command in progress
*			checks.
*   12/06/15 1.29   AGC	Do more of LookAtLatLong() in main thread when possible.
*   27/05/15 1.28   REW	Add support for UserExtra PTZ offsets.
*   22/05/15 1.27   AGC	Force blocking first check for absolute movement
*			support. Add WaitForGets() function.
*   21/05/15 1.26   AGC	Call camera controller functions in background thread.
*   18/05/15 1.25   AGC	Rename GetPan/Tilt() to GetPan/TiltSpeed().
*   15/05/15 1.24   AGC	Add control over auto-zooming.
*   11/05/15 1.23   AGC	Interpolate absolute positioning from LookAt...
*   11/05/15 1.22   AGC	Support various Look-At modes.
*   05/05/15 1.21   AGC	Support speeding up camera over time.
*   29/04/15 1.20   AGC	Add mutex protection.
*			Filter errors on position, not speed.
*   22/04/15 1.19   AGC	Add simplified LookAtLatLong().
*			Add GetPan/Tilt().
*   14/04/15 1.18   AGC	User provided filter in smooth movement functions.
*   26/03/15 1.17   AGC	Initial support for smooth camera movement.
*   17/03/15 1.16   AGC	Support camera speed configuration.
*   06/03/15 1.15   AGC	Add fixed camera settings.
*			Add home settings.
*   02/03/15 1.14   AGC	Add bearing correction.
*   30/01/15 1.13   REW	Add LookAtLatLong().
*   07/11/14 1.12   AGC	Improve management of set flags.
*   26/06/14 1.11   AGC	Add IsAbsCmdInProgress().
*   20/06/14 1.10   AGC	Add auto focus functions.
*   17/02/14 1.9    AGC	Expose camera controller functions.
*   26/11/13 1.8    AGC	Add elevation.
*			Return error code from get functions.
*   22/11/13 1.7    AGC	Add altitude.
*   04/11/13 1.6    AGC	Add Set/GetZoomRange() functions.
*   24/10/13 1.5    AGC	Add initial support for camera controls.
*   27/09/13 1.4    SP 	Make constructor public.
*			Support positioning from nav data.
*			Remove SetCameraName().
*   23/08/13 1.3    SP 	Add camera bearing and FOV.
*                       Add SetCameraName().
*   14/08/13 1.2    REW	Include more appropriate library headers.
*   14/08/13 1.1    SP 	Initial version.
**********************************************************************/

#ifndef _SPX_CAMERA_H
#define _SPX_CAMERA_H

/*
 * Other headers required.
 */
#include <cstdio> /* For std::FILE. */

/* Library headers. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibData/SPxCamCtrlBase.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibProc/SPxProcStd.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/


/*********************************************************************
*
*   Macros
*
**********************************************************************/


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward declare any other classes we need. */
class SPxNavData;
class SPxCamCtrlBase;
class SPxCamCtrlReplay;
class SPxPIDFilter;
class SPxThread;
class SPxRunProcess;
struct SPxPacketHeaderB_tag;
struct SPxTime_tag;

/*
 * Define our class.
 */
class SPxCamera : public SPxObj
{

/* The database class can access our internals directly. */
friend class SPxThreatDatabase;

public:
    /*
     * Public types.
     */
    enum LookAtMode
    {
	LOOK_AT_MODE_ABSOLUTE = 0,
	LOOK_AT_MODE_MIXED = 1,
	LOOK_AT_MODE_SMOOTH = 2
    };

    enum SpeedMode
    {
	SPEED_MODE_LINEAR = 0,
	SPEED_MODE_QUAD = 1
    };

    enum PosType
    {
	POS_NORMAL = 0,
	POS_EXTRAPOLATE = 1,
	POS_EXTRAPOLATE_NOSPEED = 2
    };

    enum Priority
    {
	PRIORITY_NORMAL = 0,
	PRIORITY_HIGH = 1
    };

    typedef SPxErrorCode (*FileWriter)(SPxRunProcess *rp,
				 UINT16 packetType,
				 const void *payload,
				 unsigned int payloadBytes,
				 UCHAR channelIndex);

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxCamera(void);
    virtual ~SPxCamera(void);

    SPxErrorCode SetSlave(int slave);
    int IsSlave(void) const;

    SPxErrorCode SetUseDevice(int useDevice);

    /* Controllers. */
    SPxErrorCode AddCameraControl(SPxCamCtrlBase *camCtrl);
    SPxErrorCode RemoveCameraControl(SPxCamCtrlBase *camCtrl);
    int HasCameraControl(void) const;
    unsigned int GetNumCameraControl(void) const;
    SPxCamCtrlBase *GetCameraControl(unsigned int index) const;

    virtual int IsAbsCmdInProgress(UINT32 dirMask=SPX_CAMERA_DIR_ALL) const;

    /* Check for support within any camera controller. */
    virtual int IsAbsPanSupported(void) const;
    virtual int IsAbsTiltSupported(void) const;
    virtual int IsAbsZoomSupported(void) const;
    virtual int IsAbsFocusSupported(void) const;
    virtual int IsRelPanSupported(void) const;
    virtual int IsRelTiltSupported(void) const;
    virtual int IsRelPanTiltSupported(void) const;
    virtual int IsRelZoomSupported(void) const;
    virtual int IsRelFocusSupported(void) const;
    virtual int IsAutoFocusSupported(void) const;
    virtual int IsMeasuredRangeSupported(void) const;

    /* Location and positioning. */
    virtual SPxErrorCode SetLatLong(double latDegs, double lonDegs);
    virtual SPxErrorCode GetLatLong(double *latDegs, double *lonDegs);
    virtual SPxErrorCode SetOffsetPixels(int offsetPixels);
    virtual SPxErrorCode GetOffsetPixels(int *offsetPixelsPtr) const;
    virtual SPxErrorCode SetAltitudeMetres(double altitudeMetres);
    virtual SPxErrorCode GetAltitudeMetres(double *altitudeMetres);
    virtual SPxErrorCode SetFixedBearingDegs(double fixedBearingDegs);
    virtual SPxErrorCode GetFixedBearingDegs(double *fixedBearingDegs) const;
    virtual SPxErrorCode SetFixedElevationDegs(double fixedElevationDegs);
    virtual SPxErrorCode GetFixedElevationDegs(double *fixedElevationDegs) const;
    virtual SPxErrorCode SetFixedFOVDegs(double fixedFovDegs);
    virtual SPxErrorCode GetFixedFOVDegs(double *fixedFovDegs) const;
    virtual SPxErrorCode SetBearingCorrectionDegs(double bearingCorrectionDegs);
    virtual SPxErrorCode GetBearingCorrectionDegs(double *bearingCorrectionDegs) const;
    virtual SPxErrorCode SetElevationCorrectionDegs(double elevationCorrectionDegs);
    virtual SPxErrorCode GetElevationCorrectionDegs(double *elevationCorrectionDegs) const;
    virtual SPxErrorCode SetLeanCorrectionTiltDegs1(double leanCorrectionTiltDegs1);
    virtual SPxErrorCode GetLeanCorrectionTiltDegs1(double *leanCorrectionTiltDegs1) const;
    virtual SPxErrorCode SetLeanCorrectionBearingDegs1(double leanCorrectionBearingDegs1);
    virtual SPxErrorCode GetLeanCorrectionBearingDegs1(double *leanCorrectionBearingDegs1) const;
    virtual SPxErrorCode SetLeanCorrectionTiltDegs2(double leanCorrectionTiltDegs2);
    virtual SPxErrorCode GetLeanCorrectionTiltDegs2(double *leanCorrectionTiltDegs2) const;
    virtual SPxErrorCode SetLeanCorrectionBearingDegs2(double leanCorrectionBearingDegs2);
    virtual SPxErrorCode GetLeanCorrectionBearingDegs2(double *leanCorrectionBearingDegs2) const;
    virtual SPxErrorCode SetUseCorrections(int useCorrections);

    /* Homing. */
    virtual SPxErrorCode SetHomeToHere(void);
    virtual SPxErrorCode SetHomeBearingDegs(double homeBearingDegs);
    virtual SPxErrorCode GetHomeBearingDegs(double *homeBearingDegs) const;
    virtual SPxErrorCode SetHomeElevationDegs(double homeElevationDegs);
    virtual SPxErrorCode GetHomeElevationDegs(double *homeElevationDegs) const;
    virtual SPxErrorCode SetHomeFOVDegs(double homeFovDegs);
    virtual SPxErrorCode GetHomeFOVDegs(double *homeFovDegs) const;
    virtual SPxErrorCode Home(void);
    virtual int IsHome(void) const;
    virtual SPxErrorCode Stop(Priority priority=PRIORITY_NORMAL);

    /* Absolute movement, if possible. */
    virtual SPxErrorCode SetBearingDegs(double bearingDegs);
    virtual SPxErrorCode GetBearingDegs(double *bearingDegs,
	PosType posType=POS_NORMAL, Priority priority=PRIORITY_NORMAL) const;
    virtual SPxErrorCode SetElevationDegs(double elevationDegs);
    virtual SPxErrorCode GetElevationDegs(double *elevationDegs,
	PosType posType=POS_NORMAL, Priority priority=PRIORITY_NORMAL) const;
    virtual SPxErrorCode GetElevationDegs(double *elevationDegs,
	PosType posType, Priority priority, double bearingDegs) const;
    virtual SPxErrorCode SetFOVDegs(double fovDegs);
    virtual SPxErrorCode GetFOVDegs(double *fovDegs) const;
    virtual SPxErrorCode SetZoomRange(double minFovDegs, double maxFovDegs);
    virtual SPxErrorCode GetZoomRange(double *minFovDegs, double *maxFovDegs) const;
    virtual SPxErrorCode SetZoomLUTFilename(const char *filename);
    virtual SPxErrorCode GetZoomLUTFilename(char *buffer, int bufLen) const;
    virtual SPxErrorCode SetFocus(double rangeMetres);
    virtual SPxErrorCode GetFocus(double *rangeMetres) const;
    virtual SPxErrorCode SetFocusLUTFilename(const char *filename);
    virtual SPxErrorCode GetFocusLUTFilename(char *buffer, int bufLen) const;

    /* Camera properties. */
    virtual SPxErrorCode SetMaxPanSpeedDegsPerSec(double maxPanSpeedDegsPerSec);
    virtual SPxErrorCode GetMaxPanSpeedDegsPerSec(double *maxPanSpeedDegsPerSec) const;
    virtual SPxErrorCode SetMaxTiltSpeedDegsPerSec(double maxTiltSpeedDegsPerSec);
    virtual SPxErrorCode GetMaxTiltSpeedDegsPerSec(double *maxTiltSpeedDegsPerSec) const;

    /* Relative movement. */
    virtual void Pan(double speed, int isTurbo=FALSE, int speedUp=FALSE);
    virtual double GetPanSpeed(int actualSpeed=FALSE, int zDep=FALSE) const;
    virtual void Tilt(double speed, int isTurbo=FALSE, int speedUp=FALSE);
    virtual double GetTiltSpeed(int actualSpeed=FALSE, int zDep=FALSE) const;
    virtual void PanTilt(double panSpeed, double tiltSpeed,
	int isTurbo=FALSE, int speedUp=FALSE,
	Priority priority=PRIORITY_NORMAL,int isZoomDependent=TRUE);
    virtual void Zoom(double speed, int continuePanTilt=FALSE);
    virtual void Focus(double speed, int continuePanTilt=FALSE);
    virtual void ZoomChange(double newProp);
    virtual SPxErrorCode SetSpeedZoomDependent(int enable);
    virtual int IsSpeedZoomDependent(void) const;
    virtual SPxErrorCode SetSpeedMode(SpeedMode mode);
    virtual SpeedMode GetSpeedMode(void) const;

    /* Auto focus. */
    virtual void AutoFocus(int autoFocus);
    virtual int IsAutoFocusEnabled(void) const;

    /* Priority control. */
    virtual SPxErrorCode GetControlStatus(SPxCameraControlStatus *status) const;
    virtual SPxErrorCode GetControllerAddress(UINT32 *address) const;
    virtual SPxErrorCode SetPriority(UINT8 priority);
    virtual UINT8 GetPriority(void) const;

    /* Presets. */
    virtual SPxErrorCode GoToPreset(UINT8 id, int controller=-1);

    /* Measured range. */
    virtual SPxErrorCode GetMeasuredRange(double *rangeMetres) const;

    /* Query settings. */
    virtual SPxErrorCode GetFirmwareVersion(UINT16 *version) const;
    virtual SPxErrorCode GetCommsMode(SPxCameraCommsMode *mode) const;
    virtual SPxErrorCode GetPanTiltResponseMode(SPxCameraPanTiltResponseMode *mode) const;
    virtual SPxErrorCode GetZoomSpeedMode(SPxCameraZoomSpeedMode *mode) const;

    /* Composite positioning and movement. */
    virtual SPxErrorCode SetLookAtMode(LookAtMode lookAtMode);
    virtual LookAtMode GetLookAtMode(void) const;
    virtual SPxErrorCode SetLookAtExtrapolateMS(UINT32 ms);
    virtual UINT32 GetLookAtExtrapolateMS(void) const;
    virtual SPxErrorCode LookAtLatLong(double latDegs, double longDegs,
				       double altitudeMetres,
				       int setElevation, int setZoom, int setFocus);
    virtual SPxErrorCode LookAtLatLong(SPxPIDFilter *filter,
				       double latDegs, double longDegs,
				       double altitudeMetres,
				       double speedMps, double courseDegs,
				       int initial, int repeat,
				       int setElevation, int setZoom, int setFocus,
				       double bndLatDegs1, double bndLongDegs1,
				       double bndLatDegs2, double bndLongDegs2,
				       double bndLatDegs3, double bndLongDegs3,
				       double bndLatDegs4, double bndLongDegs4);
    virtual SPxErrorCode LookAtLatLong(SPxPIDFilter *filter,
				       double latDegs, double longDegs,
				       double altitudeMetres,
				       double speedMps, double courseDegs,
				       int initial, int repeat,
				       int setElevation, int setZoom, int setFocus,
				       const double *extentLatLongDegs=NULL,
				       int numExtentLatLongDegs=0);
    virtual SPxErrorCode CorrectErrPos(SPxPIDFilter *filter,
				       double panErrDegs, double tiltErrDegs,
				       double maxPanSpeed=0.0, double maxTiltSpeed=0.0,
				       const SPxTime_tag *timeEpoch=NULL);
    virtual void SetMinVisibleWidthMetres(double widthMetres) { m_minVisibleWidthMetres = widthMetres; }
    virtual double GetMinVisibleWidthMetres(void) const { return m_minVisibleWidthMetres; }
    virtual void SetZoomVarianceFactor(double factor) { m_zoomVarianceFactor = factor; }
    virtual double GetZoomVarianceFactor(void) const { return m_zoomVarianceFactor; }

    /* Tweaks to apply to LookAtLatLong() position according to user input. */
    virtual SPxErrorCode SetUserExtraBearingDegs(double extraDegs);
    virtual SPxErrorCode SetUserExtraTiltDegs(double extraDegs);
    virtual SPxErrorCode SetUserExtraZoomMult(double extraZoom);
    virtual double GetUserExtraBearingDegs(void) const { return(m_userExtraBearingDegs); }
    virtual double GetUserExtraTiltDegs(void) const { return(m_userExtraTiltDegs); }
    virtual double GetUserExtraZoomMult(void) const { return(m_userExtraZoomMult); }

    /* Lat/long to metres. */
    virtual double GetMetresFromLatLong(double latDegs, double longDegs,
					double pixelsPerMetre = 0.0);

    /* Navigation data. */
    virtual SPxErrorCode SetNavData(SPxNavData *navData, 
                                    unsigned int cameraIndex=0,
                                    int setLatLong=TRUE,
                                    int setBearing=FALSE,
                                    int setAltitude=FALSE);
    virtual SPxErrorCode SetLatLongFromNavData(int set);
    virtual int IsLatLongFromNavData(void) const;
    virtual SPxErrorCode SetBearingFromNavData(int set);
    virtual int IsBearingFromNavData(void) const;
    virtual SPxErrorCode SetAltitudeFromNavData(int set);
    virtual int IsAltitudeFromNavData(void) const;

    /* User data. */
    virtual void SetUserData(void *ptr) { m_userData = ptr; }
    virtual void *GetUserData(void) const { return m_userData; }

    /* Output to SPx recording process. */
    SPxErrorCode AddRecordingProcess(UCHAR channelIndex, SPxRunProcess *recProc, 
	FileWriter fn=SPxProRecordPacketB);
    SPxErrorCode RemoveRecordingProcess(SPxRunProcess *recProc);

    /* Timer. */
    virtual SPxErrorCode Timer(void);

    virtual SPxErrorCode WaitForGets(UINT32 timeoutMS);

    /* Generic get/set parameter interface. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /*
     * Public static functions.
     */
    void SetVerbosity(UINT8 verbosity);
    UINT8 GetVerbosity(void) const;
    void SetLogFile(const char *logFile);
    const char *GetLogFile(void) const;

protected:
    enum RequestType
    {
	REQ_NONE = 0,
	REQ_IS_CMD_IN_PROGRESS = 1,
	REQ_IS_ABS_PAN = 10,
	REQ_IS_ABS_TILT = 11,
	REQ_IS_ABS_ZOOM = 12,
	REQ_IS_ABS_FOCUS = 13,
	REQ_IS_REL_PAN = 20,
	REQ_IS_REL_TILT = 21,
	REQ_IS_REL_PAN_TILT = 22,
	REQ_IS_REL_ZOOM = 23,
	REQ_IS_REL_FOCUS = 24,
	REQ_IS_AUTO_FOCUS = 30,
	REQ_IS_QUERY = 31,
	REQ_IS_MEASURED_RANGE = 32,
	REQ_IS_MAX_SPEEDS = 33,
	REQ_SET_ABS_PAN = 40,
	REQ_SET_ABS_TILT = 41,
	REQ_SET_ABS_ZOOM = 42,
	REQ_CHANGE_ZOOM = 43,
	REQ_SET_ABS_FOCUS = 44,
	REQ_GET_ABS_PAN = 50,
	REQ_GET_ABS_TILT = 51,
	REQ_GET_ABS_ZOOM = 52,
	REQ_GET_ABS_FOCUS = 53,
	REQ_SET_REL_PAN = 60,
	REQ_SET_REL_TILT = 61,
	REQ_SET_REL_PAN_TILT = 62,
	REQ_SET_REL_ZOOM = 63,
	REQ_SET_REL_FOCUS = 64,
	REQ_SET_MAX_SPEEDS = 65,
	REQ_SET_AUTO_FOCUS = 70,
	REQ_GET_AUTO_FOCUS = 71,
	REQ_GO_TO_PRESET = 75,
	REQ_GET_MEASURED_RANGE = 77,
	REQ_SET_ZOOM_RANGE = 80,
	REQ_GET_ZOOM_RANGE = 81,
	REQ_SET_ZOOM_LUT = 82,
	REQ_GET_ZOOM_LUT = 83,
	REQ_GET_FIRMWARE_VERSION = 90,
	REQ_GET_SETTINGS = 91,
	REQ_GET_MAX_SPEEDS = 92,
	REQ_GET_MIN_FOV = 93,
	REQ_GET_MAX_FOV = 94,
	REQ_SET_PRIORITY = 95,
	REQ_GET_CONTROL = 96,
	REQ_GET_CONTROLLER_ADDR = 97,
	REQ_STOP = 100
    };

    virtual SPxErrorCode WaitForRequest(RequestType reqType, UINT32 timeoutMS);

private:

    struct Request
    {
	RequestType type;
	Priority priority;
	UINT32 timeSecs;
	UINT32 timeUsecs;
	double value1;
	double value2;
	UINT8 value3;
	char str[1024];
	UINT32 mask;
	int inProgress;
    };

    /*
     * Private variables.
     */
    struct impl;
    SPxAutoPtr<impl> m_p;   /* Private structure. */
    int m_isSlave;	    /* Is this camera a slave? */
    int m_useDevice;	    /* Send queries to device? */
    void *m_userData;       /* User defined data. */
    double m_latDegs;       /* Latitude. */
    double m_lonDegs;       /* Longitude. */
    mutable int m_isLatLongSet;     /* Is that lat/long valid? */
    int m_offsetPixels;	    /* Offset left (-ve) or right (+ve) of mount */
    double m_homeBearingDegs;       /* Home position. */
    double m_homeElevationDegs;
    double m_homeFovDegs;
    double m_fixedBearingDegs;      /* Fixed positions for non-movable cameras. */
    double m_fixedElevationDegs;
    double m_fixedFovDegs;
    mutable double m_bearingDegs;   /* Bearing. */
    mutable UINT32 m_bearingDegsTime; /* Time the bearing was last received from camera. */
    mutable double m_prevBearingDegs; /* Previous bearing. */
    mutable UINT32 m_prevBearingDegsTime; /* Time of previous bearing received from camera. */
    double m_bearingCorDegs;        /* Bearing correction in degrees. */
    double m_altitudeMetres;        /* Altitude. */
    mutable int m_isAltitudeSet;    /* Is the altitude valid? */
    mutable double m_elevationDegs; /* Elevation. */
    mutable UINT32 m_elevationDegsTime;	/* Time the elevation was last received from camera. */
    mutable double m_prevElevationDegs; /* Previous elevation. */
    mutable UINT32 m_prevElevationDegsTime;	/* Time the last elevation was last received from camera. */
    double m_elevationCorDegs;	    /* Elevation correction in degrees. */
    mutable double m_fovDegs;       /* Field of view. */
    mutable double m_minFovDegs;    /* Range for field of view. */
    mutable double m_maxFovDegs;
    mutable double m_focus;	    /* Focus level. */
    mutable double m_measuredRangeMetres;   /* Measured range. */
    mutable UINT16 m_firmwareVersion;		/* Firmware version */
    mutable SPxCameraCommsMode m_commsMode;
    mutable SPxCameraPanTiltResponseMode m_panTiltResponseMode;
    mutable SPxCameraZoomSpeedMode m_zoomSpeedMode;
    double m_leanCorrectionTiltDegs1; /* Mount lean correction. */
    double m_leanCorrectionBearingDegs1;
    double m_leanCorrectionTiltDegs2;
    double m_leanCorrectionBearingDegs2;
    int m_useCorrections;
    SPxNavData *m_navData;	    /* Associated nav data object. */
    unsigned int m_navDataCameraIndex; /* Camera index. */
    int m_navDataSetLatLong;	    /* Set camera position from nav data? */
    int m_navDataSetBearing;	    /* Set camera bearing from nav data? */
    int m_navDataSetAltitude;	    /* Set camera altitude from nav data? */
    float m_panSpeed;		    /* Currently required pan speed. */
    float m_tiltSpeed;		    /* Currently required tilt speed. */
    float m_panSpeedZDep;	    /* Currently required Z dependent pan speed. */
    float m_tiltSpeedZDep;	    /* Currently required Z dependent tilt speed. */
    float m_panSpeedCurr;	    /* Actual pan speed in use. */
    float m_tiltSpeedCurr;	    /* Actual tilt speed in use. */
    float m_prevPanSpeedCurr;	    /* Previous actual pan speed in use. */
    float m_prevTiltSpeedCurr;	    /* Previous actual tilt speed in use. */
    UINT32 m_panSpeedTime;	    /* Last time pan speed was actually changed. */
    UINT32 m_tiltSpeedTime;	    /* Last time tile speed was actually changed. */
    UINT32 m_panTime;		    /* Last time pan speed manually adjusted. */
    UINT32 m_tiltTime;		    /* Last time tilt speed manually adjusted. */
    int m_speedUp;		    /* Speed up enabled? */
    float m_zoomSpeed;		    /* Current zoom speed. */
    float m_focusSpeed;		    /* Current focus speed. */
    mutable double m_maxPanSpeedDegsPerSec; /* Max supported pan speed. */
    mutable double m_maxTiltSpeedDegsPerSec; /* Max supported tilt speed. */
    int m_isSpeedZoomDependent;	    /* Zoom dependent speed enabled? */
    double m_panSpeedErr;	    /* Current pan speed error for dithering. */
    double m_tiltSpeedErr;	    /* Current tilt speed error for dithering. */
    SpeedMode m_speedMode;	    /* Speed mode. */
    LookAtMode m_lookAtMode;	    /* Look at mode. */
    float m_panAbsDegsPerSec;	    /* Speed for absolute pan interpolation. */
    UINT32 m_panAbsTime;	    /* Time when last interpolated absolute pan took place. */
    int m_lookAtQueued;		    /* Has a look-at action been queued? */
    int m_lookAtCancel;		    /* Has the look-at action been cancelled. */
    int m_lookAtTime;		    /* Time when last look-at with new data occurred. */
    UINT32 m_lookAtExtrapolateMS;   /* Extrapolate target positions for look-at by specified time. */
    double m_minVisibleWidthMetres; /* Minimum visible width when auto-zooming. */
    double m_zoomVarianceFactor;    /* Relationship between variance in pan/tilt measurements and required zoom. */
    double m_userExtraBearingDegs;  /* Additional amount for LookAt's */
    double m_userExtraTiltDegs;     /* Additional amount for LookAt's */
    double m_userExtraZoomMult;     /* Additional amount for LookAt's */
    mutable int m_isAbsPanCmdInProgress;
    mutable int m_isAbsTiltCmdInProgress;
    mutable int m_isAbsZoomCmdInProgress;
    mutable int m_isAbsFocusCmdInProgress;
    mutable int m_isAbsPanSupported;
    mutable int m_isAbsTiltSupported;
    mutable int m_isAbsZoomSupported;
    mutable int m_isAbsFocusSupported;
    mutable int m_isRelPanSupported;
    mutable int m_isRelTiltSupported;
    mutable int m_isRelPanTiltSupported;
    mutable int m_isRelZoomSupported;
    mutable int m_isRelFocusSupported;
    mutable int m_isAutoFocusSupported;
    mutable int m_isMeasuredRangeSupported;
    mutable int m_areQueriesSupported;
    mutable int m_areMaxSpeedsSupported;
    mutable int m_isAutoFocusEnabled;
    int m_haveCamControl;
    int m_haveReplayCamControl;
    int m_stopInQueue;
    mutable int m_absPanChecked;    /* Has absolute support been checked yet? */
    mutable int m_absTiltChecked;
    mutable int m_absZoomChecked;
    mutable int m_absFocusChecked;
    mutable int m_queriesChecked;
    RequestType m_reqInProgress;
    UINT8 m_priority;
    mutable SPxCameraControlStatus m_controlStatus;
    mutable UINT32 m_controllerAddr;

    /*
     * Private functions.
     */

    SPxErrorCode LookAtLatLongInit(double panSpeed);

    void panTilt(double panSpeed, double tiltSpeed,
		 int isTurbo=FALSE, int speedUp=FALSE,
		 Priority priority=PRIORITY_NORMAL, int isZoomDependent=TRUE,
		 int continueZoomFocus=FALSE);
    double getTiltSpeed(int actualSpeed, int zDep) const;
    SPxErrorCode stop(Priority priority=PRIORITY_NORMAL);
    int isAbsCmdInProgress(UINT32 dirMask=SPX_CAMERA_DIR_ALL) const;
    int isAbsPanSupported(void) const;
    int isAbsTiltSupported(void) const;
    int isAbsZoomSupported(void) const;
    int isAbsFocusSupported(void) const;
    int isRelPanSupported(void) const;
    int isRelTiltSupported(void) const;
    int isRelPanTiltSupported(void) const;
    int isRelZoomSupported(void) const;
    int isRelFocusSupported(void) const;
    int isAutoFocusSupported(void) const;
    int isMeasuredRangeSupported(void) const;
    int areQueriesSupported(void) const;
    int areMaxSpeedsSupported(void) const;
    SPxErrorCode getLatLong(double *latDegs, double *lonDegs);
    SPxErrorCode getAltitudeMetres(double *altitudeMetres);
    SPxErrorCode setBearingDegs(double bearingDegs);
    SPxErrorCode getBearingDegs(double *bearingDegs,
	PosType posType=POS_NORMAL, Priority priority=PRIORITY_NORMAL) const;
    SPxErrorCode setElevationDegs(double elevationDegs, int currentBearing=TRUE, double bearingDegs=0.0);
    SPxErrorCode getElevationDegs(double *elevationDegs,
	PosType posType=POS_NORMAL, Priority priority=PRIORITY_NORMAL,
	int haveBearing=FALSE, double bearingDegs=0.0) const;
    double getLeanDegs(int currentBearing, double bearingDegs) const;
    SPxErrorCode setFOVDegs(double fovDegs);
    SPxErrorCode getFOVDegs(double *fovDegs) const;
    SPxErrorCode setZoomRange(double minFovDegs, double maxFovDegs);
    SPxErrorCode getZoomRange(double *minFovDegs, double *maxFovDegs) const;
    SPxErrorCode setFocusMetres(double focus);
    SPxErrorCode getFocusMetres(double *focus) const;
    int isAutoFocusEnabled(void) const;
    SPxErrorCode getControlStatus(SPxCameraControlStatus *status) const;
    SPxErrorCode getControllerAddress(UINT32 *address) const;
    SPxErrorCode setPriority(UINT8 priority);
    SPxErrorCode correctErrPos(SPxPIDFilter *filter,
			       double panErrDegs, double tiltErrDegs,
			       int setElevation, int setZoom, int setFocus,
			       double maxPanSpeed=0.0, double maxTiltSpeed=0.0,
			       const SPxTime_tag *timeEpoch=NULL);
    double getFovFromDistance(double distMetres, double bearingDegs,
	const double *extentLatLongDegs, int numExtentLatLongDegs,
	SPxPIDFilter *filter);
    double applySpeedMode(double speed) const;
    void writeDebug(UINT8 verbosity, const char *format, ...) const;

    static void *threadFnWrapper(SPxThread *thread);
    void *threadFn(SPxThread *thread);
    void request(RequestType reqType, Priority priority=PRIORITY_NORMAL,
		 double value1=0.0, double value2=0.0, const char *str=NULL, UINT32 mask=0,
		 UINT8 value3=0) const;
    void reqIsAbsCmdInProgress(UINT32 dirMask=SPX_CAMERA_DIR_ALL) const;
    void reqIsAbsPanSupported(void) const;
    void reqIsAbsTiltSupported(void) const;
    void reqIsAbsZoomSupported(void) const;
    void reqIsAbsFocusSupported(void) const;
    void reqIsRelPanSupported(void) const;
    void reqIsRelTiltSupported(void) const;
    void reqIsRelPanTiltSupported(void) const;
    void reqIsRelZoomSupported(void) const;
    void reqIsRelFocusSupported(void) const;
    void reqIsAutoFocusSupported(void) const;
    void reqIsMeasuredRangeSupported(void) const;
    void reqAreQueriesSupported(void) const;
    void reqAreMaxSpeedsSupported(void) const;
    void reqSetPanDegs(double panDegs);
    void reqSetTiltDegs(double tiltDegs);
    void reqSetFOVDegs(double fovDegs);
    void reqChangeFOVDegs(double newProp);
    void reqSetFocus(double focus);
    void reqGetPanDegs(void) const;
    void reqGetTiltDegs(void) const;
    void reqGetFOVDegs(void) const;
    void reqGetFocus(void) const;
    void reqPan(double panSpeed);
    void reqTilt(double tiltSpeed);
    void reqPanTilt(double panSpeed, double tiltSpeed, UINT8 continueZoomFocus);
    void reqZoom(double zoomSpeed, UINT8 continuePanTilt);
    void reqFocus(double focusSpeed, UINT8 continuePanTilt);
    void reqSetMaxSpeeds(double maxPanSpeedDegsPerSec, double maxTiltSpeedDegsPerSec);
    void reqAutoFocus(int enable);
    void reqGetAutoFocus(void) const;
    void reqGoToPreset(UINT8 id, UINT32 controller);
    void reqGetMeasuredRange(void) const;
    void reqSetZoomRange(double minFovDegs, double maxFovDegs);
    void reqGetZoomRange(void) const;
    void reqSetZoomLUT(const char *filename);
    void reqGetZoomLUT(void) const;
    void reqGetFirmwareVersion(void) const;
    void reqGetSettings(void) const;
    void reqGetMaxSpeeds(void) const;
    void reqGetMinFov(void) const;
    void reqGetMaxFov(void) const;
    void reqSetPriority(UINT8 priority);
    void reqGetControlStatus(void) const;
    void reqGetControllerAddr(void) const;
    void reqStop(void);
    void recordCommand(RequestType reqType, double value1=0.0, double value2=0.0, UINT32 mask=0) const;
    void recordPos(UINT8 direction, REAL32 valueDegs) const;
    void recordSpeed(UINT8 direction, REAL32 valueDegsPerSec) const;
    void recordPosSpeed(UINT8 direction, int isSpeed, REAL32 value) const;
    static int camCtrlDeleted(void *invokingObject, void *userObject, void *arg);

}; /* SPxCamera */

#endif /* SPX_CAMERA_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
