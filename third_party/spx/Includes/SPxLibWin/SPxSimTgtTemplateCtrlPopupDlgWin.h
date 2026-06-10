/*********************************************************************
*
* (c) Copyright 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTgtTemplateCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimTgtTemplateCtrlPopupDlgWin.h,v 1.1 2014/05/16 11:28:26 rew Exp $
*
* Purpose:
*	Header for the Simulater Target Template control pop-up dialog.
*
*
* Revision Control:
*   16/05/14 v1.1    AGC	Initial version.
*
* Previous Changes:
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimTgtTemplateCtrlDlgWin.h"

/*
 * SPxSimTgtTemplateCtrlPopupDlgWin dialog
 */
class SPxSimTgtTemplateCtrlPopupDlgWin : public SPxSimTgtTemplateCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimTgtTemplateCtrlPopupDlgWin(CWnd* pParent, 
				     SPxRadarSimulator *obj);
    virtual ~SPxSimTgtTemplateCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimTgtTemplateCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
