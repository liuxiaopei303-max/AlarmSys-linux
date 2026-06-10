/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimModel.h,v $
 * ID: $Id: SPxVidSimModel.h,v 1.12 2016/12/01 14:47:40 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimModel class header.
 *
 * Revision Control:
 *   01/12/16 v1.12  SP 	Move ZRefMode_t to SPxVidSimObj.h.
 *
 * Previous Changes:
 *   30/11/16 1.11  SP 	Add Get/SetModelName().
 *   31/10/16 1.10  SP 	Support Z reference mode.
 *   24/06/16 1.9   AGC	Fix clang on Windows warnings.
 *   07/06/16 1.8   AGC Avoid clang on Windows warnings from assimp headers.
 *   10/09/15 1.7   SP  Add extra args to RenderGL().
 *   27/08/15 1.6   SP  Add debug support.
 *   26/08/15 1.5   SP  Add config options and parameters.
 *   20/06/14 1.4   AGC Move C++ headers to source file.
 *   04/10/13 1.3   AGC Simplify headers.
 *   19/07/13 1.2   SP  Further development.
 *   15/07/13 1.1   SP  Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_MODEL_H
#define _SPX_VID_SIM_MODEL_H

/* ASSIMP headers. */
#ifdef __clang__
#pragma clang diagnostic push
#ifdef _WIN32
#pragma clang diagnostic ignored "-Wmicrosoft-enum-value"
#endif
#endif
#include "Externals/assimp/include/assimp/Importer.hpp"
#include "Externals/assimp/include/assimp/PostProcess.h"
#include "Externals/assimp/include/assimp/scene.h"
#ifdef __clang__
#pragma clang diagnostic pop
#endif

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxGL.h"
#include "SPxLibUtils/SPxVidSimMovable.h"
#include "SPxLibUtils/SPxVidSimModelRendererGL.h"

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
class SPxVidSimScene;
class SPxVidSimModelDatabase;
class SPxVidSimDispGL;
class SPxTerrainDatabase;

class SPxVidSimModel : public SPxVidSimMovable
{
    /* Grant SPxVidSimModelDatabase private access. */
    friend class SPxVidSimModelDatabase;

public:

    /*
     * Public types.
     */

    /* Model types. */
    typedef enum
    {
        TYPE_STATIC,
        TYPE_TARGET

    } Type_t;

    /*
     * Public variables.
     */

    /* Model flags. */
    static const UINT32 MODEL_MAX_QUALITY = (1U << 0);

    /* Debug flags. */
    static const UINT32 DEBUG_RENDER = (1U << 0);

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimModel(SPxVidSimModelDatabase *database);
    virtual ~SPxVidSimModel(void);

    /* Configuration. */
    virtual SPxErrorCode LoadFile(const char *filename);
    virtual const char *GetFilename(void);
    virtual const aiScene *GetAndLockAIModel(void);
    virtual void UnlockAIModel(void);
    virtual SPxErrorCode InitialiseGL(SPxVidSimDispGL *disp);
    virtual SPxErrorCode RenderGL(SPxVidSimDispGL *disp,
                                  double xMetres=0.0, 
                                  double yMetres=0.0, 
                                  double zMetres=0.0,
                                  double pitchDegs=0.0, 
                                  double rollDegs=0.0, 
                                  double yawDegs=0.0);
    virtual SPxErrorCode SetType(Type_t type);
    virtual Type_t GetType(void) { return m_type; }
    virtual SPxErrorCode SetModelName(const char *name);
    virtual SPxErrorCode GetModelName(char *buf, unsigned int bufLen);
    virtual SPxErrorCode SetZRefMode(ZRefMode_t mode);
    virtual ZRefMode_t GetZRefMode(void) { return m_zRefMode; }

    /* Scaling. */
    virtual SPxErrorCode SetModelUnitsPerMetre(double muPerMetre);
    virtual double GetModelUnitsPerMetre(void) { return m_modelUnitsPerMetre; }

    /* Positional correction in model units. */
    virtual SPxErrorCode SetXCorrectionModelUnits(double modelUnits);
    virtual double GetXCorrectionModelUnits(void);
    virtual SPxErrorCode SetYCorrectionModelUnits(double modelUnits);
    virtual double GetYCorrectionModelUnits(void);
    virtual SPxErrorCode SetZCorrectionModelUnits(double modelUnits);
    virtual double GetZCorrectionModelUnits(void);

    /* Selection criteria. */
    virtual SPxErrorCode SetMinAltitudeMetres(double altMetres);
    virtual double GetMinAltitudeMetres(void) { return m_minAltMetres; }
    virtual SPxErrorCode SetMaxAltitudeMetres(double altMetres);
    virtual double GetMaxAltitudeMetres(void) { return m_maxAltMetres; }

    /* Parent object. */
    virtual SPxVidSimModelDatabase *GetModelDatabase(void)
    {
        return m_modelDB;
    }

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Set link to parent object. */
    virtual SPxErrorCode SetModelDatabaseLink(SPxVidSimModelDatabase *database);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private types.
     */

    /* Display information. */
    typedef struct
    {
        SPxVidSimDispGL *disp;
        SPxVidSimModelRendererGL *renderer;

    } DispInfo_t;

    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* General. */
    SPxVidSimModelDatabase *m_modelDB;       /* Parent model database. */
    SPxVidSimScene *m_scene;                 /* Scene. */
    SPxTerrainDatabase *m_terrainDB;         /* Terrain database. */
    Assimp::Importer m_importer;             /* File m_importer. */
    const aiScene *m_aiModel;                /* Model read from file. */
    SPxCriticalSection m_mutex;              /* Model mutex. */
    Type_t m_type;                           /* Model type. */
    double m_modelUnitsPerMetre;             /* Model scale factor. */
    UINT32 m_modelFlags;                     /* Model flags. */
    ZRefMode_t m_zRefMode;                   /* Z reference mode. */

    /* Debugging. */
    UINT32 m_debugFlags;
    UINT32 m_lastRenderReportTime;
    UINT32 m_renderMsecs;
    UINT32 m_renderCount;

    /* Fixed positional correction. */
    double m_xCorModelUnits;
    double m_yCorModelUnits;
    double m_zCorModelUnits;

    /* Selection criteria. */
    double m_minAltMetres;
    double m_maxAltMetres;

    /*
     * Private functions.
     */

}; /* SPxVidSimModel */

#endif /* _SPX_VID_SIM_MODEL_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
