/*********************************************************************
 *
 * (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxAreaObj.h,v $
 * ID: $Id: SPxAreaObj.h,v 1.8 2014/07/18 13:51:44 rew Exp $
 *
 * Purpose:
 *	Area object interface definition.
 *
 * Revision Control:
 *  18/07/14 v1.8    AGC	Add AppendPoint() function.
 *
 * Previous Changes:
 *  29/01/14 1.7    AGC	Add IsActive() function to interface.
 *  22/01/14 1.6    AGC	Support optional symbol scaling.
 *  04/12/13 1.5    AGC	Add inheritable options.
 *			Add delete areas function.
 *  26/11/13 1.4    AGC	Use SPxSymbol for symbol rendering.
 *  18/11/13 1.3    AGC	Fix clang warning.
 *  24/10/13 1.2    SP 	Add GetFullItemName().
 *  07/10/13 1.1    AGC	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_AREA_OBJ_H
#define _SPX_AREA_OBJ_H

#include "SPxLibUtils/SPxTypes.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxSymbol.h"

/* Forward declarations. */
class SPxAreaObj;

#define SPX_AREA_NO_POINT (static_cast<unsigned int>(-1))

/* Function types. */
typedef SPxErrorCode (*SPxAreaObjFn)(SPxAreaObj *obj, void *userArg);

enum SPxAreaUnits
{
    SPX_AREA_UNITS_NONE = 0,
    SPX_AREA_UNITS_PIXELS = 1,
    SPX_AREA_UNITS_METRES = 2,
    SPX_AREA_UNITS_LAT_LONG = 3
};

enum SPxAreaAlignment
{
    SPX_AREA_ALIGN_TOP_LEFT = 0,
    SPX_AREA_ALIGN_TOP_CENTRE,
    SPX_AREA_ALIGN_TOP_RIGHT,
    SPX_AREA_ALIGN_CENTRE_LEFT,
    SPX_AREA_ALIGN_CENTRE_CENTRE,
    SPX_AREA_ALIGN_CENTRE_RIGHT,
    SPX_AREA_ALIGN_BOTTOM_LEFT,
    SPX_AREA_ALIGN_BOTTOM_CENTRE,
    SPX_AREA_ALIGN_BOTTOM_RIGHT,
    SPX_AREA_ALIGN_MAX, /* Must be last entry. */

    /* For backwards compatibility. */
    SPX_GW_ALIGN_TOP_LEFT = SPX_AREA_ALIGN_TOP_LEFT,
    SPX_GW_ALIGN_TOP_CENTRE = SPX_AREA_ALIGN_TOP_CENTRE,
    SPX_GW_ALIGN_TOP_RIGHT = SPX_AREA_ALIGN_TOP_RIGHT,
    SPX_GW_ALIGN_CENTRE_LEFT = SPX_AREA_ALIGN_CENTRE_LEFT,
    SPX_GW_ALIGN_CENTRE_CENTRE = SPX_AREA_ALIGN_CENTRE_CENTRE,
    SPX_GW_ALIGN_CENTRE_RIGHT = SPX_AREA_ALIGN_CENTRE_RIGHT,
    SPX_GW_ALIGN_BOTTOM_LEFT = SPX_AREA_ALIGN_BOTTOM_LEFT,
    SPX_GW_ALIGN_BOTTOM_CENTRE = SPX_AREA_ALIGN_BOTTOM_CENTRE,
    SPX_GW_ALIGN_BOTTOM_RIGHT = SPX_AREA_ALIGN_BOTTOM_RIGHT,
    SPX_GW_ALIGN_MAX = SPX_AREA_ALIGN_MAX
};

enum SPxAreaInherit
{
    SPX_AREA_INHERIT_NONE 	    = 0x0000,
    SPX_AREA_INHERIT_LINE_COLOUR    = 0x0001,
    SPX_AREA_INHERIT_FILL_COLOUR    = 0x0002,
    SPX_AREA_INHERIT_LINE_WIDTH	    = 0x0004,
    SPX_AREA_INHERIT_DASH	    = 0x0008,
    SPX_AREA_INHERIT_ALL	    = 0xFFFF
};

class SPxAreaObj
{
public:
    /* Constructor and destructor. */
    SPxAreaObj(void) {}
    virtual ~SPxAreaObj(void) {}
    virtual int IsActive(void) const { return TRUE; }

    /* Naming. */
    virtual int IsNameSettable(void) const { return FALSE; }
    virtual SPxErrorCode SetItemName(const char * /*name*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual const char *GetItemName(void) const=0;
    virtual const char *GetFullItemName(void) const { return GetItemName(); }

    /* Selection. */
    virtual SPxErrorCode SetSelected(int /*selected*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual int IsSelected(void) const { return FALSE; }

    /* Read-only. */
    virtual int IsReadOnly(void) const { return FALSE; }

    /* Create/delete children. */
    virtual unsigned int GetNumTypes(void) const { return 0; }
    virtual const char *GetTypeName(unsigned int /*typeIndex*/) const { return NULL; }
    virtual SPxAreaObj *CreateArea(unsigned int /*typeIndex*/) { return NULL; }
    virtual SPxErrorCode DeleteAreas(void) { return SPX_NO_ERROR; }

    /* Iterate over children. */
    virtual SPxErrorCode IterateObjects(SPxAreaObjFn /*fn*/, void * /*userArg*/) const { return SPX_NO_ERROR; }

    /* Inherit properties. */
    virtual int IsInheritable(void) const { return FALSE; }
    virtual SPxErrorCode SetInheritMask(UINT32 /*mask*/) { return SPX_NO_ERROR; }
    virtual UINT32 GetInheritMask(void) const { return SPX_AREA_INHERIT_NONE; }

    /* Get units. */
    virtual SPxAreaUnits GetUnits(void) const { return SPX_AREA_UNITS_NONE; }

    /* Move object. */
    virtual int IsMovable(void) const { return FALSE; }
    virtual SPxErrorCode SetPos(double /*x*/, double /*y*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetPos(double * /*x*/, double * /*y*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual int IsPointWithin(double /*x*/, double /*y*/, double /*margin*/) const { return FALSE; }
    virtual SPxErrorCode Update(void) { return SPX_ERR_NOT_SUPPORTED; }

    /* Offset object. */
    virtual int IsOffsetable(void) const { return FALSE; }
    virtual SPxErrorCode SetPixelOffset(double /*x*/, double /*y*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetPixelOffset(double * /*x*/, double * /*y*/) const { return SPX_ERR_NOT_SUPPORTED; }

    /* Move points. */
    virtual unsigned int GetNumPoints(void) const { return 0; }
    virtual SPxErrorCode SetPointPos(unsigned int /*idx*/, double /*x*/, double /*y*/, int /*fixedRatio*/=FALSE) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetPointPos(unsigned int /*idx*/, double * /*x*/, double * /*y*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode SetSelectedPoint(unsigned int /*idx*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual unsigned int GetSelectedPoint(void) const { return FALSE; }

    /* Add/remove points. */
    virtual int IsEditable(void) const { return FALSE; }
    virtual SPxErrorCode AddPoint(double /*x*/, double /*y*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode RemovePoint(unsigned int /*idx*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode AppendPoint(double /*x*/, double /*y*/) { return SPX_ERR_NOT_SUPPORTED; }

    virtual int IsAutoCloseSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetAutoCloseEnabled(int /*autoClose*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual int IsAutoCloseEnabled(void) const { return SPX_ERR_NOT_SUPPORTED; }

    /* Set colours. */
    virtual int IsColourSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetLineColour(UINT32 /*argb*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual UINT32 GetLineColour(void) const { return 0; }
    virtual SPxErrorCode SetFillColour(UINT32 /*argb*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual UINT32 GetFillColour(void) const { return 0; }

    /* Line width. */
    virtual int IsLineWidthSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetLineWidthPixels(double /*lineWidthPixels*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual double GetLineWidthPixels(void) const { return SPX_ERR_NOT_SUPPORTED; }

    /* Dash style. */
    virtual int IsDashSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetDash(const double /*dashes*/[], unsigned int /*numDashes*/, double /*offsetPixels*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode GetDash(double /*dashes*/[], unsigned int * /*numDashes*/, double * /*offsetPixels*/) const { return SPX_ERR_NOT_SUPPORTED; }

    /* Text. */
    virtual int IsTextSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetText(const char * /*text*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual const char *GetText(void) const { return NULL; }
    virtual SPxErrorCode SetFontName(const char * /*font*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual const char *GetFontName(void) const { return NULL; }
    virtual SPxErrorCode SetFontSize(unsigned int /*fontSize*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual unsigned int GetFontSize(void) const { return SPX_ERR_NOT_SUPPORTED; }

    /* Alignment. */
    virtual int IsAlignmentSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetAlignment(SPxAreaAlignment /*alignment*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxAreaAlignment GetAlignment(void) const { return SPX_AREA_ALIGN_TOP_LEFT; }

    /* Symbol. */
    virtual int IsSymbolSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetSymbol(SPxSymbolShape_t /*symbol*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxSymbolShape_t GetSymbol(void) const { return SPX_SYMBOL_SHAPE_CROSS; }
    virtual SPxErrorCode EnableScaling(int /*enableScaling*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual int IsScalingEnabled(void) const { return FALSE; }

    /* Image. */
    virtual int IsImageSupported(void) const { return FALSE; }
    virtual SPxErrorCode SetImage(const char * /*path*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual const char *GetFullImagePath(void) const { return NULL; }
    virtual const char *GetShortImagePath(void) const { return NULL; }

    /* Load/save state. */
    virtual int IsSaveSupported(void) const { return FALSE; }
    virtual SPxErrorCode Load(const char * /*name*/) { return SPX_ERR_NOT_SUPPORTED; }
    virtual SPxErrorCode Save(const char * /*name*/) const { return SPX_ERR_NOT_SUPPORTED; }
    virtual const char *GetFullPath(void) const { return NULL; }
    virtual const char *GetShortPath(void) const { return NULL; }

}; /* SPxAreaObj */

#endif /* _SPX_AREA_OBJ_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

