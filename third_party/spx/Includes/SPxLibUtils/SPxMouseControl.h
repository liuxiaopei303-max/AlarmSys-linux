/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxMouseControl.h,v $
* ID: $Id: SPxMouseControl.h,v 1.24 2017/07/18 14:26:06 rew Exp $
*
* Purpose:
*   Header file for SPxMouseControl class.
*
*
* Revision Control:
*   18/07/17 v1.24   SP 	Add SetViewCtrlPanEnabled() and SetViewCtrlZoomEnabled().
*
* Previous Changes:
*   22/06/17 1.23   AGC	Add Tilt and Pan/Tilt cursors.
*   12/04/17 1.22   AGC	Enforce minimum time for click and drag.
*   31/01/17 1.21   AGC	Support separators in sub-menus.
*   11/01/17 1.20   AGC	Support sub-menus in context menus.
*   21/09/15 1.19   AGC	Support parameter control.
*   10/09/15 1.18   AGC	Ignore mouse events when showing menu.
*   14/04/15 1.17   SP 	Allow view ctrl to be enabled / disabled.
*   07/01/15 1.16   SP 	Support default cursor.
*   11/12/14 1.15   SP 	Add SPX_CURSOR_VSIZE & SPX_CURSOR_HSIZE.
*   02/06/14 1.14   AGC	Generate mouse move message on modifier key change.
*   16/05/14 1.13   AGC	Support automatic context menus.
*   05/03/14 1.12   SP 	Report mouse release as handled 
*                       when view is panned.
*   13/12/13 1.11   SP 	Add SetZoomOnDoubleClick().
*   02/12/13 1.10   SP 	Add SetClickSoundEnabled().
*   08/11/13 1.9    AGC	Fix cppcheck warnings.
*   04/11/13 1.8    AGC	Add Set/GetObj() to SPxMouseInterface.
*   15/05/13 1.7    AGC	Only respond to mouse move events where mouse has moved.
*   25/04/13 1.6    AGC	Fix panning when double-clicking title bar.
*   04/03/13 1.5    AGC	Support high-precision mice when scrolling.
*   25/01/13 1.4    AGC	Support adding interfaces to front of list.
*   12/12/12 1.3    AGC	Allow recursion of handlers.
*   23/11/12 1.2    REW	Add virtual destructor.
*   23/11/12 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_MOUSE_CONTROL_H
#define _SPX_MOUSE_CONTROL_H

/*
 * Other headers required.
 */

#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxPopupMenu.h"

/* Forward declare other classes required. */
class SPxViewControl;
class SPxRenderer;
class SPxPopupMenuLater;

/* Cursor modes. */
enum SPxCursor_t
{
    SPX_CURSOR_NONE, /* Do not set cursor. */
    SPX_CURSOR_STD,
    SPX_CURSOR_VIEW,
    SPX_CURSOR_SELECT,
    SPX_CURSOR_VSIZE,
    SPX_CURSOR_HSIZE,
    SPX_CURSOR_TILT,
    SPX_CURSOR_PAN_TILT
};

/* Interface that classes can derive from. */
class SPxMouseInterface
{
public:
    typedef void SubMenu;

    SPxMouseInterface(void)
	: m_obj(NULL), m_ctxMenu(NULL),
	m_menuEnabled(TRUE), m_menuPriority(0)
    {}
    virtual ~SPxMouseInterface(void) {}
    virtual int HandleMousePress(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseRelease(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleDoubleClick(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseLeave(void);
    virtual int HandleMouseWheel(UINT32 flags, int x, int y, double delta);

    int AppendMenuItem(int enabled, const char *label,
	SPxPopupMenuFn fn, void *userArg, SPxMouseInterface *iface=NULL);
    void AppendMenuSeparator(SPxMouseInterface *iface=NULL);

    SubMenu *AppendSubMenu(const char *label,
			   SPxMouseInterface *iface=NULL);
    int AppendSubMenuItem(SubMenu *subMenu,
			  int enabled, const char *label,
			  SPxPopupMenuFn fn, void *userArg);
    void AppendMenuSeparator(SubMenu *subMenu);

    void SetObj(SPxObj *obj) { m_obj = obj; }
    SPxObj *GetObj(void) const { return m_obj; }

    void EnableMenu(int enable) { m_menuEnabled = enable; }
    void SetMenuPriority(unsigned int priority) { m_menuPriority = priority; }
    unsigned int GetMenuPriority(void) const { return m_menuPriority; }
    int IsMenuEnabled(void) const { return m_menuEnabled; }
    int HasContextMenu(void) const { return (m_ctxMenu != NULL); }

private:
    SPxObj *m_obj;
    SPxPopupMenuLater *m_ctxMenu;
    int m_menuEnabled;
    unsigned int m_menuPriority;
 
    void SetContextMenu(SPxPopupMenuLater *menu) { m_ctxMenu = menu; }

    friend class SPxMouseControl;
};

/*
 * Define our class, derived from the SPx base object.
 */
class SPxMouseControl : public SPxObj
{
public:
    /* Public functions. */

    /* Constructor and destructor. */
    explicit SPxMouseControl(SPxViewControl *viewCtrl=NULL);
    virtual ~SPxMouseControl(void);

    /* Renderers. */
    SPxErrorCode AddInterface(SPxMouseInterface *iface, int addToFront=FALSE);
    SPxErrorCode RemoveInterface(SPxMouseInterface *iface);

    /* Enable / disable click sound. */
    void SetClickSoundEnabled(int state) { m_isClickSoundEnabled = state; }

    /* Enable / disable zoom on double click. */
    void SetZoomOnDoubleClick(int state) { m_zoomOnDoubleClick = state; }

    /* Enable / disable common right-click context menu. */
    void SetContextMenuEnabled(int state) { m_isContextMenuEnabled = state; }

    /* Set default cursor. */
    SPxErrorCode SetDefaultCursor(SPxCursor_t cursor);
    SPxCursor_t GetDefaultCursor(void) { return m_defaultCursor; }

    /* Enable / disable view control. */
    void SetViewCtrlEnabled(int isEnabled);
    int IsViewCtrlEnabled(void);
    void SetViewCtrlPanEnabled(int isEnabled) {m_isViewCtrlPanEnabled = isEnabled;}
    int IsViewCtrlPanEnabled(void) {return (m_viewCtrl && m_isViewCtrlPanEnabled);}
    void SetViewCtrlZoomEnabled(int isEnabled) {m_isViewCtrlZoomEnabled = isEnabled;}
    int IsViewCtrlZoomEnabled(void) {return (m_viewCtrl && m_isViewCtrlZoomEnabled);}

    /* Parameter setting/getting */
    virtual int SetParameter(char *name, char *value);

protected:
    /* Functions for handling mouse interaction. */
    int HandleMousePress(UINT32 flags, int x, int y);
    int HandleMouseRelease(UINT32 flags, int x, int y);
    int HandleDoubleClick(UINT32 flags, int x, int y);
    int HandleMouseMove(UINT32 flags, int x, int y);
    int HandleMouseLeave(void);
    int HandleMouseWheel(UINT32 flags, int x, int y, double delta);
    int HandleModKeyChange(UINT32 flags);
    virtual int HandleSetCursor(void)=0;
    SPxCursor_t GetSPxCursor(void) const { return m_cursor; }

private:
    /* Private types. */

    struct InterfaceInfo_t
    {
        InterfaceInfo_t *next;
	SPxMouseInterface *iface;
    };

    /* Private variables. */

    /* View control. */
    SPxViewControl *m_viewCtrl;
    int m_zoomOnDoubleClick;            /* Allow zoom on double click. */

    /* Mouse. */
    int m_lastPosValid;			/* Last mouse position is valid? */
    int m_lastX;			/* Last x mouse position. */
    int m_lastY;			/* Last y mouse position. */
    int m_currPosValid;			/* Current mouse position is valid? */
    int m_currX;			/* Current x mouse position. */
    int m_currY;			/* Current y mouse position. */
    UINT32 m_mousePressLeftTime;	/* Ticker time of last mouse press. */
    InterfaceInfo_t *m_ifaces;		/* List of mouse interfaces. */
    SPxCursor_t m_cursor;
    int m_inHandler;			/* Already in handler? */
    int m_inShowMenu;			/* Already showing menu? */
    int m_isViewPanInProgress;          /* Is view being panned? */
    int m_isViewCtrlPanEnabled;         /* View control pan enabled? */
    int m_isViewCtrlZoomEnabled;        /* View control zoom enabled? */
    
    /* Mutex. */
    SPxCriticalSection m_mutex;         /* Mutex to support multi-thread. */

    /* Play click when button pressed? */
    int m_isClickSoundEnabled;

    /* Show context menu generated by mouse interfaces. */
    int m_isContextMenuEnabled;

    /* Default cursor. */
    SPxCursor_t m_defaultCursor;

    /* Private functions. */
    virtual SPxPopupMenu *createPopupMenu(void) const=0;
    virtual int PostMousePress(UINT32 flags, int x, int y) { return HandleMousePress(flags, x, y); }
    virtual int PostMouseRelease(UINT32 flags, int x, int y) { return HandleMouseRelease(flags, x, y); }
    virtual int PostDoubleClick(UINT32 flags, int x, int y) { return HandleDoubleClick(flags, x, y); }
    virtual int PostMouseMove(UINT32 flags, int x, int y) { return HandleMouseMove(flags, x, y); }
    virtual int PostMouseLeave(void) { return HandleMouseLeave(); }
    virtual int PostMouseWheel(UINT32 flags, int x, int y, double delta) { return HandleMouseWheel(flags, x, y, delta); }

    /* Private static functions. */

}; /* SPxMouseControl */

#endif /* _SPX_MOUSE_CONTROL_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
