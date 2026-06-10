/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxRadarTrack.h,v $
* ID: $Id: SPxRadarTrack.h,v 1.35 2017/09/11 14:54:28 rew Exp $
*
* Purpose:
*   Header for SPxRadarTrack class.
*
* Revision Control:
*   11/09/17 v1.35   AGC	Add GetPrevAltitudeMetres().
*
* Previous Changes:
*   18/07/17 1.34   SP 	Add GetVelocity().
*   31/01/17 1.33   REW	Make some functions const.
*   05/10/16 1.32   REW	Add GetFirstTimestamp().
*   29/09/16 1.31   REW	Add GetRangeBearingFromLatLong().
*   30/11/15 1.30   REW	Add SetTrackName() function.
*   26/11/15 1.29   REW	Add optional duration to highlighting.
*   26/11/15 1.28   REW	Support highlighted tracks.
*   21/09/15 1.27   AGC	Make GetIsSelected() const.
*   15/07/15 1.26   SP 	Keep a copy of previous track update.
*   12/03/15 1.25   REW	Add IsSPI().
*   26/01/15 1.24   REW	Support flight plan links.
*   13/11/14 1.23   REW	Support SetLabelUserField().
*   17/10/14 1.22   REW	Support GetVerticalDirection().
*   02/10/14 1.21   REW	Support label offsets.
*   03/04/14 1.20   REW	Add IsEmergency(). Make some functions const.
*   01/04/14 1.19   REW	Add GetAltitudeMetres().
*   13/12/13 1.18   SP 	Link trail point back to track.
*   04/10/13 1.17   AGC	Simplify headers.
*   14/08/13 1.16   AGC	Support track specific shapes and images.
*   15/07/13 1.15   AGC	Prevent warnings with high VS warning level.
*   24/05/13 1.14   AGC	Add SPxTrackIsSensorMultiple().
*   09/11/12 1.13   SP 	Modify track data pointer comment.
*   30/10/12 1.12   REW	Derive from SPxObj again.
*   26/10/12 1.11   AGC	Add GetMetresFromLatLong() function.
*			Make Update() function public.
*			Do not derive from SPxObj.
*   08/05/12 1.10   SP 	Support measured and filtered lat/long.
*   04/04/12 1.9    SP 	Support user data.
*   29/03/12 1.8    SP 	Rename file as SPxRadarTrack.h.
*                       Rename SPxTrackReport as SPxRadarTrack.
*                       Rename SPxTrackTrailPoint as 
*                       SPxRadarTrackTrailPoint.
*                       Fix version numbers after V1.6.
*   21/12/11 1.7    REW	Add SPxTrackIsSensor...() functions.
*   15/12/11 1.6    REW	Add SPxTrackGetStringForSensorType().
*   25/10/11 1.5    SP 	Add SetDeleted() and Reset().
*   14/10/11 1.4    SP 	Remove reference count functions.
*                       Add Set/GetCanBeDeleted().
*                       Add linked list pointers.
*                       Make some functions protected.
*   27/07/11 1.3    SP 	Rename ClearTrail() to ClearHistory().
*   13/07/11 1.2    SP 	Add GetMetresFromPoint().
*                       Increase default num trails points to 500.
*   07/07/11 1.1    SP 	Initial Version.
**********************************************************************/

#ifndef _SPX_RADAR_TRACK_H
#define _SPX_RADAR_TRACK_H

/*
 * Other headers required.
 */

/* We need SPxSymbol for SPxSymbolShape_t enumeration. */
#include "SPxLibUtils/SPxSymbol.h"

/* We need SPxTime_t structure. */
#include "SPxLibUtils/SPxTime.h"

/* We need SPxPacketTrackExtended structure. */
#include "SPxLibData/SPxPackets.h"

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

/* Forward-declare other classes we use in case headers are in wrong order. */
class SPxRadarTrack;
class SPxTrackDatabase;
class SPxImage;
struct SPxPacketTrackMinimal_tag;
struct SPxPacketTrackNormal_tag;
struct SPxPacketTrackExtended_tag;
class SPxFlightPlan;

/* Structure used to store a linked list of track tail points */
typedef struct _SPxRadarTrackTrailPoint
{
     /* Previous item in the list or NULL if first entry */
    _SPxRadarTrackTrailPoint *prev;

    /* Next item in the list or NULL if last entry */
    _SPxRadarTrackTrailPoint *next; 

    /* Timestamp when this trail point was created */
    SPxTime_t timestamp;

    /* Additional track data preserved for this trail point */
    UINT8 status;              /* Track status (SPX_PACKET_TRACK_STATUS...) */
    UINT8 numCoasts;           /* Number of consecutive coasts */

    /* Control flags. */
    int isMeasAvail;           /* TRUE if measured values are available */
    int isLatLongAvail;        /* TRUE if lat/long values are available */

    /* Filtered range and azimuth (always available) */
    double rangeMetres;        /* Filtered range in metres */
    double azimuthDeg;         /* Filtered azimuth in degrees */

    /* Measured range and azimuth (available if isMeasAvail set) */
    double measRangeMetres;    /* Measured range in metres */
    double measAzimuthDeg;     /* Measured azimuth in degrees */

    /* Filtered lat/long (available if isLatLongAvail set) */
    double latDegs;            /* Filtered latitude in degrees. */
    double longDegs;           /* Filtered Longitude in degrees. */  

    /* Measured lat/long (available if isLatLongAvail & isLatLongAvail set) */
    double measLatDegs;        /* Measured latitude in degrees. */
    double measLongDegs;       /* Measured longitude in degrees. */ 

    SPxRadarTrack *track;      /* Link back to the radar track. */

} SPxRadarTrackTrailPoint_t;

/*
 * Define our class.
 */
class SPxRadarTrack : public SPxObj
{

/* The database class can access our internals directly. */
friend class SPxTrackDatabase;

public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Note that the constructor and destructor are not public, so the
     * class can only be instantiated by the SPxTrackDatabase.
     */

    /* Extract current track data */
    const SPxPacketTrackMinimal *GetMinRpt(void) const { return m_pMinRpt; }
    const SPxPacketTrackNormal *GetNormRpt(void) const { return m_pNormRpt; } 
    const SPxPacketTrackExtended *GetExtRpt(void) const { return m_pExtRpt; }
    const SPxTime_t *GetTimestamp(void) const { return &m_timestamp; }
    const SPxTime_t *GetFirstTimestamp(void) const { return &m_firstTimestamp;}
    UINT32 GetID(void) const { return (m_pMinRpt ? m_pMinRpt->id : 0); }
    double GetRangeMetres(int filteredPos=TRUE) const;
    double GetAzimuthDeg(int filteredPos=TRUE) const;
    SPxErrorCode GetLatLong(double *latDegsRtn,
                            double *longDegsRtn,
                            int filteredPos=TRUE) const;
    SPxErrorCode GetVelocity(double *speedMpsRtn, 
                             double *courseDegsRtn) const;
    double GetMetresFromPoint(double xMetres, 
                              double yMetres,
                              int filteredPos=TRUE) const;
    double GetMetresFromLatLong(double latDegs,
				double longDegs,
				int filteredPos=TRUE,
				unsigned int radarIndex=0,
				double labelMetresPerPixel=0.0) const;
    SPxErrorCode GetRangeBearingFromLatLong(double latDegs,
				double longDegs,
				double *rangeMetresPtr,
				double *bearingDegsPtr,
				int filteredPos=TRUE,
				unsigned int radarIndex=0,
				double labelMetresPerPixel=0.0) const;
    SPxErrorCode GetAltitudeMetres(double *altMetresPtr) const;
    SPxErrorCode GetPrevAltitudeMetres(double *altMetresPtr) const;
    SPxErrorCode GetVerticalDirection(int *verticalDirPtr) const;
    int IsEmergency(void) const;
    int IsSPI(void) const;

    /* Track configuration */
    SPxErrorCode SetIsSelected(int isSelected) 
    { 
        m_isSelected = (isSelected ? TRUE : FALSE);
        return SPX_NO_ERROR;
    }
    int GetIsSelected(void) const { return m_isSelected; }
    SPxErrorCode SetIsHighlighted(int state, unsigned int durationSecs=0);
    int GetIsHighlighted(void);
    SPxErrorCode SetUseShape(int useShape)
    {
	m_useShape = useShape;
	return SPX_NO_ERROR;
    }
    int GetUseShape(void) { return m_useShape; }
    SPxErrorCode SetShape(SPxSymbolShape_t shape, SPxImage *image=NULL)
    {
	m_shape = shape;
	m_image = image;
	return SPX_NO_ERROR;
    }
    SPxSymbolShape_t GetShape(void) { return m_shape; }
    SPxImage *GetImage(void) { return m_image; }
    SPxErrorCode SetUseColour(int useColour)
    {
	m_useColour = useColour;
	return SPX_NO_ERROR;
    }
    int GetUseColour(void) { return m_useColour; }
    SPxErrorCode SetColour(UINT32 colour)
    {
	m_colour = colour;
	return SPX_NO_ERROR;
    }
    UINT32 GetColour(void) { return m_colour; }
    void ClearHistory(void);

    /* Offset from track position for label display. */
    SPxErrorCode SetLabelOffsetPixels(int x, int y)
    {
	m_labelOffsetXPixels = x;
	m_labelOffsetYPixels = y;
	return(SPX_NO_ERROR);
    }
    int GetLabelOffsetXPixels(void) const { return(m_labelOffsetXPixels); }
    int GetLabelOffsetYPixels(void) const { return(m_labelOffsetYPixels); }

    /* String added to label by SPxTrackRenderer if %z included in format. */
    SPxErrorCode SetLabelUserField(const char *str);
    const char *GetLabelUserField(void) const;	/* NB: MAY BE NULL */

    /* Name of track (e.g. user-assigned), which may be used first in %L. */
    SPxErrorCode SetTrackName(const char *str);
    const char *GetTrackName(void) const;	/* NB: MAY BE NULL */

    /* Read trail points from a track report */
    SPxErrorCode IterateTrailPoints(unsigned int trailLenSecs,
                                    SPxCallbackListFn_t trailFn,
                                    void *userArg);

    /* Set/get flight plan for this track (used by the track database). */
    SPxErrorCode SetFlightPlan(SPxFlightPlan *fp)
    {
	m_flightPlan = fp;
	return(SPX_NO_ERROR);
    }
    SPxFlightPlan *GetFlightPlan(void)	{ return(m_flightPlan); }

    /* User data. */
    void SetUserData(void *ptr) { m_userData = ptr; }
    void *GetUserData(void) { return m_userData; }

    /*
     * Public static functions.
     */
    static double GetTrailRangeMetres(const SPxRadarTrackTrailPoint_t *trailPt,
                                      int filteredPos=TRUE);

    static double GetTrailAzimuthDeg(const SPxRadarTrackTrailPoint_t *trailPt,
                                     int filteredPos=TRUE);

    static SPxErrorCode GetTrailLatLong(
        const SPxRadarTrackTrailPoint_t *trailPt, 
        int filteredPos,
        double *latDegsRtn,
        double *longDegsRtn);

    /* Functions that can be given to the SPxTrackDatabase::IterateTracks()
     * function to do certain actions on all tracks.
     */
    static int IteratorSelect(void * /*database*/, void * /*arg*/, void *track)
    {
	if (track != NULL) { ((SPxRadarTrack *)track)->SetIsSelected(TRUE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorDeselect(void * /*database*/, void * /*arg*/, void *track)
    {
	if (track != NULL) { ((SPxRadarTrack *)track)->SetIsSelected(FALSE); }
	return(SPX_NO_ERROR);
    }
    static int IteratorClearHistory(void * /*database*/, void * /*arg*/, void *track)
    {
	if(track != NULL) { ((SPxRadarTrack *)track)->ClearHistory(); }
	return(SPX_NO_ERROR);
    }

    /* Update the track report */
    SPxErrorCode Update(const SPxTime_t *timestamp,
                        SPxPacketTrackMinimal *pMinRpt,
                        SPxPacketTrackNormal *pNormRpt,
                        SPxPacketTrackExtended *pExtRpt);

    /* Extract previous track data */
    const SPxPacketTrackMinimal *GetPrevMinRpt(void) const { return m_pPrevMinRpt; }
    const SPxPacketTrackNormal *GetPrevNormRpt(void) const { return m_pPrevNormRpt; } 
    const SPxPacketTrackExtended *GetPrevExtRpt(void) const { return m_pPrevExtRpt; }
    SPxErrorCode GetPrevLatLong(double *latDegsRtn,
                                double *longDegsRtn,
                                int filteredPos=TRUE);

protected:

    /*
     * Protected variables.
     */

    /* Object linked list pointers. */
    SPxRadarTrack *m_next;
    SPxRadarTrack *m_prev;

    /*
     * Protected functions.
     */

    /* Constructor and destructor. */
    SPxRadarTrack(unsigned int maxNumTrailPts=500);
    virtual ~SPxRadarTrack(void);

    /* Mark track as deleted */
    SPxErrorCode SetDeleted(void);

    /* Reset track internals */
    SPxErrorCode Reset(void);

    /* Get last update timestamp */
    const SPxTime_t *GetLastUpdateTimestamp(void) { return &m_timestamp; }

private:

    /*
     * Private variables.
     */

    /* Timestamp when track was updated */
    SPxTime_t m_timestamp;

    /* Timestamp when track was first received*/
    SPxTime_t m_firstTimestamp;

    /* Track information. Do not use these pointer internally for
     * accessing track data as the m_pNormRpt and m_pExtRpt pointers 
     * may be set to NULL when Update() in called. Instead use the 
     * public access functions, GetMinRpt(), GetNormRpt() and
     * GetExtRpt().
     */
    SPxPacketTrackMinimal *m_pMinRpt;
    SPxPacketTrackNormal *m_pNormRpt;
    SPxPacketTrackExtended *m_pExtRpt;

    /* Private track data referenced by above pointers. */
    SPxPacketTrackExtended _m_data;

    /* Previous track information. */
    SPxPacketTrackMinimal *m_pPrevMinRpt;
    SPxPacketTrackNormal *m_pPrevNormRpt;
    SPxPacketTrackExtended *m_pPrevExtRpt;

    /* Private track data for previous track update. */
    SPxPacketTrackExtended _m_prev_data;

    /* Copy of values from previous updates. */
    int m_isPrevAltitudeSet;
    double m_prevAltitudeMetres;

    /* Max number of trail points to keep */
    unsigned int m_maxNumTrailPts;

    /* Trail history points stored in a linked list */
    SPxRadarTrackTrailPoint_t *m_trailListHead;
    SPxRadarTrackTrailPoint_t *m_trailListTail;

    /* Number of elements in the trail point linked-list */
    unsigned int m_numTrailPts;

    /* Mutex used to protect trail list */
    SPxCriticalSection m_trailListAccess;

    /* Set to TRUE if the track is selected */
    int m_isSelected;

    /* Is track supposed to be highlighted. */
    int m_isHighlighted;			/* Current state, TRUE/FALSE */
    unsigned int m_highlightDurationSecs;	/* How long for, 0 forever */
    UINT32 m_highlightStartMsecs;		/* Timestamp */

    /* Track specific shape. */
    int m_useShape; /* Use track specific shape? */
    SPxSymbolShape_t m_shape;
    SPxImage *m_image;

    /* Track specific colour. */
    int m_useColour; /* Use track specific colour? */
    UINT32 m_colour;

    /* Track specific label offset. */
    int m_labelOffsetXPixels;
    int m_labelOffsetYPixels;

    /* Track specific label field (used by %z in renderer). */
    char *m_labelUserField;
    unsigned int m_labelUserFieldAllocatedSize;

    /* Track name, if any. */
    char *m_trackName;
    unsigned int m_trackNameAllocatedSize;

    /* Associated flight plan, if any. */
    SPxFlightPlan *m_flightPlan;

    /* User data. */
    void *m_userData;

    /*
     * Private functions.
     */

    void UpdateTrail(void);
    SPxErrorCode GetLatLongInternal(const SPxPacketTrackMinimal *pMinRpt,
                                    const SPxPacketTrackNormal *pNormRpt,
                                    const SPxPacketTrackExtended *pExtRpt,
                                    double *latDegsRtn,
                                    double *longDegsRtn,
                                    int filteredPos) const;
    
}; /* SPxRadarTrack */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

/* Utility functions. */
extern const char *SPxTrackGetStringForSensorType(UINT32 sensorType);
extern int SPxTrackIsSensorPrimary(UINT32 sensorType);
extern int SPxTrackIsSensorSecondary(UINT32 sensorType);
extern int SPxTrackIsSensorCombined(UINT32 sensorType);
extern int SPxTrackIsSensorGPSBased(UINT32 sensorType);
extern int SPxTrackIsSensorMultiple(UINT32 sensorType);

#endif /* SPX_RADAR_TRACK_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
