/*********************************************************************
*
* (c) Copyright 2014 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTerrainRadarCtrlDlgWin.h,v $
* ID: $Id: SPxSimTerrainRadarCtrlDlgWin.h,v 1.7 2017/01/31 14:46:17 rew Exp $
*
* Purpose:
*	Header for the Terrain Database Radar control child dialog,
*	this dialog is typically a child of SPxSimTerrainCtrlDlgWin.
*
*
* Revision Control:
*   31/01/17 v1.7    AGC	Support units configuration.
*
* Previous Changes:
*   03/03/16 1.6    AGC	Fix clang warnings.
*   26/11/15 1.5    AGC	Move blind sectors to radar dialog.
*   16/11/15 1.4    AGC	Add multiple blind sector support.
*   10/07/15 1.3    AGC	Support blind sectors.
*   02/06/14 1.2    AGC	Move target obscure setting here.
*   27/01/14 1.1    AGC	Initial version.
**********************************************************************/
#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxWinSpinCtrl. */
#include "SPxLibWin/SPxWinSpinCtrl.h"

/* For base class. */
#include "SPxLibWin/SPxTabbedDlgWin.h"

/* Forward declarations. */
class SPxSimTerrain;

/*
 * SPxSimTerrainRadarCtrlDlgWin dialog
 */
class SPxSimTerrainRadarCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxSimTerrainRadarCtrlDlgWin)

public:
    /* Constructor and destructor. */
    SPxSimTerrainRadarCtrlDlgWin(CWnd* pParent, 
				 SPxSimTerrain *obj);
    virtual ~SPxSimTerrainRadarCtrlDlgWin(void);
    BOOL Create(void);
    virtual void SyncFromObj(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_TERRAIN_RADAR_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }
    virtual int ShowCheck(void) { return TRUE; }
    virtual void Enable(int enable);
    virtual int GetEnable(void);

    void SetDisplayUnits(SPxUnitsDist_t unitsDist);

    /* Configuration retrieval functions. */
    SPxSimTerrain *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */

    /* The parent window */
    CWnd *m_parentWin;
    SPxUnitsDist_t m_unitsDist;

    /* The objects we are controlling */
    SPxSimTerrain *m_obj;

    /* Controls. */
    CComboBox m_comboMode;
    CButton m_checkObscureTerrain;
    CButton m_checkObscureTargets;
    SPxWinSpinCtrl m_spinGain;
    SPxWinSpinCtrl m_spinCliffRes;
    CStatic m_lblCliffRes;

    /*
     * Handler functions.
     */
    void OnSelChangeComboMode(void);
    void OnCheckObscureTerrain(void);
    void OnCheckObscureTargets(void);
    LRESULT OnSpinChange(WPARAM wParam, LPARAM lParam);

    using CDialog::Create;

}; /* class SPxSimTerrainRadarCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
