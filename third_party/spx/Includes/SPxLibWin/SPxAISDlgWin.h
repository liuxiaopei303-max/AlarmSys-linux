/*********************************************************************
*
* (c) Copyright 2014, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAISDlgWin.h,v $
* ID: $Id: SPxAISDlgWin.h,v 1.3 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*	Header for AIS control dialog.
*
*
* Revision Control:
*   03/03/16 v1.3    AGC	Fix clang warnings.
*
* Previous Changes:
*   06/06/14 1.2    AGC	Override OnOK() to prevent Enter closing dialog.
*   02/06/14 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxWinSpinCtrl. */
#include "SPxLibWin/SPxWinSpinCtrl.h"

/* For SPxWinEditCtrl */
#include "SPxLibWin/SPxWinEditCtrl.h"

/* Forward declarations. */
struct SPxAISInfo;
class SPxAISIface;

/*
 * SPxAISDlgWin dialog
 */
class SPxAISDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxAISDlgWin)

public:
    /* Constructor and destructor. */
    SPxAISDlgWin(CWnd* pParent, SPxAISIface *obj);
    virtual ~SPxAISDlgWin(void);
    BOOL Create(void);
    void SyncFromObj(void);

    /* Configuration functions. */
    void SetObj(SPxAISIface *obj);
    SPxAISIface *GetObj(void) { return m_obj; }

    /* Dialog Data */
    enum { IDD = IDD_SPX_AIS_CTRL_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void) {}
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */

    /* The parent window */
    CWnd *m_parentWin;

    /* The object we are controlling */
    SPxAISIface *m_obj;

    /* Controls. */
    SPxWinSpinCtrl m_spinMmsi;
    SPxWinSpinCtrl m_spinImo;
    SPxWinSpinCtrl m_spinNavStatus;
    SPxWinEditCtrl m_editCallSign;
    SPxWinSpinCtrl m_spinShipType;
    SPxWinEditCtrl m_editDest;
    CButton m_checkEta;
    SPxWinSpinCtrl m_spinEtaDay;
    CComboBox m_comboEtaMonth;
    SPxWinSpinCtrl m_spinEtaHour;
    SPxWinSpinCtrl m_spinEtaMin;

    /* Other variables. */

    /* Private functions. */
    LRESULT OnSpinChange(WPARAM wParam, LPARAM lParam);
    void OnSetCallSign(NMHDR*, LRESULT*);
    void OnSetDestination(NMHDR*, LRESULT*);
    void OnEtaMonthSelChange(void);
    void OnCheckEta(void);

    using CDialog::Create;

}; /* class SPxAISDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
