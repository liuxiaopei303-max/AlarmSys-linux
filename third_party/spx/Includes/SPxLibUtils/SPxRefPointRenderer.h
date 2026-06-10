/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxRefPointRenderer.h,v $
 * ID: $Id: SPxRefPointRenderer.h,v 1.1 2017/08/22 14:24:01 rew Exp $
 *
 * Purpose:
 *   Header for SPxRefPointRenderer class which is used to render
 *   reference points.
 *
 * Revision Control:
 *   22/08/17 v1.1   SP 	Initial version.
 *
 * Previous Changes:
 *
 *********************************************************************/

#ifndef _SPX_REF_POINT_RENDERER_H
#define _SPX_REF_POINT_RENDERER_H

/*
 * Other headers required.
 */

/* Include common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxRenderer.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxStrings.h"
#include "SPxLibUtils/SPxTime.h"
#include "SPxLibUtils/SPxRefPoint.h"

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

/*
 * Define our class.
 */
class SPxRefPointRenderer : virtual public SPxRenderer
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxRefPointRenderer(SPxUniTrackDatabase *database);
    virtual ~SPxRefPointRenderer(void);

    /* Utility functions. */
    virtual SPxRefPoint *GetNearestRefPoint(int x, int y);

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
    SPxUniTrackDatabase *m_database;        /* Handle of associated database object. */



    /*
     * Private functions.
     */

    SPxErrorCode renderAll(void);
    void renderRefPoint(SPxRefPoint *refPoint);

}; /* SPxRefPointRenderer */


/*********************************************************************
 *
 *   Function prototypes
 *
 *********************************************************************/

#endif /* SPX_REF_POINT_RENDERER_H */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
