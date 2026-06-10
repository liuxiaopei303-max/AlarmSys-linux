/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTrackRendererBase.h,v $
* ID: $Id: SPxTrackRendererBase.h,v 1.10 2017/04/12 09:21:12 rew Exp $
*
* Purpose:
*   Header for SPxTrackRendererBase base class which supports the display
*   of radar tracks in an application, either from a local tracker,
*   or from a track database receiving remote track reports, using
*   SPxTrackRendererMHT and SPxTrackRenderer respectively.
*
* Revision Control:
*   12/04/17 v1.10   AGC	Support auto label positioning.
*
* Previous Changes:
*   21/01/16 1.9    REW	Support SetTrailLengthSecsSelected() etc.
*   02/10/14 1.8    REW	Add label movement support.
*   18/06/14 1.7    REW	Add control over track interception selection.
*   10/04/14 1.6    AGC	Add double-click track selection support.
*   06/01/14 1.5    REW	Add track interception support.
*   14/08/13 1.4    AGC	Add symbol rotation option.
*   10/04/13 1.3    AGC	Add GetRenderConfig().
*   20/03/13 1.2    SP 	Make m_trackSymbol protected.
*   21/12/12 1.1    AGC	Initial version.
**********************************************************************/

#ifndef SPX_TRACK_RENDERER_BASE_H
#define SPX_TRACK_RENDERER_BASE_H

/*
 * Other headers required.
 */

/* We need SPxLibUtils for common types, callbacks, errors etc. */

/* We need out base class. */
#include "SPxLibUtils/SPxRenderer.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Track rendering configurations. */
enum SPxTrackRenderConfig
{
    SPX_TRACK_RENDER_CONFIG_SPX = 0,
    SPX_TRACK_RENDER_CONFIG_ECDIS = 1
};

/* Track symbol rendering modes. These values map to
 * the rendering modes supported by the SPxSymbol class.
 */
#define TRACK_MODE_ESTABLISHED_UPDATING      0
#define TRACK_MODE_ESTABLISHED_COASTING      1
#define TRACK_MODE_PROVISIONAL_UPDATING      2
#define TRACK_MODE_PROVISIONAL_COASTING      3
#define TRACK_MODE_LOST			     4
#define TRACK_MODE_ESTABLISHED_UPDATING_NPR  5
#define TRACK_MODE_ESTABLISHED_COASTING_NPR  6
#define TRACK_MODE_PROVISIONAL_UPDATING_NPR  7
#define TRACK_MODE_PROVISIONAL_COASTING_NPR  8


/* Special case used to indicate a mode we are not intested in */
#define TRACK_MODE_NO_RENDER (SPX_NUM_SYMBOL_MODES)

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
class SPxRenderer;
class SPxSymbol;

/*
 * Define our class, derived from the abstract renderer base class.
 */
class SPxTrackRendererBase : virtual public SPxRenderer
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxTrackRendererBase(SPxTrackRenderConfig renderConfig = SPX_TRACK_RENDER_CONFIG_SPX);
    virtual ~SPxTrackRendererBase(void);

    void ShowOnlySelected(int enable);
    int IsShowingOnlySelected(void) const { return m_showOnlySelected; }
    void SetSelectionEnabled(int state) { m_selectionEnabled = state; }
    int GetSelectionEnabled(void) const { return m_selectionEnabled; }
    void SetSelectWithDoubleClick(int state) { m_selectWithDoubleClick = state; }
    int GetSelectWithDoubleClick(void) const { return m_selectWithDoubleClick; }
    void SetInterceptEnabled(int state) { m_interceptEnabled = state; }
    int GetInterceptEnabled(void) const { return m_interceptEnabled; }
    void SetInterceptSelectionEnabled(int state)
				    { m_interceptSelectionEnabled = state; }
    int GetInterceptSelectionEnabled(void) const
				    { return m_interceptSelectionEnabled; }
    void SetLabelMoveEnabled(int state) { m_labelMoveEnabled = state; }
    int GetLabelMoveEnabled(void) const { return m_labelMoveEnabled; }

    /* Configuration functions */
    SPxSymbol *GetTrackSymbol(void) { return m_trackSymbol; }
    SPxErrorCode SetTrailLengthSecs(unsigned int secs);
    unsigned int GetTrailLengthSecs(void) const
					{ return m_trailLengthSecs; }
    SPxErrorCode SetTrailLengthSecsSelected(unsigned int secs);
    unsigned int GetTrailLengthSecsSelected(void) const
					{ return m_trailLengthSecsSelected; }
    SPxErrorCode SetVectorLengthSecs(unsigned int secs);
    unsigned int GetVectorLengthSecs(void) const
					{ return m_vectorLengthSecs; }
    SPxErrorCode SetUseRelativeVelocity(int useRelVelocity);
    int GetUseRelativeVelocity(void) const
					{ return m_useRelativeVelocity; }
    SPxErrorCode SetShowLabels(int isVisible);
    int GetShowLabels(void) const	{ return m_showLabels; }
    SPxErrorCode SetLabelAutoOffset(int enable);
    int IsLabelAutoOffsetEnabled(void) const { return m_labelAutoOffset; }
    SPxErrorCode SetLabelOffsetPixels(int x, int y);
    int GetLabelXOffsetPixels(void) const { return m_labelXOffsetPixels; }
    int GetLabelYOffsetPixels(void) const { return m_labelYOffsetPixels; } 
    SPxErrorCode SetLabel(const char *txt);
    const char *GetLabel(void) { return m_label; }
    SPxErrorCode SetLabelColour(UINT32 argb);
    UINT32 GetLabelColour(void) const	{ return m_labelColour; }
    SPxErrorCode SetUseFilteredPos(int state);
    int GetUseFilteredPos(void) const	{ return m_useFilteredPos; }
    SPxErrorCode SetRotateSymbols(int state)
    {
	m_rotateSymbols = state;
	return SPX_NO_ERROR;
    }
    int GetRotateSymbols(void) const { return m_rotateSymbols; }

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    /*
     * Protected functions.
     */
    SPxTrackRenderConfig GetRenderConfig(void) const { return m_renderConfig; }
    SPxSymbol *GetTrackSymbol(void) const { return m_trackSymbol; }
    int GetShowCoastedVector(void) const { return m_showCoastedVector; }
    SPxRendererDash_t GetVectorDash(void) const { return m_vectorDash; }
    unsigned int GetTrackMode(UINT8 status, UINT8 numCoasts, int principal=TRUE);
    SPxErrorCode RenderVector(unsigned int mode, 
                              double xMetres, double yMetres, 
                              double courseDegrees, double speedMps, 
                              UINT32 colour);

    /*
     * Protected variables.
     */

    /* Track symbol */
    SPxSymbol *m_trackSymbol;

private:
    /*
     * Private variables.
     */

    /* Track render configuration. */
    SPxTrackRenderConfig m_renderConfig;

    /* Show only selected tracks? */
    int m_showOnlySelected;

    /* Is track selection using the mouse enabled? */
    int m_selectionEnabled;

    /* Is track selection using double clicks instead of single clicks? */
    int m_selectWithDoubleClick;

    /* Is track interception enabled (also needs track selection). */
    int m_interceptEnabled;

    /* Is selection of interception track using the mouse enabled? */
    int m_interceptSelectionEnabled;

    /* Is label movement enabled? */
    int m_labelMoveEnabled;

    /* Use measured or filtered track position? */
    int m_useFilteredPos;

    /* Rotate symbols. */
    int m_rotateSymbols;

    /* Track trail length in seconds */
    unsigned int m_trailLengthSecs;

    /* Selected track trail length in seconds, if > m_trailLengthSecs. */
    unsigned int m_trailLengthSecsSelected;

    /* Track vector line length in seconds */
    unsigned int m_vectorLengthSecs;

    /* Dash style for vector line */
    SPxRendererDash_t m_vectorDash;

    /* Use relative instead of true velocity for vector line. */
    int m_useRelativeVelocity;

    /* Show vectors for coasted tracks? */
    int m_showCoastedVector;

    /* Track labels */
    int m_labelAutoOffset;    /* Auto offset labels to avoid overlap? */
    int m_labelXOffsetPixels; /* X offset from track symbol in pixels */
    int m_labelYOffsetPixels; /* Y offset from track symbol in pixels */
    UINT32 m_labelColour;     /* Label ARGB colour */
    int m_showLabels;         /* Is label visible? */
    const char *m_label;      /* Label string to render */

    /*
     * Private functions.
     */

    /*
     * Private static functions.
     */

}; /* SPxTrackRendererBase */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_TRACK_RENDERER_BASE_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
