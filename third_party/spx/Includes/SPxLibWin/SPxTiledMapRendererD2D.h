/*********************************************************************
*
* (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTiledMapRendererD2D.h,v $
* ID: $Id: SPxTiledMapRendererD2D.h,v 1.4 2014/12/11 14:43:36 rew Exp $
*
* Purpose:
*	Header for SPxTiledMapRendererD2D class which supports the display
*	of a tiled map in a Direct2D Windows application.
*
* Revision Control:
*   11/12/14 v1.4    AGC	SetDestination() not longer requires factory.
*				Remove brightness/contrast adjustments.
*
* Previous Changes:
*   02/06/14 1.3    SP 	Add isChanged to CheckBitmaps().
*   04/10/13 1.2    SP 	Support changes to the base class.
*   27/09/13 1.1    AGC	Initial version.
*
**********************************************************************/

#ifndef _SPX_TILED_MAP_RENDERER_D2D_H
#define _SPX_TILED_MAP_RENDERER_D2D_H

/*
 * Other headers required.
 */

/* We need the base classes. */
#include "SPxLibUtils/SPxTiledMapRenderer.h"
#include "SPxLibWin/SPxRendererD2D.h"
#include "SPxLibWin/SPxBitmapWin.h"


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
class SPxTiledMapRenderer;
class SPxTerrainDatabase;
class SPxRendererD2D;
struct ID2D1Bitmap;
struct ID2D1BitmapRenderTarget;

/*
 * Define our class, derived from the base graphics renderer object
 * and also the WIN renderer to provide the rendering functions.
 */
class SPxTiledMapRendererD2D :  public SPxTiledMapRenderer, public SPxRendererD2D
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxTiledMapRendererD2D(SPxTiledMapDatabase *db, SPxTerrainDatabase *tdb=NULL);
    virtual ~SPxTiledMapRendererD2D(void);

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
#ifdef SPX_SUPPORT_D2D
    SPxComPtr<ID2D1Bitmap> m_mapBitmap;
    SPxComPtr<ID2D1BitmapRenderTarget> m_mapTarget;
#endif

    /*
     * Private functions.
     */

    SPxErrorCode CheckBitmaps(unsigned int mapWidthPixels,
                              unsigned int mapHeightPixels,
                              int *isChanged);

    SPxErrorCode ClearMapBitmap(unsigned int mapWidthPixels,
                                unsigned int mapHeightPixels);

    SPxErrorCode RenderTileToBitmap(const char *filename,
                                    int tileXPixels,
                                    int tileYPixels,
                                    int destXPixels,
                                    int destYPixels,
                                    double scale);

    SPxErrorCode RotateMap(unsigned int mapWidthPixels,
                           unsigned int mapHeightPixels);

    SPxErrorCode RenderMapToDestination(unsigned int mapWidthPixels,
                                        unsigned int mapHeightPixels,
                                        int mapViewCentreXPixels,
                                        int mapViewCentreYPixels,
                                        double scale);

}; /* SPxTiledMapRendererD2D */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* _SPX_TILED_MAP_RENDERER_D2D_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
