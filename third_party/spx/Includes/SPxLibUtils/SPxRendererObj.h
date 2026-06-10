/*********************************************************************
*
* (c) Copyright 2013, 2015, 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxRendererObj.h,v $
* ID: $Id: SPxRendererObj.h,v 1.4 2017/08/29 14:53:53 rew Exp $
*
* Purpose:
*   Abstract base class for renderer objects that can be used with an
*   external renderer, SPxExtRenderer.
*
*
* Revision Control:
*   29/08/17 v1.4    AGC	Add Deinit() function.
*
* Previous Changes:
*   09/02/15 1.3    AGC	Add Init() function.
*   18/11/13 1.2    AGC	Fix clang warning.
*   04/11/13 1.1    AGC	Move to new header from SPxExtRenderer.
**********************************************************************/

#ifndef _SPX_RENDERER_OBJ_H
#define _SPX_RENDERER_OBJ_H

/*
 * Other headers required.
 */

/* For SPxErrorCode. */
#include "SPxLibUtils/SPxError.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward declarations. */
class SPxExtRenderer;

/* Interface for external renderer objects. */
class SPxRendererObj
{
public:
    SPxRendererObj(void) {}
    virtual ~SPxRendererObj(void) {}
    virtual SPxErrorCode Init(SPxExtRenderer * /*renderer*/) { return SPX_NO_ERROR; }
    virtual SPxErrorCode Deinit(SPxExtRenderer * /*renderer*/) { return SPX_NO_ERROR; }
    virtual SPxErrorCode Render(SPxExtRenderer *renderer)=0;
};

#endif /* _SPX_RENDERER_OBJ_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
