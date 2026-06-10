/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimScene.h,v $
 * ID: $Id: SPxVidSimScene.h,v 1.8 2016/10/31 15:37:51 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimScene class header.
 *
 * Revision Control:
 *   31/10/16 v1.8   SP 	Support terrain tiled map.
 *
 * Previous Changes:
 *   07/10/16 1.7   SP 	Derive from SPxVidSimMovable.
 *   14/10/15 1.6   SP 	Add Set/GetNearClipMetres().
 *   06/10/15 1.5   SP 	Allow track metres to be used in preference to lat/long.
 *   01/10/15 1.4   SP 	Support configurable land & sea colours.
 *   26/08/15 1.3   SP 	Add Initialise().
 *   19/07/13 1.2   SP 	Further development.
 *   15/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_SCENE_H
#define _SPX_VID_SIM_SCENE_H

/* Other headers required. */
#include "SPxLibUtils/SPxVidSimMovable.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/*********************************************************************
 *
 *  Constants
 *
 *********************************************************************/

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

/*********************************************************************
 *
 *   Class definitions
 *
 *********************************************************************/

/* Forward declare any classes required. */
class SPxTerrainDatabase;
class SPxTiledMapDatabase;
class SPxVidSimPlatform;
class SPxVidSimModelDatabase;
class SPxTrackDatabase;
class SPxTiledMapRendererWin;
class SPxRemoteServer;
class SPxBitmapWin;

class SPxVidSimScene : public SPxVidSimMovable
{
public:

    /*
     * Public types.
     */

    typedef enum
    {
        DETAIL_LEVEL_LOW,
        DETAIL_LEVEL_MEDIUM,
        DETAIL_LEVEL_HIGH,
        _DETAIL_LEVEL_LAST /* Must be last entry. */

    } DetailLevel_t;

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimScene(void);
    virtual ~SPxVidSimScene(void);
    virtual SPxErrorCode Initialise(void);

    /* Child platform. */
    virtual SPxErrorCode SetPlatform(SPxVidSimPlatform *platform);
    virtual SPxVidSimPlatform *GetPlatform(void) { return m_platform; }

    /* Object access. */
    virtual SPxTerrainDatabase *GetTerrainDB(void) { return m_terrainDB; }
    virtual SPxTiledMapDatabase *GetTiledMapDB(void) { return m_tiledMapDB; }
    virtual SPxVidSimModelDatabase *GetModelDB(void) { return m_modelDB; }
    virtual SPxTrackDatabase *GetTrackDB(void) { return m_trackDB; }

    /* Scene boundaries. */
    virtual SPxErrorCode SetLatLong(double latDegs, double lonDegs);
    virtual SPxErrorCode SetSizeMetres(double width, 
                                       double length,
                                       double height);
    virtual double GetWidthMetres(void) { return m_sceneWidthMetres; }
    virtual double GetLengthMetres(void) { return m_sceneLengthMetres; }
    virtual double GetHeightMetres(void) { return m_sceneHeightMetres; }

    virtual void ShowFloor(int state);
    virtual int IsFloorVisible(void) { return m_showFloor; }

    virtual void ShowCeiling(int state);
    virtual int IsCeilingVisible(void) { return m_showCeiling; }

    virtual void ShowNorthWall(int state);
    virtual int IsNorthWallVisible(void) { return m_showNorthWall; }

    virtual void ShowSouthWall(int state);
    virtual int IsSouthWallVisible(void) { return m_showSouthWall; }

    virtual void ShowEastWall(int state);
    virtual int IsEastWallVisible(void) { return m_showEastWall; }

    virtual void ShowWestWall(int state);
    virtual int IsWestWallVisible(void) { return m_showWestWall; }

    virtual void SetWallRGB(UINT32 rgb);
    virtual UINT32 GetWallRGB(void) { return m_wallRGB; }

    virtual void SetCeilingRGB(UINT32 rgb);
    virtual UINT32 GetCeilingRGB(void) { return m_ceilingRGB; }

    virtual void SetFloorRGB(UINT32 rgb);
    virtual UINT32 GetFloorRGB(void) { return m_floorRGB; }

    virtual void SetBackgroundRGB(UINT32 rgb);
    virtual UINT32 GetBackgroundRGB(void) { return m_backgroundRGB; }

    /* Terrain. */
    virtual SPxErrorCode SetTerrainLandThreshMetres(double metres);
    virtual double GetTerrainLandThreshMetres(void) { return m_terrainLandThreshMetres; }

    virtual SPxErrorCode SetTerrainZCorrectionMetres(double metres);
    virtual double GetTerrainZCorrectionMetres(void) { return m_terrainZCorrectionMetres; }

    virtual SPxErrorCode SetTerrainTileSizeMetres(double metres);
    virtual double GetTerrainTileSizeMetres(void) { return m_terrainTileSizeMetres; }

    virtual void ShowTerrain(int state);
    virtual int IsTerrainVisible(void) { return m_showTerrain; }

    virtual void SetTerrainLandRGB(UINT32 rgb);
    virtual UINT32 GetTerrainLandRGB(void) { return m_terrainLandRGB; }

    virtual void SetTerrainSeaRGB(UINT32 rgb);
    virtual UINT32 GetTerrainSeaRGB(void) { return m_terrainSeaRGB; }

    /* Terrain map. */
    virtual void ShowTerrainMap(int state);
    virtual int IsTerrainMapVisible(void) { return m_showTerrainMap; }
    virtual void CheckUpdateTerrainMapBitmap(void);
    virtual unsigned int GetTerrainMapBitmapWidthPixels(void);
    virtual unsigned int GetTerrainMapBitmapHeightPixels(void);
    virtual const UINT8 *GetTerrainMapBitmapDataAndLock(void);
    virtual void UnlockTerrainMapBitmap(void);
    virtual double GetTerrainMapXScale(void);
    virtual double GetTerrainMapYScale(void);

    /* Terrain radar. */
    virtual void ShowTerrainRadar(int state);
    virtual int IsTerrainRadarVisible(void) { return m_showTerrainRadar; }

    /* Lighting. */
    virtual SPxErrorCode SetAmbientBrightness(double level);
    virtual double GetAmbientBrightness(void) { return m_ambLightBrightness; }

    virtual SPxErrorCode SetDiffuseBrightness(double level);
    virtual double GetDiffuseBrightness(void) { return m_difLightBrightness; }

    virtual SPxErrorCode SetDiffuseDirection(double pitchDegs,
                                             double yawDegs);
    virtual double GetDiffusePitchDegs(void) { return m_difLightPitchDegs; }
    virtual double GetDiffuseYawDegs(void) { return m_difLightYawDegs; }

    /* Fog density control. */
    virtual SPxErrorCode SetFogDensity(double density);
    virtual double GetFogDensity(void) { return m_fogDensity; }
    virtual SPxErrorCode SetFogRGB(UINT32 rgb);
    virtual UINT32 GetFogRGB(void) { return m_fogRGB; }

    /* Detail level control. */
    virtual SPxErrorCode SetDetailLevel(DetailLevel_t level);
    virtual DetailLevel_t GetDetailLevel(void) { return m_detailLevel; }
    virtual SPxErrorCode SetNearClipMetres(double metres);
    virtual double GetNearClipMetres(void) { return m_nearClipMetres; }

    /* Tracks. */
    virtual SPxErrorCode SetTrackLatLongEnabled(int isEnabled);
    virtual int IsTrackLatLongEnabled(void) { return m_isTrackLatLongEnabled; }

    /* Configuration save and restore. */
    virtual SPxErrorCode SetStateFromConfig(void);
    virtual SPxErrorCode SetConfigFromState(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */
    
    SPxVidSimPlatform *m_platform;     /* Platform. */
    SPxVidSimModelDatabase *m_modelDB; /* Model database. */
    SPxRemoteServer *m_remoteServer;   /* Used to receive track reports. */
    SPxTrackDatabase *m_trackDB;       /* Track database. */
    DetailLevel_t m_detailLevel;       /* Level of detail to display. */
    double m_nearClipMetres;           /* Near clippling distance. */

    /* Scene boundaries. */
    double m_sceneWidthMetres;         /* Scene width (east/west). */
    double m_sceneLengthMetres;        /* Scene length (north/south). */
    double m_sceneHeightMetres;        /* Scene height (up/down). */
    int m_showFloor;                   /* Show floot. */
    int m_showCeiling;                 /* Show ceiling. */
    int m_showNorthWall;               /* Show north wall. */
    int m_showSouthWall;               /* Show south wall. */
    int m_showEastWall;                /* Show east wall. */
    int m_showWestWall;                /* Show west wall. */
    UINT32 m_wallRGB;                  /* Wall colour. */
    UINT32 m_ceilingRGB;               /* Ceiling colour. */
    UINT32 m_floorRGB;                 /* Floor colour. */

    /* Terrain. */
    int m_showTerrain;                 /* Show terrain. */
    double m_terrainZCorrectionMetres; /* Terrain altitude correction. */
    double m_terrainLandThreshMetres;  /* Terrain land/sea threshold. */
    UINT32 m_terrainLandRGB;           /* Land colour. */
    UINT32 m_terrainSeaRGB;            /* Sea colour. */
    double m_terrainTileSizeMetres;    /* Tile size, zero for auto. */
    SPxTerrainDatabase *m_terrainDB;   /* Terrain database. */
    int m_forceTiledMapRedraw;     /* Force a redraw of the terrain map. */

    /* Terrain tiled map. */
    int m_showTerrainMap;                         /* Show terrain map. */
    SPxTiledMapDatabase *m_tiledMapDB;            /* Tiled map database. */
    SPxCriticalSection m_terrainMapBitmapMutex;   /* Terrain image bitmap mutex. */
    UINT32 m_lastTerrainMapBitmapUpdateTime;      /* Last bitmap update time. */
#ifdef _WIN32
    SPxBitmapWin *m_terrainMapBitmap;             /* Terrain image bitmap. */
    SPxTiledMapRendererWin *m_tiledMapRenderer;   /* Tiled map renderer. */
#else
    /* Add Linux objects here. */
#endif

    /* Terrain radar video. */
    int m_showTerrainRadar;            /* Show radar video. */

    /* Tracks. */
    int m_isTrackLatLongEnabled;       /* Use lat/long in track reports? */

    /* Lighting. */
    double m_ambLightBrightness;       /* Ambient light brightness. */
    double m_difLightBrightness;       /* Diffuse light brightness. */
    double m_difLightPitchDegs;        /* Diffuse light pitch (0 down). */
    double m_difLightYawDegs;          /* Diffuse light yaw (0 north). */

    /* Weather effects. */
    double m_fogDensity;               /* Amount of fog to display. */
    UINT32 m_fogRGB;                   /* Fog colour. */

    /* Background. */
    UINT32 m_backgroundRGB;            /* Background colour. */

    /*
     * Private functions.
     */

}; /* SPxVidSimScene */

#endif /* _SPX_VID_SIM_SCENE_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
