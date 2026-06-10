/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTestGenCtrlDlgWin.h,v $
* ID: $Id: SPxTestGenCtrlDlgWin.h,v 1.6 2017/01/31 14:46:17 rew Exp $
*
* Purpose:
*	Header for the Test Generator control child dialog.
*
*
* Revision Control:
*   31/01/17 v1.6    AGC	Support units configuration.
*
* Previous Changes:
*   03/03/16 1.5    AGC	Fix clang warnings.
*   26/01/15 1.4    AGC	Remove unused HICON.
*   01/09/14 1.3    AGC	Use custom control for spin controls.
*   08/03/13 1.2    AGC	Support use within tabbed dialog.
*   26/10/12 1.1    AGC	Initial version.
*
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxWinSpinCtrl. */
#include "SPxLibWin/SPxWinSpinCtrl.h"

/* For base class. */
#include "SPxLibWin/SPxTabbedPopupDlgWin.h"

/* Forward declarations. */
class SPxTestGenerator;

/*
 * SPxTestGenCtrlDlgWin dialog
 */
class SPxTestGenCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxTestGenCtrlDlgWin)

public:
    /* Constructor and destructor. */
    SPxTestGenCtrlDlgWin(CWnd* pParent, 
			 SPxTestGenerator *obj);
    virtual ~SPxTestGenCtrlDlgWin();
    BOOL Create(void);
    virtual void SyncFromObj(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SRC_TEST_GEN_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }
    virtual CWnd *GetGroupBox(void) { return &m_mainGroup; }
    virtual int ShowCheck(void) { return TRUE; }
    virtual void Enable(int enable);
    virtual int GetEnable(void);

    void SetDisplayUnits(SPxUnitsDist_t distUnits);

    /* Configuration retrieval functions. */
    SPxTestGenerator *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */

    /* The parent window */
    CWnd *m_parentWin;
    SPxUnitsDist_t m_unitsDist;

    /* The object we are controlling */
    SPxTestGenerator *m_obj;

    /* Controls. */
    CStatic m_mainGroup;
    SPxWinSpinCtrl m_spinPattern;
    SPxWinSpinCtrl m_spinArg1;
    SPxWinSpinCtrl m_spinPeriod;
    SPxWinSpinCtrl m_spinPrf;
    SPxWinSpinCtrl m_spinRange;
    CStatic m_lblRange;

    /*
     * Handler functions.
     */
    LRESULT OnSpinChange(WPARAM wParam, LPARAM lParam);

    using SPxTabbedItemDlg::Create;
    
}; /* class SPxTestGenCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
