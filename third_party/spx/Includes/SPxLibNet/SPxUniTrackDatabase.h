/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxUniTrackDatabase.h,v $
 * ID: $Id: SPxUniTrackDatabase.h,v 1.5 2017/08/22 14:23:14 rew Exp $
 *
 * Purpose:
 *   Header for SPxUniTrackDatabase class which unifies support for
 *   multiple radar, fused, AIS and ADS-B track sources.
 *
 * Revision Control:
 *   22/08/17 v1.5   SP 	Remove unused functions.
 *                              Add GetTrack() and GetTracksInRange().
 *                              Add support for reference points.
 *
 * Previous Changes:
 *   31/07/17 1.4   SP 	Further development.
 *   24/07/17 1.3   SP 	Support fused tracks.
 *   18/07/17 1.2   SP 	Further development.
 *   13/07/17 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_UNI_TRACK_DATABASE_H
#define _SPX_UNI_TRACK_DATABASE_H

/*
 * Other headers required.
 */

/* Other classes that we need. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibNet/SPxUniTrack.h"

/* Forward declare any classes required. */
class SPxTrackDatabase;
class SPxAISDecoder;
class SPxADSBDecoder;
class SPxActiveRegion;
class SPxRefPoint;
class SPxGate;
class SPxProcessRegion;
class SPxShapeArea;

/*********************************************************************
 *
 *   Constants
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Macros
 *
 *********************************************************************/


/*********************************************************************
 *
 *   Type definitions
 *
 *********************************************************************/

/*
 * Define our class.
 */
class SPxUniTrackDatabase : public SPxObj
{
public:

    /*
     * Public types.     
     */

    /* Selection state. */
    typedef enum
    {
        STATE_SELECT,       /* Select a track. */
        STATE_DESELECT,     /* Deselect a track. */
        STATE_TOGGLE,       /* Toggle track selection. */
        STATE_SELECT_L1,    /* Level 1 user-defined selection. */
        STATE_SELECT_L2,    /* Level 2 user-defined selection. */
        STATE_SELECT_L3     /* Level 3 user-defined selection. */
    
    } State_t;

    /* Notification event type. */
    typedef enum
    {
        EVENT_TRACK_UPDATE,         /* Track data has been updated. */
        EVENT_TRACK_SELECT,         /* Track selected or deselected. */
        EVENT_TRACK_DELETE,         /* Track has been deleted. */
        EVENT_TRACK_ALARM_ENTER,    /* Track has entered an alarm region. */
        EVENT_TRACK_ALARM_EXIT,     /* Track has exited an alarm region. */
        EVENT_TRACK_ALARM_INSIDE,   /* Track is inside am alarm alarm. */

        EVENT_GATE_CROSS,           /* Gate boundary has been crossed. */
        EVENT_GATE_ALARM            /* Gate alarm triggered. */

    } EventType_t;

    /* Track event info. Used by:
     *  EVENT_TRACK_SELECT
     *  EVENT_TRACK_DELETE
     */
    typedef struct
    {
        SPxUniTrack *track;  /* Track handle or NULL. */
        State_t state;       /* Selection status. */

    } EventTrackInfo_t;

    /* Threat event info. Used by EVENT_TRACK_UPDATE. */
    typedef struct
    {
        SPxUniTrack *track;  /* Track handle. */
        UINT32 changeFlags;  /* What has changed. */
        char oldName[64];    /* Old name when changed. */

    } EventTrackUpdateInfo_t;

    /* Track alarm event info. Used by:
     *  EVENT_UNI_TRACK_ALARM_ENTER
     *  EVENT_UNI_TRACK_ALARM_EXIT
     *  EVENT_UNI_TRACK_ALARM_INSIDE
     *
     * Take care when using the area handle as this is only guaranteed to
     * be valid in the callback which the active region list is locked. 
     * Outside of the callback the area could be deleted. If in doubt then
     * use the area name and get the area as and when needed with the mutex
     * locked.
     */
    typedef struct
    {
        SPxUniTrack *track; /* Track handle. */
        SPxShapeArea *area; /* Alarm area handle (see note). */
        char areaName[256]; /* Alarm area name. */

    } EventTrackAlarmInfo_t;

    /* Gate event info. Used by:
     *  EVENT_GATE_CROSS
     *  EVENT_GATE_ALARM
     */
    typedef struct
    {
        SPxGate *gate;       /* Gate. */
        SPxUniTrack *track;  /* Track handle or NULL. */

    } EventGateInfo_t;

    /* Union of all event info structures. */
    typedef union
    {
        EventTrackInfo_t trackEvt;
        EventTrackUpdateInfo_t updateEvt;
        EventTrackAlarmInfo_t alarmEvt;
        EventGateInfo_t gateEvt;
    
    } EventInfo_t;

    /* Top-level event structure. Use type to determine the event type 
     * and then access one (and only one) of the event info structures 
     * that follow.
     */
    typedef struct
    {
        EventType_t type;   /* Event type. */
        EventInfo_t info;   /* Event info. */
    
    } Event_t;

    /* Used by GetTracksInRange(). */
    typedef struct
    {
        SPxUniTrack *track;     /* Track. */
        double distMetres;      /* Distance from point. */

    } TrackDist_t;

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxUniTrackDatabase(void);
    virtual ~SPxUniTrackDatabase(void);

    /* Add track sources. */
    virtual SPxErrorCode AddRadarTrackSource(SPxTrackDatabase *database);
    virtual int IsRadarTrackSourceInstalled(SPxTrackDatabase *database) const;
    virtual SPxErrorCode AddFusedTrackSource(SPxTrackDatabase *database);
    virtual int IsFusedTrackSourceInstalled(SPxTrackDatabase *database) const;
    virtual SPxErrorCode AddAISTrackSource(SPxAISDecoder *decoder);
    virtual int IsAISTrackSourceInstalled(SPxAISDecoder *decoder) const;
    virtual SPxErrorCode AddADSBTrackSource(SPxADSBDecoder *decoder);
    virtual int IsADSBTrackSourceInstalled(SPxADSBDecoder *decoder) const;

    /* Tracks. */
    unsigned int GetNumTracks(unsigned int *numRadarRtn, 
                              unsigned int *numAISRtn, 
                              unsigned int *numADSBRtn, 
                              unsigned int *numFusedRtn) const;

    virtual void SetSelectedTrack(SPxUniTrack *track, State_t state);
    virtual SPxUniTrack *GetSelectedTrack(void) const { return m_selectedTrack; }

    virtual SPxUniTrack *GetTrack(UINT32 id, 
                                  SPxUniTrack::TrackType_t type,
                                  int mutexAlreadyLocked=FALSE) const;

    virtual SPxUniTrack *GetNearestTrack(double latDegs,
                                         double longDegs,
                                         SPxUniTrack::TrackType_t types,
                                         double *distMetresRtn=NULL) const;

    virtual unsigned int GetTracksInRange(double latDegs,
                                          double longDegs,
                                          double maxDistMetres,
                                          SPxUniTrack::TrackType_t types,
                                          TrackDist_t *tracksRtn[]) const;

    virtual SPxErrorCode IterateTracksForward(SPxCallbackListFn_t fn, 
                                              void *userArg);
    virtual SPxErrorCode IterateTracksReverse(SPxCallbackListFn_t fn, 
                                              void *userArg);

    /* Gates. */
    virtual SPxErrorCode AddGate(SPxGate *gate);
    virtual SPxErrorCode RemoveGate(SPxGate *gate);
    virtual unsigned int GetNumGates(void) const;
    virtual SPxGate *GetGate(unsigned int index) const;
    virtual void SetGateAlarm(SPxGate *gate, int isAlarm);
    virtual void ClearGateAlarms(double minAgeSecs=0.0);
    virtual SPxGate *GetNearestGate(double latDegs,
                                    double longDegs,
                                    double *distMetresRtn) const;

    /* Alarm areas. */
    virtual SPxErrorCode AddAlarmArea(SPxShapeArea *area);
    virtual SPxErrorCode RemoveAlarmArea(SPxShapeArea *area);

    /* Reference points. */
    virtual SPxErrorCode AddRefPoint(SPxRefPoint *refPoint);
    virtual SPxErrorCode RemoveRefPoint(SPxRefPoint *refPoint);
    virtual unsigned int GetNumRefPoints(void) const;
    virtual SPxRefPoint *GetRefPoint(unsigned int index) const;
    virtual SPxRefPoint *GetNearestRefPoint(double latDegs,
                                            double longDegs,
                                            double *distMetresRtn) const;

    /* Event callbacks. */
    virtual SPxErrorCode AddEventCallback(SPxCallbackListFn_t fn, 
                                          void *userObj);

    virtual SPxErrorCode RemoveEventCallback(SPxCallbackListFn_t fn, 
                                             void *userObj);

    virtual SPxErrorCode ReportEvent(Event_t *evt);

    /*
     * Public static functions.
     */

    /* Area deletion callback. */
    static int AlarmAreaDeletionCallback(void *invokingObject, 
                                         void *userObject, 
                                         void *arg);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

private:

    /*
     * Private types.
     */

    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Event callback list. */
    SPxCallbackList *m_eventCBList;

    /* Selected track. */
    SPxUniTrack *m_selectedTrack;

    /* Selected position. */
    SPxLatLong_t m_selectedLatLong;
    int m_isLatLongSelected;

    /* Ruler. */
    SPxLatLong_t m_rulerStartLL;    /* Ruler start lat/long. */
    SPxLatLong_t m_rulerEndLL;      /* Ruler end lat/long. */
    int m_isRulerStartLLSet;        /* Ruler start lat/long set? */
    int m_isRulerEndLLSet;          /* Ruler end lat/long set? */
    SPxUniTrack *m_rulerStartTrack; /* Ruler start position track. */
    SPxUniTrack *m_rulerEndTrack;   /* Ruler end position track. */

    /*
     * Private functions.
     */

    void setTrackActive(SPxUniTrack *track, int isActive);
    void checkAlarmRegions(SPxUniTrack *track);
    void checkGateCrossings(SPxUniTrack *track);
    void handleTrackCreate(SPxUniTrack *track, UINT32 id);
    void handleTrackUpdate(SPxUniTrack *track);
    void handleTrackDelete(SPxUniTrack *track);

    /*
     * Private static functions.
     */

    static int radarFusedTrackCreationCallback(void *invokingObject,
                                               void *userObject,
                                               void *arg);

    static int radarFusedTrackUpdateCallback(void *invokingObject,
                                             void *userObject,
                                             void *arg);

    static int aisTrackCallback(void *invokingObject,
                                void *userObject,
                                void *arg);

    static int adsbTrackCallback(void *invokingObject,
                                 void *userObject,
                                 void *arg);

    static bool compareTrackDists(const TrackDist_t t1, 
                                  const TrackDist_t t2);

}; /* SPxUniTrackDatabase */

/*********************************************************************
 *
 *   Function prototypes
 *
 *********************************************************************/

#endif /* SPX_UNI_TRACK_DATABASE_H */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
