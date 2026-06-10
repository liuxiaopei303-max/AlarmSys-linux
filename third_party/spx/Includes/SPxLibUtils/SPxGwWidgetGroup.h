/*********************************************************************
 *
 * (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxGwWidgetGroup.h,v $
 * ID: $Id: SPxGwWidgetGroup.h,v 1.8 2014/03/10 12:54:26 rew Exp $
 *
 * Purpose:
 *   SPx graphics widget group.
 *
 * Revision Control:
 *  10/03/14 v1.8    AGC	Add DeleteAllWidgets() function.
 *
 * Previous Changes:
 *  04/12/13 1.7    AGC	Support inheritable options.
 *			Add DeleteAreas() function.
 *  22/11/13 1.6    AGC	Request redraw on modification.
 *  07/10/13 1.5    AGC	Move to SPxLibUtils.
 *			Make cross-platform.
 *			Add SPxAreaObj support.
 *  04/10/13 1.4    AGC	Simplify headers.
 *  25/01/13 1.3    SP 	Allow picture to be set in constructor.
 *			Derive class from SPxGwWidget.
 *  15/01/13 1.2    SP 	Prefix list with 'std::'.
 *  11/01/13 1.1    SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_GW_WIDGET_GROUP_H
#define _SPX_GW_WIDGET_GROUP_H

/* Other headers required. */
#include <stdio.h>
#include "SPxLibUtils/SPxAreaObj.h"
#include "SPxLibUtils/SPxGwWidget.h"

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

/* Declare classes we need. */
class SPxGwPicture;
class SPxExtRenderer;
struct SPxGwWidgetGroupPriv;

class SPxGwWidgetGroup : public SPxGwWidget
{
    /* Grant SPxGwPicture private access. */
    friend class SPxGwPicture;

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructors and destructor. */
    SPxGwWidgetGroup(SPxGwPicture *picture, SPxAreaUnits units=SPX_AREA_UNITS_PIXELS);
    SPxGwWidgetGroup(SPxGwWidgetGroup *group=NULL);
    virtual ~SPxGwWidgetGroup(void);

    /* Configuration. */
    virtual SPxErrorCode SetOffset(double x, double y);
    virtual double GetOffsetX(void) const;
    virtual double GetOffsetY(void) const;
    virtual double GetPositionX(void) const;
    virtual double GetPositionY(void) const;

    virtual SPxGwPicture *GetPicture(void);
    virtual void RequestRedraw(void);
    virtual int IsPulseEnabled(void) const;

    virtual SPxErrorCode AddWidgetAbove(SPxGwWidget *widget,
                                        SPxGwWidget *above=NULL);

    virtual SPxErrorCode AddWidgetBelow(SPxGwWidget *widget,
                                        SPxGwWidget *below=NULL);

    virtual SPxErrorCode RemoveWidget(SPxGwWidget *widget);

    virtual SPxErrorCode DeleteAllWidgets(void);

    /* Old style pixels configuration. */
    virtual SPxErrorCode SetOffsetPixels(double x, double y);
    virtual double GetOffsetXPixels(void) const { return GetOffsetX(); }
    virtual double GetOffsetYPixels(void) const { return GetOffsetY(); }
    virtual double GetPositionXPixels(void) const { return GetPositionX(); }
    virtual double GetPositionYPixels(void) const { return GetPositionY(); }

    /* SPxAreaObj interface. */
    virtual unsigned int GetNumTypes(void) const;
    virtual const char *GetTypeName(unsigned int typeIndex) const;
    virtual SPxAreaObj *CreateArea(unsigned int typeIndex);
    virtual SPxErrorCode DeleteAreas(void);

    virtual SPxErrorCode IterateObjects(SPxAreaObjFn fn, void *userArg) const;
    virtual SPxErrorCode SetSelected(int selected);
    virtual int IsChildSelected(void) const;
    virtual int AreChildrenReadOnly(void) const;
    virtual int IsMovable(void) const { return TRUE; }
    virtual SPxErrorCode SetPos(double x, double y);
    virtual SPxErrorCode GetPos(double *x, double *y) const;
    virtual int IsPointWithin(double x, double y, double margin) const;

    virtual SPxErrorCode LoadCfg(FILE *f);
    virtual SPxErrorCode SaveCfg(FILE *f) const;

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Picture. */
    virtual void SetPictureLink(SPxGwPicture *group);

    /* Rendering. */
    SPxErrorCode Render(SPxExtRenderer *renderer);

private:

    /*
     * Private variables.
     */
    SPxAutoPtr<SPxGwWidgetGroupPriv> m_p;

    /*
     * Private functions.
     */
    void CreateCommon(void);
    virtual SPxErrorCode renderAll(SPxExtRenderer * /*renderer*/) { return SPX_NO_ERROR; }
    SPxGwWidget *getNextWidget(void);

    /*
     * Private static functions.
     */

}; /* SPxGwWidgetGroup */

#endif /* _SPX_GW_WIDGET_GROUP_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

