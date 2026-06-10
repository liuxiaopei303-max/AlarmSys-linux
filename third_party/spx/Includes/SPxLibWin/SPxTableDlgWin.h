/*********************************************************************
*
* (c) Copyright 2015, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTableDlgWin.h,v $
* ID: $Id: SPxTableDlgWin.h,v 1.6 2017/01/31 14:46:17 rew Exp $
*
* Purpose:
*	Header for a re-usable dialog that lays out child dialogs in
*	a table.
*
*
* Revision Control:
*   31/01/17 v1.6    AGC	Create() must be virtual.
*
* Previous Changes:
*   22/02/16 1.5    AGC	Support a maximum height.
*   10/12/15 1.4    AGC	Add tabbed item dialog override for Add().
*   26/11/15 1.3    AGC	Support adding items by ID.
*			Support showing some or all collapsible child dialogs.
*   16/11/15 1.2    AGC	Support headers.
*   09/11/15 1.1    AGC	Initial version.
**********************************************************************/
#pragma once

#include "SPxResource.h"
#include "SPxLibWin/SPxTabbedDlgWin.h"

/* Forward declarations. */
class SPxTableHdrDlgWin;
class SPxTabbedItemDlg;

class SPxTableDlgWin : public SPxTabbedItemDlg
{
    DECLARE_DYNAMIC(SPxTableDlgWin)

public:
    /* Constructor and destructor. */
    explicit SPxTableDlgWin(CWnd* pParent);
    virtual ~SPxTableDlgWin(void);
    virtual BOOL Create(void);
    virtual void SyncFromObj(void);

    void Add(unsigned int row, unsigned int col, CWnd *wnd,
	unsigned int colSpan=1,
	const char *hdrText=NULL, int collapsible=FALSE);
    void Add(unsigned int row, unsigned int col, SPxTabbedItemDlg *wnd,
	unsigned int colSpan=1,
	const char *hdrText=NULL, int collapsible=FALSE);
    void Add(unsigned int row, unsigned int col, int id,
	unsigned int colSpan=1);

    void Layout(void);
    void SetMaxHeight(int maxHeight);

    void ShowItem(unsigned int row, unsigned int col);
    void HideItem(unsigned int row, unsigned int col);
    void ShowAllItems(void);
    void HideAllItems(void);
    void ShowOtherItems(unsigned int row, unsigned int col);
    void HideOtherItems(unsigned int row, unsigned int col);

    /* Dialog Data */
    enum { IDD = IDD_SPX_TABLE_CTRL_DIALOG };
    virtual int GetIDD(void) const { return IDD; }

protected:
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog(void);
    virtual void OnOK(void) {}
    virtual void OnCancel(void) {}
    void OnSize(UINT nType, int cx, int cy);
    void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Private functions */
    void createHeader(SPxTableHdrDlgWin *hdr, const char *text);

    using SPxTabbedItemDlg::Create;

}; 

/*********************************************************************
*
* End of file
*
**********************************************************************/
