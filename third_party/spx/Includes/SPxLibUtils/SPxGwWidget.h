/*********************************************************************
 *
 * (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxGwWidget.h,v $
 * ID: $Id: SPxGwWidget.h,v 1.13 2015/07/31 13:30:16 rew Exp $
 *
 * Purpose:
 *   SPx graphics widget abstract base class.
 *
 * Revision Control:
 *  31/07/15 v1.13   AGC	Highlight selected widget points.
 *
 * Previous Changes:
 *  04/07/14 1.12   AGC	Add required stdio header.
 *  10/03/14 1.11   AGC	Add DeleteAllWidgets() function.
 *  29/01/14 1.10   AGC	Implement new SPxAreaObj IsActive() function.
 *  27/01/14 1.9    AGC	Add overloads of units/pixels conversion
 *			functions taking doubles.
 *  04/12/13 1.8    AGC	Move configuration functions here from SPxGwShape.
 *			Support inheritable options.
 *  26/11/13 1.7    AGC	Request redraw when active state is changed.
 *  22/11/13 1.6    AGC	Request redraw on modification.
 *  07/10/13 1.5    AGC	Move to SPxLibUtils.
 *			Make cross-platform.
 *			Add SPxAreaObj support.
 *  08/03/13 1.4    SP 	Make base class virtual.
 *  25/01/13 1.3    SP 	Allow group to be set in constructor.
 *                      Split this class into SPxGwShape.
 *                      Remove reference points.
 *  15/01/13 1.2    SP 	Remove namespace std from header.
 *  11/01/13 1.1    SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_GW_WIDGET_H
#define _SPX_GW_WIDGET_H

/* Other headers required. */
#include "stdio.h" /* For FILE. */
#include "SPxLibUtils/SPxAreaObj.h"
#include "SPxLibUtils/SPxGwObj.h"

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
class SPxGwWidgetGroup;
class SPxExtRenderer;

class SPxGwWidget : virtual public SPxGwObj, public SPxAreaObj
{
    /* Grant SPxGwWidgetGroup private access. */
    friend class SPxGwWidgetGroup;

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxGwWidget(SPxGwWidgetGroup *group);
    virtual ~SPxGwWidget(void);
    virtual void SetActive(int active=TRUE);
    virtual int IsActive(void) const;

    /* Configuration. */
    virtual const SPxGwWidgetGroup *GetWidgetGroup(void) const { return m_group; }
    virtual SPxGwWidgetGroup *GetWidgetGroup(void) { return m_group; }
    virtual SPxErrorCode DeleteAllWidgets(void) { return SPX_NO_ERROR; }
    virtual void RequestRedraw(void);

    virtual int IsChildSelected(void) const { return IsSelected(); }
    virtual SPxErrorCode SetReadOnly(int readOnly) { m_readOnly = readOnly; return SPX_NO_ERROR; }
    virtual int GetReadOnly(void) const { return m_readOnly; }
    virtual int IsPulseEnabled(void) const;

	/* Configuration. */
    virtual SPxErrorCode SetLineColour(UINT32 argb);
    virtual SPxErrorCode SetLineColour(UINT8 r, UINT8 g, 
                                       UINT8 b, UINT8 a=255);
    virtual SPxErrorCode SetLineColour(const char *name);
    virtual UINT32 GetLineColour(void) const;
    virtual UINT8 GetLineColourA(void) const;
    virtual UINT8 GetLineColourR(void) const;
    virtual UINT8 GetLineColourG(void) const;
    virtual UINT8 GetLineColourB(void) const;

    virtual SPxErrorCode SetFillColour(UINT32 argb);
    virtual SPxErrorCode SetFillColour(UINT8 r, UINT8 g, 
                                       UINT8 b, UINT8 a=255);
    virtual SPxErrorCode SetFillColour(const char *name);
    virtual UINT32 GetFillColour(void) const;
    virtual UINT8 GetFillColourA(void) const;
    virtual UINT8 GetFillColourR(void) const;
    virtual UINT8 GetFillColourG(void) const;
    virtual UINT8 GetFillColourB(void) const;

    virtual SPxErrorCode SetDash(const double dashes[], 
                                 unsigned int numDashes, 
                                 double offsetPixels);
    virtual SPxErrorCode GetDash(double dashes[],
                                 unsigned int *numDashes,
                                 double *offsetPixels) const;

    virtual SPxErrorCode SetLineWidthPixels(double lineWidth);
    virtual double GetLineWidthPixels(void) const;

    /* SPxAreaObj interface. */
    virtual int IsReadOnly(void) const;
    virtual int IsNameSettable(void) const { return TRUE; }
    virtual SPxErrorCode SetItemName(const char *name);
    virtual const char *GetItemName(void) const;
    virtual SPxErrorCode SetSelected(int selected);
    virtual int IsSelected(void) const { return m_isSelected; }
    virtual SPxErrorCode SetSelectedPoint(unsigned int idx);
    virtual unsigned int GetSelectedPoint(void) const { return m_currPoint; }
    virtual SPxAreaUnits GetUnits(void) const { return m_units; }
    virtual SPxErrorCode GetPos(double *x, double *y) const;

    virtual int IsColourSupported(void) const { return TRUE; }
    virtual int IsLineWidthSupported(void) const { return TRUE; }
    virtual int IsDashSupported(void) const { return TRUE; }
    virtual int IsInheritable(void) const { return (m_group != NULL); }

    virtual SPxErrorCode SetInheritMask(UINT32 mask);
    virtual UINT32 GetInheritMask(void) const;

    /* Implemented by derived class. */
    virtual SPxErrorCode LoadCfg(FILE *f)=0;
    SPxErrorCode LoadLine(const char *param, const char *value);
    virtual SPxErrorCode SaveCfg(FILE *f) const;
    SPxErrorCode SaveName(FILE *f) const;
    SPxErrorCode SaveCommon(FILE *f) const;

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    SPxErrorCode SetUnits(SPxAreaUnits units);

    /* Reference position. */
    virtual double GetRefX(void) const;
    virtual double GetRefY(void) const;

    SPxErrorCode UnitsToPixels(SPxExtRenderer *renderer,
	double x, double y, double *px, double *py) const;
    SPxErrorCode PixelsToUnits(SPxExtRenderer *renderer,
	double px, double py, double *x, double *y) const;
    SPxErrorCode UnitsToPixels(SPxExtRenderer *renderer,
	double x, double y, int *px, int *py) const;
    SPxErrorCode PixelsToUnits(SPxExtRenderer *renderer,
	int px, int py, double *x, double *y) const;

    UINT32 GetFillColourP(void) const { return PulseColour(GetFillColour()); }
    UINT32 GetLineColourP(void) const { return PulseColour(GetLineColour()); }
    UINT32 GetSelPointColour(void) const { return 0xFFFFFF00; }
    UINT32 PulseColour(UINT32 colour) const;

    /* Widget group. */
    virtual void SetWidgetGroupLink(SPxGwWidgetGroup *group);

    /* Rendering. */
    virtual SPxErrorCode Render(SPxExtRenderer *renderer);

private:

    /*
     * Private variables.
     */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Group that this widget belongs to. */
    SPxGwWidgetGroup *m_group;

    /* Units. */
    SPxAreaUnits m_units;

    /* Selected? */
    int m_isSelected;

    /* Selected point. */
    unsigned int m_currPoint;

    /* Read-only? */
    int m_readOnly;

    /* Widget colours. */
    UINT32 m_argbLine;
    UINT32 m_argbFill;

    /* Widget line dashing. */
    double m_dashOffsetPixels;
    double m_oldDashOffsetPixels;

    /* Widget line width. */
    double m_lineWidthPixels;
    double m_oldLineWidthPixels;

    /* Mask of which properties to inherit from parent. */
    UINT32 m_inheritMask;

    /*
     * Private functions.
     */
    SPxErrorCode startRendering(SPxExtRenderer *renderer);
    virtual SPxErrorCode renderAll(SPxExtRenderer *renderer)=0;
    void renderPoints(SPxExtRenderer *renderer);
    SPxErrorCode endRendering(SPxExtRenderer *renderer);

    /*
     * Private static functions.
     */

}; /* SPxGwWidget */

#endif /* _SPX_GW_WIDGET_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

