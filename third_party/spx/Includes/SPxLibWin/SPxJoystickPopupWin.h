/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxJoystickPopupWin.h,v $
* ID: $Id: SPxJoystickPopupWin.h,v 1.1 2016/10/13 09:48:16 rew Exp $
*
* Purpose:
*	Header for the joystick control pop-up dialog.
*
*
* Revision Control:
*   13/10/16 v1.1 AGC	Initial version.
*
* Previous Changes:
*
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxJoystickWin.h"

/*
 * SPxAISPopupDlgWin dialog
 */
class SPxJoystickPopupWin : public SPxJoystickWin
{
public:
    /* Constructor and destructor. */
    explicit SPxJoystickPopupWin(BOOL touchMode=FALSE);
    virtual ~SPxJoystickPopupWin();

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxJoystickPopupWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
