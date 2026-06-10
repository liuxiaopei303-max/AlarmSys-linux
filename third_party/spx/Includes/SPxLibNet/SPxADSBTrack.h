/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxADSBTrack.h,v $
* ID: $Id: SPxADSBTrack.h,v 1.12 2017/07/13 14:01:14 rew Exp $
*
* Purpose:
*	Header for SPxADSBTrack class which keeps information on
*	targets received via ADSB.
*
*
* Revision Control:
*   13/07/17 v1.12   SP 	Add Set/GetUserData().
*
* Previous Changes:
*   31/07/15 1.11   AGC	Add on ground and fixed support.
*   14/10/14 1.10   AGC	Add test bit support.
*   18/07/14 1.9    AGC	Add simulated bit support.
*   04/07/14 1.8    AGC	Add mode 3/A support.
*   21/01/14 1.7    REW	Clear flags if not lost or not deleted.
*   15/10/13 1.6    AGC	Fix warnings from SPxLibAVMfc.
*   04/10/13 1.5    AGC	Simplify headers.
*   03/05/13 1.4    AGC	Support manually creating SPxADSBTrack objects.
*			Add high res flag to altitude.
*			Support configurable number of history points.
*   28/09/12 1.3    REW	Support creation time.
*   02/01/12 1.2    REW	Support vertical rate.
*   23/12/11 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_ADSB_TRACK_H
#define _SPX_ADSB_TRACK_H

/*
 * Other headers required.
 */
/* We need SPxLibUtils for common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxTime.h"
#include "SPxLibUtils/SPxObj.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Flags for why callbacks are being invoked. */
#define	SPX_ADSB_TRACK_FLAGS_CREATED	0x00000001	/* Created track */
#define	SPX_ADSB_TRACK_FLAGS_LOST	0x00000002	/* Lost track */
#define	SPX_ADSB_TRACK_FLAGS_DELETED	0x00000004	/* Deleted track */
#define	SPX_ADSB_TRACK_FLAGS_POSITION	0x00000010	/* New position */
#define	SPX_ADSB_TRACK_FLAGS_VELOCITY	0x00000020	/* New velocity */
#define	SPX_ADSB_TRACK_FLAGS_ALTITUDE	0x00000040	/* New altitude */
#define	SPX_ADSB_TRACK_FLAGS_VERTICALRATE 0x000080	/* New vertical rate */
#define	SPX_ADSB_TRACK_FLAGS_CAPABILITIES 0x000100	/* New value */
#define	SPX_ADSB_TRACK_FLAGS_CALLSIGN	0x00000200	/* New callsign */
#define SPX_ADSB_TRACK_FLAGS_MODE3A	0x00000400	/* New mode 3/A code */
#define SPX_ADSB_TRACK_FLAGS_SIMULATED	0x00000800	/* Simulated bit change */
#define SPX_ADSB_TRACK_FLAGS_TEST	0x00001000	/* Test bit change */
#define SPX_ADSB_TRACK_FLAGS_ON_GROUND	0x00002000	/* On ground bit change */
#define SPX_ADSB_TRACK_FLAGS_FIXED	0x00004000	/* Fixed transponder bit change */

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

/* Forward-declare other classes we use in case headers are in wrong order. */
class SPxADSBDecoder;

/* Define a history point. */
typedef struct SPxADSBTrackHistoryPt_tag {
    /* Timestamp. */
    SPxTime_t timestamp;

    /* Position. */
    double latDegs;		/* Latitude, degrees positive north */
    double longDegs;		/* Longitude, degrees positive east */
} SPxADSBTrackHistoryPt_t;


/*
 * Define our class.
 */
class SPxADSBTrack :public SPxObj
{
/* The decoder class can access our internals directly. */
friend class SPxADSBDecoder;

public:
    /*
     * Public functions.
     */
    /* Note that the constructor is not public, so the
     * class can only be instantiated by the SPxADSBDecoder.
     */
    static SPxADSBTrack *Create(unsigned int aircraftAddress,
				double latDegs, double longDegs,
				double speedMps, double courseDegs,
				double altitudeMetres=0.0, int isAltitudeValid=FALSE,
				double verticalRateMps=0.0, int isVerticalRateValid=FALSE,
				unsigned int capabilities=0,
				const char *callsign=NULL,
				unsigned int category=0);
    virtual ~SPxADSBTrack(void);

    /*
     * Value access functions.
     */
    UINT32 GetAA(void) const		{ return(m_aa); }
    SPxADSBDecoder *GetDecoder(void) const { return(m_parentDecoder); }
    SPxErrorCode GetCreationTime(SPxTime_t *timestampPtr,
				double *ageSecsPtr) const;

    /* Dynamic values. */
    SPxErrorCode GetPosition(double *latDegsPtr, double *longDegsPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetVelocity(double *speedMpsPtr, double *courseDegsPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetAltitude(double *altitudeMetresPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0,
				int *isHighRes=NULL) const;
    SPxErrorCode GetVerticalRate(double *rateMpsPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetCapabilities(unsigned int *capabilitiesPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    static const char *GetStringForCapabilities(unsigned int capabilities);
    const char *GetCallsign(SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL) const;
    SPxErrorCode GetCategory(unsigned int *categoryPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    static const char *GetStringForCategory(unsigned int category);

    /* Convenience function to get composite information about a track. */
    SPxErrorCode GetDescriptiveString(char *buf, int bufSize);

    /* History values. */
    unsigned int GetHistory(const SPxADSBTrackHistoryPt_t **bufferPtr,
				double desiredSeconds=0.0,
				unsigned int desiredPoints=0);
    void ClearHistory(void)	{ m_historyPtsNumUsed = 0; }

    /*
     * Functions relating to the status of the track.
     */
    SPxErrorCode SetIsActivated(int isActivated)
    {
	m_isActivated = (isActivated ? TRUE : FALSE);
	return(SPX_NO_ERROR);
    }
    SPxErrorCode SetIsSelected(int isSelected)
    {
	m_isSelected = (isSelected ? TRUE : FALSE);
	return(SPX_NO_ERROR);
    }
    SPxErrorCode SetCanBeDeleted(int canBeDeleted)
    {
	m_canBeDeleted = (canBeDeleted ? TRUE : FALSE);
	if( m_canBeDeleted ) { m_flags |= SPX_ADSB_TRACK_FLAGS_DELETED; }
	else		     { m_flags &= ~SPX_ADSB_TRACK_FLAGS_DELETED; }
	return(SPX_NO_ERROR);
    }
    int GetIsActivated(void) const	{ return(m_isActivated); }
    int GetIsSelected(void) const	{ return(m_isSelected); }
    int GetCanBeDeleted(void) const	{ return(m_canBeDeleted); }
    int GetIsLost(void) const		{ return(m_isLost); }

    /* Additional track information. */
    SPxErrorCode SetMode3A(UINT16 mode3A, SPxTime_t *timestamp);
    SPxErrorCode GetMode3A(UINT16 *mode3APtr, SPxTime_t *timestampPtr=NULL,
		     double *ageSecsPtr=NULL,
		     unsigned int expirySecs=0) const;
    SPxErrorCode SetSimulated(int simulated, SPxTime_t *timestamp);
    SPxErrorCode GetSimulated(int *simulatedPtr, SPxTime_t *timestampPtr=NULL,
		     double *ageSecsPtr=NULL,
		     unsigned int expirySecs=0) const;
    SPxErrorCode SetTest(int test, SPxTime_t *timestamp);
    SPxErrorCode GetTest(int *testPtr, SPxTime_t *timestampPtr=NULL,
		     double *ageSecsPtr=NULL,
		     unsigned int expirySecs=0) const;
    SPxErrorCode SetOnGround(int onGround, SPxTime_t *timestamp);
    SPxErrorCode GetOnGround(int *onGroundPtr, SPxTime_t *timestampPtr=NULL,
		     double *ageSecsPtr=NULL,
		     unsigned int expirySecs=0) const;
    SPxErrorCode SetFixed(int fixed, SPxTime_t *timestamp);
    SPxErrorCode GetFixed(int *fixedPtr, SPxTime_t *timestampPtr=NULL,
		     double *ageSecsPtr=NULL,
		     unsigned int expirySecs=0) const;

    /* Flags describing what has changed (for callbacks etc.). */
    UINT32 GetFlags(void) const		{ return(m_flags); }
    void ClearFlags(void)		{ m_flags = 0; }

    /* Functions that can be given to the SPxADSBDecoder::IterateTracks()
     * function to do certain actions on all tracks.
     */
    static int IteratorActivate(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxADSBTrack *)track)->SetIsActivated(TRUE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorDeactivate(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxADSBTrack *)track)->SetIsActivated(FALSE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorSelect(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxADSBTrack *)track)->SetIsSelected(TRUE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorDeselect(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxADSBTrack *)track)->SetIsSelected(FALSE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorClearHistory(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxADSBTrack *)track)->ClearHistory(); }
	return(SPX_NO_ERROR);
    }

    /* User data. */
    void SetUserData(void *ptr) { m_userData = ptr; }
    void *GetUserData(void) { return m_userData; }

protected:
    /*
     * Protected functions, likely to be used by SPxADSBDecoder.
     */
    /* Constructor. */
    SPxADSBTrack(SPxADSBDecoder *parent, unsigned int numHistoryPts=500);

    /* Value setting functions. */
    SPxErrorCode SetAA(UINT32 aa, SPxTime_t *timestamp);
    SPxErrorCode SetPosition(double latDegs, double longDegs,
				SPxTime_t *timestamp);
    SPxErrorCode SetVelocity(double speedMps, double courseDegs,
				SPxTime_t *timestamp);
    SPxErrorCode SetAltitude(double altitudeMetres,
				int isHighRes,
				SPxTime_t *timestamp);
    SPxErrorCode SetVerticalRate(double rateMps,
				SPxTime_t *timestamp);
    SPxErrorCode SetCapabilities(unsigned int capabilities,
				SPxTime_t *timestamp);
    SPxErrorCode SetCallsign(const char *callsign,
				SPxTime_t *timestamp);
    SPxErrorCode SetCategory(unsigned int category,
				SPxTime_t *timestamp);

    /*
     * Functions relating to the status of the track.
     */
    SPxErrorCode SetIsLost(int isLost)
    {
	m_isLost = (isLost ? TRUE : FALSE);
	if( m_isLost )	{ m_flags |= SPX_ADSB_TRACK_FLAGS_LOST; }
	else		{ m_flags &= ~SPX_ADSB_TRACK_FLAGS_LOST; }
	return(SPX_NO_ERROR);
    }

    /*
     * Protected variables.
     */
    /* Linked list pointer, used by parent. */
    SPxADSBTrack *m_next;		/* Next in list */

/*private*/public:
    /*
     * Private variables.
     */
    SPxADSBDecoder *m_parentDecoder;	/* Parent decoder */
    UINT32 m_aa;			/* Unique number */

    /* Flags for what's changed recently. */
    UINT32 m_flags;			/* SPX_ADSB_TRACK_FLAGS_... */

    /*
     * Dynamic data.
     */
    /* Creation time. */
    SPxTime_t m_creationTime;		/* Time of first report */

    /* Position. */
    double m_latDegs;			/* Latitude, +ve north */
    double m_longDegs;			/* Longitude, +ve east */
    SPxTime_t m_latLongTime;		/* Time of lat/long */

    /* Velocity. */
    double m_speedMps;			/* Speed in metres per second */
    double m_courseDegs;		/* Course in degs clockwise */
    SPxTime_t m_velocityTime;		/* Time of velocity */

    /* Altitude. */
    double m_altitudeMetres;		/* Altitude in metres */
    int m_isAltitudeHighRes;		/* Is the altitude low or high resolution? */
    SPxTime_t m_altitudeTime;		/* Time of value */

    /* Vertical rate. */
    double m_verticalRateMps;		/* Rate in metres/sec. +ve upwards. */
    SPxTime_t m_verticalRateTime;	/* Time of value */

    /* Capabilities. */
    unsigned int m_capabilities;	/* 0 to 7 */
    SPxTime_t m_capabilitiesTime;	/* Time of value */

    /* Callsign */
    char m_callsign[9];			/* 8 character string */
    SPxTime_t m_callsignTime;		/* Time of value */

    /* Category. */
    unsigned int m_category;		/* Bits 7-4=cateogry, 0-3=value */
    SPxTime_t m_categoryTime;		/* Time of value */

    /* Additional information. */
    UINT16 m_mode3A;			/* IFF mode 3/A */
    SPxTime_t m_mode3ATime;		/* Time of value */
    int m_simulated;			/* Simulated? */
    SPxTime_t m_simulatedTime;		/* Time of value */
    int m_test;				/* Test target? */
    SPxTime_t m_testTime;		/* Time of value */
    int m_onGround;			/* On ground target? */
    SPxTime_t m_onGroundTime;		/* Time of value */
    int m_fixed;			/* Fixed transponder? */
    SPxTime_t m_fixedTime;		/* Time of value */

    /* Rendering control. */
    int m_isActivated;			/* True for actived, false sleeping */
    int m_isSelected;			/* True for selected. */
    int m_canBeDeleted;			/* True if can be deleted */
    int m_isLost;			/* True if lost */

    /* History points. */
    SPxADSBTrackHistoryPt_t *m_historyPts;	/* Buffer for points */
    unsigned int m_historyPtsSize;		/* Size of buffer, in points */
    unsigned int m_historyPtsNumUsed;		/* Number of stored points */

    /* User data. */
    void *m_userData;

    /*
     * Private functions.
     */
    SPxErrorCode clearValues(SPxTime_t *timestamp);

}; /* SPxADSBTrack */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_ADSB_TRACK_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
