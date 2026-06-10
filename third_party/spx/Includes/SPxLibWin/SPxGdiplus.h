/*********************************************************************
*
* (c) Copyright 2014, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxGdiplus.h,v $
* ID: $Id: SPxGdiplus.h,v 1.2 2016/09/09 14:23:16 rew Exp $
*
* Purpose:
*   Header for SPxGdiplus class used to automatically initialise
*   and shutdown GDI+.
*
*
* Revision Control:
*   09/09/16 v1.2    AGC	Add SPxGdiplusScaleBitmap().
*
* Previous Changes:
*   20/06/14 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_GDIPLUS_H
#define	_SPX_GDIPLUS_H

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Automatically initialise/shutdown GDI+. */
class SPxGdiplus
{
public:
    SPxGdiplus(void);
    ~SPxGdiplus(void);

private:
    struct impl;
    SPxAutoPtr<impl> m_p;
};

extern HBITMAP SPxGdiplusScaleBitmap(HBITMAP bitmap,
				     int widthPixels,
				     int heightPixels);

#endif /* _SPX_GDIPLUS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
