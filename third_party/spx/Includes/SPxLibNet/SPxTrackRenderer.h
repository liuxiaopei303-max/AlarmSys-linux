/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTrackRenderer.h,v $
* ID: $Id: SPxTrackRenderer.h,v 1.30 2017/07/13 14:01:14 rew Exp $
*
* Purpose:
*   Header for SPxTrackRenderer base class which supports the display
*   of radar tracks in an application, but only via a derived class.
*
* Revision Control:
*   13/07/17 v1.30   SP 	Add GetDatabase().
*
* Previous Changes:
*   12/04/17 1.29   AGC	Support auto track label positioning.
*   13/10/16 1.28   AGC	Skip trail points when appropriate.
*   26/11/15 1.27   REW	Support flashing of highlighted threats.
*   12/03/15 1.26   REW	Support flashing of SPI targets.
*   02/10/14 1.25   REW	Support track label offsets.
*   02/06/14 1.24   AGC	Support captions.
*   16/05/14 1.23   AGC	Improve cursor selection.
*   10/04/14 1.22   AGC	Support double-click track selection.
*   08/04/14 1.21   REW	Support pre-render callback.
*   03/04/14 1.20   REW	Support flashing of emergency targets.
*   01/04/14 1.19   REW	Add Get/SetInterceptorID().
*   27/03/14 1.18   REW	Support different colour for selected targets.
*   09/01/14 1.17   REW	Support colour for track interception.
*   06/01/14 1.16   REW	Support track interception.
*   19/12/13 1.15   REW	Support threat colours.
*   07/10/13 1.14   AGC	Move rendering to renderAll().
*   14/08/13 1.13   SP 	Allow functions to be overridden.
*   14/08/13 1.12   AGC	Support track specific shapes and images.
*   20/03/13 1.11   SP 	Support fusion symbols.
*   21/12/12 1.10   AGC	Use new SPxTrackRendererBase base class.
*   23/11/12 1.9    AGC	Support SPxMouseInterface.
*			Add TrackSelect callbacks.
*   09/11/12 1.8    AGC	Add option to only show selected tracks.
*   29/03/12 1.7    SP 	Rename SPxTrackReport as SPxRadarTrack.
*   08/02/12 1.6    AGC	Add dashed vector and coasted vector support.
*   06/10/11 1.5    SP 	Add control to select filtered position.
*   27/07/11 1.4    SP 	Add control to show/hide labels.
*   07/07/11 1.3    SP 	Move track modes to header.
*                       Use SPxTrackReport.
*   26/04/11 1.2    SP 	Use base class view control.
*                       Render track trails and vectors.
*   12/04/11 1.1    SP 	Initial Version.
* 
**********************************************************************/

#ifndef _SPX_TRACK_RENDERER_H
#define _SPX_TRACK_RENDERER_H

/*
 * Other headers required.
 */

/* We need SPxLibUtils for common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/* We need the SPxTrackDatabase class. */
#include "SPxLibNet/SPxTrackDatabase.h"

/* We need our base class. */
#include "SPxLibUtils/SPxTrackRendererBase.h"

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

/* Need to forward-declare other classes in case headers are in wrong order.*/
class SPxTrackDatabase;
class SPxTrackRendererBase;
class SPxSymbol;

typedef void (*SPxTrackRendererSelectFn_t)(SPxRadarTrack *track, void *userArg);
typedef SPxErrorCode (*SPxTrackRendererPreRenderFn_t)(SPxRadarTrack *track,
							void *userArg,
							SPxSymbol *symbol,
							UINT32 *colourPtr);

/*
 * Define our class, derived from the abstract renderer base class.
 */
class SPxTrackRenderer : public SPxTrackRendererBase
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxTrackRenderer(SPxTrackDatabase *database, 
		     SPxTrackRenderConfig renderConfig = SPX_TRACK_RENDER_CONFIG_SPX);
    virtual ~SPxTrackRenderer(void);

    /* Get database. */
    SPxTrackDatabase *GetDatabase(void) { return m_database; }

    /* Mouse handling. */
    virtual int HandleMousePress(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseRelease(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleDoubleClick(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseLeave(void);

    /* Track selection handlers. */
    SPxErrorCode AddTrackSelectCallback(SPxTrackRendererSelectFn_t fn, void *userArg);
    SPxErrorCode RemoveTrackSelectCallback(SPxTrackRendererSelectFn_t fn, void *userArg);

    /* Pre-render callback. */
    SPxErrorCode SetPreRenderCallback(SPxTrackRendererPreRenderFn_t fn,
					void *userArg);

    /* Fusion symbols. */
    SPxErrorCode SetUseFusionSymbols(int state);
    int GetUseFusionSymbols(void) const { return m_useFusionSymbols; }

    /* Threat colours. */
    SPxErrorCode SetUseThreatColours(int state);
    int GetUseThreatColours(void) const { return(m_useThreatColours); }
    SPxErrorCode SetThreatColourUnclassified(UINT32 argb);
    SPxErrorCode SetThreatColourFriendly(UINT32 argb);
    SPxErrorCode SetThreatColourNeutral(UINT32 argb);
    SPxErrorCode SetThreatColourHostile(UINT32 argb);
    SPxErrorCode SetThreatColourCivilian(UINT32 argb);
    UINT32 GetThreatColourUnclassified(void) const;
    UINT32 GetThreatColourFriendly(void) const;
    UINT32 GetThreatColourNeutral(void) const;
    UINT32 GetThreatColourHostile(void) const;
    UINT32 GetThreatColourCivilian(void) const;

    /* Interception support. */
    SPxErrorCode SetInterceptorID(UINT32 id)
    {
	m_interceptorID = id;
	return(SPX_NO_ERROR);
    }
    UINT32 GetInterceptorID(void) const { return(m_interceptorID); }
    SPxErrorCode SetInterceptionSpeed(double mps);
    double GetInterceptionSpeed(void) const { return(m_interceptionSpeedMps); }
    SPxErrorCode SetInterceptionColour(UINT32 argb);
    UINT32 GetInterceptionColour(void) const;

    /* Colour of selected target (if non-zero). */
    SPxErrorCode SetSelectedColour(UINT32 argb);
    UINT32 GetSelectedColour(void) const;

    /* Flashing of various targets. */
    SPxErrorCode SetEmergencyFlashHz(unsigned int hz);	/* 0 for no flash */
    unsigned int GetEmergencyFlashHz(void) const { return(m_emergencyFlashHz);}
    SPxErrorCode SetSPIFlashHz(unsigned int hz);	/* 0 for no flash */
    unsigned int GetSPIFlashHz(void) const { return(m_spiFlashHz); }
    SPxErrorCode SetHighlightFlashHz(unsigned int hz);	/* 0 for no flash */
    unsigned int GetHighlightFlashHz(void) const { return(m_highlightFlashHz);}

    /* Cursor caption text. */
    SPxErrorCode SetCaption(const char *title, const char *caption);

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    /*
     * Protected functions.
     */
    virtual SPxErrorCode renderAll(void);
    virtual void GetTrackLabel(SPxRadarTrack *track,
                               char *buffer,
                               unsigned int bufferSizeBytes);

    virtual SPxErrorCode RenderTrack(SPxRadarTrack *track);
    virtual SPxErrorCode RenderTrailPoint(const SPxRadarTrackTrailPoint_t *trailPt,int index =-1);

private:
    /*
     * Private variables.
     */

    struct TrackSelectInfo_t
    {
	TrackSelectInfo_t *next;
	SPxTrackRendererSelectFn_t fn;
	void *userArg;
    };

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Mutex protection. */
    SPxCriticalSection m_mutex;

    /* Handle of associated database object. */
    SPxTrackDatabase *m_database;

    /* Track select functions. */
    TrackSelectInfo_t *m_trackSelectFns;

    /* Pre-render callback function. */
    SPxTrackRendererPreRenderFn_t m_preRenderFn;
    void *m_preRenderArg;

    /* Use fusion symbols? */
    int m_useFusionSymbols;

    /* Use threat colours? */
    int m_useThreatColours;
    UINT32 m_threatColour;
    UINT32 m_threatColourUnclassified;
    UINT32 m_threatColourFriendly;
    UINT32 m_threatColourNeutral;
    UINT32 m_threatColourHostile;
    UINT32 m_threatColourCivilian;

    /* Track interception. */
    UINT32 m_interceptorID;
    double m_interceptionSpeedMps;	/* Zero to use track speed */
    UINT32 m_interceptionColour;	/* Zero to use threat colour */

    /* Colour for selected targets, if non-zero. */
    UINT32 m_selectedColour;

    /* Target flash support. */
    unsigned int m_emergencyFlashHz;	/* Zero for no flash */
    unsigned int m_spiFlashHz;		/* Zero for no flash */
    unsigned int m_highlightFlashHz;	/* Zero for no flash */

    /* Track label deconfliction. */
    UINT32 m_movingLabelTrackID;	/* ID of track we're moving */

    /* Cursor caption text. */
    SPxAutoPtr<char[]> m_captionTitle;
    SPxAutoPtr<char[]> m_caption;
    int m_mouseX;
    int m_mouseY;
    int m_mouseValid;

    /* Trail point history. */
    int m_prevTrailX;
    int m_prevTrailY;

    /*
     * Private functions.
     */
    void setCursor(int x, int y, SPxCursor_t *cursor);
    int selectTrack(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    SPxErrorCode getXYForTrack(SPxRadarTrack *track,
				int *xPixelsPtr,
				int *yPixelsPtr,
				double *xMetresPtr = NULL,
				double *yMetresPtr = NULL);
    SPxErrorCode renderCaption(void);

    /*
     * Private static functions.
     */
    static int staticRenderTrack(void *database, 
                                 void *arg, 
                                 void *track);

    static int staticRenderTrailPoint(void *database, 
                                      void *arg, 
                                      void *trailPt);

    static int staticGetTextOffsets(void *database,
				    void *arg,
				    void *track);
    static int staticUpdateLabelOffsets(void *database,
					void *arg,
					void *track);
    static int staticFindLabelOverlap(void *database,
				      void *arg,
				      void *track);

}; /* SPxTrackRenderer */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_TRACK_RENDERER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
