/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxThreat.h,v $
* ID: $Id: SPxThreat.h,v 1.29 2017/09/28 13:39:50 rew Exp $
*
* Purpose:
*   Header for SPxThreat class.
*
* Revision Control:
*   28/09/17 v1.29   AGC	Fix mutex lock error.
*
* Previous Changes:
*   11/09/17 1.28   AGC	Remove altitude functions.
*   22/06/17 1.27   AGC	Support threat altitude.
*   12/04/17 1.26   SP 	Add Set/GetSecondaryName().
*   08/03/17 1.25   SP 	Add IsTypeIgnore().
*   21/02/17 1.24   SP 	Support provisional threat type.
*                       Add GetColour().
*   31/01/17 1.23   SP 	Support timestamps on threat type.
*   22/11/16 1.22   SP 	Reorder threat types.
*   16/11/16 1.21   SP 	Support configurable threat types.
*   16/11/16 1.20   SP 	Fix version history.
*   20/04/16 1.19   AGC	Remove undefined static BuildPacketTrackThreat() function.
*   27/11/15 1.18   REW	Make more Get functions const.
*   26/11/15 1.17   REW	Support timestamps on threat level.
*   17/07/15 1.16   SP 	Add GetGateLastCrossingTime().
*   17/07/15 1.15   REW	Support SetUserData() etc.
*   15/07/15 1.14   SP 	Support handling of gate crossings.
*   02/03/15 1.13   SP 	Remove unused deletion handler.
*   17/02/14 1.12   SP 	Support alarm segments.
*   09/12/13 1.11   SP 	Add mutex.
*   06/12/13 1.10   SP 	Changes to support CxEye C2 operation.
*   08/11/13 1.9    SP 	Handle alarm poly deletion correctly.
*   04/11/13 1.8    SP 	Support alarm polys.
*   21/10/13 1.7    SP 	Changes to support per-polygon 
*                       threat weighting.
*   07/10/13 1.6    SP 	Use track ID in threat name.
*   04/10/13 1.5    AGC	Simplify headers.
*   05/09/13 1.4    SP 	Add UNCLASSIFIED threat level.
*   28/08/13 1.3    SP 	Add snapshot support.
*   14/08/13 1.2    REW	Fix compiler errors.
*   14/08/13 1.1    SP 	Initial version.
**********************************************************************/

#ifndef _SPX_THREAT_H
#define _SPX_THREAT_H

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
**********************************************************************/

/* Change flags used in update event. */
#define SPX_THREAT_CHANGE_NAME             (1U << 0)
#define SPX_THREAT_CHANGE_NAME_PREFIX      (1U << 1)
#define SPX_THREAT_CHANGE_GLOBAL_ID        (1U << 2)
#define SPX_THREAT_CHANGE_TYPE             (1U << 3)
#define SPX_THREAT_CHANGE_LEVEL            (1U << 4)

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
class SPxThreatDatabase;
class SPxRadarTrack;
struct SPxThreatPriv;
struct SPxTime_tag;

/*
 * Define our class.
 */
class SPxThreat : public SPxObj
{

/* The database class can access our internals directly. */
friend class SPxThreatDatabase;

public:
    /*
     * Public types.
     */

     /* Threat type - do not change order or values. */ 
     typedef enum
     {
        /* Current threat types. */
        TYPE_UNKNOWN = 0,
        TYPE_1 = 1,
        TYPE_2 = 2,
        TYPE_3 = 3,
        TYPE_4 = 4,
        TYPE_5 = 5,
        TYPE_6 = 6,
        NUM_TYPES,    /* Must be last entry. */

        /* Fixed threat types. */
        TYPE_HELO = TYPE_1,
        TYPE_PLANE = TYPE_2,
        TYPE_CAR = TYPE_3,
        TYPE_TRUCK = TYPE_4,
        TYPE_ARMVECH = TYPE_5,
        TYPE_MAN = TYPE_6

     } Type_t;

     /* Threat level - do not change order or values. */ 
     typedef enum
     {
        LEVEL_UNCLASSIFIED = 0,
        LEVEL_FRIENDLY = 1,
        LEVEL_NEUTRAL = 2,
        LEVEL_HOSTILE = 3,
        NUM_LEVELS   /* Must be last entry. */

     } Level_t;

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Note that the constructor and destructor are not public, so the
     * class can only be instantiated by the SPxThreatDatabase.
     */

     virtual SPxRadarTrack *GetRadarTrack(void) { return m_radarTrack; }
     virtual SPxErrorCode SetThreatNamePrefix(const char *prefix);
     virtual const char *GetThreatNamePrefix(void);
     virtual SPxErrorCode SetSecondaryName(const char *name);
     virtual const char *GetSecondaryName(void);
     virtual SPxErrorCode SetGlobalID(UINT32 id);
     virtual UINT32 GetGlobalID(void) const { return m_globalID; }
     virtual const char *GetThreatName(void);
     virtual SPxErrorCode  SetType(Type_t type, int isProvisional=FALSE);
     virtual Type_t GetType(void) const { return m_type; } 
     virtual const char *GetTypeName(void);
     virtual const char *GetTypeStr(void) { return GetTypeName(); } /* Deprecated. */
     virtual UINT32 GetTypeTimestampMsecs(void) const {return(m_typeMsecs);}
     virtual UINT32 GetMsecsSinceType(void) const;
     virtual int IsTypeProvisional(void) const { return m_isTypeProvisional; }
     virtual int IsTypeIgnore(void);
     virtual SPxErrorCode SetLevel(Level_t level);
     virtual Level_t GetLevel(void) const { return m_level; } 
     virtual const char *GetLevelName(void);
     virtual const char *GetLevelStr(void) { return GetLevelName(); } /* Deprecated. */
     virtual UINT32 GetLevelTimestampMsecs(void) const {return(m_levelMsecs);}
     virtual UINT32 GetMsecsSinceLevel(void) const;
     virtual UINT32 GetColour(void) const;
     virtual void SetUserData(void *ptr) { m_userData = ptr; }
     virtual void *GetUserData(void) { return(m_userData); }
     virtual SPxErrorCode UpdateSeverity(void);
     virtual double GetSeverity(void);
     virtual unsigned int GetNumSnapshots(void) const { return m_numSnapshots;}
     virtual int IsInAlarmArea(SPxShapeArea *area);
     virtual int IsGateCrossed(SPxGate *gate);
     virtual SPxErrorCode BuildPacketTrackThreat(SPxPacketTrackThreat *packet,
                                                 UINT32 setterID=0);

     /* Gate crossing information. */
     SPxErrorCode RemoveAllCrossedGates(unsigned int *numCrossed, SPxTime_tag *crossingTime=NULL,
                                        unsigned int *preNumCrossed=NULL, SPxTime_tag *preCrossingTime=NULL,
                                        unsigned int *postNumCrossed = NULL, SPxTime_tag *postCrossingTime=NULL);
     
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
    SPxThreat(SPxThreatDatabase *threatDB, SPxRadarTrack *radarTrack);
    virtual ~SPxThreat(void);
    virtual void AddSnapshot(void) { m_numSnapshots++; }

private:

    /*
     * Private variables.
     */
    SPxAutoPtr<SPxThreatPriv> m_p;
    SPxCriticalSection m_mutex;     /* Object mutex. */
    SPxThreatDatabase *m_threatDB;  /* Track database . */
    SPxRadarTrack *m_radarTrack;    /* Associated radar track. */
    UINT32 m_globalID;              /* Global threat ID. */
    Type_t m_type;                  /* Assigned threat type. */
    UINT32 m_typeMsecs;             /* Time when threat type was set. */
    int m_isTypeProvisional;        /* Is type set to a provisional value? */
    Level_t m_level;                /* Assigned threat level. */
    UINT32 m_levelMsecs;            /* Time when threat level was set. */
    double m_severity;              /* Severity level 0.0 to 1.0. */
    unsigned int m_numSnapshots;    /* Number of snapshots taken. */
    SPxCriticalSection m_alarmAreaListMutex; /* Mutex used to protect alarm area list. */
    mutable SPxCriticalSection m_crossedGateListMutex; /* Mutex used to protect crossed gate list. */

    /*
     * Private functions.
     */

    SPxErrorCode Reset(void);
    int UpdateThreatName(const char **oldName);
    SPxErrorCode SetTypeInternal(Type_t type, int isProvisional=FALSE);

    /* Alarm area handling. */
    SPxShapeArea *GetAlarmArea(unsigned int index);
    unsigned int GetNumAlarmAreas(void);
    SPxErrorCode AddAlarmArea(SPxShapeArea *area);
    SPxErrorCode RemoveAlarmArea(SPxShapeArea *area);
    double GetAlarmAreaWeighting(void);

    /* Gate handling. */
    SPxGate *GetCrossedGate(unsigned int index) const;
    SPxErrorCode ReportCrossedGate(SPxGate *gate);
    SPxErrorCode RemoveCrossedGate(SPxGate *gate);
    UINT32 GetGateLastCrossingTime(SPxGate *gate) const;
    double GetCrossedGatesWeighting(void) const;
    unsigned int GetNumCrossedGates(void) const;

    /* User data. */
    void *m_userData;

    /*
     * Private static functions.
     */

}; /* SPxThreat */

#endif /* SPX_THREAT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
