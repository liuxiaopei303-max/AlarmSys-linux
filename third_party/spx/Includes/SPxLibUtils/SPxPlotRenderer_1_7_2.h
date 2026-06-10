/*********************************************************************
*
* (c) Copyright 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxPlotRenderer.h,v $
* ID: $Id: SPxPlotRenderer.h,v 1.1 2015/11/13 11:58:26 rew Exp $
*
* Purpose:
*	Header for SPxPlotRenderer base class which supports the display
*	of plots.
*
* Revision Control:
*   13/11/15 v1.1    REW	Initial Version.
*
* Previous Changes:
**********************************************************************/

#ifndef _SPX_PLOT_RENDERER_H
#define _SPX_PLOT_RENDERER_H

/*
 * Other headers required.
 */

/* Include common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxRenderer.h"
#include "SPxLibUtils/SPxError.h"

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

/* Forward declare other classes required. */
class SPxTargetDB;

/*
 * Define our class, derived from the abstract renderer base class.
 */
class SPxPlotRenderer : virtual public SPxRenderer
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxPlotRenderer(SPxTargetDB *plotDatabase);
    virtual ~SPxPlotRenderer(void);

    /* Configuration functions. */
    SPxErrorCode SetColour(UINT32 argb);
    UINT32 GetColour(void) const { return m_colour; }

    /* What are we drawing? */
    SPxErrorCode SetDrawBBoxes(int state);
    int GetDrawBBoxes(void) const { return(m_drawBBoxes); }

protected:
    /*
     * Protected functions.
     */
    /* Implement the function in the base class. */
    virtual SPxErrorCode renderAll(void);

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, 
                             char *parameterValue);

    virtual int GetParameter(char *parameterName, 
                             char *valueBuf, int bufLen);

private:
    /*
     * Private variables.
     */
    /* Database of plots to draw. */
    SPxTargetDB * const m_plotDatabase;	/* Database of SPxBlob_t reports */

    /* Appearance. */
    UINT32 m_colour;			/* Colour to use. */

    /* Options. */
    int m_drawBBoxes;			/* Draw bounding boxes? */

    /*
     * Private functions.
     */
    SPxErrorCode renderPlot(struct SPxBlob_tag *plot);

}; /* SPxPlotRenderer */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_PLOT_RENDERER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
