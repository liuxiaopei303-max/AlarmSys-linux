/*********************************************************************
*
* (c) Copyright 2013 - 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxViewControlRendererD2D.h,v $
* ID: $Id: SPxViewControlRendererD2D.h,v 1.3 2015/02/09 12:39:03 rew Exp $
*
* Purpose:
*	Header for SPxViewControlRendererWin class which supports the
*	rendering of view control objects: view control soft buttons,
*	range scale, and cursor position.
*
* Revision Control:
*   09/02/15 v1.3    AGC	Soft buttons now managed by base class.
*
* Previous Changes:
*   11/12/14 1.2    AGC	SetDestination() no longer requires factory.
*   27/09/13 1.1    AGC	Initial Version.
* 
**********************************************************************/

#ifndef _SPX_VIEW_CONTROL_RENDERER_D2D_H
#define _SPX_VIEW_CONTROL_RENDERER_D2D_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxViewControlRenderer.h"
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
class SPxViewControlRenderer;
class SPxRendererD2D;
class SPxWorldMap;
class SPxTerrainDatabase;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxViewControlRendererD2D : public SPxViewControlRenderer, public SPxRendererD2D
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    explicit SPxViewControlRendererD2D(SPxWorldMap *worldMap=NULL,
				       SPxTerrainDatabase *tdb=NULL);
    virtual ~SPxViewControlRendererD2D(void);

}; /* SPxViewControlRendererD2D */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_VIEW_CONTROL_RENDERER_D2D_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
