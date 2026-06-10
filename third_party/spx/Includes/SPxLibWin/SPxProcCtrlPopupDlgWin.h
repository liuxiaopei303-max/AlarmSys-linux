/*********************************************************************
*
* (c) Copyright 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxProcCtrlPopupDlgWin.h,v $
* ID: $Id: SPxProcCtrlPopupDlgWin.h,v 1.1 2015/03/06 15:24:15 rew Exp $
*
* Purpose:
*	Header for the process control pop-up dialog.
*
*
* Revision Control:
*   06/03/15 v1.1    AGC	Initial version.
*
* Previous Changes:
*
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxProcCtrlDlgWin.h"

/* Forward declarations. */
class SPxProcParamIface;

/*
 * SPxProcCtrlPopupDlgWin dialog
 */
class SPxProcCtrlPopupDlgWin : public SPxProcCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxProcCtrlPopupDlgWin(CWnd* pParent, 
			   SPxProcParamIface *obj,
			   const char *title=NULL);
    virtual ~SPxProcCtrlPopupDlgWin();

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

}; /* class SPxProcCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
