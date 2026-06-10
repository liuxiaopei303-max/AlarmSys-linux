/*********************************************************************
 *
 * (c) Copyright 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxLanguageDlgWin.h,v $
 * ID: $Id: SPxLanguageDlgWin.h,v 1.1 2016/04/29 09:19:58 rew Exp $
 *
 * Purpose:
 *   A pop-up dialog used to perform language selection.
 *
 * Revision Control:
 *   29/04/16 v1.1   SP 	Initial version..
 *
 * Previous Changes:
 *
 **********************************************************************/

#pragma once

/* Library headers */
#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

/* Library headers. */
#include "SPxResource.h"
#include "SPxLibUtils/SPxObj.h"

/*
 * SPxLanguageDlgWin dialog.
 */
class SPxLanguageDlgWin : public CDialog, public SPxObj
{
    DECLARE_DYNAMIC(SPxLanguageDlgWin)

public:

    /*
     * Public variables.
     */

    /* Dialog Data */
    enum { IDD = IDD_SPX_LANGUAGE_DIALOG };

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxLanguageDlgWin(CWnd *parentWin, HICON hIcon);
    virtual ~SPxLanguageDlgWin();

    /* General. */
    virtual SPxErrorCode LoadLanguages(const char *filename,
                                       const char *languageName);
    virtual BOOL IsLanguageChanged(void) { return m_isLanguageChanged; }
    virtual const char *GetLanguageName(void);
    virtual void OnOK(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    DECLARE_MESSAGE_MAP()
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

private:

    /*
     * Private types.
     */

    /*
     * Private variables.
     */

    /* General. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    HICON m_hIcon;
    BOOL m_isLanguageChanged;

    /* Dialog items. */
    CComboBox *m_combo;

    /*
     * Private functions.
     */

    SPxErrorCode parseFile(const char *filename, FILE *fd);
    SPxErrorCode setParams(const char *filename);

    /* GUI callback functions. */

}; /* Class SPxLanguageDlgWin */

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
