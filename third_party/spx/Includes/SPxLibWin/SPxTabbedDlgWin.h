/*********************************************************************
*
* (c) Copyright 2014 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTabbedDlgWin.h,v $
* ID: $Id: SPxTabbedDlgWin.h,v 1.16 2017/04/12 08:34:22 rew Exp $
*
* Purpose:
*	Header for a generic tabbed dialog for displaying
*	multiple copies of the same dialog (for different objects)
*	in tabs or in a row (with optional scrollbars).
*
*
* Revision Control:
*   12/04/17 v1.16   SP 	Add SetAutoResize().
*
* Previous Changes:
*   21/02/17 1.15   AGC	Add show tabs only option.
*   31/01/17 1.14   AGC	Support single base name.
*   03/03/16 1.13   AGC	Fix clang warnings.
*   07/01/16 1.12   AGC	Add ShowTab().
*   01/12/15 1.11   AGC	Add SyncParent() to SPxTabbedItemDlg.
*   26/11/15 1.10   AGC	Support layout change message.
*   10/07/15 1.9    AGC	Add SelectTab() function.
*   02/07/15 1.8    AGC	Simplify requirements for SPxTabbedItemName interface.
*   23/06/15 1.7    AGC	Configurable base for tab indices.
*   22/04/15 1.6    SP 	Support single tab display.
*   22/10/14 1.5    AGC	Close child dialogs when closed.
*   03/06/14 1.4    AGC	Add required SPxError header.
*   02/06/14 1.3    AGC	Support update of tab titles.
*   17/02/14 1.2    AGC	Update layout when style changes.
*   27/01/14 1.1    AGC	Convert SPxTabbedPopupDlgWin into child dialog.
**********************************************************************/

#pragma once

/* Standard headers. */

/* Include the SPx resource header */
#include "SPxResource.h"

#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"

#include "SPxLibUtils/SPxAutoPtr.h"

#include "SPxLibUtils/SPxError.h"

#define SPX_TABBED_LAYOUT_CHANGE (WM_USER + 1)

/* Base class for child dialogs for tabbed control. */
class SPxTabbedItemDlg : public CDialog
{
public:
    explicit SPxTabbedItemDlg(
	UINT nIDTemplate,
	CWnd* pParentWnd = NULL)
	: CDialog(nIDTemplate, pParentWnd) {};

    virtual void SyncFromObj(void) = 0;
    virtual int GetIDD(void) const = 0;
    virtual CWnd *GetGroupBox(void) { return NULL; }
    virtual int ShowCheck(void) { return FALSE; }
    virtual void Enable(int /*enable*/) {}
    virtual int GetEnable(void) { return FALSE; }
    virtual int HasObj(void) const { return TRUE; }

protected:
    /* Function called to send message to parent dialog 
     * indicating that a resync is required.
     */
    void SyncParent(void);
};

class SPxTabbedItemName
{
public:
    typedef void (*NameChangedHandler)(void *userArg);

    explicit SPxTabbedItemName(const char *name=NULL);
    virtual ~SPxTabbedItemName(void);

    virtual SPxErrorCode GetTabName(char *buffer, int bufLen) const;
    virtual SPxErrorCode AddNameChangedHandler(NameChangedHandler fn, void *userArg);
    virtual SPxErrorCode RemoveNameChangedHandler(NameChangedHandler fn, void *userArg);

    SPxErrorCode SignalNameChange(void);

private:
    struct impl;
    SPxAutoPtr<impl> m_p;
};

/*
 * SPxTabbedDlgWin dialog
 */
class SPxTabbedDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxTabbedDlgWin)

public:
    /* Constructor and destructor. */
    SPxTabbedDlgWin(CWnd* pParent,
	SPxTabbedItemDlg *children[],
	unsigned int numChildren,
	void (*syncFn)(void *)=NULL, void *syncArg=NULL);
    virtual ~SPxTabbedDlgWin(void);
    virtual BOOL Create(void);
    void SyncFromObj(void);

    void SelectTab(int tab);

    void SetTabIndexBase(unsigned int tabIndexBase);
    void SetTabTitle(const char *tabTitle);
    void SetTabTitleObjects(SPxTabbedItemName *objects[]);
    void SetShowSingleTab(int show);
    void SetShowTabsOnly(int show);
    void SetAutoResize(int enabled, unsigned int minWidth=0, unsigned int minHeight=0);
    void ShowTab(int tab, int show);

    // Dialog Data
    enum { IDD = IDD_SPX_TABBED_DIALOG };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog(void);
    void OnOK(void){};
    void OnCancel(void);
    void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar);
    void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
    void OnTimer(UINT_PTR nIDEvent);
    void Reinit(void);
    DECLARE_MESSAGE_MAP()

private:

    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    CButton m_radioTabs;
    CButton m_radioSide;
    CTabCtrl m_tabCtrl;

    /* Handler functions. */
    void OnLayoutChange(void);
    void OnTabChanged(NMHDR *pNotifyStruct, LRESULT *result);
    void OnBnClickedCheck(UINT nID);
    LRESULT OnLayoutChangeMessage(WPARAM, LPARAM);
    void updateTabTitles(void);
    static void tabNameChanged(void *userArg);

    using CDialog::Create;

}; /* class SPxTabbedDlgWin */

/*********************************************************************
*
* End of file
*
**********************************************************************/
