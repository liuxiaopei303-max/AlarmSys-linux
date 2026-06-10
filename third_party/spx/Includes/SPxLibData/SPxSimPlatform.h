/*********************************************************************
*
* (c) Copyright 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimPlatform.h,v $
* ID: $Id: SPxSimPlatform.h,v 1.7 2017/05/26 10:07:55 rew Exp $
*
* Purpose:
*	Header for SPxSimPlatform object which is a simulated platform
*	for use with the SPxRadarSimulator radar source.
*
*
* Revision Control:
*   26/05/17 v1.7    AGC	Add position noise.
*				Improve nav data output.
*
* Previous Changes:
*   16/05/17 1.6    AGC	Use separate nav data objects for I/O and display.
*			Support set and drift.
*   12/04/17 1.5    AGC	Support set/get of noise seed.
*   15/03/17 1.4    AGC	Improve platform positioning on jump-to-time.
*   08/03/17 1.3    AGC	Add platform roll noise.
*   02/02/17 1.2    AGC	Improve nav data replay.
*   31/01/17 1.1    AGC	Initial version.
**********************************************************************/
#ifndef SPX_SIM_PLATFORM_H_
#define SPX_SIM_PLATFORM_H_

/*
 * Other headers required.
 */

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPxEventHandler. */
#include "SPxLibUtils/SPxEventHandler.h"

/* For base classes. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibData/SPxJoystickSim.h"

/* For SPxRadarSimHeightAbove. */
#include "SPxLibData/SPxRadarSimulator.h"

/*
 * Types
 */

enum SPxRadarSimPlatformSource
{
    SPX_RADAR_SIM_PLAT_SRC_STATIC = 0,
    SPX_RADAR_SIM_PLAT_SRC_MOTION = 1,
    SPX_RADAR_SIM_PLAT_SRC_NAV_DATA = 2,
    SPX_RADAR_SIM_PLAT_SRC_JOYSTICK = 3
};

/* Forward declarations. */
class SPxRadarSimulator;
class SPxNavData;
class SPxSimMotion;
struct SPxLatLong_tag;
struct SPxSimState;

/* Define our class. */
class SPxSimPlatform : public SPxObj, public SPxAISIface, public SPxJoystickSimIface
{
public:
    /* Public types. */
    typedef SPxErrorCode (*NameChangeHandler)(void);

    /* Public functions. */
    explicit SPxSimPlatform(SPxRadarSimulator *radarSim,
			    SPxNavData *navData,
			    SPxNavData *navDataDisplay=NULL,
			    unsigned int index=0);
    virtual ~SPxSimPlatform(void);

    SPxErrorCode GetRadarSim(SPxRadarSimulator **radarSim);
    SPxErrorCode GetIndex(unsigned int *index) const;

    SPxErrorCode SetPlatformName(const char *platformName);
    SPxErrorCode GetPlatformName(char *buffer, unsigned int bufLen) const;

    /* Configuration. */
    SPxErrorCode SetPlatformSource(SPxRadarSimPlatformSource platformSource);
    SPxErrorCode GetPlatformSource(SPxRadarSimPlatformSource *platformSource) const;
    int IsSourceJoystick(void) const;
    SPxErrorCode SetReplayNavData(int replayNavData);
    SPxErrorCode GetReplayNavData(int *replayNavData) const;
    SPxErrorCode SetExtrapolateNavData(int extrapolateNavData);
    int IsExtrapolateNavData(void) const;
    SPxErrorCode SetGenNavDataFixChar(int genNavDataFixChar);
    int IsGenNavDataFixChar(void) const;
    SPxErrorCode SetHeadingOffsetDegs(double headingOffsetDegs);
    SPxErrorCode GetHeadingOffsetDegs(double *headingOffsetDegs) const;
    SPxErrorCode SetNavDataMessages(UINT32 navDataMessages);
    SPxErrorCode GetNavDataMessages(UINT32 *navDataMessages) const;
    SPxErrorCode SetPlatformLatLong(const SPxLatLong_tag *latLong);
    SPxErrorCode GetPlatformLatLong(SPxLatLong_tag *latLong) const;
    SPxErrorCode GetInitialPlatformLatLong(SPxLatLong_tag *latLong) const;
    SPxErrorCode SetMotionProfile(SPxSimMotion *motionProfile);
    SPxErrorCode GetMotionProfile(const SPxSimMotion **motionProfile) const;
    SPxErrorCode GetMotionProfile(SPxSimMotion **motionProfile);
    SPxErrorCode SetHeightMetres(double heightMetres);
    SPxErrorCode GetHeightMetres(double *heightMetres) const;
    SPxErrorCode SetHeightAbove(SPxRadarSimHeight heightAbove);
    SPxErrorCode GetHeightAbove(SPxRadarSimHeight *heightAbove) const;
    SPxErrorCode GetHeightAboveSeaMetres(double *heightAboveSeaMetres) const;
    SPxErrorCode SetMagneticVariation(double magVarDegs);
    SPxErrorCode GetMagneticVariation(double *magVarDegs) const;
    SPxErrorCode SetSetMps(double setMps);
    SPxErrorCode GetSetMps(double *setMps) const;
    SPxErrorCode SetDriftDegs(double driftDegs);
    SPxErrorCode GetDriftDegs(double *driftDegs) const;

    /* Set/get current state. */
    virtual SPxErrorCode SetCurrentState(const SPxSimState *state);
    virtual SPxErrorCode GetCurrentState(SPxSimState *state) const;
    SPxErrorCode GetCurrentState(SPxSimState *state, int mutexLocked) const;

    /* Configure AIS VDO information. */
    SPxErrorCode EnableAISOwnShip(int enable);
    int IsAISOwnShipEnabled(void) const;
    virtual int IsAISEditable(void) const;
    SPxErrorCode SetAISName(const char *aisName);
    SPxErrorCode GetAISName(char *aisName, unsigned int bufLen,
			    int mutexAlreadyLocked = FALSE) const;
    virtual SPxErrorCode SetAISInfo(const SPxAISInfo *aisInfo);
    virtual SPxErrorCode GetAISInfo(SPxAISInfo *aisInfo) const;
    SPxErrorCode GetAISInfo(SPxAISInfo *aisInfo, int mutexAlreadyLocked) const;
    SPxErrorCode SetAISMaxTargetRangeMetres(double aisMaxTargetRangeMetres);
    SPxErrorCode GetAISMaxTargetRangeMetres(double *aisMaxTargetRangeMetres) const;

    /* Platform noise. */
    SPxErrorCode SetPositionNoiseStdDev(double posNoiseMetres);
    SPxErrorCode GetPositionNoiseStdDev(double *posNoiseMetres) const;
    SPxErrorCode SetCourseNoiseStdDev(double courseNoiseDegs);
    SPxErrorCode GetCourseNoiseStdDev(double *courseNoiseDegs) const;
    SPxErrorCode SetHeadingNoiseStdDev(double headingNoiseDegs);
    SPxErrorCode GetHeadingNoiseStdDev(double *headingNoiseDegs) const;
    SPxErrorCode SetSpeedNoiseStdDev(double speedNoiseMps);
    SPxErrorCode GetSpeedNoiseStdDev(double *speedNoiseMps) const;
    SPxErrorCode GetCurrentCourseNoiseDegs(double *courseNoiseDegs) const;
    SPxErrorCode GetCurrentHeadingNoiseDegs(double *headingNoiseDegs) const;
    SPxErrorCode SetMaxRollDegs(double maxRollDegs);
    SPxErrorCode GetMaxRollDegs(double *maxRollDegs) const;
    SPxErrorCode GetCurrentRollDegs(double *rollDegs) const;
    SPxErrorCode SetNoiseSeed(const UINT32 *seeds, unsigned int numSeeds);
    SPxErrorCode GetNoiseSeed(UINT32 *seeds, unsigned int numSeeds) const;

    /* Get internal objects. */
    SPxErrorCode GetNavData(SPxNavData **navData) const;
    SPxErrorCode GetNavDataDisplay(SPxNavData **navData) const;
    
    /* Add/remove name change callbacks. */
    template<typename F, typename O>
    SPxErrorCode AddNameChangeCallback(F fn, O *userArg)
    {
	return m_nameChangeFns.Add(fn, userArg);
    }
    template<typename F, typename O>
    SPxErrorCode RemoveNameChangeCallback(F fn, O *userArg)
    {
	return m_nameChangeFns.Remove(fn, userArg);
    }
    
    /* Set/get parameter interface. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);
    
private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    SPxEventHandler<NameChangeHandler> m_nameChangeFns;
    
    /* Called by SPxRadarSimulator. */
    SPxErrorCode RemoveRadarSim(void);
    void SetNavDataUpdateRequired(void);
    void SetCurrentTimeSecs(double currentTimeSecs);
    SPxErrorCode Update(UINT64 msecs, double speed, double radCurrSecs);
    SPxErrorCode OutputNavData(double speed);
    SPxErrorCode OutputHeading(void);
    SPxErrorCode RemoveMotion(const SPxSimMotion *motion);
    void MotionChanged(const SPxSimMotion *motionProfile);
    const char *GetDesc(void) const;
    void SetFile(const char *filename);
    int WasFromFile(const char *filename);
    SPxErrorCode ApplyDelayed(void);
    SPxErrorCode SetDelayedMotion(const char *delayedMotion);

    /* Private functions. */
    static int navDataFnWrapper(void *invokingObject,
				void *userObject,
				void *arg);
    int navDataFn(int mutexLocked);
    SPxErrorCode setPlatformSource(SPxRadarSimPlatformSource platformSource);
    SPxErrorCode setMotionProfile(SPxSimMotion *motionProfile);
    SPxErrorCode updatePlatformPos(UINT64 diffMs,
				   double speed,
				   double radCurrSecs,
				   int updateRequired);
    SPxErrorCode getNewPlatformPos(double intervalSecs,
				   SPxSimState *state,
				   int updateRequired,
				   int realUpdate);
    SPxErrorCode outputNavData(double speed);
    SPxErrorCode outputHeading(void);
    SPxErrorCode updateNavDataDisplay(void);

    friend class SPxRadarSimulator;

}; /* SPxSimPlatform. */

#endif /* SPX_SIM_PLATFORM_H_ */

/*********************************************************************
*
* End of file
*
**********************************************************************/
