/*********************************************************************
*
* (c) Copyright 2016, 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadarMtiCtrlDlgWin.h,v $
* ID: $Id: SPxSimRadarMtiCtrlDlgWin.h,v 1.3 2017/01/31 14:46:17 rew Exp $
*
* Purpose:
*	Header for the Simulated Radar control MTI child dialog.
*
*
* Revision Control:
*   31/01/17 v1.3    AGC	Support units configuration.
*
* Previous Changes:
*   03/03/16 1.2    AGC	Fix clang warnings.
*   11/01/16 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

#include "SPxLibWin/SPxTabbedDlgWin.h"

#include "SPxLibWin/SPxWinSpinCtrl.h"

/* Forward declarations. */
class SPxSimRadar;

/*
 * SPxSimRadarIffCtrlDlgWin dialog
 */
class SPxSimRadarMtiCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxSimRadarMtiCtrlDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxSimRadarMtiCtrlDlgWin(CWnd* pParent, 
				      SPxSimRadar *obj);
    virtual ~SPxSimRadarMtiCtrlDlgWin(void);
    BOOL Create(void);
    virtual void SyncFromObj(void);

    void SetDisplayUnits(SPxUnitsSpeed_t unitsSpeed);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_RADAR_MTI_CTRL_DIALOG };
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
    SPxUnitsSpeed_t m_unitsSpeed;

    /* Controls. */
    CStatic m_lblMinSpeed;
    CStatic m_lblMaxSpeed;
    SPxWinSpinCtrl m_spinMinSpeed;
    SPxWinSpinCtrl m_spinMaxSpeed;

    /* Private functions */
    void OnChangeMinSpeed(NMHDR*, LRESULT*);
    void OnChangeMaxSpeed(NMHDR*, LRESULT*);

    using SPxTabbedItemDlg::Create;

}; /* class SPxSimRadarMtiCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
