/*********************************************************************
 *
 * (c) Copyright 2013, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimDispPopupDlgWin.h,v $
 * ID: $Id: SPxVidSimDispPopupDlgWin.h,v 1.2 2013/10/04 15:31:08 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimDispPopupDlgWin class header.
 *
 * Revision Control:
 *   04/10/13 v1.2    AGC	Simplify headers.
 *
 * Previous Changes:
 *   15/07/13 1.1    SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_DISP_POPUP_DLG_WIN_H
#define _SPX_VID_SIM_DISP_POPUP_DLG_WIN_H

/* Include the SPx resource header */
#include "SPxResource.h"

/* Other headers required. */
#include "SPxLibWin/SPxVidSimDispDlgWin.h"

/*********************************************************************
 *
 *  Constants
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Macros
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Type definitions
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Class definitions
 *
 *********************************************************************/

/*===================================================================
 *
 * SPxVidSimDispPopupDlgWin class
 *      Main display dialog.
 *
 *===================================================================*/
class SPxVidSimDispPopupDlgWin : public SPxVidSimDispDlgWin
{
    DECLARE_DYNAMIC(SPxVidSimDispPopupDlgWin)

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimDispPopupDlgWin(SPxVidSimScene *scene);
    virtual ~SPxVidSimDispPopupDlgWin(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_VID_SIM_DISP_DIALOG };

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Dialog functions. */
    DECLARE_MESSAGE_MAP()
    virtual BOOL OnInitDialog();
    virtual void PreSubclassWindow(void);
    virtual void OnCancel(void);
    virtual afx_msg void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    /* Minimum dialog size. */
    unsigned int m_minWidth;
    unsigned int m_minHeight;

    /*
     * Private functions.
     */

}; /* SPxVidSimDispPopupDlgWin */

#endif /* _SPX_VID_SIM_DISP_POPUP_DLG_WIN_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
