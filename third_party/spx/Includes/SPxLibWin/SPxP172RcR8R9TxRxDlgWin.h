/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxP172RcR8R9TxRxDlgWin.h,v $
* ID: $Id: SPxP172RcR8R9TxRxDlgWin.h,v 1.11 2017/09/13 14:31:33 rew Exp $
*
* Purpose:
*	Header for the P172 SBS-800/900 and SxV series transceiver 
*       control child dialog.
*
* Revision Control:
*   13/09/17 v1.11   SP 	Support clutter map control.
*
* Previous Changes:
*   04/12/15 1.10   SP 	Changes to use Mk11 TWS mode status field.
*   26/11/15 1.9    SP 	Changes to support Mk11 radar.
*   23/09/15 1.8    SP 	Make status fields const.
*   26/08/15 1.7    SP 	Support linking of transceivers.
*   26/01/15 1.6    AGC	Remove unused HICON.
*   14/10/14 1.5    SP 	Add params to disable gain/sea/rain/IR (TODO 117-3).
*   26/03/14 1.4    SP 	Further changes for SxV support.
*   04/10/13 1.3    AGC	Simplify headers.
*   26/10/12 1.2    SP  Further development.
*   18/10/12 1.1    SP  Initial version.
*
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

/* Include the classes we need. */
#include "SPxLibWin/SPxP172RcBaseDlgWin.h"

/* Foward declare classes we use. */
class SPxP172RcDlgWin;
class SPxP172RcSectorDlgWin;
class SPxP172RcClutterDlgWin;

/*
 * SPxP172RcR8R9TxRxDlgWin dialog.
 */
class SPxP172RcR8R9TxRxDlgWin : public SPxP172RcBaseDlgWin
{
    DECLARE_DYNAMIC(SPxP172RcR8R9TxRxDlgWin)

public:

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxP172RcR8R9TxRxDlgWin(SPxP172RcInterfaceWin *rdrIf,
                            BOOL isXBand);
    virtual ~SPxP172RcR8R9TxRxDlgWin(void);
    virtual BOOL Create(CWnd* pParent);
    virtual void SetTxRxLink(SPxP172RcR8R9TxRxDlgWin *txRx);
    virtual BOOL Update(void);
    virtual void SendConfigSettings(void);
    virtual void SyncAfterEmergencyStop(void);
    virtual void SetRangeControl(const char *valueStr);
    virtual SPxErrorCode SetStateFromConfig(void);
    virtual SPxErrorCode SetConfigFromState(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_P172_RC_R8R9_TXRX_DIALOG };

protected:

    /*
     * Protected functions.
     */

    DECLARE_MESSAGE_MAP()
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual afx_msg void OnHScroll(UINT nSBCode, UINT nPos, 
                                   CScrollBar *pScrollBar);
    virtual afx_msg void OnDestroy(void);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen); 

private:

    /* 
     * Private types. 
     */

    /* 
     * Private functions. 
     */

    BOOL UpdateRangeControl(const SPxP172RcInterfaceWin::StatusField_t *field);
    BOOL UpdatePowerControl(const SPxP172RcInterfaceWin::StatusField_t *powerField,
                            const SPxP172RcInterfaceWin::StatusField_t *rpiField,
                            const SPxP172RcInterfaceWin::StatusField_t *sartField,
                            const SPxP172RcInterfaceWin::StatusField_t *twsField);
    BOOL UpdateFreqNumControl(const SPxP172RcInterfaceWin::StatusField_t *field);
    void SendFreqNumChirpCommand(void);
    void SendFilterCommand(void);

    /* GUI callback functions */
    afx_msg void OnCbnSelchangeModeCombo();
    afx_msg void OnBnClickedSwapButton();
    afx_msg void OnCbnSelchangeRangeCombo();
    afx_msg void OnCbnSelchangePowerCombo();
    afx_msg void OnCbnSelchangeFreqNumCombo();
    afx_msg void OnCbnSelchangeChirpCombo();
    afx_msg void OnCbnSelchangeIrCombo();

    /* 
     * Private static functions. 
     */

    /* Swap transceiver button. */
    CButton *m_txRxSwapBn;

    /* Controls and status information. Note that these
     * structures are linked together to form a linked-list.
     */
    Control_t m_runMode;
    Control_t m_range;
    Control_t m_power;
    Control_t m_freqNum;
    Control_t m_chirp;
    Control_t m_gain;
    Control_t m_seaFilter;
    Control_t m_rainFilter;
    Control_t m_ir;

    /* User-defined availability of filter controls. */
    BOOL m_isGainAvail;
    BOOL m_isSeaFilterAvail;
    BOOL m_isRainFilterAvail;
    BOOL m_isIRAvail;

    /* Sector blanking dialog. */
    SPxP172RcSectorDlgWin *m_sectBlankDlg;
    SPxP172RcClutterDlgWin *m_clutterMapDlg;

    /* Link to second transceiver. */
    SPxP172RcR8R9TxRxDlgWin *m_txRxLink;

}; /* class SPxP172RcR8R9TxRxDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
