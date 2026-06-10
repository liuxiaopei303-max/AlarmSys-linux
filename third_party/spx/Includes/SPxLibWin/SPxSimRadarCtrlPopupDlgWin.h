/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimRadarCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimRadarCtrlPopupDlgWin.h,v 1.4 2017/01/31 14:46:17 rew Exp $
*
* Purpose:
*	Header for the Simulater Radar control pop-up dialog.
*
*
* Revision Control:
*   31/01/17 v1.4    AGC	Support multiple platforms.
*
* Previous Changes:
*   11/12/14 1.3    AGC	Allow any SPxScDestDisplay.
*   20/06/14 1.2    AGC	Clear display when radar disabled.
*   26/10/12 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimRadarCtrlDlgWin.h"

/*
 * SPxSimRadarCtrlPopupDlgWin dialog
 */
class SPxSimRadarCtrlPopupDlgWin : public SPxSimRadarCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimRadarCtrlPopupDlgWin(CWnd* pParent, 
			       SPxSimRadar *obj,
			       SPxScSourceLocal *sc,
			       SPxScDestDisplay *dest);
    virtual ~SPxSimRadarCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimRadarCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
