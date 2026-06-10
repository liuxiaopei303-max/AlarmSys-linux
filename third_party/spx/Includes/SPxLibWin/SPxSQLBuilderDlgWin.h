/*********************************************************************
 *
 * (c) Copyright 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxSQLBuilderDlgWin.h,v $
 * ID: $Id: SPxSQLBuilderDlgWin.h,v 1.2 2016/07/28 13:19:10 rew Exp $
 *
 * Purpose:
 *   A modal pop-up dialog used to build SQL queries.
 *
 * Revision Control:
 *   28/07/16 v1.2   SP 	Remove m_selectSQLPart.
 *
 * Previous Changes:
 *   14/07/16 1.1   SP 	Initial version.
 *
 **********************************************************************/

#pragma once

#define SPX_AFXWIN
#include "SPxResource.h"
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxError.h"

/* Forward declare any classes required. */
class SPxSQLDatabase;
class SPxThread;

class SPxSQLBuilderDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxSQLBuilderDlgWin)

public:

    /*
     * Public variables.
     */

    /* Dialog Data */
    enum { IDD = IDD_SPX_SQL_BUILDER_DIALOG };

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxSQLBuilderDlgWin(CWnd* pParent, SPxSQLDatabase *db, const char *sql);
    virtual ~SPxSQLBuilderDlgWin();

    /* Get SQL from dialog. */
    virtual const char *GetSQL(void) { return m_fullSQL.GetBuffer(); }

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
    virtual void OnSize(UINT nType, int cWidth, int cHeight);
    virtual void OnGetMinMaxInfo(MINMAXINFO* lpMMI);

private:

    /*
     * Private types.
     */

    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* General. */
    SPxSQLDatabase *m_db;
    unsigned int m_minWidth;
    unsigned int m_minHeight;
    CStringA m_inputSQL;
    CStringA m_fullSQL;
    CStringA m_filterSQLPart;
    CStringA m_sortSQLPart;
    unsigned int m_numColumns;

    /* Dialog items. */
    CComboBox *m_tablesCombo;
    CListBox *m_columnsListBox;
    CButton *m_columnsSelectAllBn;
    CComboBox *m_filterColumnCombo;
    CComboBox *m_filterOpCombo;
    CEdit *m_filterValueEdit;
    CButton *m_filterAddBn;
    CButton *m_filterClearAllBn;
    CComboBox *m_sortColumnCombo;
    CComboBox *m_sortOpCombo;
    CButton *m_sortAddBn;
    CButton *m_sortClearAllBn;
    CButton *m_enableEditingCheck;
    CEdit *m_sqlEdit;

    /*
     * Private functions.
     */

    /* General. */
    void resize(int cWidth, int cHeight);
    void syncGUI(BOOL isTableChanged);
    void syncGUIFromSQL(const char *sql);

    /* GUI callback functions. */
    afx_msg void OnCbnSelchangeTablesCombo(void);
    afx_msg void OnLbnSelchangeColumnsList(void);
    afx_msg void OnBnClickedSelectAllColumnsButton(void);
    afx_msg void OnBnClickedFilterAddButton(void);
    afx_msg void OnBnClickedFilterClearAllButton(void);
    afx_msg void OnEnChangeFilterValueEdit(void);
    afx_msg void OnBnClickedSortAddButton(void);
    afx_msg void OnBnClickedSortClearAllButton(void);
    afx_msg void OnBnClickedEnableEditingCheck(void);
    afx_msg void OnEnChangeSQLEdit(void);

    /*
     * Private static functions.
     */
};

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
