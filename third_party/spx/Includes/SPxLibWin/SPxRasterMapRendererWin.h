/*********************************************************************
*
* (c) Copyright 2013 - 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxRasterMapRendererWin.h,v $
* ID: $Id: SPxRasterMapRendererWin.h,v 1.4 2015/05/18 13:52:03 rew Exp $
*
* Purpose:
*	Header for SPxRasterMapRendererWin class which supports the
*	rendering of raster images under Windows.
*
* Revision Control:
*   18/05/15 v1.4    AGC	Support rotation with multiple images.
*
* Previous Changes:
*   20/06/14 1.3    SP 	Support contrast adjustment.
*   04/10/13 1.2    SP 	Support display rotation.
*   01/08/13 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_RASTER_MAP_RENDERER_WIN_H
#define _SPX_RASTER_MAP_RENDERER_WIN_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxRasterMapRenderer.h"
#include "SPxLibWin/SPxRendererWin.h"
#include "SPxLibWin/SPxBitmapWin.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Need to forward-declare other classes in case headers are in wrong order. */
class SPxRasterMapRenderer;
class SPxRendererWin;
class SPxImage;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxRasterMapRendererWin : public SPxRasterMapRenderer, public SPxRendererWin
{
public:
    /* Constructor and destructor. */
    SPxRasterMapRendererWin(void);
    virtual ~SPxRasterMapRendererWin(void);

private:
    /* Private variables. */

    /* Private functions. */
    virtual SPxErrorCode renderBitmap(SPxImage *image,
	int x, int y, double scale, double brightness, double contrast,
	void **cachedImagePtr, double *rotationDegs, int changed);
    virtual void freeCachedBitmap(void *cachedBitmap);

}; /* SPxRasterMapRendererWin */

#endif /* _SPX_RASTER_MAP_RENDERER_WIN_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
