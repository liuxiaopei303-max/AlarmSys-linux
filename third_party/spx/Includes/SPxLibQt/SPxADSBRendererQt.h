/*********************************************************************
*
* (c) Copyright 2015, Cambridge Pixel Ltd.
*
*
* Purpose:
*	Header for SPxADSBRendererQt class which supports the display
*	of ADSB messages in a Qt-based application.
*
* Revision Control:
*   13/04/15 v1.1    JP 	Initial Version.
*
* Previous Changes:
*
**********************************************************************/

#ifndef _SPX_ADSB_RENDERER_QT_H
#define _SPX_ADSB_RENDERER_QT_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibNet/SPxADSBRenderer.h"
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
class SPxADSBRenderer;
class SPxRendererQt;

/*
 * Define our class, derived from the base ADSB renderer object
 * and also the Qt renderer to provide the rendering functions.
 */
class SPxADSBRendererQt :  public SPxADSBRenderer, public SPxRendererQt
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxADSBRendererQt(SPxADSBDecoder *decoder);
    virtual ~SPxADSBRendererQt(void);

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

}; /* SPxADSBRendererQt */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* SPX_ADSB_RENDERER_QT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
