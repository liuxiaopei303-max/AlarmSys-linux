/*********************************************************************
*
* (c) Copyright 2015, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTextEntryDlgWin.h,v $
* ID: $Id: SPxTextEntryDlgWin.h,v 1.3 2016/10/10 12:58:54 rew Exp $
*
* Purpose:
*	Header for Text Entry control dialog.
*
*
* Revision Control:
*   10/10/16 v1.3    AGC	Fix Unicode use of GetContent().
*
* Previous Changes:
*   09/09/16 1.2    AGC	Support masked password entry.
*   30/11/15 1.1    REW	Initial Version.
**********************************************************************/

#pragma once

/* Include the SPx resource header. */
#include "SPxResource.h"

/*
 *  SPxTextEntryDlgWin dialog
 */
class SPxTextEntryDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxTextEntryDlgWin)

public:
    /* Construction and destruction. */
    explicit SPxTextEntryDlgWin(CWnd *pParent = NULL,
				int isPassword = FALSE);
    virtual ~SPxTextEntryDlgWin();

    /* Text content. */
    void SetContent(const char *content);
    const char *GetContent(void);

    /* Dialog Data. */
    enum { IDD = IDD_SPX_TEXT_ENTRY_DIALOG };

protected:
    /* DDX/DDV support */
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog(void);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    int m_isPassword;
    CEdit m_contentEditor;
    CStringA m_contentString;

    /* Private functions */
    void OnContentChange(void);
}; /* SPxTextEntryDlgWin class. */

/*********************************************************************
*
* End of file
*
**********************************************************************/
