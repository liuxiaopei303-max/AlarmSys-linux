/*********************************************************************
*
* (c) Copyright 2012 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxP226CtrlDlgWin.h,v $
* ID: $Id: SPxP226CtrlDlgWin.h,v 1.7 2016/09/23 13:54:45 rew Exp $
*
* Purpose:
*	Header for the P226 control child dialog.
*
*
* Revision Control:
*   23/09/16 v1.7    AGC	Support channel control.
*
* Previous Changes:
*   03/03/16 1.6    AGC	Fix clang warnings.
*			Support setting Local IR.
*   22/02/16 1.5    SP 	Disable controls when dialog disabled.
*   05/06/15 1.4    SP 	Support advanced control pop-up.
*   07/01/15 1.3    AGC	Derive from SPxTabbedItemDlg.
*   27/09/13 1.2    AGC	Include SPxCommon header.
*   12/12/12 1.1    REW	Initial Version.
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

#include "SPxLibWin/SPxTabbedDlgWin.h"

/* Forward declarations. */
class SPxP226AdvCtrlDlgWin;
class SPxNetworkReceiveP226;

/*
 * SPxP226CtrlDlgWin dialog
 */
class SPxP226CtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxP226CtrlDlgWin)

public:
    /* Constructor and destructor. */
    SPxP226CtrlDlgWin(CWnd* pParent, 
                      SPxNetworkReceiveP226 *obj,
                      BOOL showAdvCtrlBtn=TRUE);
    virtual ~SPxP226CtrlDlgWin();
    BOOL Create(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SRC_P226_CTRL_DIALOG };
    int GetIDD(void) const { return IDD; }

    /* Sync functions. */
    void SyncFromObj(void);
    void UpdateStatusVals(void);

    /* Configuration retrieval functions. */
    SPxNetworkReceiveP226 *GetObj(void) { return m_obj; }

    virtual HICON SetIcon(HICON icon, BOOL bigIcon);

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual void OnTimer(UINT_PTR nIDEvent);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    DECLARE_MESSAGE_MAP()

private:
    /* Private functions */

    void UpdateControlEnables(void);

    /*
     * Handler functions.
     */

    afx_msg void OnEnable(BOOL bEnable);
    afx_msg void OnQuery(void);
    afx_msg void OnChannel(void);
    afx_msg void OnRangeSet(void);
    afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    afx_msg void OnGainAuto(void);
    afx_msg void OnSeaClutterAuto(void);
    afx_msg void OnAdvOptionsButton(void);

    /* GUI control variables. */
    CComboBox m_comboChannel;		/* Output channel */
    double m_rangeMetres;		/* Requested range in metres */
    CSliderCtrl m_scanModeSlider;	/* Scan rate */
    CSliderCtrl m_gainSlider;		/* Gain */
    CButton m_gainAuto;			/* Auto-gain button */
    CSliderCtrl m_seaClutterSlider;	/* Sea clutter */
    CButton m_seaClutterAuto;		/* Auto-seaclutter button */
    CSliderCtrl m_rainSlider;		/* Rain */
    CSliderCtrl m_ftcSlider;		/* FTC */
    CSliderCtrl m_interferenceSlider;	/* Interference rejection */
    CSliderCtrl m_localIRSlider;	/* Local interference rejection */

    /* The parent window */
    CWnd *m_parentWin;

    /* The object we are controlling */
    SPxNetworkReceiveP226 *m_obj;

    /* The advanced control dialog */
    SPxP226AdvCtrlDlgWin *m_advCtrlDlg;
    BOOL m_showAdvCtrlBtn; /* True if available */

    using SPxTabbedItemDlg::Create;

}; /* class SPxP226CtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
