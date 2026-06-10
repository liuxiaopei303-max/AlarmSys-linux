/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxWinImageCtrl.h,v $
* ID: $Id: SPxWinImageCtrl.h,v 1.6 2016/07/28 13:12:32 rew Exp $
*
* Purpose:
*	Header for a control to display a scaled image loaded from
*	a file.
*
*	The Clip Children option should be enable for the parent
*	control to prevent flicker on resizing.
*
* Revision Control:
*   28/07/16 v1.6    AGC	Support clickable images.
*
* Previous Changes:
*   03/03/16 1.5    AGC	Fix clang warnings.
*   09/06/14 1.4    AGC	Support not maintaining aspect ratio.
*   24/04/14 1.3    AGC	Use SPxObjPtr for improved deletion handling.
*   14/04/14 1.2    AGC	Add bitmap support.
*   25/07/13 1.1    AGC	Initial Version.
**********************************************************************/
#pragma once

/* Includes. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxObjPtr.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibWin/SPxBitmapWin.h"

/* Forward declarations. */
namespace ATL
{
    class CImage;
}

/*
 * SPxWinImageCtrl control
 */
class SPxWinImageCtrl : public CWnd
{
    DECLARE_DYNAMIC(SPxWinImageCtrl)

public:
    /* Construction and desctruction. */
    SPxWinImageCtrl(void);
    virtual ~SPxWinImageCtrl(void);

    virtual BOOL Create(CWnd *pParentWnd);
    SPxErrorCode SetFilename(const char *filename);
    SPxErrorCode SetFilename(const CString &filename);
    SPxErrorCode GetFilename(char *buffer, int bufLen) const;
    SPxErrorCode GetFilename(CString &filename) const;
    SPxErrorCode SetBitmap(HBITMAP bitmap);
    SPxErrorCode SetBitmap(int resourceID);
    SPxErrorCode SetBackColour(COLORREF colour);
    UINT32 GetBackColour(void) const { return m_backColour; }
    SPxErrorCode SetBorder(int border);
    int GetBorder(void) const { return m_border; }
    SPxErrorCode SetMaintainAspectRatio(int maintainAspectRatio);
    int IsMaintainingAspectRatio(void) const { return m_maintainAspectRatio; }
    SPxErrorCode SetClickable(int clickable);
    int IsClickable(void) const { return m_clickable; }

protected:
    /* Protected functions. */
    virtual void PreSubclassWindow(void);
    void OnSize(UINT nType, int cx, int cy);
    void OnLButtonDown(UINT nFlags, CPoint point);
    void OnLButtonUp(UINT nFlags, CPoint point);
    void OnMouseMove(UINT nFlags, CPoint point);
    void OnMouseLeave(void);
    void OnPaint(void);
    BOOL OnEraseBkgnd(CDC *pDC);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    CString m_filename;
    SPxObjPtr<SPxBitmapWin> m_bitmap;
    SPxAutoPtr<CImage> m_image;
    COLORREF m_backColour;
    int m_border;
    int m_maintainAspectRatio;
    int m_clickable;
    int m_isMouseOver;
    int m_isTrackingMouse;
    int m_isLButtonDown;

    /* Private functions. */
    void recreateBitmap(int width, int height);

    using CWnd::Create;

}; /* SPxWinImageCtrl */

/*********************************************************************
*
* End of file
*
**********************************************************************/
