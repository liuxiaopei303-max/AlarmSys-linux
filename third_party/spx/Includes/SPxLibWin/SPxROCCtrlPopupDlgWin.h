/*********************************************************************
*
* (c) Copyright 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxROCCtrlPopupDlgWin.h,v $
* ID: $Id: SPxROCCtrlPopupDlgWin.h,v 1.1 2013/07/25 10:16:08 rew Exp $
*
* Purpose:
*	Header for the SPxROC control pop-up dialog.
*
*
* Revision Control:
*   25/07/13 v1.1    REW	Initial Version.
*
* Previous Changes:
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxROCCtrlDlgWin.h"

/*
 * SPxROCCtrlPopupDlgWin dialog
 */
class SPxROCCtrlPopupDlgWin : public SPxROCCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxROCCtrlPopupDlgWin(CWnd* pParent, 
                          SPxROC *obj);
    virtual ~SPxROCCtrlPopupDlgWin();

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxROCCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
