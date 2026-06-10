/*********************************************************************
*
* (c) Copyright 2012 - 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxViewControlRendererWin.h,v $
* ID: $Id: SPxViewControlRendererWin.h,v 1.5 2015/02/09 12:39:03 rew Exp $
*
* Purpose:
*	Header for SPxViewControlRendererWin class which supports the
*	rendering of view control objects: view control soft buttons,
*	range scale, and cursor position.
*
* Revision Control:
*   09/02/15 v1.5    AGC	Soft buttons now managed by base class.
*
* Previous Changes:
*   25/01/13 1.4    AGC	Support height cursor position option.
*   11/01/13 1.3    AGC	Remove need for nav data object.
*   12/12/12 1.2    AGC	Support moving-platform.
*   09/11/12 1.1    AGC	Initial Version.
* 
**********************************************************************/

#ifndef _SPX_VIEW_CONTROL_RENDERER_WIN_H
#define _SPX_VIEW_CONTROL_RENDERER_WIN_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxViewControlRenderer.h"
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
**********************************************************************/


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Need to forward-declare other classes in case headers are in wrong order. */
class SPxViewControlRenderer;
class SPxRendererWin;
class SPxWorldMap;
class SPxTerrainDatabase;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxViewControlRendererWin : public SPxViewControlRenderer, public SPxRendererWin
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    explicit SPxViewControlRendererWin(SPxWorldMap *worldMap=NULL,
				       SPxTerrainDatabase *tdb=NULL);
    virtual ~SPxViewControlRendererWin(void);

}; /* SPxViewControlRendererWin */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_VIEW_CONTROL_RENDERER_WIN_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
