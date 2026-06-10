/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimObj.h,v $
 * ID: $Id: SPxVidSimObj.h,v 1.9 2016/12/01 14:47:40 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimObj class header.
 *
 * Revision Control:
 *   01/12/16 v1.9   SP 	Move ZRefMode_t here.
 *
 * Previous Changes:
 *   22/11/16 1.8   SP 	Add SPX_VID_SIM_EVENT_CAMERA_MOUNT_PRESET.
 *   31/10/16 1.7   SP 	Move TriStripPoint_t to SPxVidSimTerrainRenderer.h.
 *                      Add SPX_VID_SIM_EVENT_SCENE_TERRAIN_MAP.
 *   13/10/16 1.6   SP 	Move some utility functions here.
 *   07/10/16 1.5   SP 	Move event types here.
 *   14/10/15 1.4   SP  Add NUM_COLOUR_MODES.
 *   06/10/15 1.3   SP 	Add ColourMode_t.
 *   04/10/13 1.2   AGC	Simplify headers.
 *   02/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_OBJ_H
#define _SPX_VID_SIM_OBJ_H

/* Other headers required. */
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxCallbackList.h"
#include "SPxLibUtils/SPxGL.h"

/*********************************************************************
 *
 *  Constants
 *
 *********************************************************************/

/* Event types. */
#define SPX_VID_SIM_EVENT_POS_LATLONG           1
#define SPX_VID_SIM_EVENT_POS_XYZ               2
#define SPX_VID_SIM_EVENT_POS_PTZ               3
#define SPX_VID_SIM_EVENT_SCENE_SIZE            4
#define SPX_VID_SIM_EVENT_SCENE_APPEARANCE      5
#define SPX_VID_SIM_EVENT_SCENE_TERRAIN_MESH    6
#define SPX_VID_SIM_EVENT_SCENE_TERRAIN_MAP     7
#define SPX_VID_SIM_EVENT_SCENE_TERRAIN_COLOUR  8
#define SPX_VID_SIM_EVENT_CAMERA_IMAGE          9
#define SPX_VID_SIM_EVENT_CAMERA_MOUNT_PRESET   10

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

class SPxVidSimObj : public SPxObj
{
public:

    /*
     * Public types.
     */

    /* Event info. */
    typedef struct
    {
        UINT32 type;
    
    } EventInfo_t;

    /* Colour modes. */
    typedef enum
    {
        COLOUR_MODE_COLOUR = 0,
        COLOUR_MODE_GREYSCALE = 1,
        NUM_COLOUR_MODES /* MUST be last entry. */

    } ColourMode_t;

    /* Z reference modes. */
    typedef enum
    {
        Z_REF_MODE_ZERO,    /* Sea level. */
        Z_REF_MODE_TERRAIN  /* Terrain. */

    } ZRefMode_t;

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimObj(void);
    virtual ~SPxVidSimObj(void);

    /* Event callbacks. */
    virtual SPxErrorCode AddEventCallback(SPxCallbackListFn_t fn, void *userArg);
    virtual SPxErrorCode RemoveEventCallback(SPxCallbackListFn_t fn, void *userArg);

protected:

    /*
     * Protected types.
     */

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Utility functions. */
    void GetSurfaceNormal(const GLfloat v1[], const GLfloat v2[], const GLfloat v3[], GLfloat n[]);
    GLfloat GetRed(UINT32 rgb);
    GLfloat GetGreen(UINT32 rgb);
    GLfloat GetBlue(UINT32 rgb);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

    /* Event reporting. */
    virtual SPxErrorCode ReportEvent(UINT32 eventType);

private:

    /*
     * Private variables.
     */

    SPxCallbackList *m_eventCallbackList;

    /*
     * Private functions.
     */

}; /* SPxVidSimObj */

#endif /* _SPX_VID_SIM_OBJ_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
