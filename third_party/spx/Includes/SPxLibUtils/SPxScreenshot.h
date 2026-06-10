/*********************************************************************
*
* (c) Copyright 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxScreenshot.h,v $
* ID: $Id: SPxScreenshot.h,v 1.1 2015/08/27 14:26:30 rew Exp $
*
* Purpose:
*   Header file for screenshot utilities.
*
*   Using this file in a Linux application will require that the
*   gdk-pixbuf-x11 library is linked.
*
* Revision Control:
*   27/08/15 v1.1    AGC	Initial version.
*
* Previous Changes:
**********************************************************************/
#ifndef SPX_SCREENSHOT_H_
#define SPX_SCREENSHOT_H_

/* Include before testing if X11 supported. */
#include "SPxLibUtils/SPxCommon.h"

#ifdef SPX_X11_SUPPORTED
#include "X11/Xlib.h"
#endif

/* Forward declarations. */
class SPxImage;

/* Take a screenshot. */
#if defined(WIN32) || defined(__BORLAND__)
extern SPxErrorCode SPxTakeScreenshot(SPxImage *image,
				      HWND hwnd=NULL,
				      int useScreenArea=TRUE,
				      int bringToFront=TRUE,
				      int captureDropShadow=TRUE);
#else
#if defined(SPX_X11_SUPPORTED)
extern SPxErrorCode SPxTakeScreenshot(SPxImage *image,
				      Display *display=NULL,
				      int screenNum=-1,
				      Drawable drawable=0);
#endif
#endif

#endif /* SPX_SCREENSHOT_H_ */

/*********************************************************************
*
* End of file
*
**********************************************************************/
