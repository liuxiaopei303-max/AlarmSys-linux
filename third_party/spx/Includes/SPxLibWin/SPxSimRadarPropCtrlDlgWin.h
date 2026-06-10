/*********************************************************************
*
* (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadarPropCtrlDlgWin.h,v $
* ID: $Id: SPxSimRadarPropCtrlDlgWin.h,v 1.5 2017/04/12 09:22:18 rew Exp $
*
* Purpose:
*	Header for the Simulated Radar properties control child dialog.
*
*
* Revision Control:
*   12/04/17 v1.5    AGC	Add target position type control.
*
* Previous Changes:
*   31/01/17 1.4    AGC	Support units configuration.
*   03/03/16 1.3    AGC	Fix clang warnings.
*   01/12/15 1.2    AGC	Use project code to disable some controls.
*   26/11/15 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxWinSpinCtrl. */
#include "SPxLibWin/SPxWinSpinCtrl.h"

#include "SPxLibWin/SPxTabbedDlgWin.h"

/* Forward declarations. */
class SPxSimRadar;

/*
 * SPxSimRadarPropCtrlDlgWin dialog
 */
class SPxSimRadarPropCtrlDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxSimRadarPropCtrlDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxSimRadarPropCtrlDlgWin(CWnd* pParent, 
				       SPxSimRadar *obj,
				       unsigned int projectCode);
    virtual ~SPxSimRadarPropCtrlDlgWin(void);
    BOOL Create(void);
    virtual void SyncFromObj(void);

    void SetDisplayUnits(SPxUnitsDist_t unitsDist);

    /* Dialog Data */
    enum { IDD = IDD_SPX_SIM_RADAR_PROP_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }

    /* Configuration retrieval functions. */
    SPxSimRadar *GetObj(void) { return m_obj; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    CWnd *m_parentWin;
    SPxSimRadar *m_obj;
    const unsigned int m_projectCode;
    SPxUnitsDist_t m_unitsDist;

    CComboBox m_comboScanMode;
    SPxWinSpinCtrl m_spinScanStart;
    SPxWinSpinCtrl m_spinScanEnd;
    CStatic m_lblScanStartDegs;
    CStatic m_lblScanEndDegs;
    SPxWinSpinCtrl m_spinPeriod;
    SPxWinSpinCtrl m_spinPrf;
    CStatic m_lblStartRange;
    CStatic m_lblEndRange;
    SPxWinSpinCtrl m_spinStartRange;
    SPxWinSpinCtrl m_spinEndRange;
    SPxWinSpinCtrl m_spinBeamwidthH;
    SPxWinSpinCtrl m_spinMinBeamAngle;
    SPxWinSpinCtrl m_spinMaxBeamAngle;
    CSliderCtrl m_sliderSensitivity;
    CStatic m_lblSensitivity;
    SPxWinSpinCtrl m_spinPulseLength;
    CComboBox m_comboTargetPos;
    SPxWinSpinCtrl m_spinSenderID;
    CButton m_checkSlantRange;
    CButton m_checkEarthCurvature;
    CButton m_checkAntiClockwise;
    CStatic m_lblBeamHoriz;
    CStatic m_lblBeamHeightMin;
    CStatic m_lblBeamHeightMax;
    CStatic m_lblPulseLen;

    /* Private functions */
    void OnChangeScanMode(void);
    void OnSpinScanStart(NMHDR *, LRESULT *);
    void OnSpinScanEnd(NMHDR *, LRESULT *);
    void OnSpinChangePeriod(NMHDR *, LRESULT *);
    void OnSpinChangePrf(NMHDR *, LRESULT *);
    void OnSpinChangeStartRange(NMHDR *, LRESULT *);
    void OnSpinChangeEndRange(NMHDR *, LRESULT *);
    void OnSpinChangeBeamwidthH(NMHDR *, LRESULT *);
    void OnSpinChangeMinBeamAngle(NMHDR *, LRESULT *);
    void OnSpinChangeMaxBeamAngle(NMHDR *, LRESULT *);
    void OnSpinChangePulseLength(NMHDR *, LRESULT *);
    void OnChangeTargetPos(void);
    void OnSpinChangeSenderID(NMHDR *, LRESULT *);
    void OnCheckSlantRange(void);
    void OnCheckEarthCurvature(void);
    void OnCheckAntiClockwise(void);

    using SPxTabbedItemDlg::Create;

}; /* class SPxSimRadarPropCtrlDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
