/*********************************************************************
*
* (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTabbedPopupDlgWin.h,v $
* ID: $Id: SPxTabbedPopupDlgWin.h,v 1.6 2014/01/27 14:52:15 rew Exp $
*
* Purpose:
*	Header for a generic tabbed popup dialog for displaying
*	multiple copies of the same dialog (for different objects)
*	in tabs or in a row (with optional scrollbars).
*
*
* Revision Control:
*   27/01/14 v1.6    AGC	Move content to SPxTabbedDlgWin.
*
* Previous Changes:
*   21/10/13 1.5    AGC	Remove GetHeight() and GetMinWidth()
*			functions from SPxTabbedItemDlg.
*   15/10/13 1.4    AGC	Add GetHeight() and GetMinWidth()
*			functions to SPxTabbedItemDlg.
*   27/09/13 1.3    AGC	Include SPxCommon header.
*   30/07/13 1.2    AGC	Add HasObj() function to SPxTabbedItemDlg.
*   08/03/13 1.1    AGC	Initial Version.
**********************************************************************/
#pragma once

/* Include the base class header file */
#include "SPxTabbedDlgWin.h"

/*
 * SPxTabbedPopupDlgWin dialog
 */
class SPxTabbedPopupDlgWin : public SPxTabbedDlgWin
{
public:
    /* Constructor and destructor. */
    SPxTabbedPopupDlgWin(CWnd* pParent,
	SPxTabbedItemDlg *children[],
	unsigned int numChildren,
	void (*syncFn)(void *)=NULL, void *syncArg=NULL);
    virtual ~SPxTabbedPopupDlgWin();

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxTabbedPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
