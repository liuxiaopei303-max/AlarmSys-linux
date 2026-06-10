/*********************************************************************
*
* (c) Copyright 2012 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxP172RcSectorDlgWin.h,v $
* ID: $Id: SPxP172RcSectorDlgWin.h,v 1.6 2017/09/13 14:31:33 rew Exp $
*
* Purpose:
*	Header for the P172 sector blanking control child dialog.
*
* Revision Control:
*   13/09/17 v1.6    SP 	Remove unused icons.
*
* Previous Changes:
*   11/05/16 1.5    SP 	Support multi-language.
*   26/01/15 1.4    AGC	Remove unused HICON.
*   26/11/13 1.3    AGC	Remove unused m_parentWin variable.
*   26/10/12 1.2    SP 	Further development.
*   18/10/12 1.1    SP 	Initial version.
*
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

/* Include the classes we need. */
#include "SPxLibWin/SPxP172RcBaseDlgWin.h"
#include "SPxP172RcSectorSizeDlgWin.h"

/*
 * SPxP172RcSectorDlgWin dialog.
 */
class SPxP172RcSectorDlgWin : public SPxP172RcBaseDlgWin
{
    DECLARE_DYNAMIC(SPxP172RcSectorDlgWin)

public:

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxP172RcSectorDlgWin(SPxP172RcInterfaceWin *rdrIf,
                          BOOL isXBand);
    virtual ~SPxP172RcSectorDlgWin(void);
    virtual BOOL Create(CWnd* pParent);
    virtual BOOL Update(void);
    virtual void SendConfigSettings(void);
    SPxErrorCode SetStateFromConfig(void);
    SPxErrorCode SetConfigFromState(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_P172_RC_SECTOR_DIALOG };

protected:

    /*
     * Protected functions.
     */

    DECLARE_MESSAGE_MAP()
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
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
    void SendSectorEnableCommand(void);

    /* GUI callback functions */
    afx_msg void OnBnClickedSectorS1Check();
    afx_msg void OnBnClickedSectorS2Check();
    afx_msg void OnBnClickedSectorS3Check();
    afx_msg void OnBnClickedSectorS4Check();
    afx_msg void OnBnClickedSectorTxMuteCheck();
    afx_msg void OnBnClickedSectorSizeButton();

    /* 
     * Private static functions. 
     */

    /* 
     * Private variables. 
     */

    /* General GUI objects. */
    CButton *m_sectorSizeBn;
    CStatic *m_sectorSizeLed;

    /* Controls and status information. Note that these
     * structures are linked together to form a linked-list.
     */
    Control_t m_sectors[SPX_P172_NUM_SECTORS];
    Control_t m_txMute;

    /* Sector setup dialog. */
    SPxP172RcSectorSizeDlgWin *m_sectorSizeDlg;

    /* Labels. */
    CStringA m_transmitLabel;
    CStringA m_blankingLabel;
    CStringA m_sectorLabel;
    CStringA m_controlLabel;

}; /* class SPxP172RcSectorDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
