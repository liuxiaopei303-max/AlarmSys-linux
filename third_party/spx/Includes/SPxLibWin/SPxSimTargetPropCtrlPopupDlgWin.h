/*********************************************************************
*
* (c) Copyright 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTargetPropCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimTargetPropCtrlPopupDlgWin.h,v 1.1 2013/03/28 15:36:02 rew Exp $
*
* Purpose:
*	Header for the Simulater Target Properties control pop-up dialog.
*
*
* Revision Control:
*   28/03/13 v1.1    AGC	Initial version.
*
* Previous Changes:
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimTargetPropCtrlDlgWin.h"

/*
 * SPxSimTargetPropCtrlPopupDlgWin dialog
 */
class SPxSimTargetPropCtrlPopupDlgWin : public SPxSimTargetPropCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimTargetPropCtrlPopupDlgWin(CWnd* pParent, 
				    SPxSimTarget *obj);
    virtual ~SPxSimTargetPropCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimTargetPropCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
