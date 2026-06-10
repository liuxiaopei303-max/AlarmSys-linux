/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxADSBRenderer.h,v $
* ID: $Id: SPxADSBRenderer.h,v 1.4 2017/07/13 14:01:14 rew Exp $
*
* Purpose:
*	Header for SPxADSBRenderer abstract class which supports the display
*	of ADSB messages in an application, but only via a derived class.
*
*
* Revision Control:
*   13/07/17 v1.4    SP 	Add GetDecoder().
*
* Previous Changes:
*   07/10/13 1.3    AGC	Move rendering to renderAll().
*   04/10/13 1.2    AGC	Simplify headers.
*   23/12/11 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_ADSB_RENDERER_H
#define _SPX_ADSB_RENDERER_H

/*
 * Other headers required.
 */
/* We need SPxLibUtils for common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxRenderer.h"

/* We need the SPxADSBDecoder class. */
#include "SPxLibNet/SPxADSBDecoder.h"


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

/* Need to forward-declare other classes in case headers are in wrong order. */
class SPxADSBDecoder;
class SPxADSBTrack;
class SPxRenderer;

/*
 * Define our class, derived from the abstract renderer base class.
 */
class SPxADSBRenderer : virtual public SPxRenderer
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxADSBRenderer(SPxADSBDecoder *decoder);
    virtual ~SPxADSBRenderer(void);

    /* Get decoder. */
    SPxADSBDecoder *GetDecoder(void) { return m_decoder; }

    /* Symbol control. */
    void SetSymbolLength(UINT32 pixels)		{ m_symbolLength = pixels; }
    UINT32 GetSymbolLength(void)		{ return(m_symbolLength); }

    /* Colour control. */
    void SetSymbolColour(UINT32 argb)		{ m_symbolColour = argb; }
    UINT32 GetSymbolColour(void)		{ return(m_symbolColour); }

    /* Vector control. */
    void SetVectorSeconds(UINT32 secs)		{ m_vectorSeconds = secs; }
    UINT32 GetVectorSeconds(void)		{ return(m_vectorSeconds); }

    /* Label control. */
    void SetShowAllLabels(int show)		{m_showAllLabels = show;}
    int GetShowAllLabels(void)			{return(m_showAllLabels);}
    void SetShowActivatedLabels(int show)	{m_showActivatedLabels = show;}
    int GetShowActivatedLabels(void)		{return(m_showActivatedLabels);}
    void SetShowSelectedLabels(int show)	{m_showSelectedLabels = show;}
    int GetShowSelectedLabels(void)		{return(m_showSelectedLabels);}

    /* History trail control. */
    void SetHistorySeconds(UINT32 secs)		{ m_historySeconds = secs; }
    UINT32 GetHistorySeconds(void)		{ return(m_historySeconds); }
    void SetShowAllTrails(int show)		{m_showAllTrails = show;}
    int GetShowAllTrails(void)			{return(m_showAllTrails);}
    void SetShowActivatedTrails(int show)	{m_showActivatedTrails = show;}
    int GetShowActivatedTrails(void)		{return(m_showActivatedTrails);}
    void SetShowSelectedTrails(int show)	{m_showSelectedTrails = show;}
    int GetShowSelectedTrails(void)		{return(m_showSelectedTrails);}

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
    /* Handle of associated decoder object. */
    SPxADSBDecoder *m_decoder;		/* Decoder */

    /* Rendering options. */
    UINT32 m_symbolLength;		/* Pixels */
    UINT32 m_vectorSeconds;		/* Length of vector */
    UINT32 m_historySeconds;		/* Length of trail history */
    int m_showAllLabels;		/* Show all vessel names */
    int m_showActivatedLabels;		/* Show activated vessel names */
    int m_showSelectedLabels;		/* Show selected vessel names */
    int m_showAllTrails;		/* Show all history trails */
    int m_showActivatedTrails;		/* Show activated history trails */
    int m_showSelectedTrails;		/* Show selected history trails */

    /* Rendering colours */
    UINT32 m_symbolColour;		/* Colour, 0xAARRGGBB */

    /*
     * Private functions.
     */
    virtual SPxErrorCode renderAll(void);
    SPxErrorCode renderTrack(SPxADSBTrack *track);
    static int staticRenderTrack(void *decoder, void *arg, void *track);

}; /* SPxADSBRenderer */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* SPX_ADSB_RENDERER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
