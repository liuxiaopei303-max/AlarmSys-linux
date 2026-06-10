/*********************************************************************
 * (c) Copyright 2014, 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxWinSplashDlg.h,v $
 * ID: $Id: SPxWinSplashDlg.h,v 1.2 2016/03/03 10:59:08 rew Exp $
 *
 * Purpose:
 *   Header file for re-usable splash dialog class.
 *
 * Revision Control:
 *  03/03/16 v1.2    AGC	Fix clang warnings.
 *
 * Previous Changes:
 *  20/06/14 1.1    AGC	Initial version based on
 *			SPxRadarSimulator splash screen.
 **********************************************************************/

#pragma once

#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

#include "SPxResource.h"

#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibWin/SPxWinImageCtrl.h"

class SPxWinSplashDlg : public CDialog
{
    DECLARE_DYNAMIC(SPxWinSplashDlg)

public:
    // Methods
    explicit SPxWinSplashDlg(CWnd* pParent=NULL);
    virtual ~SPxWinSplashDlg(void);
    BOOL Create(void);
    void SetBitmap(int id);
    void SetBorder(int border);
    void SetBackgroundColour(COLORREF col);
    void SetTextColour(COLORREF col);
    void SetProgressTextColour(COLORREF col);
    void SetBackgroundTextColour(COLORREF col);
    void SetName(const char *name, ...);
    void SetLicense(const char *license, ...);
    void SetText(const char *txt, ...);
    void SetProgress(int val);
    void IncProgress(int delta);

    // Variables
    enum { IDD = IDD_SPX_SPLASH_DIALOG };

protected:
    // Methods
    virtual BOOL OnInitDialog();
    HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    void DoDataExchange(CDataExchange *pDX);
    DECLARE_MESSAGE_MAP()

private:
    CWnd *m_parentWin;
    SPxAutoPtr<CBrush> m_brush;
    COLORREF m_textCol;
    COLORREF m_progressTextCol;
    SPxWinImageCtrl m_imgCtrl;
    CStatic m_lblName;
    CStatic m_lblLicense;
    CStatic m_lblProgress;
    CProgressCtrl m_progress;

    using CDialog::Create;

}; // SPxRadarSimSplashDlg

/*********************************************************************
 * End of File
 **********************************************************************/
