/*********************************************************************
*
* (c) Copyright 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTargetTerrainCtrlDlgWin.h,v $
* ID: $Id: SPxSimTargetTerrainCtrlDlgWin.h,v 1.2 2015/08/06 14:25:23 rew Exp $
*
* Purpose:
*	Header for the Simulated Target Name control child dialog.
*
*
* Revision Control:
*   06/08/15 v1.2    AGC	Only show terrain/buildings when enabled.
*
* Previous Changes:
*   31/07/15 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */
#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

/* Include the SPx resource header */
#include "SPxResource.h"

/* Forward declarations. */
class SPxRadarSimulator;
class SPxBitmapWin;

/*
 * SPxSimTargetTerrainCtrlDlgWin dialog
 */
class SPxSimTargetTerrainCtrlDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxSimTargetTerrainCtrlDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxSimTargetTerrainCtrlDlgWin(CWnd *parent,
	SPxRadarSimulator *radarSim);
    virtual ~SPxSimTargetTerrainCtrlDlgWin(void);

    void SetObj(SPxSimTarget *target);
    SPxSimTarget *GetObj(void) const;

    void SetRadarIndex(unsigned int radarIndex);
    unsigned int GetRadarIndex(void) const;

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_TARGET_TERRAIN_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    BOOL OnEraseBkgnd(CDC *pDC);
    void OnTimer(UINT_PTR nIDEvent);
    void OnShowWindow(BOOL bShow, UINT nStatus);
    void OnPaint(void);
    virtual void OnOK(void) {}
    virtual void OnCancel(void) {}
    DECLARE_MESSAGE_MAP()

private:
    SPxRadarSimulator *m_radarSim;
    SPxSimTarget *m_target;
    SPxAutoPtr<SPxBitmapWin> m_bitmap;
    unsigned int m_radarIndex;

    void draw(void);
    static SPxErrorCode drawRadarWrapper(unsigned int index, SPxSimRadar *radar, void *userArg);
    SPxErrorCode drawRadar(unsigned int index, const SPxSimRadar *radar);

}; /* class SPxSimTargetTerrainCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
