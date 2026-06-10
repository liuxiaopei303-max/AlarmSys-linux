/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTrackDatabase.h,v $
* ID: $Id: SPxTrackDatabase.h,v 1.29 2017/03/08 15:32:50 rew Exp $
*
* Purpose:
*   Header for SPxTrackDatabase class which supports the receipt
*   and handling of SPx track reports.
*
* Revision Control:
*   08/03/17 v1.29   SP 	Add SetRemoteServer().
*
* Previous Changes:
*   08/03/17 1.28   AGC	Improve mutex locking.
*   21/02/17 1.27   SP 	Add GetNextTrack().
*   23/08/16 1.26   AGC	Make GetNearest... functions const.
*   16/06/16 1.25   SP 	Support channel index for SQL database.
*   08/04/16 1.24   SP 	Add GetMaxTrackRangeMetres().
*   30/03/16 1.23   SP 	Support link with SQL database.
*   26/01/15 1.22   REW	Support link with SPxFlightPlan modules.
*   02/10/14 1.21   REW	Support label offsets.
*   02/10/14 1.20   REW	Add GetSensorLatLong().
*   24/09/14 1.19   REW	Add SetCalculateLatLongs().
*   12/09/14 1.18   REW	Add mutexAlreadyLocked to IterateTracks().
*   13/05/14 1.17   REW	Support link with NavData object.
*   17/02/14 1.16   SP 	Add GetLastTrackTimeMsecs() and
*			GetLastStatusTimeMsecs().
*   07/01/14 1.15   REW	Add GetTrack_InternalLocked().
*   07/10/13 1.14   SP 	Pass track data to CreateTrack().
*   05/09/13 1.13   AGC	Allow specifying desired track ID in AddTrack().
*   19/04/13 1.12   REW	Support SPxAsterixDecoder input.
*   23/11/12 1.11   AGC	Add ReportTrack/ReportTrackerStatus() functions.
*   26/10/12 1.10   AGC	Add manual track management functions.
*			Add GetNearestTrackLatLong() function.
*   04/04/12 1.9    SP 	Add track creation callback.
*   29/03/12 1.8    SP 	Rename SPxTrackReport as SPxRadarTrack.
*   25/10/11 1.7    SP 	Change deleted list to free list.
*                       Add CreateTrack().
*                       Add track update callbacks.
*   14/10/11 1.6    SP 	Add m_deletedTrackListHead.
*                       Remove SPxTrackInfo_t.
*   27/07/11 1.5    SP 	Return distance from GetNearestTrack().
*                       Remove ReleaseTrack().
*                       Remove ClearTrails().
*   13/07/11 1.4    SP 	Increase default num trails points to 500.
*   07/07/11 1.3    SP 	Add optional arg to set max trail pts.
*                       Use SPxTrackReport class.
*   26/04/11 1.2    SP 	General enhancements.
*   12/04/11 1.1    SP 	Initial Version.
**********************************************************************/

#ifndef _SPX_TRACK_DATABASE_H
#define _SPX_TRACK_DATABASE_H

/*
 * Other headers required.
 */

/* We need the SPxRemoteServer class. */
#include "SPxLibNet/SPxRemoteServer.h"

/* We need the SPxRadarTrack class. */
#include "SPxLibNet/SPxRadarTrack.h"

/* We need the SPxAsterixDecoder class. */
#include "SPxLibNet/SPxAsterixDecoder.h"

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


/* Need to forward-declare the class for use in the handler function type. */
class SPxTrackDatabase;
class SPxNavData;
class SPxFlightPlanDb;
class SPxSQLDatabase;

/*
 * Define our class, derived from the generic packet decoder.
 */
class SPxTrackDatabase : public SPxObj
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxTrackDatabase(unsigned int maxNumTrailPts=500);
    virtual ~SPxTrackDatabase(void);

    /* Create the database */
    SPxErrorCode Create(SPxRemoteServer *remoteServer);

    /* Set optional remote server track input. */
    SPxErrorCode SetRemoteServer(SPxRemoteServer *remoteServer);

    /* Add an optional Asterix input. */
    SPxErrorCode AddAsterixDecoder(SPxAsterixDecoder *asterixDecoder);

    /* Add an optional NavData object to pass radar lat/long to. */
    SPxErrorCode AddNavData(SPxNavData *navData);
    SPxErrorCode SetUseLatLongForNavData(int enable)
    {
	m_useLatLongForNavData = (enable ? TRUE : FALSE);
	return(SPX_NO_ERROR);
    }
    int GetUseLatLongForNavData(void) const { return(m_useLatLongForNavData); }

    /* Set an optional SPxFlightPlanDb database. */
    SPxErrorCode SetFlightPlanDb(SPxFlightPlanDb *fpdb);
    SPxFlightPlanDb *GetFlightPlanDb(void) { return(m_flightPlanDb); }

    /* Set an optional SQL database. */
    SPxErrorCode SetSQLDatabase(SPxSQLDatabase *sqlDB, UINT8 channelIndex=0);
    SPxSQLDatabase *GetSQLDatabase(void) { return m_sqlDB; } 

    /* Get the sensor lat/long, if known (returns error otherwise). */
    SPxErrorCode GetSensorLatLong(REAL32 *latDegsPtr, REAL32 *longDegsPtr);

    /* Control whether the database calculates target lat/longs from range/azi
     * if possible (i.e. if radar lat/long is known).
     */
    SPxErrorCode SetCalculateLatLongs(int enable)
    {
	m_calculateLatLongs = (enable ? TRUE : FALSE);
	return(SPX_NO_ERROR);
    }
    int GetCalculateLatLongs(void)	{ return(m_calculateLatLongs); }

    /* Read tracks from the database. If calling this from within a callback,
     * the mutexAlreadyLocked flag should be set to TRUE to avoid a deadlock,
     * but the caller is responsible for ensuring the invoked function behaves
     * appropriately and does not modify the database.
     */
    SPxErrorCode IterateTracks(SPxCallbackListFn_t trackFn,
				void *userArg, int mutexAlreadyLocked=FALSE);

    /* Configure the track database */
    SPxErrorCode SetSenderID(UINT8 senderID);
    UINT8 GetSenderID(void) { return m_senderID; }
    SPxErrorCode SetPurgeInterval(int interval);
    int GetPurgeInterval(void) { return m_purgeInterval; }

    /* Manual track management. */
    SPxRadarTrack *AddTrack(UINT32 id=0);
    SPxErrorCode RemoveTrack(UINT32 id);

    /* Track access functions */
    SPxRadarTrack *GetTrack(UINT32 id);
    SPxRadarTrack *GetNextTrack(UINT32 id, 
                                int nextHighest,
                                int allowWrap) const;
    SPxRadarTrack *GetNearestTrack(double xMetres, 
                                   double yMetres, 
                                   int filteredPos=TRUE,
                                   double *distMetresRtn=NULL) const;
    SPxRadarTrack *GetNearestTrackLatLong(double latDegs,
					  double longDegs,
					  int filteredPos=TRUE,
					  double *distMetresRtn=NULL,
					  unsigned int radarIndex=0,
					  double labelMetresPerPixel=0.0) const;
    double GetMaxTrackRangeMetres(void);

    /* Do not use the following function in normal situations. */
    SPxRadarTrack *GetTrack_InternalLocked(UINT32 id);

    /* Creation callback handling functions. The callback will be invoked with
     * a pointer to an SPxRadarTrack object whenever an object is created, but
     * the handle is only valid for the duration of the callback function.
     */
    SPxErrorCode AddCreationCallback(SPxCallbackListFn_t fn, void *userObj)
    {
	SPxErrorCode err = SPX_ERR_NOT_INITIALISED;
	if( m_creationCallbackList )
	{
	    err = (SPxErrorCode)
                m_creationCallbackList->AddCallback(fn, userObj);
	}
	return(err);
    }
    SPxErrorCode RemoveCreationCallback(SPxCallbackListFn_t fn, void *userObj)
    {
	SPxErrorCode err = SPX_ERR_NOT_INITIALISED;
	if( m_creationCallbackList )
	{
	    err = (SPxErrorCode)
                m_creationCallbackList->RemoveCallback(fn, userObj);
	}
	return(err);
    }

    /* Update callback handling functions. The callback will be invoked with
     * a pointer to an SPxRadarTrack object whenever a track is updated, but
     * the handle is only valid for the duration of the callback function.
     */
    SPxErrorCode AddCallback(SPxCallbackListFn_t fn, void *userObj)
    {
	SPxErrorCode err = SPX_ERR_NOT_INITIALISED;
	if( m_updateCallbackList )
	{
	    err = (SPxErrorCode)
                m_updateCallbackList->AddCallback(fn, userObj);
	}
	return(err);
    }
    SPxErrorCode RemoveCallback(SPxCallbackListFn_t fn, void *userObj)
    {
	SPxErrorCode err = SPX_ERR_NOT_INITIALISED;
	if( m_updateCallbackList )
	{
	    err = (SPxErrorCode)
                m_updateCallbackList->RemoveCallback(fn, userObj);
	}
	return(err);
    }

    /* Functions for reporting tracks directly from SPxMHTnetReport. */
    SPxErrorCode ReportTrack(struct SPxPacketTrackMinimal_tag *pMinRpt,
                             struct SPxPacketTrackNormal_tag *pNormRpt,
                             struct SPxPacketTrackExtended_tag *pExtRpt);
    SPxErrorCode ReportTrackerStatus(struct SPxPacketTrackerStatus_tag 
                                     *pStatusRpt);

    /* Get last message times. */
    UINT32 GetLastTrackTimeMsecs(void) { return m_lastTrackTimeMsecs; }
    UINT32 GetLastStatusTimeMsecs(void) { return m_lastStatusTimeMsecs; }

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    /*
     * Protected functions.
     */

private:
    /*
     * Private variables.
     */

    /* Sender ID used to filter track reports */
    UINT8 m_senderID;

    /* Remote server object used to supply track reports */
    SPxRemoteServer *m_remoteServer;

    /* Optional asterix decoder for track reports. */
    SPxAsterixDecoder *m_asterixDecoder;

    /* Optional nav data object to be given lat/long received from radar. */
    SPxNavData *m_navData;
    int m_useLatLongForNavData;	/* Flag to enable/disable its use. */

    /* Optional database of flight plans. */
    SPxFlightPlanDb *m_flightPlanDb;

    /* Optional SQL database to record track reports and tracker status. */
    SPxSQLDatabase *m_sqlDB;
    UINT8 m_sqlDBChannelIndex;

    /* Linked-list of active tracks maintained in ascending track ID order */
    SPxRadarTrack *m_trackListHead;
    SPxRadarTrack *m_trackListTail;

    /* Linked-list of deleted tracks that are free to be reused */
    SPxRadarTrack *m_freeTrackListHead;
    SPxRadarTrack *m_freeTrackListTail;

    /* Track linked-list mutex */
    mutable SPxCriticalSection m_mutex;

    /* Creation callback list, i.e. functions to invoke when new track
     * radar track object is created.
     */
    SPxCallbackList *m_creationCallbackList;

    /* Update callback list, i.e. functions to invoke with new reports. */
    SPxCallbackList *m_updateCallbackList;

    /* Interval after which non-updating tracks are purged */
    int m_purgeInterval;

    /* Max number of trail points to keep */
    unsigned int m_maxNumTrailPts;

    /* Do we use radar lat/long to calculate track lat/longs from rng/azi? */
    int m_calculateLatLongs;

    /* Last tracker status report */
    SPxPacketTrackerStatus_t m_lastTrackerStatus;

    /* Database maintenance thread */
    SPxThread *m_thread;

    /* Time last report received. */
    UINT32 m_lastTrackTimeMsecs;
    UINT32 m_lastStatusTimeMsecs;
   
    /*
     * Private functions.
     */

    /* Create and delete tracks */
    SPxRadarTrack *CreateTrack(const SPxTime_t *timestamp,
                               SPxPacketTrackMinimal *pMinRpt,
                               SPxPacketTrackNormal *pNormRpt,
                               SPxPacketTrackExtended *pExtRpt);
    SPxRadarTrack *DeleteTrack(SPxRadarTrack *trk);
    SPxRadarTrack *getTrackInternal(UINT32 id, int alreadyLocked);

    /*
     * Private static functions.
     */

    static void *databaseMaintenanceHandler(SPxThread *thread);

    static void trackHandler(SPxRemoteServer *svr,
                             void *userArg,
                             struct sockaddr_in *from,
                             SPxTime_t *timestamp,
                             struct SPxPacketTrackMinimal_tag *pMinRpt,
                             struct SPxPacketTrackNormal_tag *pNormRpt,
                             struct SPxPacketTrackExtended_tag *pExtRpt);

    static void trackerStatusHandler(SPxRemoteServer *svr,
                                     void *userArg,
                                     struct sockaddr_in *from,
                                     SPxTime_t *timestamp,
                                     struct SPxPacketTrackerStatus_tag 
                                     *pStatusRpt);

    static void asterixTrackHandler(const SPxAsterixDecoder *decoder,
				    void *userArg,
				    struct SPxPacketTrackExtended_tag *rpt);
    static void asterixStatusHandler(const SPxAsterixDecoder *decoder,
				    void *userArg,
				    struct SPxPacketTrackerStatus_tag *rpt);
    
}; /* SPxTrackDatabase */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_TRACK_DATABASE_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
