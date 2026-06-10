/*********************************************************************
*
* (c) Copyright 2012 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimTerrainCtrlPopupDlgWin.h,v $
* ID: $Id: SPxSimTerrainCtrlPopupDlgWin.h,v 1.3 2014/01/27 14:51:13 rew Exp $
*
* Purpose:
*	Header for the Terrain Database control pop-up dialog.
*
*
* Revision Control:
*   27/01/14 v1.3    AGC	Split terrain dialog into server and radar sections.
*
* Previous Changes:
*   09/11/12 1.2    AGC	Interface with SPxSimTerrain object.
*   26/10/12 1.1    AGC	Initial version.
**********************************************************************/

#pragma once

/* Include the base class header file */
#include "SPxSimTerrainCtrlDlgWin.h"

/*
 * SPxSimTerrainCtrlPopupDlgWin dialog
 */
class SPxSimTerrainCtrlPopupDlgWin : public SPxSimTerrainCtrlDlgWin
{
public:
    /* Constructor and destructor. */
    SPxSimTerrainCtrlPopupDlgWin(CWnd* pParent, 
				 SPxTerrainDatabase *tdb,
				 SPxSimTerrain *simTerrain[],
				 unsigned int numSimTerrain);
    virtual ~SPxSimTerrainCtrlPopupDlgWin(void);

protected:
    /* Protected functions */
    virtual BOOL OnInitDialog(void);
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);

public:
    /* Nothing at present */
   
}; /* class SPxSimTerrainCtrlPopupDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
