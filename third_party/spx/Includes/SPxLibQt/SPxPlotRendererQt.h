/*********************************************************************
*
* (c) Copyright zh.
*
* File: $RCSfile: SPxPlotRendererQt.h,v $
 
* Purpose:
*	Header for SPxPlotRendererQt class which supports the display
*	of plots in a Qt-based application.
*
*
* Previous Changes:
**********************************************************************/

#ifndef _SPX_PLOT_RENDERER_Qt_H
#define _SPX_PLOT_RENDERER_Qt_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxPlotRenderer.h"
#include "SPxLibQt/SPxRendererQt.h"


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
class SPxPlotRenderer;
class SPxRendererQt;
class SPxTargetDB;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxPlotRendererQt : public SPxPlotRenderer, public SPxRendererQt
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxPlotRendererQt(SPxTargetDB *plotDatabase);
    virtual ~SPxPlotRendererQt(void);

protected:
    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

private:
    /*
     * Private variables.
     */

    /*
     * Private functions.
     */

}; /* SPxPlotRendererQt */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_PLOT_RENDERER_Qt_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
