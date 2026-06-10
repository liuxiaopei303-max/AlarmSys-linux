/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAboutDlgWin.h,v $
* ID: $Id: SPxAboutDlgWin.h,v 1.5 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*	Header for generic SPx server about dialog.
*
*
* Revision Control:
*   03/03/16 v1.5    AGC	Fix clang warnings.
*
* Previous Changes:
*   04/07/14 1.4    AGC	Remove unused SPxCriticalSection header.
*   26/06/14 1.3    AGC	Use appBit instead of SPxServerBase.
*			Support start year.
*   20/06/14 1.2    AGC	Support showing OSM copyright.
*   21/10/13 1.1    AGC	Initial version.
**********************************************************************/
#pragma once

/* Include the SPx resource header */
#include "SPxResource.h"

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* Forward declarations. */

/* Define our class. */
class SPxAboutDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxAboutDlgWin)

public:
    /* Construction and destruction */
    SPxAboutDlgWin(CWnd* pParentWin,
	UINT32 appBit,
	const char *appName,
	const char *titleExtra=NULL,
	int showCP=TRUE,
	int showOSM=FALSE,
	int startYear=0);
    virtual ~SPxAboutDlgWin(void);
    virtual BOOL Create(void);

    /* Dialog data. */
    enum { IDD = IDD_SPX_ABOUT_DIALOG };

protected:
    /* Protected functions */
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog(void);
    void OnBnClickedEmail(void);
    void OnBnClickedWebsite(void);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    using CDialog::Create;

}; /* SPxAboutDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
