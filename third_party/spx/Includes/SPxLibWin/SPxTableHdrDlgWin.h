/*********************************************************************
*
* (c) Copyright 2015, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTableHdrDlgWin.h,v $
* ID: $Id: SPxTableHdrDlgWin.h,v 1.3 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*	Header for a re-usable dialog that acts as a header in the
*	SPxTableDlgWin.
*
*
* Revision Control:
*   03/03/16 v1.3    AGC	Fix clang warnings.
*
* Previous Changes:
*   26/11/15 1.2    AGC	Support control of another dialog.
*   16/11/15 1.1    AGC	Initial version.
**********************************************************************/
#pragma once

#include "SPxResource.h"

#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

#include "SPxLibWin/SPxWinButton.h"

class SPxTableDlgWin;

class SPxTableHdrDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxTableHdrDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxTableHdrDlgWin(SPxTableDlgWin* pParent, CWnd *collapseDlg=NULL);
    virtual ~SPxTableHdrDlgWin(void);
    BOOL Create(void);

    void SetText(const char *text);

    void ShowDialog(int show);

    /* Dialog Data */
    enum { IDD = IDD_SPX_TABLE_HDR_DIALOG };
    virtual int GetIDD(void) const { return IDD; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void) {}
    virtual void OnCancel(void) {}
    void OnSize(UINT nType, int cx, int cy);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    SPxTableDlgWin *m_parentWin;
    CWnd *m_collapseDlg;

    CFont m_labelFont;
    CStatic m_label;
    CStatic m_line;
    SPxWinButton m_btnCollapse;

    /* Private functions. */
    void OnCollapse(void);

    using CDialog::Create;

}; 

/*********************************************************************
*
* End of file
*
**********************************************************************/
