/*********************************************************************
*
* (c) Copyright 2015, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxMesgCtrlDlgWin.h,v $
* ID: $Id: SPxMesgCtrlDlgWin.h,v 1.4 2016/10/07 11:53:18 rew Exp $
*
* Purpose:
*   A pop-up dialog used to show messages.
*
* Revision Control:
*   07/10/16 v1.4    AGC	Use rich edit control.
*
* Previous Changes:
*   03/03/16 1.3    AGC	Fix clang warnings.
*   01/10/15 1.2    AGC	Update messages when window shown.
*   21/09/15 1.1    AGC	Initial Version.
**********************************************************************/

#pragma once

#define SPX_AFXWIN
#include "SPxResource.h"
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxCriticalSection.h"

class SPxMesgCtrlDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxMesgCtrlDlgWin)

public:

    /*
     * Public variables.
     */

    /* Dialog Data */
    enum { IDD = IDD_SPX_MESG_DIALOG };

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxMesgCtrlDlgWin(void);
    virtual ~SPxMesgCtrlDlgWin();
    virtual BOOL Create(CWnd* pParent);
    void ReportMesg(const char *text);

    void Sync(void);

    /* Access to internal controls. */
    CWnd *GetClearButton(void);
    CWnd *GetFreezeCheck(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    DECLARE_MESSAGE_MAP()
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnTimer(UINT_PTR nIDEvent);
    virtual void OnSize(UINT nType, int cWidth, int cHeight);
    virtual void OnGetMinMaxInfo(MINMAXINFO* lpMMI);
    void OnShowWindow(BOOL bShow, UINT nStatus);

private:

    /*
     * Private variables.
     */
    /* Critical section. */
    SPxCriticalSection m_mutex;
    
    /* GUI objects. */
    CRichEditCtrl *m_mesgWin;

    /* Messages. */
    CStringA m_mesgBuf;        /* Message buffer. */
    BOOL m_isMesgPending;      /* Mssage pending flag. */

    /* Minimum dialog size. */
    unsigned int m_minWidth;
    unsigned int m_minHeight;

    /* GUI data exchange. */
    int m_freezeWin;

    /*
     * Private functions.
     */

    /* GUI callback functions. */
    void OnBnClickedMesgClearButton();
    void OnBnClickedMesgFreezeCheck();

    using CDialog::Create;

};

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
