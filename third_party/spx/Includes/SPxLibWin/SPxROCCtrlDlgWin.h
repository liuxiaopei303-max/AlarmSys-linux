/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxROCCtrlDlgWin.h,v $
* ID: $Id: SPxROCCtrlDlgWin.h,v 1.7 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*	Header for the SPxROC control child dialog.
*
*
* Revision Control:
*   03/03/16 v1.7    AGC	Fix clang warnings.
*
* Previous Changes:
*   24/02/16 1.6    SP 	Add OnEnable().
*   23/06/15 1.5    AGC	Replace up-down controls with spin controls.
*   22/04/15 1.4    SP 	Force sync when card opened or closed.
*   02/06/14 1.3    AGC	Use micro symbol for pulse widths label.
*   30/07/13 1.2    AGC	Derive from SPxTabbedItemDlg.
*   25/07/13 1.1    REW	Initial Version.
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxWinSpinCtrl. */
#include "SPxLibWin/SPxWinSpinCtrl.h"

/* Include base class header. */
#include "SPxLibWin/SPxTabbedPopupDlgWin.h"

/* Forward declarations. */
class SPxROC;

/*
 * SPxROCCtrlDlgWin class.
 */
class SPxROCCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxROCCtrlDlgWin)

public:
    /* Constructor and destructor. */
    SPxROCCtrlDlgWin(CWnd* pParent, SPxROC *obj);
    virtual ~SPxROCCtrlDlgWin();
    BOOL Create(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_ROC_CTRL_DIALOG };

    /* Sync functions. */
    virtual void SyncFromObj(void);
    void UpdateStatus(void);
    virtual int GetIDD(void) const { return IDD; }
    virtual int ShowCheck(void) { return TRUE; }
    virtual void Enable(int enable);
    virtual int GetEnable(void);
    virtual int HasObj(void) const { return (m_obj != NULL); }

    /* Configuration retrieval functions. */
    SPxROC *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual void OnTimer(UINT_PTR nIDEvent);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    DECLARE_MESSAGE_MAP()

private:
    /* Private functions */

    /*
     * Handler functions.
     */
    afx_msg void OnRestart(void);
    afx_msg void OnTestPattern(void);
    afx_msg void OnGainASpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnGainBSpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnOffASpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnOffBSpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnTRGSpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnACPSpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnARPSpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSIG1Spin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSIG2Spin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnSHMSpin(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnEnable(BOOL bEnable);

    /* GUI control variables. */
    CStatic m_lblPulseWidths;
    unsigned int m_testPattern;		/* TPG pattern */
    SPxWinSpinCtrl m_gainA;		/* Video A gain */
    SPxWinSpinCtrl m_gainB;		/* Video B gain */
    SPxWinSpinCtrl m_offsetA;		/* Video A offset */
    SPxWinSpinCtrl m_offsetB;		/* Video B offset */
    SPxWinSpinCtrl m_trgWidth;		/* TRG width */
    SPxWinSpinCtrl m_acpWidth;		/* ACP width */
    SPxWinSpinCtrl m_arpWidth;		/* ARP width */
    SPxWinSpinCtrl m_sig1Width;		/* SIG1 width */
    SPxWinSpinCtrl m_sig2Width;		/* SIG2 width */
    SPxWinSpinCtrl m_shmWidth;		/* SHM width */

    /* The parent window */
    CWnd *m_parentWin;

    /* The object we are controlling */
    SPxROC *m_obj;

    /* Last card open state. */
    int m_isBoardOpen;

    using CDialog::Create;

}; /* class SPxROCCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
