/*********************************************************************
*
* (c) Copyright 2012 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTargetCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimTargetCtrlPopupDlgWin.h,v 1.5 2014/08/18 16:05:40 rew Exp $
*
* Purpose:
*	Header for the Simulater Target control pop-up dialog.
*
*
* Revision Control:
*   18/07/14 v1.5    AGC	Add target visibility display.
*
* Previous Changes:
*   18/03/13 1.4    AGC	Support fixed test targets.
*   04/03/13 1.3    AGC	Improve display when using track input.
*   23/11/12 1.2    AGC	Add track renderer interaction.
*   26/10/12 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimTargetCtrlDlgWin.h"

/*
 * SPxSimTargetCtrlPopupDlgWin dialog
 */
class SPxSimTargetCtrlPopupDlgWin : public SPxSimTargetCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimTargetCtrlPopupDlgWin(CWnd* pParent, 
				SPxRadarSimServer *server,
				SPxTrackRenderer *renderer=NULL);
    virtual ~SPxSimTargetCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimTargetCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
