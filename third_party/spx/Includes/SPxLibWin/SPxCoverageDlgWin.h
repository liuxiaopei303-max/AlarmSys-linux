/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCoverageDlgWin.h,v $
* ID: $Id: SPxCoverageDlgWin.h,v 1.7 2016/09/16 14:12:14 rew Exp $
*
* Purpose:
*	Header for radar coverage control dialog.
*
*
* Revision Control:
*   16/09/16 v1.7    AGC	Add tooltip support.
*
* Previous Changes:
*   02/09/16 1.6    AGC	Improve traditional distance units.
*   28/07/16 1.5    AGC	Remove num samples/azimuths controls.
*			Use slider to control alpha.
*   14/07/16 1.4    AGC	Support alpha.
*			Renderer/database changes moved to application.
*   08/07/16 1.3    AGC	Continue development.
*   16/06/16 1.2    AGC	Add missing forward declaration.
*   15/06/16 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

#include "SPxLibUtils/SPxEventHandler.h"
#include "SPxLibWin/SPxWinButton.h"
#include "SPxLibWin/SPxWinSpinCtrl.h"
#include "SPxLibWin/SPxWinEditCtrl.h"

/* Forward declarations. */
class SPxTiledMapSrcCoverage;
class SPxTiledMapDatabase;
class SPxTiledMapRenderer;
class SPxViewControl;

/*
 * SPxCoverageDlgWin dialog
 */
class SPxCoverageDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxCoverageDlgWin)

public:
    typedef void (*UnitsChanged)(int isLatLongDMS, int isDistTrad);

    /* Constructor and destructor. */
    SPxCoverageDlgWin(CWnd* pParent, SPxTiledMapSrcCoverage *obj,
		      unsigned int radarIndex=0);
    virtual ~SPxCoverageDlgWin(void);
    BOOL Create(void);
    void SyncFromObj(void);

    template<typename F, typename O>
    SPxErrorCode AddUnitsHandler(F fn, O *obj)
    {
	return m_unitsFn.Add(fn, obj);
    }
    template<typename F, typename O>
    SPxErrorCode RemoveUnitsHandler(F fn, O *obj)
    {
	return m_unitsFn.Remove(fn, obj);
    }

    /* Configuration functions. */
    SPxTiledMapSrcCoverage *GetObj(void) { return m_obj; }

    /* Dialog Data */
    enum { IDD = IDD_SPX_COVERAGE_CTRL_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual BOOL PreTranslateMessage(MSG *pMsg);
    virtual void OnOK(void) {}
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    SPxEventHandler<UnitsChanged> m_unitsFn;

    /* The parent window */
    CWnd *m_parentWin;

    /* The object we are controlling */
    SPxTiledMapSrcCoverage *m_obj;
    unsigned int m_radarIndex;

    /* Controls. */
    CButton m_radioPosDMS;
    CButton m_radioPosDecimal;
    SPxWinSpinCtrl m_spinDegLat;
    SPxWinSpinCtrl m_spinDegLong;
    CStatic m_lblDegLat;
    CStatic m_lblDegLong;
    SPxWinSpinCtrl m_spinMinLat;
    SPxWinSpinCtrl m_spinMinLong;
    CStatic m_lblMinLat;
    CStatic m_lblMinLong;
    SPxWinSpinCtrl m_spinSecLat;
    SPxWinSpinCtrl m_spinSecLong;
    CStatic m_lblSecLat;
    CStatic m_lblSecLong;
    CButton m_radioPosN;
    CButton m_radioPosS;
    CButton m_radioPosE;
    CButton m_radioPosW;
    SPxWinSpinCtrl m_spinDecLat;
    SPxWinSpinCtrl m_spinDecLong;
    CStatic m_lblDecLat;
    CStatic m_lblDecLong;
    CButton m_radioUnitsMetric;
    CButton m_radioUnitsTrad;
    CStatic m_lblHeight;
    SPxWinSpinCtrl m_spinHeight;
    CStatic m_lblRange;
    SPxWinSpinCtrl m_spinRange;
    CStatic m_lblBeamMin;
    CStatic m_lblBeamMax;
    SPxWinSpinCtrl m_spinBeamMin;
    SPxWinSpinCtrl m_spinBeamMax;
    CButton m_checkEarth;
    CStatic m_lblVisHeights;
    SPxWinEditCtrl m_editVisHeights;
    CButton m_radioHeightSea;
    CButton m_radioHeightGround;
    CSliderCtrl m_sliderAlpha;
    CToolTipCtrl m_toolTip;

    /* Other variables. */
    int m_isLatLongDMS;
    int m_isDistTrad;

    /* Private functions. */
    void syncValues(void);
    LRESULT OnSpinChange(WPARAM wParam, LPARAM lParam);
    void OnRadioPos(void);
    void OnRadioPosDMS(void);
    void OnRadioUnits(void);
    void OnToggleEarthCurvature(void);
    void OnHeightColourChange(UINT id);
    void OnEditHeights(NMHDR *, LRESULT *);
    void OnRadioHeight(void);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    void getPosDMS(double *latDegs, double *longDegs);
    double metresToUnits(double metres, int isHeight);
    double unitsToMetres(double units, int isHeight);

    using CDialog::Create;

}; /* class SPxCoverageDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
