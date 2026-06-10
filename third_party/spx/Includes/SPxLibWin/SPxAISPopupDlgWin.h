/*********************************************************************
*
* (c) Copyright 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAISPopupDlgWin.h,v $
* ID: $Id: SPxAISPopupDlgWin.h,v 1.1 2014/06/02 15:16:14 rew Exp $
*
* Purpose:
*	Header for the AIS control pop-up dialog.
*
*
* Revision Control:
*   02/06/14 v1.1 AGC	Initial version.
*
* Previous Changes:
*
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxAISDlgWin.h"

/*
 * SPxAISPopupDlgWin dialog
 */
class SPxAISPopupDlgWin : public SPxAISDlgWin
{
public:
    /* Constructor and destructor. */
    SPxAISPopupDlgWin(CWnd* pParent, SPxAISIface *obj);
    virtual ~SPxAISPopupDlgWin();

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxAISPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
