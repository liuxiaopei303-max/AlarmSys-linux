/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimDispDlgWin.h,v $
 * ID: $Id: SPxVidSimDispDlgWin.h,v 1.6 2016/10/07 11:59:58 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimDispDlgWin class header.
 *
 * Revision Control:
  *  07/10/16 v1.6    SP 	Show view position.
 *
 * Previous Changes:
 *   03/03/16 1.5   AGC	Fix clang warnings.
 *   14/10/15 1.4   SP 	Remove InitialiseGL() override.
 *   04/09/15 1.3   SP 	Only redraw when visible.
 *   04/10/13 1.2   AGC	Simplify headers.
 *   15/07/13 1.1   SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_DISP_DLG_WIN_H
#define _SPX_VID_SIM_DISP_DLG_WIN_H

/* Include the SPx resource header */
#include "SPxResource.h"

/* Other headers required. */
#define SPX_AFXWIN
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxVidSimDispGL.h"

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

/* Forward declare child classes. */
class SPxVidSimDispDrawDlgWin;
class SPxVidSimDispCtrlDlgWin;

/*===================================================================
 *
 * SPxVidSimDispDlgWin class
 *      Main display dialog.
 *
 *===================================================================*/
class SPxVidSimDispDlgWin : public CDialog, public SPxVidSimDispGL
{
    DECLARE_DYNAMIC(SPxVidSimDispDlgWin)

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimDispDlgWin(SPxVidSimScene *scene);
    virtual ~SPxVidSimDispDlgWin(void);
    virtual BOOL Create(CWnd* pParent);

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
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    virtual afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual afx_msg void OnTimer(UINT_PTR nIDEvent);

    /* Override some OpenGL rendering functions. */
    virtual SPxErrorCode RedrawGL(void);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    SPxVidSimDispDrawDlgWin *m_drawDlg;   /* Drawing window. */
    SPxVidSimDispCtrlDlgWin *m_ctrlDlg;   /* Control dialog. */ 

    /*
     * Private functions.
     */

    void Resize(int cWidth, int cHeight);

    using CDialog::Create;

}; /* SPxVidSimDispDlgWin */

/*===================================================================
 *
 * SPxVidSimDispDrawDlgWin class
 *      Window to use for OpenGL drawing.
 *
 *===================================================================*/
class SPxVidSimDispDrawDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxVidSimDispDrawDlgWin)

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimDispDrawDlgWin(void);
    virtual ~SPxVidSimDispDrawDlgWin(void);
    BOOL Create(SPxVidSimDispDlgWin *pParent);

    /* Dialog Data */
    enum { IDD = IDD_SPX_VID_SIM_DISP_DRAW_DIALOG };

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Dialog functions. */
    DECLARE_MESSAGE_MAP()
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    virtual afx_msg BOOL OnEraseBkgnd(CDC *pDC);
    virtual afx_msg void OnPaint(void);
    virtual afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    virtual afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    virtual afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint point);

private:

    /*
     * Private variables.
     */

    SPxVidSimDispDlgWin *m_parentDlg;

    /*
     * Private functions.
     */

    using CDialog::Create;

}; /* SPxVidSimDispDrawDlgWin. */

/*===================================================================
 *
 * SPxVidSimDispCtrlDlgWin class
 *      Display control dialog.
 *
 *===================================================================*/
class SPxVidSimDispCtrlDlgWin : public CDialog
{
    DECLARE_DYNAMIC(SPxVidSimDispCtrlDlgWin)

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimDispCtrlDlgWin(SPxVidSimDispDlgWin *disp);
    virtual ~SPxVidSimDispCtrlDlgWin(void);
    virtual BOOL Create(CWnd* pParent);

    /* Update GUI. */
    void SyncGUI(void);

    /* Dialog Data */
    enum { IDD = IDD_SPX_VID_SIM_DISP_CTRL_DIALOG };

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Dialog functions. */
    DECLARE_MESSAGE_MAP()
    virtual void DoDataExchange(CDataExchange* pDX); 
    virtual BOOL OnInitDialog();
    virtual void OnOK(void){ /* Don't call the CDialog handler */ };
    virtual void OnCancel(void){ /* Don't call the CDialog handler */ };
    virtual afx_msg void OnSize(UINT nType, int cx, int cy);
    virtual afx_msg void OnTimer(UINT_PTR nIDEvent);

private:

    /*
     * Private variables.
     */

    SPxVidSimScene *m_scene;           /* Scene context. */
    SPxVidSimDispDlgWin *m_disp;   /* Display that we are controlling. */

    /* GUI objects. */
    CComboBox *m_viewModeCombo;    /* View selection combo box. */
    CStatic *m_viewLabel1;         /* XYZ/PY position. */
    CStatic *m_viewLabel2;         /* Lat/long position. */

    /* GUI data exchange. */
    UINT32 m_viewMode;
          
    /*
     * Private functions.
     */

    void Resize(int cWidth, int cHeight);

    /* GUI callbacks. */
    afx_msg void OnCbnSelchangeSPxVidSimDispCtrlViewCombo();

    using CDialog::Create;

}; /* SPxVidSimDispCtrlDlgWin */

#endif /* _SPX_VID_SIM_DISP_DLG_WIN_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
