/*********************************************************************
*
* (c) Copyright 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamCtrlCreate.h,v $
* ID: $Id: SPxCamCtrlCreate.h,v 1.1 2013/10/24 14:56:23 rew Exp $
*
* Purpose:
*	Header for camera controller creation functions.
*
* Revision Control:
*   24/10/13 v1.1    AGC	Initial Version.
*
* Previous Changes:
**********************************************************************/
#ifndef _SPX_CAM_CTRL_CREATE_H
#define _SPX_CAM_CTRL_CREATE_H

/* For SPxCameraType ernum. */
#include "SPxLibData/SPxCamCtrlBase.h"

/* Forward declarations. */
class SPxCamCtrlBase;

/* Create a new camera controller. */
extern SPxCamCtrlBase *SPxCamCtrlCreate(SPxCameraType camType);

#endif /* _SPX_CAM_CTRL_CREATE_H */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
