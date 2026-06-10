/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadarNoise.h,v $
* ID: $Id: SPxSimRadarNoise.h,v 1.2 2016/03/03 10:53:36 rew Exp $
*
* Purpose:
*	Header for SPxSimRadarNoise object which generates noise for
*	a a simulated radar for use with the SPxRadarSimulator.
*
*
* Revision Control:
*   03/03/16 v1.2    AGC	Generate 16-bit noise.
*
* Previous Changes:
*   11/01/16 1.1    AGC	Initial version.
**********************************************************************/
#ifndef SPX_SIM_RADAR_NOISE_H_
#define SPX_SIM_RADAR_NOISE_H_

/*
 * Other headers required.
 */

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For base class. */
#include "SPxLibUtils/SPxObj.h"

/* For SPxPackingChannel. */
#include "SPxLibData/SPxRib.h"

/*
 * Types
 */

/* Forward declarations. */
class SPxSimRadar;

/* Define our class. */
class SPxSimRadarNoise : public SPxObj
{
public:
    /* Public functions. */
    explicit SPxSimRadarNoise(SPxSimRadar *radar,
	unsigned int numSamples,
	unsigned int numAzimuths, int supportNearNoise);
    virtual ~SPxSimRadarNoise(void);

    SPxErrorCode SetBackgroundNoiseLevel(double noiseLevel);
    SPxErrorCode GetBackgroundNoiseLevel(double *noiseLevel) const;
    SPxErrorCode EnableNearNoise(int enableNearNoise);
    int IsNearNoiseEnabled(void) const;
    SPxErrorCode SetNearNoiseGain(double nearNoiseGain);
    SPxErrorCode GetNearNoiseGain(double *nearNoiseGain) const;
    SPxErrorCode SetNearNoiseRangeMetres(double nearNoiseRangeMetres);
    SPxErrorCode GetNearNoiseRangeMetres(double *nearNoiseRangeMetres) const;
    SPxErrorCode SetAlwaysNearNoise(int alwaysNearNoise);
    int IsAlwaysNearNoise(void) const;
    SPxErrorCode SetSeaClutterDensity(double seaClutterDensity);
    SPxErrorCode GetSeaClutterDensity(double *seaClutterDensity) const;
    SPxErrorCode SetNearNoiseVariation(double nearNoiseVariation);
    SPxErrorCode GetNearNoiseVariation(double *nearNoiseVariation) const;

    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Functions called by SPxSimRadar. */
    SPxErrorCode SetNumSamples(unsigned int numSamples);
    SPxErrorCode SetNumAzimuths(unsigned int numAzimuths);
    SPxErrorCode ClearNearNoise(void);

    SPxErrorCode BuildNoise(unsigned char *buffer,
			    unsigned int numSamples,
			    UCHAR packing,
			    SPxPackingChannel channel);
    SPxErrorCode BuildNoise(unsigned int numSamples);
    SPxErrorCode BuildNearNoise(unsigned int numSamples,
	unsigned int bytesPerSample,
	double startRangeMetres, double endRangeMetres,
	unsigned int azi, double prf, double beamwidthAzi,
	double pulseLengthUsecs);
    const UINT16 *GetNoise(void) const;
    const UINT16 *GetNearNoise(void) const;
    unsigned int GetNearNoiseSamplesUsed(void) const;

    /* Private functions. */
    SPxErrorCode updateNearNoiseBlobs(unsigned int nearNoiseSamples,
	unsigned int blobWidth);

    friend class SPxSimRadar;

}; /* SPxSimRadarNoise. */

#endif /* SPX_SIM_RADAR_NOISE_H_ */

/*********************************************************************
*
* End of file
*
**********************************************************************/
