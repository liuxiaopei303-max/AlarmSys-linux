/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxHtmlCtrlDlgWin.h,v $
* ID: $Id: SPxHtmlCtrlDlgWin.h,v 1.2 2016/11/16 14:59:31 rew Exp $
*
* Purpose:
*	Header for the HTML dialog for displaying a web page.
*
*
* Revision Control:
*   16/11/16 v1.2    AGC	Derive from SPxWinCustomDlg.
*
* Previous Changes:
*   02/09/16 1.1    AGC	Initial version.
*
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"
#include "afxdhtml.h"

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For base class. */
#include "SPxLibWin/SPxWinCustomDlg.h"

/*
 * SPxHtmlCtrlDlgWin dialog
 */
class SPxHtmlCtrlDlgWin : public SPxWinCustomDlg
{
    DECLARE_DYNAMIC(SPxHtmlCtrlDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxHtmlCtrlDlgWin(CWnd *pParent);
    virtual ~SPxHtmlCtrlDlgWin(void);
    BOOL Create(void);
    void Navigate(const TCHAR *address);

    /* Dialog Data */
    enum { IDD = IDD_SPX_HTML_CTRL_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    void OnSize(UINT nType, int cx, int cy);
    BOOL OnEraseBkgnd(CDC *pDC);
    virtual void OnOK(void) {}
    DECLARE_MESSAGE_MAP()

private:
    CWnd* m_parent;
    CDHtmlDialog m_htmlDlg;

    using CDialog::Create;

}; /* class SPxHtmlCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
