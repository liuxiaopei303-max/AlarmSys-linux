/*********************************************************************
 *
 * (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxAreaEditorMouse.h,v $
 * ID: $Id: SPxAreaEditorMouse.h,v 1.7 2017/05/04 12:56:36 rew Exp $
 *
 * Purpose:
 *   SPx area editing mouse interface - supports mouse editing of
 *   SPxGw... objects and SPxActiveRegion objects.
 *
 * Revision Control:
 *  04/05/17 v1.7    AGC	Fix convertFromPixels() parameter names.
 *
 * Previous Changes:
 *  10/09/15 1.6    AGC	Add specification of window handle.
 *  02/07/15 1.5    AGC	Improve area object creation.
 *  24/10/13 1.4    SP 	Allow click margin to be changed.
 *  21/10/13 1.3    AGC	Add creation options to right-click menu.
 *  15/10/13 1.2    AGC	Request renderer redraw when areas changed.
 *  07/10/13 1.1    AGC	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_AREA_EDITOR_MOUSE_H
#define _SPX_AREA_EDITOR_MOUSE_H

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxAreaObj.h"
#include "SPxLibUtils/SPxMouseControl.h"
#include "SPxLibUtils/SPxObj.h"

/* Forward declare any classes we need. */
class SPxAreaObj;
class SPxPopupMenu;

class SPxAreaEditorMouse : public SPxObj, public SPxMouseInterface
{
public:
    /* Public functions. */
    explicit SPxAreaEditorMouse(SPxAreaObj *obj, SPxRenderer *renderer);
    virtual ~SPxAreaEditorMouse(void);

    virtual int HandleMousePress(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseRelease(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    virtual int HandleMouseLeave(void);

    virtual void Repopulate(void) { return; }
    virtual void SyncFromObj(void);
    virtual void SetCurrentObj(SPxAreaObj *obj, int selectNext=FALSE);
    virtual void SetCurrentPoint(unsigned int index);
    virtual void SetClickMarginPixels(unsigned int pixels) { m_clickMarginPx = pixels; }
    virtual unsigned int GetClickMarginPixels(void) { return m_clickMarginPx; }

#ifdef _WIN32
    SPxErrorCode SetWindow(HWND hwnd);
    SPxErrorCode SetMenu(SPxWinMenu *menu);
#endif

    SPxRenderer *GetRenderer(void) const { return m_renderer; }

protected:
    /* Protected functions. */
    SPxAreaObj *createAreaObj(SPxAreaObj *parent, unsigned int type, int x, int y);

private:
    /* Private variables. */
    SPxAreaObj *m_obj;			    /* Top level object we are controlling. */
    SPxRenderer *m_renderer;		    /* Renderer object (for unit conversions). */
    SPxAreaObj *m_currObj;		    /* Currently selected object. */
    unsigned int m_currPoint;		    /* Currently selected point. */
#ifdef _WIN32
    HWND m_hwnd;			    /* Window handle for positioning menu. */
    SPxWinMenu *m_menu;			    /* SPx menu to use. */
#endif

    struct impl;
    SPxAutoPtr<impl> m_p;		    /* Private class structure. */
    SPxAreaObj *m_foundSingleObj;	    /* Object found while searching for object to select. */
    unsigned int m_delPoint;		    /* Point that may get deleted. */
    int m_isMovePoint;			    /* Is a point being moved? */
    int m_hasMoved;			    /* Has the selected object been moved? */
    int m_newObj;			    /* New object to select. */

    int m_x;				    /* Current mouse position x. */
    int m_y;				    /* Current mouse position y. */
    int m_refX;				    /* Reference x position for moving objects. */
    int m_refY;				    /* Reference y position for moving objects. */
    unsigned int m_clickMarginPx;           /* Click margin for finding objects in pixels. */

    /* Private functions. */
    int mouseMove(UINT32 flags, int x, int y, SPxCursor_t *cursor);
    int selectObjAtPoint(int x, int y, int nextObj);
    void setReference(int x, int y);
    unsigned int findPoint(const SPxAreaObj *obj, int x, int y);
    int isPointWithin(const SPxAreaObj *obj, int x, int y);
    static void createTop(SPxPopupMenu *menu, int id, void *userArg);
    static void createChild(SPxPopupMenu *menu, int id, void *userArg);
    static void addPoint(SPxPopupMenu *menu, int id, void *userArg);
    static void delPoint(SPxPopupMenu *menu, int id, void *userArg);
    static void deleteItem(SPxPopupMenu *menu, int id, void *userArg);
    static SPxErrorCode findObjs(SPxAreaObj *obj, void *userArg);
    static SPxErrorCode findSingleObj(SPxAreaObj *obj, void *userArg);
    static SPxErrorCode findObjArg(SPxAreaObj *obj, void *userArg);
    void convertFromPixels(const SPxAreaObj *obj, int px, int py, double *x, double *y);
    void convertToPixels(const SPxAreaObj *obj, double x, double y, int *px, int *py);
    void redraw(void);

}; /* SPxAreaEditorMouse */

#endif /* _SPX_AREA_EDITOR_MOUSE_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

