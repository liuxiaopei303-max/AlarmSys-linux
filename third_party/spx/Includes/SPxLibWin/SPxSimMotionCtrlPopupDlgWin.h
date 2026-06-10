/*********************************************************************
*
* (c) Copyright 2012, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimMotionCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimMotionCtrlPopupDlgWin.h,v 1.1 2012/10/26 13:52:35 rew Exp $
*
* Purpose:
*	Header for the Simulater Motion control pop-up dialog.
*
*
* Revision Control:
*   26/10/12 v1.1    AGC	Initial version.
*
* Previous Changes:
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimMotionCtrlDlgWin.h"

/*
 * SPxSimMotionCtrlPopupDlgWin dialog
 */
class SPxSimMotionCtrlPopupDlgWin : public SPxSimMotionCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimMotionCtrlPopupDlgWin(CWnd* pParent, 
			        SPxRadarSimulator *obj,
				SPxSimMotionRenderer *renderer=NULL);
    virtual ~SPxSimMotionCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimMotionCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
