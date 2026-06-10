/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxWinCustomDlg.h,v $
* ID: $Id: SPxWinCustomDlg.h,v 1.1 2016/11/16 14:59:31 rew Exp $
*
* Purpose:
*	Dialog to allow customisation of window title bar.
*
* Revision Control:
*   16/11/16 v1.1    AGC	Initial Version.
*
* Previous Changes:
**********************************************************************/

#pragma once

#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

#include "SPxLibUtils/SPxAutoPtr.h"

class SPxWinCustomDlg : public CDialog
{
    DECLARE_DYNAMIC(SPxWinCustomDlg)

public:
    explicit SPxWinCustomDlg(UINT nIDTemplate, CWnd* pParentWnd=NULL);
    virtual ~SPxWinCustomDlg(void);

    void EnableCustomTitleBar(int enable);
    void SetTitleBackColour(COLORREF colour);
    COLORREF GetTitleBackColour(void) const;
    void SetTitleTextColour(COLORREF colour);
    COLORREF GetTitleTextColour(void) const;
    void SetBackColour(COLORREF colour);
    COLORREF GetBackColour(void) const;

protected:
    /* Message handlers. */
    DECLARE_MESSAGE_MAP()
    virtual void OnCancel(void);
    void OnShowWindow(BOOL bShow, UINT nStatus);
    void OnPaint(void);
    HBRUSH OnCtlColor(CDC *pDC, CWnd *pWnd, UINT nCtlColor);
    void OnSize(UINT nType, int cx, int cy);
    void OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized);
    void OnNcCalcSize(BOOL validate, NCCALCSIZE_PARAMS *lpncsp);
    LRESULT OnNcHitTest(CPoint point);
    void OnNcMouseMove(UINT nFlags, CPoint point);
    void OnNcLButtonDown(UINT nFlags, CPoint point);
    void OnNcLButtonUp(UINT nFlags, CPoint point);
    void OnNcRButtonUp(UINT nFlags, CPoint point);
    void OnNcMouseLeave(void);

    void GetClientRect(LPRECT rect);

private:
    enum Button
    {
	BTN_CLOSE = 0,
	BTN_MAX = 1,
	BTN_MIN = 2
    };

    int m_enabled;
    int m_isMouseOver[3];
    int m_isMouseDownOver[3];
    COLORREF m_titleBackColour;
    COLORREF m_titleTextColour;
    COLORREF m_backColour;
    SPxAutoPtr<CBrush> m_backBrush;

    int getTitleBarHeight(void) const;
    void showSystemMenu(CPoint point);
    void handleMouseMove(CPoint point);
    int getIconSize(void) const;
    CRect getIconRect(int iconSize) const;
    CRect getButtonRect(Button btn) const;
    int isButtonEnabled(Button btn) const;
    int isEnabled(void) const;
    int isWindows8OrNewer(void) const;
};

/*********************************************************************
*
*	End of file
*
**********************************************************************/
