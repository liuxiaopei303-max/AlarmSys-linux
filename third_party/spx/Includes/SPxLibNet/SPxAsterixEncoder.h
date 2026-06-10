/*********************************************************************
*
* (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAsterixEncoder.h,v $
* ID: $Id: SPxAsterixEncoder.h,v 1.7 2014/09/15 14:45:15 rew Exp $
*
* Purpose:
*	Header for SPxAsterixEncoder module.
*
*	This module can be used to encode an Asterix message stream.
*
*
* Revision Control:
*   15/09/14 v1.7    REW	Add BuildCat4().
*
* Previous Changes:
*   26/06/14 1.6    REW	Break out BuildCat10Status().
*   21/01/14 1.5    AGC	Support user FSPECS for CAT10/48.
*			Improved information in CAT34 messages.
*   17/05/13 1.4    REW	Add BuildCat19() and BuildCat20().
*   03/05/13 1.3    AGC	Add BuildCat21().
*   30/04/13 1.2    REW	Add BuildCat48() and BuildCat34().
*   25/04/13 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_ASTERIX_ENCODER_H
#define _SPX_ASTERIX_ENCODER_H

/*
 * Other headers required.
 */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxError.h"

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

/* Forward declarations. */
struct SPxTime_tag;
struct SPxPacketTrackMinimal_tag;
struct SPxPacketTrackNormal_tag;
struct SPxPacketTrackExtended_tag;
class SPxADSBTrack;

/*
 * Define our class.
 */
class SPxAsterixEncoder : public SPxObj
{
public:
    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxAsterixEncoder(void);
    virtual ~SPxAsterixEncoder(void);

    /*
     * Fields that use needed for various messages.
     */
    /* SAC/SIC control. */
    void SetSAC(UINT8 sac)	{ m_sac = sac; }
    UINT8 GetSAC(void)		{ return(m_sac); }
    void SetSIC(UINT8 sic)	{ m_sic = sic; }
    UINT8 GetSIC(void)		{ return(m_sic); }

    /* Period. */
    void SetPeriod(double periodSecs)	{ m_periodSecs = periodSecs; }
    double GetPeriod(void)		{ return(m_periodSecs); }

    /* Sensor position. */
    void SetLatLong(double latDegs, double longDegs)
    {
	m_latDegs = latDegs;
	m_longDegs = longDegs;
    }
    void SetHeight(double heightMetres)	{ m_heightMetres = heightMetres; }
    double GetLat(void)			{ return(m_latDegs); }
    double GetLong(void)		{ return(m_longDegs); }
    double GetHeight(void)		{ return(m_heightMetres); }

    /*
     * Message creation.
     */
    unsigned int BuildCat4(unsigned char *buf, unsigned int bufSizeBytes,
				UINT32 fspec,
				const struct SPxTime_tag *timestamp,
				const struct SPxSafetyNetAlert_tag *alert)
				const;
    unsigned int BuildCat10(unsigned char *buf, unsigned int bufSizeBytes,
				UINT32 fspec,
				const struct SPxTime_tag *timestamp,
				const struct SPxPacketTrackMinimal_tag *min,
				const struct SPxPacketTrackNormal_tag *norm,
				const struct SPxPacketTrackExtended_tag *ext)
				const;
    unsigned int BuildCat10Status(unsigned char *buf, unsigned int bufSizeBytes,
				UINT32 fspec,
				const struct SPxTime_tag *timestamp,
				int msgType,	/* SPxAsterix10MsgType */
				int overload)	/* TRUE or FALSE */
				const;
    unsigned int BuildCat19(unsigned char *buf, unsigned int bufSizeBytes,
				const struct SPxTime_tag *timestamp,
				int msgType,	/* SPxAsterix19MsgType */
				int overload)	/* TRUE or FALSE */
				const;
    unsigned int BuildCat20(unsigned char *buf, unsigned int bufSizeBytes,
				const struct SPxTime_tag *timestamp,
				const struct SPxPacketTrackMinimal_tag *min,
				const struct SPxPacketTrackNormal_tag *norm,
				const struct SPxPacketTrackExtended_tag *ext,
				int polarValid, int cartesianValid,
				int velocityValid)
				const;
    unsigned int BuildCat21(unsigned char *buf, unsigned int bufSizeBytes,
				const struct SPxTime_tag *timestamp,
				const SPxADSBTrack *track) const;
    unsigned int BuildCat34(unsigned char *buf, unsigned int bufSizeBytes,
				const struct SPxTime_tag *timestamp,
				int msgType,	/* SPxAsterix34MsgType */
				int psr, int ssr,
				int overload,	/* TRUE or FALSE */
				unsigned int loadReductionState,
				unsigned int numPSR,
				unsigned int numSSR,
				unsigned int numPSSR,
				unsigned int numMisses,
				double sectorDegrees)
				const;
    unsigned int BuildCat48(unsigned char *buf, unsigned int bufSizeBytes,
				UINT32 fspec,
				const struct SPxTime_tag *timestamp,
				const struct SPxPacketTrackMinimal_tag *min,
				const struct SPxPacketTrackNormal_tag *norm,
				const struct SPxPacketTrackExtended_tag *ext,
				int velocityValid = TRUE,
				int typOverride = -1)
				const;

    /* Static functions. */
    static UINT32 Get128thSecsSinceMidnight(
				const struct SPxTime_tag *timestamp);

private:
    /*
     * Private variables.
     */
    /* SAC/SIC codes to use when building messages. */
    UINT8 m_sac;
    UINT8 m_sic;

    /* Period to use when building messages. */
    double m_periodSecs;

    /* Sensor position. */
    double m_latDegs;
    double m_longDegs;
    double m_heightMetres;

    /*
     * Private functions.
     */

}; /* SPxAsterixEncoder */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* _SPX_ASTERIX_ENCODER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
