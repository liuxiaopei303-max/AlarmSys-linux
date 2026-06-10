/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadar.h,v $
* ID: $Id: SPxSimRadar.h,v 1.66 2017/09/15 13:11:53 rew Exp $
*
* Purpose:
*	Header for SPxSimRadar object which is a simulated radar
*	for use with the SPxRadarSimulator radar source.
*
*
* Revision Control:
*   15/09/17 v1.66   AGC	Do not use slant range in video sim messages.
*
* Previous Changes:
*   18/07/17 1.65   AGC	Support new IFF encode mode.
*   20/04/17 1.64   AGC	Fix for N/C 447.
*   12/04/17 1.63   AGC	Support set/get of noise seeds.
*   15/03/17 1.62   AGC	Add target size noise.
*   06/02/17 1.61   AGC	Fix mutex issues.
*   31/01/17 1.60   AGC	Support multiple platforms.
*   23/01/17 1.59   AGC	Always use simulator NavData.
*   04/01/17 1.58   AGC	Expand target noise options.
*   15/12/16 1.57   AGC	Add option to clear aux video in blind sectors.
*			Add support for SART targets.
*   29/11/16 1.56   AGC	Support muting radar.
*   07/11/16 1.55   AGC	Improve 360 degree blind sectors.
*   01/11/16 1.54   AGC	Make Update functions virtual for testing.
*   02/09/16 1.53   AGC	Support testing of HTTP network sender.
*   14/07/16 1.52   AGC	Support more radar video data types.
*   15/06/16 1.51   AGC	Support ramping intensity across vertical beam angle.
*   23/05/16 1.50   AGC	Modify SPX_SIM_RADAR_CHANGE_ALL value.
*   08/04/16 1.49   AGC	Improve efficiency of target updates.
*   17/03/16 1.48   AGC	Avoid breaking strict aliasing rules.
*   03/03/16 1.47   AGC	Support various radar data types.
*   22/02/16 1.46   AGC	Initial support for TTM output.
*   04/02/16 1.45   AGC	Preserve numTriggers and pimFlags from aux source.
*   01/02/16 1.44   AGC	Improve Primary+MTI behaviour.
*   11/01/16 1.43   AGC	Move noise generation to separate SPxSimRadarNoise objects.
*			Support MTI and Primary+MTI video generation.
*   23/12/15 1.42   AGC	Support MTI and combined Primary and MTI video.
*   14/12/15 1.41   AGC	Rearrange auxiliary source input.
*   01/12/15 1.40   AGC	Add change flag for enable/disable.
*   26/11/15 1.39   AGC	Support curvature of Earth.
*			Support multiple visibility heights.
*   16/11/15 1.38   AGC	Add multiple blind sector support.
*   22/10/15 1.37   AGC	Support sector scan.
*   06/08/15 1.36   AGC	Update target position relative to radar in UpdateTarget().
*			Add const overload for GetSimTerrain().
*   27/07/15 1.35   AGC	Add support for video simulator messages.
*   11/05/15 1.34   AGC	Add RemoveTargetID() notification function.
*   17/03/15 1.33   AGC	Add AreRangeAndPrfValid().
*   22/10/14 1.32   AGC	Add SPX_SIM_RADAR_CHANGE_ALL.
*   14/10/14 1.31   AGC	Support including IFF mode encoded in video.
*			Output track deletion messages.
*   24/09/14 1.30   AGC	Improve accuracy of dynamic load control.
*   15/09/14 1.29   AGC	Add dynamic load control.
*   18/07/14 1.28   AGC	Add GetTargetIntensity/Visibility() functions.
*			Add Racon support.
*			Add IFF support.
*   04/07/14 1.27   AGC	Add radar visibility map.
*   01/07/14 1.26   AGC	Support periodic Asterix messages.
*   02/06/14 1.25   AGC	Support for radar name.
*   16/05/14 1.24   AGC	Support hiding obscured targets independent of terrain settings.
*   08/05/14 1.23   AGC	Support start range.
*			Only apply settings when they have changed.
*   10/04/14 1.22   AGC	Allow tests to provide own packet senders.
*   26/03/14 1.21   AGC	Move pulse length calculation into function.
*			Slant range in track reports.
*   21/01/14 1.20   AGC	Move SPx and Asterix track distribution from SPxRadarSimulator.
*			Support optional Asterix fields.
*   30/07/13 1.19   AGC	Remove ShowTerrain option.
*   25/07/13 1.18   AGC	Add AlwaysNearNoise option.
*   19/07/13 1.17   AGC	Fix #254: Certain periods result in no video.
*			Rename upside down option to anticlockwise.
*   06/06/13 1.16   AGC	Support target positioning on leading edge.
*			Support multiple simulation files.
*			Synchronise with aux sources.
*   03/05/13 1.15   AGC	Support height above ground for static radars.
*			Support separate Asterix output.
*   25/04/13 1.14   AGC	New primary target rendering method.
*			Support optional fractional positioning.
*			Output tracker status messages.
*   28/03/13 1.13   AGC	Add sea clutter and near noise variation.
*			Move load config here from SPxRadarSimulator.
*			Improve performance.
*   18/03/13 1.12   AGC	Fix radar offset update.
*			Support minimum beam angle.
*			Support sensitivity.
*   08/03/13 1.11   AGC	Support static radars.
*   21/02/13 1.10   AGC	Add flag for beam height.
*   13/02/13 1.9    AGC	Use percentage for background noise level.
*   12/02/13 1.8    AGC	Support Asterix track reports.
*			Add more info to SPx track reports.
*   25/01/13 1.7    AGC	Support jumping to time in simulation.
*			Support radar offsets from platform.
*   21/12/12 1.6    AGC	Support extended track reports.
*   12/12/12 1.5    AGC	Add Rewind() function.
*   28/11/12 1.4    AGC	Fix the shape of targets with large beamwidth.
*   09/11/12 1.3    AGC	Rename VertBeamWidth to MaxBeamAngle.
*			Add radar index to constructor.
*   30/10/12 1.2    REW	Add GetSimTerrain() for AGC.
*   26/10/12 1.1    AGC	Initial version.
**********************************************************************/
#ifndef _SPX_SIM_RADAR_H
#define _SPX_SIM_RADAR_H

/*
 * Other headers required.
 */

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For base class. */
#include "SPxLibUtils/SPxObj.h"

/* For SPxLUTtype enumeration. */
#include "SPxLibData/SPxLUT.h"

/* For SPxRadarSimHeight enumeration. */
#include "SPxLibData/SPxRadarSimulator.h"

/* For SPxSimTargetVis enumeration. */
#include "SPxLibData/SPxSimTarget.h"

/* For Distribution enumeration. */
#include "SPxLibData/SPxSimTargetNoise.h"

/* For SPxMesgIOType. */
#include "SPxLibNet/SPxDecoderRepeater.h"

/*
 * Types
 */

enum SPxSimRadarVideoType
{
    SPX_SIM_RADAR_VID_PRIMARY		= 0,
    SPX_SIM_RADAR_VID_IFF		= 1,
    SPX_SIM_RADAR_VID_VIS_MAP		= 2,
    SPX_SIM_RADAR_VID_MTI		= 3,
    SPX_SIM_RADAR_VID_PRIMARY_AND_MTI	= 4
};

/* Change flags. */
enum SPxSimRadarChangeFlags
{
    SPX_SIM_RADAR_CHANGE_NONE		    = 0x00000000,
    SPX_SIM_RADAR_CHANGE_NUM_SAMPLES	    = 0x00000001,
    SPX_SIM_RADAR_CHANGE_POSITION	    = 0x00000002,
    SPX_SIM_RADAR_CHANGE_HEIGHT		    = 0x00000004,
    SPX_SIM_RADAR_CHANGE_PERIOD		    = 0x00000008,
    SPX_SIM_RADAR_CHANGE_PRF		    = 0x00000010,
    SPX_SIM_RADAR_CHANGE_RANGE		    = 0x00000020,
    SPX_SIM_RADAR_CHANGE_BEAM_WIDTH	    = 0x00000040,
    SPX_SIM_RADAR_CHANGE_BEAM_HEIGHT	    = 0x00000080,
    SPX_SIM_RADAR_CHANGE_PULSE_WIDTH	    = 0x00000100,
    SPX_SIM_RADAR_CHANGE_NOISE		    = 0x00000200,
    SPX_SIM_RADAR_CHANGE_SLANT_RANGE	    = 0x00000400,
    SPX_SIM_RADAR_CHANGE_SENSITIVITY	    = 0x00000800,
    SPX_SIM_RADAR_CHANGE_ANTI_CLOCKWISE	    = 0x00001000,
    SPX_SIM_RADAR_CHANGE_NAME		    = 0x00002000,
    SPX_SIM_RADAR_CHANGE_VIS_MAP	    = 0x00004000,
    SPX_SIM_RADAR_CHANGE_VID_TYPE	    = 0x00008000,
    SPX_SIM_RADAR_CHANGE_IFF_INTERLACE	    = 0x00010000,
    SPX_SIM_RADAR_CHANGE_IFF_ENCODE_MODE    = 0x00020000,
    SPX_SIM_RADAR_CHANGE_SCAN_MODE	    = 0x00040000,
    SPX_SIM_RADAR_CHANGE_BLIND_SECTOR	    = 0x00080000,
    SPX_SIM_RADAR_CHANGE_EARTH_CURVATURE    = 0x00100000,
    SPX_SIM_RADAR_CHANGE_ENABLED	    = 0x00200000,
    SPX_SIM_RADAR_CHANGE_PACKING	    = 0x00400000,
    SPX_SIM_RADAR_CHANGE_MUTE		    = 0x00800000,
    SPX_SIM_RADAR_CHANGE_PLATFORM	    = 0x01000000,
    SPX_SIM_RADAR_CHANGE_ALL		    = 0x7FFFFFFF
};

/* Target positioning. */
enum SPxSimRadarTgtPos
{
    SPX_SIM_RADAR_TGT_POS_CENTRE = 0,
    SPX_SIM_RADAR_TGT_POS_LEADING_EDGE = 1
};

/* IFF modes. */
enum SPxSimRadarIffMode
{
    SPX_SIM_RADAR_IFF_MODE_1,
    SPX_SIM_RADAR_IFF_MODE_2,
    SPX_SIM_RADAR_IFF_MODE_3A,
    SPX_SIM_RADAR_IFF_MODE_C
};

/* IFF encode mode. */
enum SPxSimRadarIffEncodeMode
{
    SPX_SIM_RADAR_IFF_ENCODE_MODE_NONE = 0,
    SPX_SIM_RADAR_IFF_ENCODE_MODE_TRIG = 1,
    SPX_SIM_RADAR_IFF_ENCODE_MODE_P1 = 2
};

/* Blind sector reference. */
enum SPxBlindSectorRef
{
    SPX_BLIND_SECTOR_REF_HEADING = 0,
    SPX_BLIND_SECTOR_REF_NORTH = 1
};


/* Forward declarations. */
class SPxRadarSimulator;
class SPxSimPlatform;
class SPxRIB;
class SPxSimMotion;
class SPxSimRadar;
class SPxSimRadarNoise;
struct SPxSimTargetRad;
class SPxSimTarget;
class SPxSimTerrain;
class SPxRadarTrack;
class SPxPacketSender;
class SPxSerialPort;
class SPxNetworkSendHttp;
struct SPxSimState;
struct SPxLatLong_tag;
struct SPxReturnHeader_tag;
struct SPxPacketTrackerStatus_tag;
struct SPxPacketTrackMinimal_tag;
struct SPxPacketTrackNormal_tag;
struct SPxPacketTrackExtended_tag;

/* Callback types. */
typedef SPxErrorCode (*SPxSimRadarChangeFn_t)(SPxSimRadar *radar,
					      UINT32 flags,
					      void *userArg);

/* Define our class. */
class SPxSimRadar : public SPxObj
{
public:
    typedef SPxErrorCode (*BlindSectorFn)(void *userArg, const char *name,
	double startDegs, double endDegs, SPxBlindSectorRef reference);
    typedef SPxErrorCode (*TargetNoiseFn)(void *userArg, const char *name);

    /* Public functions. */
    explicit SPxSimRadar(SPxRadarSimulator *radarSim, 
			 SPxSimPlatform *platform,
			 SPxRIB *rib, 
			 unsigned int index=0);
    virtual ~SPxSimRadar(void);

    virtual void SetActive(int active);

    SPxErrorCode GetIndex(unsigned int *index) const;
    SPxErrorCode GetRadarSim(SPxRadarSimulator **radarSim);

    /* Platform. */
    SPxErrorCode SetPlatformIndex(unsigned int platformIndex);
    SPxErrorCode GetPlatformIndex(unsigned int *platformIndex) const;
    SPxErrorCode SetPlatform(SPxSimPlatform *platform);
    SPxErrorCode GetPlatform(SPxSimPlatform **platform);

    /* Radar name. */
    SPxErrorCode SetRadarName(const char *radarName);
    SPxErrorCode GetRadarName(char *buffer, unsigned int bufLen) const;

    /* Radar video properties. */
    SPxErrorCode SetVideoType(SPxSimRadarVideoType videoType);
    SPxErrorCode GetVideoType(SPxSimRadarVideoType *videoType) const;
    SPxErrorCode SetPacking(UINT8 packing);
    SPxErrorCode GetPacking(UINT8 *packing) const;
    SPxErrorCode GetLUTFromChannel(SPxPackingChannel channel, SPxLUTtype *lutType) const;
    SPxErrorCode GetChannelFromLUT(SPxLUTtype lutType, SPxPackingChannel *channel) const;

    SPxErrorCode SetScanMode(SPxScanMode scanMode);
    SPxErrorCode GetScanMode(SPxScanMode *scanMode) const;
    SPxErrorCode SetSectorScanStartDegs(double sectorScanStartDegs);
    SPxErrorCode GetSectorScanStartDegs(double *sectorScanStartDegs) const;
    SPxErrorCode SetSectorScanEndDegs(double sectorScanEndDegs);
    SPxErrorCode GetSectorScanEndDegs(double *sectorScanEndDegs) const;

    /* Radar position. */
    SPxErrorCode SetPlatformRelative(int platformRelative);
    int IsPlatformRelative(void) const;
    SPxErrorCode SetOffsetXMetres(double offsetXMetres);
    SPxErrorCode GetOffsetXMetres(double *offsetXMetres) const;
    SPxErrorCode SetOffsetYMetres(double offsetYMetres);
    SPxErrorCode GetOffsetYMetres(double *offsetYMetres) const;
    SPxErrorCode SetOffsetHeightMetres(double offsetHeightMetres);
    SPxErrorCode GetOffsetHeightMetres(double *offsetHeightMetres) const;
    SPxErrorCode SetLatLong(const SPxLatLong_tag *latLong);
    SPxErrorCode GetLatLong(SPxLatLong_tag *latLong,
			    int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode SetHeightMetres(double heightMetres);
    SPxErrorCode GetHeightMetres(double *heightMetres) const;
    SPxErrorCode SetHeightAbove(SPxRadarSimHeight heightAbove);
    SPxErrorCode GetHeightAbove(SPxRadarSimHeight *heightAbove) const;
    SPxErrorCode GetHeightAboveSeaMetres(double *heightAboveSeaMetres,
					 int mutexAlreadyLocked=FALSE) const;

    /* Radar properties. */
    SPxErrorCode SetSenderID(UINT32 senderID);
    SPxErrorCode GetSenderID(UINT32 *senderID) const;
    SPxErrorCode SetNumSamples(unsigned int numSamples);
    SPxErrorCode GetNumSamples(unsigned int *numSamples) const;
    SPxErrorCode GetNumAzimuths(unsigned int *numAzimuths) const;
    SPxErrorCode SetPeriodSecs(double periodSecs);
    SPxErrorCode GetPeriodSecs(double *periodSecs) const;
    SPxErrorCode SetPrf(double prf);
    SPxErrorCode GetPrf(double *prf) const;
    SPxErrorCode SetStartRangeMetres(double startRangeMetres);
    SPxErrorCode GetStartRangeMetres(double *startRangeMetres) const;
    SPxErrorCode SetEndRangeMetres(double endRangeMetres);
    SPxErrorCode GetEndRangeMetres(double *endRangeMetres) const;
    SPxErrorCode SetSensitivity(double sensitivity);
    SPxErrorCode GetSensitivity(double *sensitivity) const;
    SPxErrorCode SetHorizBeamWidthDegs(double horizBeamWidthDegs);
    SPxErrorCode GetHorizBeamWidthDegs(double *horizBeamWidthDegs) const;
    SPxErrorCode SetMinBeamAngleDegs(double minBeamAngleDegs);
    SPxErrorCode GetMinBeamAngleDegs(double *minBeamAngleDegs) const;
    SPxErrorCode SetMaxBeamAngleDegs(double maxBeamAngleDegs);
    SPxErrorCode GetMaxBeamAngleDegs(double *maxBeamAngleDegs) const;
    SPxErrorCode SetPulseLengthUsecs(double pulseLengthUsecs);
    SPxErrorCode GetPulseLengthUsecs(double *pulseLengthUsecs) const;
    SPxErrorCode SetMuted(int muted);
    int IsMuted(void) const;

    /* Blind sectors. */
    SPxErrorCode SetBlankAuxInBlindSector(int blankAuxInBlindSector);
    int IsBlankingAuxInBlindSector(void) const;
    SPxErrorCode SetBlindSector(const char *name,
	double startDegs, double endDegs, SPxBlindSectorRef reference);
    SPxErrorCode GetBlindSector(const char *name,
	double *startDegs, double *endDegs, SPxBlindSectorRef *reference) const;
    SPxErrorCode IterateBlindSectors(BlindSectorFn fn, void *userArg) const;
    int IsBlindSector(const char *name) const;
    int IsInBlindSector(unsigned int aziNum, int isMutexLocked=FALSE) const;
    int IsInBlindSector(double bearingDegs) const;

    /* Target position noise. */
    SPxErrorCode SetTargetNoiseDist(const char *name, SPxSimTargetNoise::Distribution dist);
    SPxErrorCode GetTargetNoiseDist(const char *name, SPxSimTargetNoise::Distribution *dist) const;
    SPxErrorCode SetRangeNoiseStdDev(const char *name, double rangeNoiseMetres);
    SPxErrorCode GetRangeNoiseStdDev(const char *name, double *rangeNoiseMetres) const;
    SPxErrorCode SetAziNoiseStdDev(const char *name, double aziNoiseDegs);
    SPxErrorCode GetAziNoiseStdDev(const char *name, double *aziNoiseDegs) const;
    SPxErrorCode SetRangeNoiseMean(const char *name, double rangeNoiseMean);
    SPxErrorCode GetRangeNoiseMean(const char *name, double *rangeNoiseMean) const;
    SPxErrorCode SetAziNoiseMean(const char *name, double aziNoiseMean);
    SPxErrorCode GetAziNoiseMean(const char *name, double *aziNoiseMean) const;
    SPxErrorCode RemoveTargetNoise(const char *name);
    SPxErrorCode IterateTargetNoise(TargetNoiseFn fn, void *userArg) const;
    int IsTargetNoise(const char *name) const;
    SPxErrorCode SetTargetNoiseSeed(const char *name, const UINT32 *seeds, unsigned int numSeeds);
    SPxErrorCode GetTargetNoiseSeed(const char *name, UINT32 *seeds, unsigned int numSeeds) const;

    /* Target size noise. */
    SPxErrorCode SetLengthNoiseStdDev(double lengthNoiseMetres);
    SPxErrorCode GetLengthNoiseStdDev(double *lengthNoiseMetres) const;
    SPxErrorCode SetBreadthNoiseStdDev(double breadthNoiseMetres);
    SPxErrorCode GetBreadthNoiseStdDev(double *breadthNoiseMetres) const;
    SPxErrorCode SetTargetSizeNoiseSeed(const UINT32 *seeds, unsigned int numSeeds);
    SPxErrorCode GetTargetSizeNoiseSeed(UINT32 *seeds, unsigned int numSeeds) const;

    /* Advanced configuration. */
    SPxErrorCode UseSlantRange(int useSlantRange);
    int IsUsingSlantRange(void) const;
    SPxErrorCode UseEarthCurvature(int useEarthCurvature);
    int IsUsingEarthCurvature(void) const;
    SPxErrorCode SetRampHorizTargetIntensity(int rampHorizTargetIntensity);
    int IsRampingHorizTargetIntensity(void) const;
    SPxErrorCode SetRampVertTargetIntensity(int rampVertTargetIntensity);
    int IsRampingVertTargetIntensity(void) const;
    SPxErrorCode UseFractionalPosition(int enableFractionalPosition);
    int IsUsingFractionalPosition(void) const;
    SPxErrorCode SetTargetPosition(SPxSimRadarTgtPos targetPosition);
    SPxErrorCode GetTargetPosition(SPxSimRadarTgtPos *targetPosition) const;
    SPxErrorCode SetAntiClockwise(int antiClockwise);
    int IsAntiClockwise(void) const;
    SPxErrorCode SetHiddenObscured(int hiddenObscured);
    int IsHiddenObscured(void) const;

    /* Visibility map. */
    SPxErrorCode SetVisibilityMapHeightMetres(float visibilityMapHeightMetres);
    SPxErrorCode SetVisibilityMapHeightMetres(float *visibilityMapHeightMetres, unsigned int numHeights);
    SPxErrorCode GetVisibilityMapHeightMetres(float *visibilityMapHeightMetres) const;
    SPxErrorCode GetVisibilityMapHeightMetres(float *visibilityMapHeightMetres, unsigned int *numHeights) const;
    SPxErrorCode SetVisibilityMapHeightAbove(SPxRadarSimHeight visibilityMapHeightAbove);
    SPxErrorCode GetVisibilityMapHeightAbove(SPxRadarSimHeight *visibilityMapHeightAbove) const;

    /* IFF. */
    SPxErrorCode SetIffInterlace(const SPxSimRadarIffMode *modes, unsigned int numModes);
    SPxErrorCode GetIffInterlace(SPxSimRadarIffMode *modes, unsigned int *numModes) const;
    SPxErrorCode SetIffInterlace(const char *modes);
    SPxErrorCode GetIffInterlace(char *buffer, unsigned int bufLen) const;
    SPxErrorCode SetIffEncodeMode(SPxSimRadarIffEncodeMode iffEncodeMode);
    SPxSimRadarIffEncodeMode GetIffEncodeMode(void) const;

    /* MTI video. */
    SPxErrorCode SetMinMtiSpeedMps(double minMtiSpeedMps);
    SPxErrorCode GetMinMtiSpeedMps(double *minMtiSpeedMps) const;
    SPxErrorCode SetMaxMtiSpeedMps(double maxMtiSpeedMps);
    SPxErrorCode GetMaxMtiSpeedMps(double *maxMtiSpeedMps) const;

    /* Configure track output. */
    SPxErrorCode EnableTrackOutput(int enable);
    int IsTrackOutputEnabled(void) const;
    SPxErrorCode SetTrackReportType(UINT16 trackReportType);
    SPxErrorCode GetTrackReportType(UINT16 *trackReportType) const;
    SPxErrorCode SetTrackReportExtFlags(UINT32 extFlags);
    SPxErrorCode GetTrackReportExtFlags(UINT32 *extFlags) const;
    SPxErrorCode EnableAsterixOutput(int enable);
    int IsAsterixOutputEnabled(void) const;
    SPxErrorCode SetAsterixReportType(UINT16 asterixReportType);
    SPxErrorCode GetAsterixReportType(UINT16 *asterixReportType) const;
    SPxErrorCode SetAsterixFields(UINT16 reportType, UINT32 asterixFields);
    SPxErrorCode GetAsterixFields(UINT16 reportType, UINT32 *asterixFields) const;
    SPxErrorCode SetAsterixPeriodicIntervalMS(unsigned int ms);
    SPxErrorCode GetAsterixPeriodicIntervalMS(unsigned int *ms) const;
    SPxErrorCode SetTTMOutputType(SPxMesgIOType_t ttmOutputType);
    SPxErrorCode GetTTMOutputType(SPxMesgIOType_t *ttmOutputType) const;
    SPxErrorCode SetMaxTrackCoasts(unsigned int maxCoasts);
    SPxErrorCode GetMaxTrackCoasts(
	unsigned int *maxTrackCoasts) const;

    SPxErrorCode EnableDynamicLoadControl(int enable);
    int IsDynamicLoadControlEnabled(void) const;

    /* Internal objects and properties. */
    SPxErrorCode GetNoise(SPxSimRadarNoise **noise);
    SPxErrorCode GetNoise(SPxSimRadarNoise const **noise) const;
    SPxErrorCode GetBlindSectorNoise(SPxSimRadarNoise **blindSectorNoise);
    SPxErrorCode GetBlindSectorNoise(SPxSimRadarNoise const **blindSectorNoise) const;
    SPxErrorCode GetMtiNoise(SPxSimRadarNoise **mtiNoise);
    SPxErrorCode GetMtiNoise(SPxSimRadarNoise const **mtiNoise) const;
    SPxErrorCode GetSimTerrain(SPxSimTerrain **simTerrain);
    SPxErrorCode GetSimTerrain(SPxSimTerrain const **simTerrain) const;
    SPxErrorCode GetTrackSender(SPxPacketSender **trackSender);
    SPxErrorCode GetAsterixSender(SPxPacketSender **asterixSender);
    SPxErrorCode GetTTMSenderNet(SPxPacketSender **ttmSenderNet);
    SPxErrorCode GetTTMSenderSerial(SPxSerialPort **ttmSenderSerial);
    SPxErrorCode ResetNumRevs(void);
    unsigned int GetNumRevs(void) const;
    SPxErrorCode GetTargetIntensity(const SPxSimTarget *target, double *intensity) const;
    SPxErrorCode GetTargetVisibility(const SPxSimTarget *target, SPxSimTargetVis *targetVis) const;
    SPxErrorCode GetTargetPosition(const SPxSimTarget *target,
				   double *rangeMetres,
				   double *bearingDegs) const;
    unsigned int GetPulseSamples(void) const;
    int AreRangeAndPrfValid(void) const;

    /* Change handlers. */
    SPxErrorCode AddChangeFn(SPxSimRadarChangeFn_t fn, UINT32 flags, void *userArg);
    SPxErrorCode RemoveChangeFn(SPxSimRadarChangeFn_t fn, UINT32 flags, void *userArg);

    /* Set/get parameter interface. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);
    virtual SPxErrorCode SetConfigFromState(void);

protected:
    /* Testing constructor. */
    explicit SPxSimRadar(SPxRadarSimulator *radarSim, SPxSimPlatform *platform, 
			 SPxRIB *rib, unsigned int index,
			 SPxPacketSender *trackSender, SPxPacketSender *asterixSender,
			 SPxPacketSender *ttmSender, SPxNetworkSendHttp *netSendHttp);

    /* Called by SPxRadarSimulator. */
    virtual SPxErrorCode Update(UINT64 rotateMsecs);
    virtual SPxErrorCode UpdateTargets(int tgtTimeUpdateRequired);

private:
    /* Private types. */
    enum SectorScanDirection
    {
	SECTOR_FORWARDS,
	SECTOR_BACKWARDS
    };

    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Called by SPxRadarSimulator. */
    SPxErrorCode SetCurrentTimeSecs(double currentTimeSecs);
    SPxErrorCode AddTarget(const SPxSimTarget *target);
    SPxErrorCode RemoveTarget(const SPxSimTarget *target, UINT32 id);
    SPxErrorCode RemoveRadarSim(void);
    SPxErrorCode BuildTrackReport(const SPxSimTarget *target,
	UINT16 trackReportType, UINT32 trackReportExtFlags,
	int deleted, int vidSimMessage,
	SPxPacketTrackExtended *tgtExtRpt) const;
    void ApplyDesc(const char *buffer);
    const char *GetDesc(void) const;
    void SetFile(const char *filename);
    int WasFromFile(const char *filename);
    double GetRadialTargetSpeedMps(double tgtBearingRads,
	double tgtSpeedMps, double tgtCourseDegs) const;
 
    /* Called by SPxRadarSimServer. */
    SPxErrorCode SetStandby(int standby);
    int GetStandby(void) const;
    SPxErrorCode Update(SPxReturnHeader_tag *header);

    /* Called by SPxSimRadarNoise. */
    SPxErrorCode CallChangeFns(UINT32 flags);

    /* Private functions. */
    void initialise(void);
    SPxErrorCode getLatLong(SPxLatLong_tag *latLong) const;
    SPxErrorCode update(UINT16 timeInterval, UINT8 numTriggers,
	float azi, UINT8 pimFlags);
    unsigned int getSectorScanAzi(double degs) const;
    void incrementAzimuth(unsigned int azi,
	unsigned int *lastAzimuthPtr, unsigned int *nextAzimuthPtr,
	SectorScanDirection *sectorScanDirPtr);
    SPxErrorCode updateTargetState(const SPxSimTarget *target, 
	SPxSimTargetRad &tgtRad);
    SPxErrorCode updateRadarOffset(void);
    SPxErrorCode resizeReturnDataBuffers(void);
    SPxErrorCode fillReturnHeader(void);
    SPxErrorCode buildDataForReturn(unsigned int azimuth);
    SPxErrorCode buildPrimaryReturn(unsigned int azimuth, unsigned int beamwidthAzi,
	SPxPackingChannel channel, int isFirst, int isLast);
    SPxErrorCode buildIffReturn(unsigned int azimuth, SPxPackingChannel channel);
    SPxErrorCode buildVisMapReturn(unsigned int azimuth, SPxPackingChannel channel);
    SPxErrorCode buildMtiReturn(unsigned int azimuth,
	SPxPackingChannel channel, int isFirst, int isLast);
    SPxErrorCode buildTargetData(SPxSimRadarVideoType videoType, SPxPackingChannel channel,
	int isFirst, int isLast);
    SPxErrorCode applyPulseLength(SPxPackingChannel channel);
    double samplesFromUS(double microseconds) const;
    REAL32 getSlantRange(const SPxSimState &state, double distMetres) const;
    SPxErrorCode drawTarget(const SPxSimTarget *target, SPxSimTargetRad &tgtRad,
	SPxSimRadarVideoType videoType, SPxPackingChannel channel, int isFirst, int isLast);
    SPxErrorCode drawPrimaryOrMtiTarget(const SPxSimTarget *target, SPxSimTargetRad &tgtRad,
	SPxPackingChannel channel, int isMti, int isFirst, int isLast);
    SPxErrorCode drawIffTarget(const SPxSimTarget *target, SPxSimTargetRad &tgtRad, SPxPackingChannel channel);
    SPxErrorCode drawIffTargetSpoke(double sample, UINT16 code, int mode1=FALSE, int spi=FALSE, int x=FALSE,
	SPxPackingChannel channel=SPX_PACKING_CHAN_A);
    SPxErrorCode drawIffMode(void);
    SPxErrorCode drawRaconTarget(double sample, char code, SPxPackingChannel channel);
    SPxErrorCode drawSart(double sample, double pulseUsecs, SPxPackingChannel channel);
    SPxSimRadarIffMode getIffMode(void) const;
    double getIffModeWidth(void) const;
    SPxErrorCode addReturnDataNoSpread(SPxPackingChannel channel);
    SPxErrorCode calcPolarIntervals(const SPxSimTarget *target, SPxSimTargetRad &tgtRad);
    SPxErrorCode outputTrackReports(const SPxSimTarget *target, SPxSimTargetRad &tgtRad, int deleted=FALSE);
    SPxErrorCode outputTrack(const SPxSimTarget *target, SPxSimTargetRad &tgtRad, int deleted);
    SPxErrorCode outputTrackerStatus(void);
    SPxErrorCode outputAsterixTrack(const SPxSimTarget *target, SPxSimTargetRad &tgtRad, int deleted);
    SPxErrorCode outputAsterixTrackerStatus(int periodic);
    SPxErrorCode outputTTMTrack(const SPxSimTarget *target, SPxSimTargetRad &tgtRad, int deleted);
    void buildTrackerStatus(SPxPacketTrackerStatus_tag *msg, UINT16 numTracks);
    SPxErrorCode buildTrackReport(const SPxSimTarget *target,
        SPxSimTargetRad &tgtRad,
	UINT16 trackReportType, UINT32 trackReportExtFlags,
	int deleted, int vidSimMessage, SPxPacketTrackExtended *tgtExtRpt) const;
    void buildTrackReportMin(SPxPacketTrackMinimal_tag *min, const SPxSimTarget *target, 
			     const SPxSimState& state, double bearingRads, double distMetres,
			     UINT32 numTargetSamples, UINT32 strength, unsigned int numCoasts,
			     UINT8 ttmID, int deleted, int vidSimMessage,
			     const SPxPacketTrackExtended *tgtExtRpt=NULL) const;
    void buildTrackReportNorm(SPxPacketTrackNormal_tag *norm, const SPxSimTarget *target, 
			      const SPxSimState& state, double bearingRads, double distMetres,
			      UINT32 numTargetSamples, UINT32 strength, unsigned int numCoasts,
			      UINT8 ttmID, int deleted, int vidSimMessage,
			      const SPxPacketTrackExtended *tgtExtRpt=NULL) const;
    void buildTrackReportExt(SPxPacketTrackExtended_tag *ext, UINT32 extFlags,
			     const SPxSimTarget *target, SPxSimTargetRad& tgtRad,
			     const SPxSimState& state, double bearingRads, double distMetres,
			     UINT32 numTargetSamples, UINT32 strength, unsigned int numCoasts,
			     UINT8 ttmID, int deleted, int vidSimMessage, UINT32 ageScans,
			     const SPxPacketTrackExtended *tgtExtRpt=NULL) const;
    int isInBlindSector(unsigned int aziNum) const;
    int isInBlindSector(double bearingDegs) const;
    void deleteStarredParam(const char *paramName, const char *sectorName);
    double getRadialTargetSpeedMps(double tgtBearingRads,
	double tgtSpeedMps, double tgtCourseDegs) const;
    UINT8 getTTMID(const SPxSimTarget *target);
    void freeTTMID(UINT8 id);

    /* Utility functions. */
    inline unsigned int aziAdd(unsigned int a1, unsigned int a2);
    inline unsigned int aziSub(unsigned int a1, unsigned int a2);
    inline unsigned int aziDiff(unsigned int a1, unsigned int a2);
    inline int aziOverlap(unsigned int a1, unsigned int a2,
	unsigned int b1, unsigned int b2);
    inline int aziOverlap(unsigned int a1, unsigned int a2,
	unsigned int b1);

    friend class SPxRadarSimulator;
    friend class SPxRadarSimServer;
    friend class SPxSimRadarNoise;

}; /* SPxSimRadar. */

#endif /* _SPX_SIM_RADAR_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
