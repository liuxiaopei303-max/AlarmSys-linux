/*********************************************************************
*
*
* File: $RCSfile: SPxMHTtrackRenderer.h,v $
* ID: $Id: SPxMHTtrackRenderer.h 
*
* Purpose:
*   Header for SPxMHTtrackRenderer base class which supports the display
*   of radar tracks in an application,  from a local tracker,
*
**********************************************************************/

#ifndef SPX_TRACK_RENDERER_MHT_H
#define SPX_TRACK_RENDERER_MHT_H

/*
 * Other headers required.
 */

/* We need SPxLibUtils for common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/* We need the SPxMHT class. */
#include "SPxLibTracker/SPxMHT.h"

/* We need our base class. */
#include "SPxLibUtils/SPxTrackRendererBase.h"
/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Need to forward-declare other classes in case headers are in wrong order.*/

class SPxTrackRendererBase;


/*
 * Define our class, derived from the abstract renderer base class.
 */
class SPxMHTtrackRenderer : public SPxTrackRendererBase
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxMHTtrackRenderer(SPxMHT *pMHT,SPxTrackRenderConfig renderConfig = SPX_TRACK_RENDER_CONFIG_SPX);
    virtual ~SPxMHTtrackRenderer(void);

   /* Get database. */
    SPxMHT *GetMHT(void) { return m_pMHT; }

    /* Mouse handling. */
   // virtual int HandleMousePress(UINT32 flags, int x, int y, SPxCursor_t *cursor);
   // virtual int HandleMouseRelease(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleDoubleClick(UINT32 flags, int x, int y, SPxCursor_t *cursor);
   // virtual int HandleMouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
   // virtual int HandleMouseLeave(void);



    /* Colour of selected target (if non-zero). */
    SPxErrorCode SetSelectedColour(UINT32 argb);
    UINT32 GetSelectedColour(void) const;
	/* Info panel control. */
	void SetShowInfoPanel(int show) { m_showInfoPanel = show; }
	int GetShowInfoPanel(void) { return(m_showInfoPanel); }
	void SetInfoPanelPos(int x, int y) { m_infoPanelX = x; m_infoPanelY = y; }
	void GetInfoPanelPos(int *x, int *y);

	/* What are we drawing? */
	SPxErrorCode SetDrawMarkers(int state);
	SPxErrorCode SetDrawSearchGate(int state);
	SPxErrorCode SetDrawProvisionalTracks(int state);
	SPxErrorCode SetDrawAllHypos(int state);
	SPxErrorCode SetDrawTrackClusterBoxes(int state);
	SPxErrorCode SetDrawTrackerClutter(int state);

	/* Vector control. */
	void SetVectorSeconds(UINT32 secs) { m_vectorSeconds = secs; }
	UINT32 GetVectorSeconds(void) { return(m_vectorSeconds); }
    /* Generic parameter assignment. */
   // virtual int SetParameter(char *parameterName, char *parameterValue);
   // virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    /*
     * Protected functions.
     */
    virtual SPxErrorCode renderAll(void);

	SPxErrorCode renderClusterBoxes(SPxMHTcluster *pCluster);
	SPxErrorCode renderHypothesis(SPxMHThypothesis *pHypo, int isPrincipleHypo);
    SPxErrorCode renderTrack(SPxMHTtrack *track,bool isInPrincipalHypothesis, int saveTrail);
	/*绘制雷达标志*/
	SPxErrorCode         renderTrackMerkers(UINT32 colour, SPxMHTtrack *pTrack, double trkXPixels, double trkYPixels);
	SPxErrorCode         renderTrackInfo(UINT32 colour, SPxMHTtrack *pTrack, double trkXPixels, double trkYPixels);
	/*绘制雷达当前的搜索波门*/
	SPxErrorCode         renderTrackSearchGate(SPxMHTtrack *pTrack);

    SPxErrorCode         renderTrackVector(UINT32 colour,SPxMHTtrack *pTrack,int trackVectorSeonds, double trkXPixels, double trkYPixels);
	SPxErrorCode         renderTrackSelected(UINT32 colour, SPxMHTtrack *pTrack, double trkXPixels, double trkYPixels);
    SPxErrorCode         renderTrailPoint();
	/*绘制信息板*/
	SPxErrorCode         renderInfoPanel(SPxMHTtrack *pTrack);
private:
    /*
     * Private variables.
     */



    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Mutex protection. */
    SPxCriticalSection m_mutex;

    /* Handle of associated SPxMHT object. */
    SPxMHT *m_pMHT;


	UINT32 m_colourEstablishedTrack_Updated_InPrincipalHypothesis;//established track Updated, in principal hypothesis – Green
	UINT32 m_colourProvisionalTrack_Updated_InPrincipalHypothesis;//provisional track Updated, in principal hypothesis – Half intensity Green

	UINT32 m_colourEstablishedTrack_Coasted_InPrincipalHypothesis;//established track Coasted, in principal hypothesis – Red
	UINT32 m_colourProvisionalTrack_Coasted_InPrincipalHypothesis;//provisional track Coasted, in principal hypothesis – Half intensity Red


	UINT32 m_colourEstablishedTrack_Updated_InNonPrincipalHypothesis;//established track Updated, in non - principal hypothesis – Bright Blue
	UINT32 m_colourProvisionalTrack_Updated_InNonPrincipalHypothesis;//provisional track Updated, in non - principal hypothesis – Half intensity Blue

	UINT32 m_colourEstablishedTrack_Coasted_InNonPrincipalHypothesis;//established track Coasted, in non - principal hypothesis – Magenta
	UINT32 m_colourProvisionalTrack_Coasted_InNonPrincipalHypothesis;//provisional track Coasted, in non - principal hypothesis – Half intensity Magenta

	UINT32 m_colourLostTrack;
	UINT32 m_colorTrackSearchGate;//搜索波门的颜色
	UINT32 m_colorDiagonalLineInTrackSearchGate;//搜索波门斜角线的颜色
	//other
	UINT32 m_colorTrackAttachedMBTModelOrClassText;//属于mbt模型或且 class 文字的颜色
	/*考虑机动情况*/
	UINT32 m_colourEstablishedTrack_Manoeuvering_Updated_InPrincipalHypothesis;//established track Updated, in principal hypothesis – Green



    /* Colour for selected targets, if non-zero. */
    UINT32 m_selectedColour;
	/* Track   selected. */
	UINT32 m_selectedTrackID;	/* ID of track we're selected */

	/* Rendering options. */
	UINT32 m_vectorSeconds;		/* Length of vector */
	/* Options. */
	int m_isDrawTrackMarkers;			/* 是否标绘制航迹标志. */
	int m_isDrawTrackSearchGate;			/* Draw 是否显示航迹的搜索波门 ? */
	int m_isDrawProvisionalTracks;//是否显示处于暂态状态的航迹
	int m_isDrawAllHypos;//是否显示所有的假设，如果不是那么只显示主假设
	int m_isDrawTrackClusterBoxes;//
	int m_isDrawTrackerClutter;

	int m_isDrawTrackVectors;
	int m_isDrawTrackTrails;

	UINT32 m_panelColourBg;		/* Background colour for info panel. */
	UINT32 m_panelColourBorder;		/* Border colour for info panel. */
	UINT32 m_panelColourText;		/* Text colour for info panel. */
	int m_showInfoPanel;		/* Show info panel for selected track. */

	/* Info panel. */
	int m_infoPanelDrawn;		/* Has an info panel been drawn? */
	int m_infoPanelX;			/* X Position for info panel. */
	int m_infoPanelY;			/* Y Position for info panel. */

    /*
     * Private functions.
     */
    int selectTrack(UINT32 flags, int x, int y, SPxCursor_t *cursor);
	SPxErrorCode getWinXYFromRadarCoordMetres(double xMetresRelativeToRadar, double yMetresRelativeToRadar, 
		double *xPixelsRtn, double *yPixelsRtn) const;
 
}; /* SPxMHTtrackRenderer */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_TRACK_RENDERER_MHT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
