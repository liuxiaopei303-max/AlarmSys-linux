/*********************************************************************
*
* (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadarIffCtrlDlgWin.h,v $
* ID: $Id: SPxSimRadarIffCtrlDlgWin.h,v 1.3 2017/07/18 14:33:35 rew Exp $
*
* Purpose:
*	Header for the Simulated Radar control IFF child dialog.
*
*
* Revision Control:
*   18/07/17 v1.3    AGC	Support new IFF encode in video mode.
*
* Previous Changes:
*   03/03/16 1.2    AGC	Fix clang warnings.
*   26/11/15 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

#include "SPxLibWin/SPxWinEditCtrl.h"
#include "SPxLibWin/SPxTabbedDlgWin.h"

/* Forward declarations. */
class SPxSimRadar;

/*
 * SPxSimRadarIffCtrlDlgWin dialog
 */
class SPxSimRadarIffCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxSimRadarIffCtrlDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxSimRadarIffCtrlDlgWin(CWnd* pParent, 
				      SPxSimRadar *obj);
    virtual ~SPxSimRadarIffCtrlDlgWin(void);
    BOOL Create(void);
    virtual void SyncFromObj(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_RADAR_IFF_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }

    /* Configuration retrieval functions. */
    SPxSimRadar *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    CWnd *m_parentWin;
    SPxSimRadar *m_obj;

    /* Controls. */
    SPxWinEditCtrl m_editIffInterlace;
    CComboBox m_comboIffEncode;

    /* Private functions */

    /*
     * Handler functions.
     */
    void OnEditIffInterlace(NMHDR*, LRESULT*);
    void OnChangeIffEncode(void);

    using SPxTabbedItemDlg::Create;

}; /* class SPxSimRadarIffCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
