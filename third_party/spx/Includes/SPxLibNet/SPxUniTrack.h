/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxUniTrack.h,v $
 * ID: $Id: SPxUniTrack.h,v 1.8 2017/09/07 10:09:22 rew Exp $
 *
 * Purpose:
 *   Header for SPxUniTrack class, which unifies support for
 *   radar, fused, AIS and ADS-B tracks.
 *
 * Revision Control:
 *   07/09/17 v1.8    AGC       Add ageSecs arg to GetLatLong().
 *                              Add ageSecs arg to GetAltitude().
 *                              Add GetVerticalRate().
 *
 * Previous Changes:
 *   22/08/17 1.7   SP 	Add GetSecondaryTrack().
 *                      Add GetDistanceFromPoint().
 *   03/08/17 1.6   SP 	Add GetUpdateTime().
 *   01/08/17 1.5   AGC	Add GetAltitude().
 *   31/07/17 1.4   SP 	Further development.
 *   24/07/17 1.3   SP 	Support fused tracks.
 *   18/07/17 1.2   SP 	Further development.
 *   13/07/17 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_UNI_TRACK_H
#define _SPX_UNI_TRACK_H

/*
 * Other headers required.
 */

#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxGate.h"
#include "SPxLibData/SPxProcessRegion.h"
#include "SPxLibData/SPxPackets.h"

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

/* Forward-declare other classes we use in case headers are in wrong order. */
struct SPxUniTrackPriv;
class SPxUniTrackDatabase;
class SPxRadarTrack;
class SPxAISTrack;
class SPxADSBTrack;

/*
 * Define our class.
 */
class SPxUniTrack : public SPxObj
{
    /* The database class can access our internals directly. */
    friend class SPxUniTrackDatabase;

public:
    /*
     * Public types.
     */

    /* Track type bitfields. */
    typedef UINT32 TrackType_t;
    static const UINT32 TRACK_TYPE_NONE  = 0x00000000;
    static const UINT32 TRACK_TYPE_RADAR = 0x00000001;
    static const UINT32 TRACK_TYPE_AIS   = 0x00000002;
    static const UINT32 TRACK_TYPE_ADSB  = 0x00000004;
    static const UINT32 TRACK_TYPE_FUSED = 0x00000008;
    static const UINT32 TRACK_TYPE_ANY =   0xFFFFFFFF;

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Note that the constructor and destructor are not public, so the
     * class can only be instantiated by the SPxUniTrackDatabase.
     */

    /* Access to underlying track object (only one is set). */
    virtual SPxRadarTrack *GetRadarTrack(void) const { return m_radarFusedTrack; }
    virtual SPxRadarTrack *GetFusedTrack(void) const { return m_radarFusedTrack; }
    virtual SPxAISTrack *GetAISTrack(void) const { return m_aisTrack; }
    virtual SPxADSBTrack *GetADSBTrack(void) const { return m_adsbTrack; }

    /* General. */
    virtual TrackType_t GetTrackType(void) { return m_trackType; }
    virtual const char *GetTrackTypeName(void) const;
    virtual int GetIsSelected(void) const;
    virtual SPxErrorCode SetClassCode(unsigned int code);
    virtual unsigned int GetClassCode(void) const { return m_classCode; }
    virtual SPxErrorCode GetUpdateTime(SPxTime_t *time) const;
    virtual SPxUniTrack *GetSecondaryTrack(int mutexAlreadyLocked=FALSE) const;
    virtual SPxErrorCode GetDistanceFromPoint(double latDegs, 
                                              double longDegs, 
                                              double *distMetresRtn) const;

    /* Common interface to track data. */
    virtual UINT32 GetID(void) const;
    SPxErrorCode SetTrackName(const char *name);
    virtual const char *GetTrackName(void) const;
    virtual SPxErrorCode GetLatLong(double *latDegsRtn, 
                                    double *longDegsRtn,
                                    int useFiltered=TRUE,
                                    double *ageSecsRtn=NULL) const;
    virtual SPxErrorCode GetAltitude(double *altMetresRtn,
                                     double *ageSecsRtn=NULL) const;
    virtual SPxErrorCode GetVelocity(double *speedMpsRtn, 
                                     double *courseDegsRtn) const;
    virtual SPxErrorCode GetHeading(double *headingDegsRtn) const;
    virtual SPxErrorCode GetVerticalRate(double *verticalRateMpsRtn) const;

    /* User data. */
    virtual void SetUserData(void *ptr) { m_userData = ptr; }
    virtual void *GetUserData(void) const { return(m_userData); }

    /* Alarms and gates. */
    virtual int IsInAlarmArea(SPxShapeArea *area) const;
    virtual int IsGateCrossed(SPxGate *gate) const;

    /*
     * Public static functions.
     */

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Constructor and destructor. */
    SPxUniTrack(SPxUniTrackDatabase *trackDB);
    virtual ~SPxUniTrack(void);

private:

    /*
     * Private variables.
     */
    SPxAutoPtr<SPxUniTrackPriv> m_p;
    mutable SPxCriticalSection m_mutex;      /* Object mutex. */
    SPxUniTrackDatabase *m_database;         /* Track database . */
    TrackType_t m_trackType;                 /* Track type. */
    SPxRadarTrack *m_radarFusedTrack;        /* Linked radar/fused track or NULL. */
    SPxAISTrack *m_aisTrack;                 /* Linked AIS track or NULL. */
    SPxADSBTrack *m_adsbTrack;               /* Linked ADS-B track or NULL. */
    UINT32 m_globalID;                       /* Global track ID. */
    unsigned int m_classCode;                /* Class code. */
    void *m_userData;                        /* User data. */
    SPxTime_t m_updateTime;                  /* Time of last update. */

    /*
     * Private functions.
     */

    /* General. */
    SPxErrorCode setIsSelected(int isSelected);

    /* Configuration. */
    SPxErrorCode reset(void);
    SPxErrorCode setRadarTrack(SPxRadarTrack *radarTrack);
    SPxErrorCode setFusedTrack(SPxRadarTrack *fusedTrack);
    SPxErrorCode setAISTrack(SPxAISTrack *aisTrack);
    SPxErrorCode setADSBTrack(SPxADSBTrack *adsbTrack);
    SPxErrorCode setUpdateTime(const SPxTime_t *time);

    /* Alarm area handling. */
    SPxShapeArea *getAlarmArea(unsigned int index) const;
    unsigned int getNumAlarmAreas(void) const;
    SPxErrorCode addAlarmArea(SPxShapeArea *area);
    SPxErrorCode removeAlarmArea(SPxShapeArea *area);
    double getAlarmAreaWeighting(void) const;

    /* Gate handling. */
    SPxGate *getCrossedGate(unsigned int index) const;
    SPxErrorCode reportCrossedGate(SPxGate *gate);
    SPxErrorCode removeCrossedGate(SPxGate *gate);
    UINT32 getGateLastCrossingTime(SPxGate *gate) const;
    double getCrossedGatesWeighting(void) const;
    unsigned int getNumCrossedGates(void) const;
    SPxErrorCode removeAllCrossedGates(unsigned int *numCrossed, 
                                       SPxTime_tag *crossingTime=NULL,
                                       unsigned int *preNumCrossed=NULL, 
                                       SPxTime_tag *preCrossingTime=NULL,
                                       unsigned int *postNumCrossed = NULL, 
                                       SPxTime_tag *postCrossingTime=NULL);

    /*
     * Private static functions.
     */

}; /* SPxUniTrack */

#endif /* SPX_UNI_TRACK_H */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
