/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxThreatRenderer.h,v $
* ID: $Id: SPxThreatRenderer.h,v 1.17 2017/08/31 15:35:36 rew Exp $
*
* Purpose:
*   Header for SPxThreatRenderer base class which supports the display
*   of threats in an application, but only via a derived class.
*
* Revision Control:
*   31/08/17 v1.17   AGC	Tidy up indentation.
*
* Previous Changes:
*   23/08/16 1.16   AGC	Support minimal gate display.
*			Support interaction with gates.
*   30/11/15 1.15  REW	Support AllCameraSectorsVisible.
*   15/07/15 1.14  SP 	Support rendering of gate symbols.
*   23/06/15 1.13  AGC	Allow small movement when selecting.
*   29/04/15 1.12  AGC	Add camera fov edge display option.
*   16/02/15 1.11  AGC	Set cursor correctly on mouse press/release.
*   30/01/15 1.10  REW	Add options for track, camera and label display.
*   20/06/14 1.9   AGC	Remove unused vector header.
*   13/12/13 1.8   SP 	Support parameter save and restore.
*   24/10/13 1.7   SP 	Allow click margin to be changed.
*   15/10/13 1.6   SP 	Add m_hasMouseMoved flag.
*   07/10/13 1.5   AGC	Move rendering to renderAll().
*   17/09/13 1.4   SP  	Move RotateXY to SPxMaths.
*   09/09/13 1.3   SP  	Add mouse handling functions.
*                       Add GetObjectAtPoint().
*   23/08/13 1.2   SP  	Add RotateXY().
*   14/08/13 1.1   SP  	Initial version.
* 
**********************************************************************/

#ifndef _SPX_THREAT_RENDERER_H
#define _SPX_THREAT_RENDERER_H

/*
 * Other headers required.
 */

/* Other classes we need. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibNet/SPxTrackRenderer.h"
#include "SPxLibNet/SPxThreatDatabase.h"

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

/*
 * Define our class.
 */
class SPxThreatRenderer : public SPxTrackRenderer
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxThreatRenderer(SPxThreatDatabase *database);
    virtual ~SPxThreatRenderer(void);

    /* Top-level display filters. */
    virtual SPxErrorCode SetThreatsVisible(int visible);
    virtual int GetThreatsVisible(void) const { return(m_threatsVisible); }
    virtual SPxErrorCode SetCamerasVisible(int visible);
    virtual int GetCamerasVisible(void) const { return(m_camerasVisible); }
    virtual SPxErrorCode SetAllCameraSectorsVisible(int visible);
    virtual int GetAllCameraSectorsVisible(void) const { return(m_allCameraSectorsVisible); }
    virtual SPxErrorCode SetCameraFovEdgesVisible(int visible);
    virtual int GetCameraFovEdgesVisible(void) const { return(m_cameraFovEdgesVisible); }
    virtual SPxErrorCode SetGatesVisible(int visible);
    virtual int GetGatesVisible(void) const { return(m_gatesVisible); }
    virtual SPxErrorCode SetGatesMinimal(int minimal);
    virtual int GetGatesMinimal(void) const { return(m_gatesMinimal); }
    virtual SPxErrorCode SetUseThreatLabels(int use);
    virtual int GetUseThreatLabels(void) const { return(m_useThreatLabels); }

    /* Track display threat level filters. */
    virtual SPxErrorCode SetThreatLevelVisible(SPxThreat::Level_t level, int isVisible);
    virtual int IsThreatLevelVisible(SPxThreat::Level_t level);

    /* Track display type filters. */
    virtual SPxErrorCode SetThreatTypeVisible(SPxThreat::Type_t type, int isVisible);
    virtual int IsThreatTypeVisible(SPxThreat::Type_t type);

    /* Track display label filters. */
    virtual SPxErrorCode SetThreatLabelVisible(SPxThreat::Level_t level, int isVisible);
    virtual int IsThreatLabelVisible(SPxThreat::Level_t level);

    /* Get object in window. */
    virtual void SetClickMarginPixels(unsigned int pixels) { m_clickMarginPx = pixels; }
    virtual unsigned int GetClickMarginPixels(void) { return m_clickMarginPx; }
    virtual void GetObjectAtPoint(int xPixels, int yPixels,
                                  SPxThreat **threatRtn,
                                  SPxCamera **cameraRtn=NULL,
                                  SPxGate **gateRtn=NULL,
                                  SPxLatLong_t *llRtn=NULL);

    /* Start / clear ruler. */
    virtual void SetRulerInProgress(int state);
    virtual int IsRulerInProgress(void) { return m_isRulerInProgress; }

    /* Gate adjustment. */
    void SetGateInProgress(int state);
    int IsGateInProgress(void) const { return m_isGateInProgress; }

    /* Mouse handling. */
    virtual int HandleMousePress(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseRelease(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseLeave(void);

    /* Colour control. */
    void SetCameraColour(UINT32 argb) 	{ m_argbCamera = argb; }
    UINT32 GetCameraColour(void) const	{ return(m_argbCamera); }
    void SetSelectedCameraColour(UINT32 argb)  { m_argbSelectedCamera = argb; }
    UINT32 GetSelectedCameraColour(void) const {return(m_argbSelectedCamera);}

protected:
    /*
     * Protected variables.
     */

    /* Parameter setting. */
    int SetParameter(char *parameterName, char *parameterValue);
    int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /* Handle of associated database object. */
    SPxThreatDatabase *m_database;

    /*
     * Protected functions.
     */

    SPxErrorCode RenderTrack(SPxRadarTrack *track);

    void GetTrackLabel(SPxRadarTrack *track,
                       char *buffer,
                       unsigned int bufferSizeBytes);

private:
    /*
     * Private variables.
     */
    /* Top level display filters. */
    int m_threatsVisible;
    int m_camerasVisible;
    int m_allCameraSectorsVisible;
    int m_gatesVisible;
    int m_gatesMinimal;
    int m_useThreatLabels;
    int m_cameraFovEdgesVisible;

    /* Track rendering filters. */
    int m_isTypeVisible[SPxThreat::NUM_TYPES];
    int m_isLevelVisible[SPxThreat::NUM_LEVELS];
    int m_isLabelVisible[SPxThreat::NUM_LEVELS];

    /* Ruler. */
    int m_isRulerInProgress;       /* Is ruler being defined? */

    /* Gate. */
    int m_isGateInProgress;
    SPxGate *m_movingGate;
    int m_isMovingGateStart;

    /* Mouse handling. */
    int m_pressPosX;
    int m_pressPosY;
    unsigned int m_clickMarginPx;  /* Click margin for finding objects in pixels. */
    
    /* Colours. */
    UINT32 m_argbCamera;		/* Standard camera colour */
    UINT32 m_argbSelectedCamera;	/* Selected camera colour */

    /*
     * Private functions.
     */

    virtual SPxErrorCode renderAll(void);
    void setCursor(int x, int y, SPxCursor_t *cursor);
    void RenderCameraSymbols(void);
    SPxErrorCode RenderCameraSymbol(SPxCamera *camera);
    SPxErrorCode RenderRuler(void);
    void RenderGateSymbols(void);
    SPxErrorCode RenderGateSymbol(SPxGate *gate);
    int HandleRulerMouseRelease(int x, int y);

    /*
     * Private static functions.
     */

}; /* SPxThreatRenderer */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_THREAT_RENDERER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
