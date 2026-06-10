/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTarget.h,v $
* ID: $Id: SPxSimTarget.h,v 1.41 2017/09/27 14:05:06 rew Exp $
*
* Purpose:
*	Header for SPxSimTarget object which represents a simulated
*	target within the SPxRadarSimulator radar source.
*
*
* Revision Control:
*   27/09/17 v1.41   AGC	Improve name change robustness.
*
* Previous Changes:
*   07/06/17 1.40   AGC	Support target POD.
*   15/12/16 1.39   AGC	Add support for SART targets.
*   29/11/16 1.38   AGC	Support target description.
*   31/10/16 1.37   AGC	Add TTM input flag.
*   23/08/16 1.36   AGC	Event function prototype updated.
*			Improve ordering of targets.
*   08/04/16 1.35   AGC	Improve efficieny of targets updates.
*			Support IFF values of zero.
*   16/03/16 1.34   AGC	Support extended IFF mode 1.
*   22/10/15 1.33   AGC	Support joystick control.
*   26/08/15 1.32   AGC	Support undo shape change.
*   06/08/15 1.31   AGC	Add const GetTrack() overload.
*   31/07/15 1.30   AGC	Support reporting targets as on ground or fixed.
*   10/07/15 1.29   AGC	Support reporting targets as manually initiated.
*   11/05/15 1.28   AGC	Signal event when target ID changed at end of motion.
*   17/03/15 1.27   AGC	Improve SPI handling.
*   07/11/14 1.26   AGC	Add GetRadarSim() function.
*			Support event handlers.
*   22/10/14 1.25   AGC	Check for changes to additional properties
*			before signalling target event.
*   14/10/14 1.24   AGC	Support IFF SPI and X pulse.
*			Support reporting targets as test targets.
*   01/09/14 1.23   AGC	Improve change detection for input targets.
*   18/07/14 1.22   AGC	Add Racon support.
*   02/06/14 1.21   AGC	Add motion offset height.
*			Use common AIS interface.
*   19/05/14 1.20   AGC	Fix target visibility when hidden due to motion.
*   16/05/14 1.19   AGC	Support symmetric shapes.
*   10/04/14 1.18   AGC	Move aircraft address to Mode-S structure.
*			Update track info when state changed.
*   26/03/14 1.17   AGC	Support reporting targets as simulated.
*			Support parameter control.
*			Support free running targets.
*   22/01/14 1.16   AGC	Support heading offset for targets.
*   21/01/14 1.15   AGC	Support setting IFF information.
*   05/09/13 1.14   AGC	Support passing platform lat/long to SetCurrentState().
*   06/06/13 1.13   AGC	Support multiple simulation files.
*   24/05/13 1.12   AGC	Support multilateration targets.
*   03/05/13 1.11   REW	Remove trailing comma from last change.
*   03/05/13 1.10   AGC	Support ADS-B info.
*   25/04/13 1.9    AGC	Support target templates.
*			Support custom target shapes.
*   28/03/13 1.8    AGC	Support extended AIS info.
*			Pre-calculate radar offsets.
*			Move load config here from SPxRadarSimulator.
*   18/03/13 1.7    AGC	Replace intensity with RCS.
*			Support flags.
*   13/02/13 1.6    AGC	Use percentage for intensity.
*   12/02/13 1.5    AGC	Prevent duplicated target names.
*   25/01/13 1.4    AGC	Add copy constructor.
*   21/12/12 1.3    AGC	Respond to motion change events.
*   09/11/12 1.2    AGC	Support configurable target size.
*			Support static targets.
*			Support AIS output.
*			Support configurable target intensity.
*   26/10/12 1.1    AGC	Initial version.
**********************************************************************/
#ifndef _SPX_SIM_TARGET_H
#define _SPX_SIM_TARGET_H

/*
 * Other headers required.
 */

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPxErrorCode. */
#include "SPxLibUtils/SPxError.h"

/* For SPxEventHandler. */
#include "SPxLibUtils/SPxEventHandler.h"

/* For base classes. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibNet/SPxAISDecoder.h"
#include "SPxLibData/SPxJoystickSim.h"

/*
 * Types
 */

/* Flags describing the type of target. */
enum SPxSimTargetFlags
{
    SPX_SIM_TARGET_FLAGS_NONE		= 0x00000000,
    SPX_SIM_TARGET_FLAGS_INPUT		= 0x00000001,
    SPX_SIM_TARGET_FLAGS_TEST		= 0x00000002,
    SPX_SIM_TARGET_FLAGS_INPUT_SPX	= 0x00000010,
    SPX_SIM_TARGET_FLAGS_INPUT_ASTERIX	= 0x00000020,
    SPX_SIM_TARGET_FLAGS_INPUT_AIS	= 0x00000040,
    SPX_SIM_TARGET_FLAGS_INPUT_ADSB	= 0x00000080,
    SPX_SIM_TARGET_FLAGS_INPUT_TTM	= 0x00000100
};

/* Target visibility. */
enum SPxSimTargetVis
{
    SPX_SIM_TARGET_VISIBLE = 0,
    SPX_SIM_TARGET_HIDDEN = 1,
    SPX_SIM_TARGET_POD_HIDDEN = 2
};

/* Position source. */
enum SPxSimTargetDynamicsSource
{
    SPX_SIM_TARGET_DYN_SRC_FREE		= 0,
    SPX_SIM_TARGET_DYN_SRC_MOTION	= 1,
    SPX_SIM_TARGET_DYN_SRC_JOYSTICK	= 2
};

/* Structure for setting/getting ADSB information. */
struct SPxSimTargetAdsbInfo
{
    unsigned int capabilities;
    unsigned int category;
};

/* Structure for setting/getting Mode-S information. */
struct SPxSimTargetModeSInfo
{
    UINT32 aircraftAddress;
};

/* Structure for setting/getting IFF information. */
struct SPxSimTargetIffInfo
{
    UINT8 mode1;
    UINT16 mode1Extended;
    UINT16 mode2;
    UINT16 mode3A;
    UINT8 flags;
};

/* Structure for setting/getting Racon information. */
struct SPxSimTargetRaconInfo
{
    char code;
    double onSecs;
    double offSecs;
};

/* Structure for setting/getting SART information. */
struct SPxSimTargetSartInfo
{
    int enabled;
    double pulseUsecs;
};

/* Forward declarations. */
struct SPxAISInfo;
class SPxRadarSimulator;
class SPxSimTargetTemplate;
class SPxSimMotion;
class SPxSimRadar;
struct SPxSimState;
class SPxRadarTrack;
struct SPxLatLong_tag;
struct SPxRadarSimEventParams;

/* Define our class. */
class SPxSimTarget : public SPxObj, public SPxAISIface, public SPxJoystickSimIface
{
public:
    /* Public types. */
    typedef SPxErrorCode (*EventHandler)(UINT32 flags);

    /* Public functions. */
    explicit SPxSimTarget(SPxRadarSimulator *radarSim, const char *name=NULL,
	UINT32 flags=SPX_SIM_TARGET_FLAGS_NONE);
    explicit SPxSimTarget(SPxSimTarget *target, const char *name=NULL, int checkName=TRUE);
    virtual ~SPxSimTarget(void);

    /* Target name. */
    SPxErrorCode SetName(const char *name);
    SPxErrorCode GetName(char *buffer, unsigned int bufLen) const;

    /* Is target editable etc? */
    SPxErrorCode SetFlags(UINT32 flags);
    UINT32 GetFlags(int mutexAlreadyLocked=FALSE) const;

    /* Target properties. */
    SPxErrorCode SetTemplate(SPxSimTargetTemplate *tgtTemplate);
    SPxErrorCode GetTemplate(SPxSimTargetTemplate **tgtTemplate) const;
    SPxErrorCode EnablePrimary(int enable);
    int IsPrimary(int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode EnableTrack(int enable);
    int IsTrack(int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode EnableMultilat(int enable);
    int IsMultilat(void) const;
    SPxErrorCode EnableAIS(int enable);
    int IsAIS(int mutexAlreadyLocked = FALSE) const;
    int IsAISEditable(void) const;
    SPxErrorCode EnableADSB(int enable);
    int IsADSB(int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetLengthMetres(double lengthMetres);
    SPxErrorCode GetLengthMetres(double *lengthMetres, int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetBreadthMetres(double breadthMetres);
    SPxErrorCode GetBreadthMetres(double *breadthMetres, int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetShape(const SPxPoint *points, unsigned int numPoints, int symmetric=FALSE);
    SPxErrorCode GetShape(SPxPoint *points, unsigned int *pointsLen,
			  int *symmetric = NULL, int useTemplate = TRUE,
			  int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetRcs(double rcs);
    SPxErrorCode GetRcs(double *rcs, int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetRegularPoD(double regularPoD);
    SPxErrorCode GetRegularPoD(double *regularPoD,
			       int mutexAlreadyLocked = FALSE) const;

    /* Additional properties. */
    SPxErrorCode SetAISInfo(const SPxAISInfo *aisInfo);
    virtual SPxErrorCode GetAISInfo(SPxAISInfo *aisInfo) const
    {
	return GetAISInfo(aisInfo, FALSE);
    }
    SPxErrorCode GetAISInfo(SPxAISInfo *aisInfo, int mutexAlreadyLocked) const;
    SPxErrorCode SetAdsbInfo(const SPxSimTargetAdsbInfo *adsbInfo);
    SPxErrorCode GetAdsbInfo(SPxSimTargetAdsbInfo *adsbInfo) const;
    SPxErrorCode SetModeSInfo(const SPxSimTargetModeSInfo *modeSInfo);
    SPxErrorCode GetModeSInfo(SPxSimTargetModeSInfo *modeSInfo,
			      int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetIffInfo(const SPxSimTargetIffInfo *iffInfo);
    SPxErrorCode GetIffInfo(SPxSimTargetIffInfo *iffInfo,
			    int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode GetSpiRemainingMS(UINT32 *spiRemainingMS) const;
    SPxErrorCode SetRaconInfo(const SPxSimTargetRaconInfo *raconInfo);
    SPxErrorCode GetRaconInfo(SPxSimTargetRaconInfo *raconInfo,
			      int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode SetSartInfo(const SPxSimTargetSartInfo *sartInfo);
    SPxErrorCode GetSartInfo(SPxSimTargetSartInfo *sartInfo,
			     int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode SetReportSimulated(int reportSimulated);
    int IsReportSimulated(int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetReportTest(int reportTest);
    int IsReportTest(int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetReportOnGround(int reportOnGround);
    int IsReportOnGround(int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetReportManuallyInitiated(int reportManuallyInitiated);
    int IsReportManuallyInitiated(int mutexAlreadyLocked = FALSE) const;
    SPxErrorCode SetReportFixed(int reportFixed);
    int IsReportFixed(int mutexAlreadyLocked = FALSE) const;

    /* Target description. */
    SPxErrorCode SetDescription(const char *description);
    SPxErrorCode GetDescription(char *buffer, unsigned int bufLen) const;

    SPxErrorCode SetDynamicsSource(SPxSimTargetDynamicsSource dynamicsSource);
    SPxErrorCode GetDynamicsSource(SPxSimTargetDynamicsSource *dynamicsSource) const;
    virtual int IsSourceJoystick(void) const;

    /* Free-running targets. */
    virtual SPxErrorCode SetCurrentState(const SPxSimState *state);
    SPxErrorCode SetCurrentState(const SPxSimState *state,
	const SPxLatLong_tag *initPlatLatLong);
    virtual SPxErrorCode GetCurrentState(SPxSimState *state) const
    {
	return GetCurrentState(state, FALSE);
    }
    SPxErrorCode GetCurrentState(SPxSimState *state,
				 int mutexAlreadyLocked) const;
    SPxErrorCode SetHeadingOffsetDegs(double headingOffsetDegs);
    SPxErrorCode GetHeadingOffsetDegs(double *headingOffsetDegs,
				      int mutexAlreadyLocked = FALSE) const;

    /* Position from motion profile. */
    SPxErrorCode SetMotionProfile(SPxSimMotion *motionProfile);
    SPxErrorCode GetMotionProfile(SPxSimMotion **motionProfile) const;
    SPxErrorCode SetMotionOffsetSecs(double motionOffsetSecs);
    SPxErrorCode GetMotionOffsetSecs(double *motionOffsetSecs) const;
    SPxErrorCode SetMotionOffsetXMetres(double motionOffsetXMetres);
    SPxErrorCode GetMotionOffsetXMetres(double *motionOffsetXMetres) const;
    SPxErrorCode SetMotionOffsetYMetres(double motionOffsetYMetres);
    SPxErrorCode GetMotionOffsetYMetres(double *motionOffsetYMetres) const;
    SPxErrorCode SetMotionOffsetHMetres(double motionOffsetHMetres);
    SPxErrorCode GetMotionOffsetHMetres(double *motionOffsetHMetres) const;
    SPxSimTargetVis IsVisible(int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode GetMotionTimeSecs(double *motionTimeSecs) const;

    /* Track information. */
    SPxRadarTrack *GetTrack(void);
    const SPxRadarTrack *GetTrack(void) const;

    /* Retrieve parent radar simulator. */
    SPxErrorCode GetRadarSim(SPxRadarSimulator **radarSim);

    /* Add/remove event callbacks. */
    template<typename F, typename O>
    SPxErrorCode AddEventCallback(F fn, O *obj)
    {
	return m_fns.Add(fn, obj);
    }
    template<typename F, typename O>
    SPxErrorCode RemoveEventCallback(F fn, O *obj)
    {
	return m_fns.Remove(fn, obj);
    }

    /* SPxLock interface. */
    SPxErrorCode Enter(void) const;
    SPxErrorCode Leave(void) const;

    /* Generic parameter assignment. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    SPxEventHandler<EventHandler> m_fns;

    /* Called by SPxRadarSimulator. */
    SPxErrorCode SetNameInternal(const char *name);
    SPxErrorCode SetTrack(SPxRadarTrack *track);
    SPxErrorCode SetUniqueID(UINT32 uniqueID);
    UINT32 GetUniqueID(void) const;
    SPxErrorCode Update(double currSecs, double diffSecs, 
	SPxLatLong_tag *initPlatLatLong, int changed=FALSE);
    SPxErrorCode RemoveRadarSim(void);
    void ApplyDesc(const char *buffer);
    const char *GetDesc(void) const;
    void SetFile(const char *filename);
    int WasFromFile(const char *filename);
    SPxErrorCode ApplyDelayed(void);
    SPxErrorCode SetDelayedMotion(const char *delayedMotion);
    SPxErrorCode SignalEvent(UINT32 flags, const SPxRadarSimEventParams *params);

    /* Called by SPxSimMotion. */
    SPxErrorCode RemoveMotion(void);

    /* Private functions. */
    int isAIS(void) const;
    SPxErrorCode updateFree(double currSecs, double diffSecs, 
	SPxLatLong_tag *initPlatLatLong, int changed=FALSE);
    SPxErrorCode updateMotion(double currSecs, double diffSecs, 
	SPxLatLong_tag *initPlatLatLong, int changed=FALSE);
    SPxErrorCode getShape(SPxPoint *points, unsigned int *pointsLen,
			  int *symmetric=NULL, int useTemplate=TRUE) const;
    static SPxErrorCode eventFnWrapper(UINT32 flags,
				       const SPxRadarSimEventParams *params,
				       void *userArg);
    SPxErrorCode eventFn(UINT32 flags);
    SPxSimMotion *findMotion(const char *name);
    SPxErrorCode setDynamicsSource(SPxSimTargetDynamicsSource dynamicsSource);
    SPxErrorCode setDelayedTemplate(const char *delayedTemplate);
    SPxErrorCode setInitialState(void);
    SPxErrorCode getInitialState(SPxSimState *state) const;
    SPxErrorCode updateTrackInfo(void);
    SPxErrorCode signalEvent(void);
    SPxErrorCode signalEvent(UINT32 flags,
			     const SPxRadarSimEventParams *params=NULL);

    friend class SPxRadarSimulator;
    friend class SPxSimMotion;

}; /* SPxSimTarget. */

#endif /* _SPX_SIM_TARGET_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
