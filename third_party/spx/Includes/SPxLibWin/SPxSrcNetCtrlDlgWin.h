/*********************************************************************
*
* (c) Copyright 2015, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSrcNetCtrlDlgWin.h,v $
* ID: $Id: SPxSrcNetCtrlDlgWin.h,v 1.4 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*	Header for SPx Radar Simulator network source dialog.
*
*
* Revision Control:
*   03/03/16 v1.4    AGC	Fix clang warnings.
*
* Previous Changes:
*   27/04/15 1.3    AGC	Override OnCancel().
*			Move SetStandardAddress() implementation to source file.
*   24/04/15 1.2    REW	Add SetStandardAddress().
*   07/01/15 1.1    AGC	Initial version.
**********************************************************************/
#pragma once

#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For base class. */
#include "SPxLibWin/SPxTabbedPopupDlgWin.h"

/*
 * Types
 */

/* Forward declarations. */
class SPxNetworkReceive;
class SPxNetCtrlDlgWin;

/* SPxRadarSimSrcNetDlg dialog. */
class SPxSrcNetCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxSrcNetCtrlDlgWin)

public:
    /* Public functions. */
    explicit SPxSrcNetCtrlDlgWin(CWnd* pParent, SPxNetworkReceive *srcNet);
    virtual ~SPxSrcNetCtrlDlgWin(void);
    BOOL Create(void);

    // Dialog Data
    enum { IDD = IDD_SPX_SRC_NET_CTRL_DIALOG };

    /* Virtual functions defined by SPxTabbedItemDlg. */
    virtual void SyncFromObj(void);
    virtual int GetIDD(void) const { return IDD; }
    virtual CWnd *GetGroupBox(void) { return NULL; }
    virtual int ShowCheck(void) { return TRUE; }
    virtual void Enable(int enable);
    virtual int GetEnable(void);

    /* Convenience function. */
    void SetStandardAddress(const char *addr, int port);

protected:
    /* Generated message map functions. */
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK(void) {};
    virtual void OnCancel(void) {};
    virtual BOOL OnInitDialog(void);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    CWnd *m_parent;
    SPxNetworkReceive *m_obj;
    SPxAutoPtr<SPxNetCtrlDlgWin> m_netDlg;

    using SPxTabbedItemDlg::Create;

}; /* SPxSrcNetCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
