/*********************************************************************
*
* (c) Copyright 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxP172RcClutterDlgWin.h,v $
* ID: $Id: SPxP172RcClutterDlgWin.h,v 1.1 2017/09/13 14:31:33 rew Exp $
*
* Purpose:
*   Header for the P172 clutter map control child dialog.
*
* Revision Control:
*   13/09/17 v1.1    SP 	Initial version.
*
* Previous Changes:
*
**********************************************************************/

#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

/* Include the classes we need. */
#include "SPxLibWin/SPxP172RcBaseDlgWin.h"

/*
 * SPxP172RcClutterDlgWin dialog.
 */
class SPxP172RcClutterDlgWin : public SPxP172RcBaseDlgWin
{
    DECLARE_DYNAMIC(SPxP172RcClutterDlgWin)

public:

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxP172RcClutterDlgWin(SPxP172RcInterfaceWin *rdrIf, BOOL isXBand);
    virtual ~SPxP172RcClutterDlgWin(void);
    virtual BOOL Create(CWnd* pParent);
    virtual BOOL Update(void);
    virtual void SendConfigSettings(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_P172_RC_CLUTTER_DIALOG };

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
    void SendClutterMapCommand(BOOL doClear);

    /* GUI callback functions */
    afx_msg void OnBnClickedClutterEnableCheck(void);
    afx_msg void OnCbnSelchangeClutterScansCombo(void);
    afx_msg void OnBnClickedClutterClearButton(void);

    /* 
     * Private static functions. 
     */

    /* 
     * Private variables. 
     */

    /* General GUI objects. */
    CButton *m_enableCheck;
    CComboBox *m_scansCombo;
    CStatic *m_scansLabel;
    CButton *m_clearBn;

    /* GUI data exchange variables. */
    BOOL m_isEnabled;
    unsigned int m_numScans;

}; /* class SPxP172RcClutterDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
