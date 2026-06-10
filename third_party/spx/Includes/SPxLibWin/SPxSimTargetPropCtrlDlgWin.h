/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTargetPropCtrlDlgWin.h,v $
*
* Purpose:
*	Header for the Simulated Target properties control child dialog.
*
*
* Revision Control:
*   15/12/16 v1.16   AGC	Support SART targets.
*
* Previous Changes:
*   29/11/16 1.15   AGC	Support target description.
*   08/04/16 1.14   AGC	Support IFF values of zero.
*   16/03/16 1.13   AGC	Support extended IFF mode 1.
*   03/03/16 1.12   AGC	Fix clang warnings.
*   31/07/15 1.11   AGC	Support on ground and fixed flags.
*   10/07/15 1.10   AGC	Support manually initiated flag.
*   17/03/15 1.9    AGC	Improve SPI handling.
*   22/10/14 1.8    AGC	Add display of current Mode-C code.
*   14/10/14 1.7    AGC	Support IFF SPI and X pulse.
*			Support test target bit.
*   18/07/14 1.6    AGC	Add Racon support.
*   02/06/14 1.5    AGC	Use new re-usable AIS dialog.
*   26/03/14 1.4    AGC	Support simulated target bit.
*   21/01/14 1.3    AGC	Support IFF.
*   03/05/13 1.2    AGC	Support ADS-B.
*   28/03/13 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPxWinSpinCtrl. */
#include "SPxLibWin/SPxWinSpinCtrl.h"

/* For SPxWinNumEditCtrl. */
#include "SPxLibWin/SPxWinNumEditCtrl.h"

/* For SPxWinEditCtrl. */
#include "SPxLibWin/SPxWinEditCtrl.h"

/* Forward declarations. */
class SPxSimTarget;
class SPxAISDlgWin;

/*
 * SPxSimTargetPropCtrlDlgWin dialog
 */
class SPxSimTargetPropCtrlDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxSimTargetPropCtrlDlgWin)

public:
    /* Constructor and destructor. */
    SPxSimTargetPropCtrlDlgWin(CWnd* pParent, 
			       SPxSimTarget *obj);
    virtual ~SPxSimTargetPropCtrlDlgWin(void);
    BOOL Create(void);
    void SyncFromObj(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_TARGET_PROP_CTRL_DIALOG };

    /* Configuration functions. */
    void SetObj(SPxSimTarget *obj);
    SPxSimTarget *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */

    /* The parent window */
    CWnd *m_parentWin;

    /* The objects we are controlling */
    SPxSimTarget *m_obj;
    int m_spiTimerRunning;

    /* Controls. */
    CStatic m_lblName;
    SPxWinEditCtrl m_editDesc;

    /* AIS controls. */
    SPxAutoPtr<SPxAISDlgWin> m_aisDlg;

    /* ADS-B controls. */
    SPxWinSpinCtrl m_spinAA;
    CComboBox m_comboCapabilities;
    CComboBox m_comboCategory;
    CComboBox m_comboSubCat;

    /* IFF controls. */
    CButton m_checkMode1;
    CButton m_checkMode2;
    CButton m_checkMode3A;
    SPxWinNumEditCtrl m_editMode1;
    CButton m_checkMode1Ext;
    SPxWinNumEditCtrl m_editMode2;
    SPxWinNumEditCtrl m_editMode3A;
    CStatic m_lblModeC;
    CButton m_checkSpi;
    CButton m_checkX;

    /* Racon controls. */
    CComboBox m_comboRacon;
    SPxWinSpinCtrl m_spinRaconOn;
    SPxWinSpinCtrl m_spinRaconOff;

    /* SART controls. */
    CButton m_checkSart;
    CStatic m_lblSartPulse;
    SPxWinSpinCtrl m_spinSartPulse;

    /* Other controls. */
    CButton m_checkSim;
    CButton m_checkTest;
    CButton m_checkGround;
    CButton m_checkManInit;
    CButton m_checkFixed;

    /* Private functions */
    void OnEditDesc(NMHDR*, LRESULT*);
    LRESULT OnSpinChange(WPARAM wParam, LPARAM lParam);
    void OnCapabilitiesSelChange(void);
    void OnCategorySelChange(void);
    void OnSubCatSelChange(void);
    void OnCheckMode1(void);
    void OnCheckMode2(void);
    void OnCheckMode3A(void);
    void OnSetMode1(NMHDR *, LRESULT *);
    void OnCheckMode1Ext(void);
    void OnSetMode2(NMHDR *, LRESULT *);
    void OnSetMode3A(NMHDR *, LRESULT *);
    void OnCheckSpi(void);
    void OnCheckX(void);
    void OnCheckSim(void);
    void OnCheckTest(void);
    void OnCheckGround(void);
    void OnCheckManInit(void);
    void OnCheckFixed(void);
    void OnRaconSelChange(void);
    void OnCheckSart(void);
    void syncModeC(void);
    void syncSpi(UINT8 iffFlags);

    using CDialog::Create;

}; /* class SPxSimTargetPropCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
