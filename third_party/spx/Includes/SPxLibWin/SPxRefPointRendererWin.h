/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxRefPointRendererWin.h,v $
 * ID: $Id: SPxRefPointRendererWin.h,v 1.1 2017/08/22 14:24:38 rew Exp $
 *
 * Purpose:
 *   Header for SPxRefPointRendererWin class which supports the display
 *   of reference points in a Windows-based application.
 *
 * Revision Control:
 *   22/08/17 v1.1    SP     Initial Version.
 *
 * Previous Changes:
 *
 *********************************************************************/

#ifndef _SPX_REF_POINT_RENDERER_WIN_H
#define _SPX_REF_POINT_RENDERER_WIN_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxRefPointRenderer.h"
#include "SPxLibWin/SPxRendererWin.h"


/*********************************************************************
 *
 *   Constants
 *
 **********************************************************************/


/*********************************************************************
 *
 *   Macros
 *
 *********************************************************************/


/*********************************************************************
 *
 *   Type definitions
 *
 *********************************************************************/

/* Need to forward-declare other classes in case headers are in wrong order. */
class SPxRefPointRenderer;
class SPxRendererWin;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxRefPointRendererWin : public SPxRefPointRenderer, public SPxRendererWin
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxRefPointRendererWin(SPxUniTrackDatabase *database);
    virtual ~SPxRefPointRendererWin(void);

protected:
    /*
     * Protected functions.
     */

private:
    /*
     * Private variables.
     */

}; /* SPxRefPointRendererWin */


/*********************************************************************
 *
 *   Function prototypes
 *
 *********************************************************************/

#endif /* SPX_REF_POINT_RENDERER_WIN_H */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
