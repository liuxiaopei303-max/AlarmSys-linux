/*********************************************************************
*
* (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTiledMapPopupDlgWin.h,v $
* ID: $Id: SPxTiledMapPopupDlgWin.h,v 1.2 2014/01/15 14:30:07 rew Exp $
*
* Purpose:
*	Header for the tiled map control pop-up dialog.
*
*
* Revision Control:
*   15/01/14 v1.2    AGC	Support any number of custom sources.
*
* Previous Changes:
*   04/11/13 1.1    SP 	Initial ersion.
*
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxTiledMapDlgWin.h"

/*
 * SPxTiledMapPopupDlgWin dialog
 */
class SPxTiledMapPopupDlgWin : public SPxTiledMapDlgWin
{
public:
    /* Constructor and destructor. */
    SPxTiledMapPopupDlgWin(CWnd *parentWin,
                           SPxTiledMapDatabase *tdb,
                           SPxViewControl *viewControl);
    virtual ~SPxTiledMapPopupDlgWin();

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxTiledMapPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
