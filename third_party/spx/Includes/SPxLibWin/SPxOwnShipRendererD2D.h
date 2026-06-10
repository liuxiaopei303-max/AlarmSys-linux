/*********************************************************************
*
* (c) Copyright 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxOwnShipRendererD2D.h,v $
* ID: $Id: SPxOwnShipRendererD2D.h,v 1.1 2013/09/27 13:59:31 rew Exp $
*
* Purpose:
*   Header for SPxOwnShipRendererD2D class which supports the display
*   of an own ship marker in a Windows-based application.
*
* Revision Control:
*   27/09/13 v1.1    AGC	Initial Version.
*
* Previous Changes:
* 
**********************************************************************/

#ifndef _SPX_OWN_SHIP_RENDERER_D2D_H
#define _SPX_OWN_SHIP_RENDERER_D2D_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxOwnShipRenderer.h"
#include "SPxLibWin/SPxRendererD2D.h"


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
class SPxOwnShipRenderer;
class SPxRendererD2D;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxOwnShipRendererD2D : public SPxOwnShipRenderer, public SPxRendererD2D
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxOwnShipRendererD2D(SPxNavData *navData);
    virtual ~SPxOwnShipRendererD2D(void);

protected:
    /*
     * Protected functions.
     */

private:
    /*
     * Private variables.
     */

}; /* SPxOwnShipRendererD2D */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_OWN_SHIP_RENDERER_D2D_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
