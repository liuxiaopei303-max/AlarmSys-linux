/*********************************************************************
*
* (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadarVisMapCtrlDlgWin.h,v $
* ID: $Id: SPxSimRadarVisMapCtrlDlgWin.h,v 1.4 2017/01/31 14:46:17 rew Exp $
*
* Purpose:
*	Header for the Simulated Radar visibility map controls child dialog.
*
*	GDI+ must be initialised by the application before using this
*	dialog.
*
* Revision Control:
*   31/01/17 v1.4    AGC	Support units configuration.
*
* Previous Changes:
*   03/03/16 1.3    AGC	Fix clang warnings.
*   01/12/15 1.2    AGC	Add SPxScDestDisplay forward declaration.
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
class SPxScDestDisplay;

/*
 * SPxSimRadarVisMapCtrlDlgWin dialog
 */
class SPxSimRadarVisMapCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxSimRadarVisMapCtrlDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxSimRadarVisMapCtrlDlgWin(CWnd* pParent, 
					 SPxSimRadar *obj,
					 SPxScDestDisplay *dest=NULL);
    virtual ~SPxSimRadarVisMapCtrlDlgWin(void);
    BOOL Create(void);
    virtual void SyncFromObj(void);

    void SetDisplayUnits(SPxUnitsDist_t unitsDist);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_RADAR_VIS_MAP_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }

    /* Configuration retrieval functions. */
    SPxSimRadar *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    virtual void OnPaint(void);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    CWnd *m_parentWin;
    SPxSimRadar *m_obj;
    SPxScDestDisplay *m_dest;
    SPxUnitsDist_t m_unitsDist;

    /* Controls. */
    CStatic m_lblVisMapHeights;
    SPxWinEditCtrl m_editVisMapHeights;
    CButton m_radioVisMapHeightSea;
    CButton m_radioVisMapHeightGround;

    /* Private functions */

    /*
     * Handler functions.
     */
    void OnEditChangeVisMapHeights(NMHDR *, LRESULT *);
    void OnBnClickedSpxSimRadarRadioVisMapHeightSea(void);
    void OnBnClickedSpxSimRadarRadioVisMapHeightGround(void);

    using SPxTabbedItemDlg::Create;

}; /* class SPxSimRadarVisMapCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
