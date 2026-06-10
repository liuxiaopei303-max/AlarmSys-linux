/*********************************************************************
*
* (c) Copyright 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimBuildingCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimBuildingCtrlPopupDlgWin.h,v 1.2 2014/05/16 11:28:26 rew Exp $
*
* Purpose:
*	Header for the Simulater Buildings control pop-up dialog.
*
*
* Revision Control:
*   16/05/14 v1.2    AGC	Use SPxAreaEditorMouse.
*
* Previous Changes:
*   30/07/13 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimBuildingCtrlDlgWin.h"

/*
 * SPxSimBuildingCtrlPopupDlgWin dialog
 */
class SPxSimBuildingCtrlPopupDlgWin : public SPxSimBuildingCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimBuildingCtrlPopupDlgWin(CWnd* pParent, 
				  SPxBuildingDatabase *obj,
				  SPxAreaRenderer *renderer=NULL,
				  SPxViewControl *viewCtrl=NULL);
    virtual ~SPxSimBuildingCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimBuildingCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
