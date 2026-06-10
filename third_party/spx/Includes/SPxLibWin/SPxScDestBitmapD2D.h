/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxScDestBitmapD2D.h,v $
* ID: $Id: SPxScDestBitmapD2D.h,v 1.4 2016/05/11 13:49:24 rew Exp $
*
* Purpose:
*	Header for Direct2D bitmap destination class.
*
* Revision Control:
*   11/05/16 v1.4    AGC	Add moving platform reference mode support.
*
* Previous Changes:
*   05/02/15 1.3    AGC	Remove unused SPxRendererD2D header.
*   11/12/14 1.2    AGC	Add ClearBitmap() overload.
*   27/09/13 1.1    AGC	Initial version.
*
**********************************************************************/

#ifndef _SPX_SC_DEST_BITMAP_D2D_H
#define _SPX_SC_DEST_BITMAP_D2D_H

/*
 * Other headers required.
 */

/* Required for SPX_SUPPORT_D2D define. */
#include "SPxLibWin/SPxWinD2DUtils.h"

/* Base class header. */
#include "SPxLibSc/SPxScDestBitmap.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward declarations. */
struct ID2D1RenderTarget;
struct ID2D1Bitmap;
struct D2D_RECT_F;

/* SPxScDestBitmapD2D class definition. */
class SPxScDestBitmapD2D : public SPxScDestBitmap
{
public:
    /* Constructor, destructor etc. */
    SPxScDestBitmapD2D(void);
    virtual ~SPxScDestBitmapD2D();
    int Create(UINT16 maxScreenW, 
               UINT16 maxScreenH,
               ID2D1RenderTarget *target);

    /* Get the D2D bitmap. */
    ID2D1Bitmap *GetBitmap(void);

    /* Render to D2D bitmap. */
    SPxErrorCode UpdateBitmap(void);

    /* Render D2D bitmap to target. */
    SPxErrorCode DrawBitmap(ID2D1RenderTarget *target, const D2D_RECT_F& rect);

    virtual void ClearBitmap(UINT32 val=0);
    
private:
#ifdef SPX_SUPPORT_D2D
    ID2D1Bitmap *m_bitmap;
#endif

    void RenderBitmap(void);

}; /* SPxScDestBitmapD2D class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_SC_DEST_BITMAP_D2D_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
