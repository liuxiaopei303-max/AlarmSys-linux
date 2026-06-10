/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAISTrack.h,v $
* ID: $Id: SPxAISTrack.h,v 1.12 2017/07/13 14:01:14 rew Exp $
*
* Purpose:
*	Header for SPxAISTrack class which keeps information on
*	targets received via AIS.
*
*
* Revision Control:
*   13/07/17 v1.12   SP 	Add Set/GetUserData().
*
* Previous Changes:
*   16/06/16 1.11   SP 	Correct prototype arg.
*   16/03/16 1.10   AJH	Support set/get for source address/port.
*   27/01/14 1.9    REW	Clear flags if not lost or not deleted.
*   15/10/13 1.8    AGC	Fix warnings from SPxLibAVMfc.
*   04/10/13 1.7    AGC	Simplify headers.
*   14/07/11 1.6    REW	Add ClearHistory() support.
*   13/07/11 1.5    REW	Avoid compiler warnings.
*   05/07/11 1.4    REW	Support history trails.
*			Correct order of args in iterator functions.
*   27/06/11 1.3    REW	Support many more fields etc.
*   28/01/11 1.2    REW	Support heading and dimension.
*   27/01/11 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_AIS_TRACK_H
#define _SPX_AIS_TRACK_H

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
#define	SPX_AIS_TRACK_FLAGS_CREATED	0x00000001	/* Created track */
#define	SPX_AIS_TRACK_FLAGS_LOST	0x00000002	/* Lost track */
#define	SPX_AIS_TRACK_FLAGS_DELETED	0x00000004	/* Deleted track */
#define	SPX_AIS_TRACK_FLAGS_POSITION	0x00000010	/* New position */
#define	SPX_AIS_TRACK_FLAGS_VELOCITY	0x00000020	/* New velocity */
#define	SPX_AIS_TRACK_FLAGS_HEADING	0x00000040	/* New heading */
#define	SPX_AIS_TRACK_FLAGS_ROT		0x00000080	/* New rate of turn */
#define	SPX_AIS_TRACK_FLAGS_NAVSTATUS	0x00000100	/* New nav status */
#define	SPX_AIS_TRACK_FLAGS_STATICDATA	0x00010000	/* New static data */

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
class SPxAISDecoder;

/* Define a history point. */
typedef struct SPxAISTrackHistoryPt_tag {
    /* Timestamp. */
    SPxTime_t timestamp;

    /* Position. */
    double latDegs;		/* Latitude, degrees positive north */
    double longDegs;		/* Longitude, degrees positive north */
} SPxAISTrackHistoryPt_t;


/*
 * Define our class.
 */
class SPxAISTrack :public SPxObj
{
/* The decoder class can access our internals directly. */
friend class SPxAISDecoder;

public:
    /*
     * Public functions.
     */
    /* Note that the constructor and destructor are not public, so the
     * class can only be instantiated by the SPxAISDecoder.
     */

    /*
     * Value access functions.
     */
    UINT32 GetMMSI(void) const		{ return(m_mmsi); }
    SPxAISDecoder *GetDecoder(void) const { return(m_parentDecoder); }

    /* Dynamic values. */
    SPxErrorCode GetPosition(double *latDegsPtr, double *longDegsPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetVelocity(double *speedMpsPtr, double *courseDegsPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetHeading(double *headingDegsPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetRateOfTurn(double *rotDegsPerMinPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetNavStatus(unsigned int *navStatusPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    const char *GetStringForNavStatus(unsigned int navStatus);

    /* Ship-static values. */
    const char *GetCallsign(SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL) const;
    const char *GetVesselName(SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL) const;
    const char *GetDestination(SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL) const;
    SPxErrorCode GetDimensions(unsigned int *metresToBowPtr,
				unsigned int *metresToSternPtr,
				unsigned int *metresToPortPtr,
				unsigned int *metresToStarboardPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetIMONumber(UINT32 *imoNumberPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetDraught(double *draughtMetresPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    SPxErrorCode GetShipCargoType(unsigned int *shipCargoTypePtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;
    const char *GetStringForShipCargoType(unsigned int shipCargoType);
    SPxErrorCode GetETA(UINT32 *etaPtr,
				unsigned int *etaMonthPtr,
				unsigned int *etaDayPtr,
				unsigned int *etaHourPtr,
				unsigned int *etaMinPtr,
				SPxTime_t *timestampPtr=NULL,
				double *ageSecsPtr=NULL,
				unsigned int expirySecs=0) const;

    /* Convenience function to get composite information about a track. */
    SPxErrorCode GetDescriptiveString(char *buf, int bufSize);

    /* History values. */
    unsigned int GetHistory(const SPxAISTrackHistoryPt_t **bufferPtr,
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
	if( m_canBeDeleted )	{ m_flags |= SPX_AIS_TRACK_FLAGS_DELETED; }
	else			{ m_flags &= ~SPX_AIS_TRACK_FLAGS_DELETED; }
	return(SPX_NO_ERROR);
    }
    int GetIsActivated(void) const	{ return(m_isActivated); }
    int GetIsSelected(void) const	{ return(m_isSelected); }
    int GetCanBeDeleted(void) const	{ return(m_canBeDeleted); }
    int GetIsLost(void) const		{ return(m_isLost); }

    /* Flags describing what has changed (for callbacks etc.). */
    UINT32 GetFlags(void) const		{ return(m_flags); }
    void ClearFlags(void)		{ m_flags = 0; }

    /* Functions that can be given to the SPxAISDecoder::IterateTracks()
     * function to do certain actions on all tracks.
     */
    static int IteratorActivate(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxAISTrack *)track)->SetIsActivated(TRUE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorDeactivate(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxAISTrack *)track)->SetIsActivated(FALSE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorSelect(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxAISTrack *)track)->SetIsSelected(TRUE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorDeselect(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxAISTrack *)track)->SetIsSelected(FALSE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorClearHistory(void * /*decoder*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxAISTrack *)track)->ClearHistory(); }
	return(SPX_NO_ERROR);
    }
    void GetSrcAddr(UINT32 *addr, int *port)
    {
	if (addr) { *addr = m_srcAddr; }
	if (port) { *port = m_srcPort; }
    }

    /* User data. */
    void SetUserData(void *ptr) { m_userData = ptr; }
    void *GetUserData(void) { return m_userData; }

protected:
    /*
     * Protected functions, likely to be used by SPxAISDecoder.
     */
    /* Constructor and destructor. */
    SPxAISTrack(SPxAISDecoder *parent);
    virtual ~SPxAISTrack(void);

    /* Value setting functions. */
    SPxErrorCode SetMMSI(UINT32 mmsi);
    SPxErrorCode SetPosition(double latDegs, double longDegs,
				SPxTime_t *timestamp);
    SPxErrorCode SetVelocity(double speedMps, double courseDegs,
				SPxTime_t *timestamp);
    SPxErrorCode SetHeading(double headingDegs,
				SPxTime_t *timestamp);
    SPxErrorCode SetRateOfTurn(double degsPerMin,
				SPxTime_t *timestamp);
    SPxErrorCode SetNavStatus(unsigned int navStatus,
				SPxTime_t *timestamp);
    SPxErrorCode SetShipStaticData(const char *callsign,
				const char *vesselName,
				const char *destination,
				unsigned int metresToBow,
				unsigned int metresToStern,
				unsigned int metresToPort,
				unsigned int metresToStarboard,
				double draughtMetres,
				UINT32 imoNumber,
				unsigned int shipCargoType,
				UINT32 eta,
				SPxTime_t *timestamp);

    /*
     * Functions relating to the status of the track.
     */
    SPxErrorCode SetIsLost(int isLost)
    {
	m_isLost = (isLost ? TRUE : FALSE);
	if( m_isLost )	{ m_flags |= SPX_AIS_TRACK_FLAGS_LOST; }
	else		{ m_flags &= ~SPX_AIS_TRACK_FLAGS_LOST; }
	return(SPX_NO_ERROR);
    }

    /*
     * Set/retrieve the track's source.
     */
    void SetSrcAddr(UINT32 addr, int port) { m_srcAddr = addr; m_srcPort = port; }

    /*
     * Protected variables.
     */
    /* Linked list pointer, used by parent. */
    SPxAISTrack *m_next;		/* Next in list */

/*private*/public:
    /*
     * Private variables.
     */
    SPxAISDecoder *m_parentDecoder;	/* Parent decoder */
    UINT32 m_mmsi;			/* Unique number */
    UINT32 m_srcAddr;			/* The sender network address */
    int m_srcPort;			/* The sender network port */

    /* Flags for what's changed recently. */
    UINT32 m_flags;			/* SPX_AIS_TRACK_FLAGS_... */

    /*
     * Dynamic data.
     */
    /* Position. */
    double m_latDegs;			/* Latitude, +ve north */
    double m_longDegs;			/* Longitude, +ve east */
    SPxTime_t m_latLongTime;		/* Time of lat/long */

    /* Velocity. */
    double m_speedMps;			/* Speed in metres per second */
    double m_courseDegs;		/* Course in degs clockwise */
    SPxTime_t m_velocityTime;		/* Time of velocity */

    /* Heading. */
    double m_headingDegs;		/* Heading in degs clockwise */
    SPxTime_t m_headingTime;		/* Time of heading */

    /* Rate of turn. */
    double m_rotDegsPerMin;		/* Rate in degs/min clockwise */
    SPxTime_t m_rotTime;		/* Time of rate of turn */

    /* Nav status. */
    unsigned int m_navStatus;		/* 0 to 15 */
    SPxTime_t m_navStatusTime;		/* Time of navStatus */

    /*
     * Ship static data.
     */
    char m_callsign[8];			/* 7 character string */
    char m_vesselName[21];		/* 20 character string */
    char m_destination[21];		/* 20 character string */
    unsigned int m_metresToBow;		/* Dimension, or 0 if not known */
    unsigned int m_metresToStern;	/* Dimension, or 0 if not known */
    unsigned int m_metresToPort;	/* Dimension, or 0 if not known */
    unsigned int m_metresToStarboard;	/* Dimension, or 0 if not known */
    double m_draughtMetres;		/* Draught in metres */
    UINT32 m_imoNumber;			/* IMO number */
    unsigned int m_shipCargoType;	/* Ship and cargo type */
    UINT32 m_eta;			/* ETA, 0xMMDDHHMM */
    SPxTime_t m_staticDataTime;		/* Time of static data */

    /* Rendering control. */
    int m_isActivated;			/* True for actived, false sleeping */
    int m_isSelected;			/* True for selected. */
    int m_canBeDeleted;			/* True if can be deleted */
    int m_isLost;			/* True if lost */

    /* History points. */
    SPxAISTrackHistoryPt_t *m_historyPts;	/* Buffer for points */
    unsigned int m_historyPtsSize;		/* Size of buffer, in points */
    unsigned int m_historyPtsNumUsed;		/* Number of stored points */

    /* User data. */
    void *m_userData;

    /*
     * Private functions.
     */
    SPxErrorCode clearValues(void);

}; /* SPxAISTrack */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_AIS_TRACK_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
