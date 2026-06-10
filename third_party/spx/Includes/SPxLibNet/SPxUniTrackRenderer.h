/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxUniTrackRenderer.h,v $
 * ID: $Id: SPxUniTrackRenderer.h,v 1.4 2017/08/22 14:23:14 rew Exp $
 *
 * Purpose:
 *   Header for SPxUniTrackRenderer class which is used to render
 *   radar, fused, AIS and ADS-B tracks from a unified track database.
 *
 * Revision Control:
 *   22/08/17 v1.4   SP 	Add support for fused tracks.
 *                              Improvements to track selection.
 *
 * Previous Changes:
 *   03/08/17 1.3   SP 	Add GetNearestTrack().
 *   31/07/17 1.2   SP 	Set state when selecting tracks.
 *   13/07/17 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_UNI_TRACK_RENDERER_H
#define _SPX_UNI_TRACK_RENDERER_H

/*
 * Other headers required.
 */

/* Include common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxStrings.h"
#include "SPxLibUtils/SPxTime.h"
#include "SPxLibNet/SPxUniTrack.h"

/*********************************************************************
 *
 *   Constants
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

/* Forward declare other classes required. */
class SPxUniTrackDatabase;
class SPxTrackRenderer;
class SPxAISRenderer;
class SPxADSBRenderer;

/*
 * Define our class.
 */
class SPxUniTrackRenderer : virtual public SPxObj, public SPxMouseInterface
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxUniTrackRenderer(SPxUniTrackDatabase *database);
    virtual ~SPxUniTrackRenderer(void);

    /* Add track renderers. */
    virtual SPxErrorCode AddRadarTrackRenderer(SPxTrackRenderer *renderer);
    virtual SPxErrorCode AddFusedTrackRenderer(SPxTrackRenderer *renderer);
    virtual SPxErrorCode AddAISTrackRenderer(SPxAISRenderer *renderer);
    virtual SPxErrorCode AddADSBTrackRenderer(SPxADSBRenderer *renderer);

    /* Utility functions. */
    virtual SPxUniTrack *GetNearestTrack(int x, int y);
    virtual SPxUniTrack *GetBestTrack(int x, int y);

    /* Rendering. */
    virtual SPxErrorCode Render(void);

    /* Functions used to implement SPxMouseInterface. */
    virtual int HandleMousePress(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseRelease(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleDoubleClick(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseLeave(void);

protected:
    /*
     * Protected functions.
     */

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName,
                             char *parameterValue);

    virtual int GetParameter(char *parameterName,
                             char *valueBuf, 
                             int bufLen);

private:
    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    mutable SPxCriticalSection m_mutex;     /* Mutex used to protect the various lists. */
    SPxUniTrackDatabase *m_database;        /* Handle of associated database object. */
    SPxRenderer *m_rdr;                     /* Primary track render. */

    /*
     * Private functions.
     */

    void setCursor(int x, int y, SPxCursor_t *cursor);
    int selectTrack(UINT32 flags, 
                    int x, int y,
                    int isDoubleClick);
    void requestRedraw(void);
    SPxUniTrack::TrackType_t getVisibleTrackTypes(void);

}; /* SPxUniTrackRenderer */


/*********************************************************************
 *
 *   Function prototypes
 *
 *********************************************************************/

#endif /* SPX_UNI_TRACK_RENDERER_H */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
