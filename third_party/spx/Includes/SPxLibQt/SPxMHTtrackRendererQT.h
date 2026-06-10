/*********************************************************************
*
* (c) Copyright zh.
*
* File: $RCSfile: SPxMHTtrackRendererQT.h,v $
 
* Purpose:
*	Header for SPxMHTtrackRendererQT class which supports the display
*	of tracks in a Qt-based application.
*
*
* Previous Changes:
**********************************************************************/

#ifndef _SPX_MHTtrack_RENDERER_Qt_H
#define _SPX_MHTtrack_RENDERER_Qt_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibTracker/SPxMHTtrackRenderer.h"
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
class SPxMHTtrackRenderer;
class SPxRendererQt;
class SPxMHT;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxMHTtrackRendererQT : public SPxMHTtrackRenderer, public SPxRendererQt
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxMHTtrackRendererQT(SPxMHT *pMHT);
    virtual ~SPxMHTtrackRendererQT(void);

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

}; /* _SPX_MHTtrack_RENDERER_Qt_H */


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
