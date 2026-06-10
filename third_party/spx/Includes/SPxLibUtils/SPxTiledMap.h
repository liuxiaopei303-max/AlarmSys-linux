/*********************************************************************
*
* (c) Copyright 2012 - 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTiledMap.h,v $
* ID: $Id: SPxTiledMap.h,v 1.5 2015/11/09 12:10:12 rew Exp $
*
* Purpose:
*   Header for SPxTiledMap base class. Note that this class
*   is not derived from SPxObj to enable classes to be derived
*   from this class and another class, e.g. SPxRenderer, which
*   is derived from SPxObj.
*
* Revision Control:
*   09/11/15 v1.5    SP 	Change CalculateRotation() arg names.
*
* Previous Changes:
*   04/10/13 1.4    SP 	Add InflateTiles().
*   10/04/13 1.3    AGC	Add MercatorYPixelsToLat().
*   30/08/12 1.2    SP 	Make all functions public.
*   09/08/12 1.1    SP 	Initial version.
* 
**********************************************************************/

#ifndef _SPX_TILED_MAP_H
#define _SPX_TILED_MAP_H

/*
 * Other headers required.
 */

/* Include common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxStrings.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* X and Y dimensions of a map tile in pixels. */
#define SPX_TILED_MAP_TILE_PIXELS     256

/* Minimum and maximum tile zoom. */
#define SPX_TILED_MAP_MIN_ZOOM        0
#define SPX_TILED_MAP_MAX_ZOOM        18

/* Max/min latitude is +/- this value. The value of (2 * 85.0511) 
 * is the height of the world in degrees latitude using a 
 * Mercator projection. 
 */
#define SPX_TILED_MAP_MAX_LAT_DEGS    85.0511

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

/*
 * Define our class.
 */
class SPxTiledMap
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxTiledMap(void);
    virtual ~SPxTiledMap(void);   

    /*
     * Public static functions.
     */

    static int GetNumXYTiles(int z);

    static int GetTileXFromLon(double lonDegs, int z);
    static int GetTileYFromLat(double latDegs, int z);

    static double GetLonWFromTileX(int x, int z);
    static double GetLatNFromTileY(int y, int z);

    static double GetTilesWidthDegs(int eastX, int westX, int z);
    static double GetTilesHeightDegs(int northY, int southY, int z);

    static int GetNumXTiles(double lonEDegs, 
                            double lonWDegs,
                            int z);
    static int GetNumYTiles(double latNDegs, 
                            double latSDegs,
                            int z);

    static void InflateTiles(int *x, int *y, 
                             int *numXTiles, int *numYTiles,
                             int z,
                             int extraXTiles, int extraYTiles);

    static int GetClosestZoom(double degsLonPerPixel);

    static int IsTileInView(int x, int y, int z,
                            double latNDegs, double lonWDegs,
                            double latSDegs, double lonEDegs);

    static void GetTilesInView(double latNDegs, double lonWDegs,
                               double latSDegs, double lonEDegs,
                               int z,
                               int *xRtn, int *yRtn,
                               int *numXRtn, int *numYRtn);

    static void GetTileSubArea(int zIn,
                               int xSub, int ySub, int zSub,
                               int *xPixelsRtn, int *yPixelsRtn,
                               double *scaleRtn);

    static double MercatorLatToYPixels(int mapHeightPixels, 
                                       double latNorthDegs, 
                                       double latSouthDegs,
                                       double latDegs);

    static double MercatorYPixelsToLat(int mapHeightPixels, 
                                       double latNorthDegs, 
                                       double latSouthDegs,
                                       double yPixels);

    static double MercatorLatToLinearY(double latDegs);

    static double MercatorLinearYToLat(double linearY);

    static double LatAdd(double latDegs, double valDegs);

    static double LongAdd(double longDegs, double valDegs);

    static double CalculateRotation(double startDegs, double endDegs);

    static double NormaliseAngle(double degs);

    /*
     * Protected variables.
     */

protected:

    /*
     * Protected functions.
     */

    /*
     * Protected variables.
     */

private:

    /*
     * Private variables.
     */
   
    /*
     * Private functions.
     */

}; /* SPxTiledMap */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* SPX_TILED_MAP_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
