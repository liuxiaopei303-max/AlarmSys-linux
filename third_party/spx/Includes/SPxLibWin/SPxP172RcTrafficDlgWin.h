/*********************************************************************
*
* (c) Copyright 2012 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxP172RcTrafficDlgWin.h,v $
* ID: $Id: SPxP172RcTrafficDlgWin.h,v 1.5 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*	A pop-up dialog used to show traffic, i.e. messages
*       exchanged between the radar and the display.
*
* Revision Control:
*   03/03/16 v1.5    AGC	Fix clang warnings.
*
* Previous Changes:
*   26/01/15 1.4    AGC	Remove unused HICON and parent window.
*   01/09/14 1.3    AGC	VS2015 format string changes.
*   17/02/14 1.2    SP 	Changes to support SxV protocol.
*   18/10/12 1.1    SP 	Initial version.
*
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

/* Include the classes we need. */
#include "SPxLibWin/SPxP172RcInterfaceWin.h"

/*
 * SPxP172RcTrafficDlgWin dialog.
 */
class SPxP172RcTrafficDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxP172RcTrafficDlgWin)

public:
    /* Constructor and destructor. */
    SPxP172RcTrafficDlgWin(SPxP172RcInterfaceWin *rdrIf);
    virtual ~SPxP172RcTrafficDlgWin();
    virtual BOOL Create(CWnd* pParent);

    /* Dialog Data */
    enum { IDD = IDD_SPX_P172_RC_TRAFFIC_DIALOG };

protected:
    DECLARE_MESSAGE_MAP()
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual afx_msg void OnTimer(UINT_PTR nIDEvent);
    virtual afx_msg void OnSize(UINT nType, int cWidth, int cHeight);
    virtual afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

private:
    /* Private functions. */
    void ReportMesg(SPxP172RcInterfaceWin::MesgInfo_t *mesg, int isTx);

    /* GUI callback functions */
    afx_msg void OnBnClickedTrafficClearButton();
    afx_msg void OnBnClickedHeartbeatsLogTxCheck();
    afx_msg void OnBnClickedHeartbeatsLogRxCheck();
    afx_msg void OnBnClickedStatusLogTxCheck();
    afx_msg void OnBnClickedStatusLogRxCheck();
    afx_msg void OnBnClickedCmdLogTxCheck();
    afx_msg void OnBnClickedCmdLogRxCheck();
    afx_msg void OnBnClickedTrafficFreezeCheck();

    /* Private static functions. */
    static int rxMesgTextHandler(void *invArg, void *userArg, void *textArg);
    static int txMesgTextHandler(void *invArg, void *userArg, void *textArg);

    /* GUI objects. */
    CWnd *m_trafficWin;

    /* Messages. */
    CStringA m_trafficBuf;        /* Message text buffer. */
    BOOL m_isTrafficPending;      /* Mssage pending flag. */

    /* The radar interface. */
    SPxP172RcInterfaceWin *m_interface;

    /* Minimum dialog size. */
    unsigned int m_minWidth;
    unsigned int m_minHeight;

    /* GUI data exchange. */
    int m_logTxCommand;
    int m_logRxCommand;
    int m_logTxStatus;
    int m_logRxStatus;
    int m_logTxHeartbeats;
    int m_logRxHeartbeats;
    int m_freezeWin;

    using CDialog::Create;

}; /* class SPxP172RcTrafficDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
