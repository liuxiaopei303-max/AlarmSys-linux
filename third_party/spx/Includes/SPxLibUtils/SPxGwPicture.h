/*********************************************************************
 *
 * (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxGwPicture.h,v $
 * ID: $Id: SPxGwPicture.h,v 1.15 2017/08/22 13:37:08 rew Exp $
 *
 * Purpose:
 *   SPx graphics widget picture class.
 *
 * Revision Control:
 *  04/08/17 v1.15   AJH 	Add support for DXF HATCH and rotation.
 *
 * Previous Changes:
 *  01/08/17 1.14   AJH	Support import of DXF file.
 *  16/07/14 1.13   SP 	Avoid compiler warning.
 *  15/07/14 1.12   SP 	Support import of XML file.
 *  10/03/14 1.11   AGC	Add DeleteAllWidgetGroups() function.
 *  04/12/13 1.10   AGC	Add DeleteAreas() function.
 *  22/11/13 1.9    AGC	Request redraw on modification.
 *  04/11/13 1.8    AGC	Move SPxRendererObj to new header.
 *  08/10/13 1.7    AGC	Exclude X11 when not supported.
 *  07/10/13 1.6    AGC	Move to SPxLibUtils.
 *			Make cross-platform.
 *			Add SPxAreaObj support.
 *  04/10/13 1.5    AGC	Simplify headers.
 *  29/01/13 1.4    SP 	Add Flush().
 *  25/01/13 1.3    SP 	Add some new functions.
 *  15/01/13 1.2    SP 	Remove namespace std from header.
 *			Add GetRenderer().
 *			Add static colour allocation functions.
 *  11/01/13 1.1    SP 	Initial version.
 *
 *********************************************************************/

#ifndef _SPX_GW_PICTURE_H
#define _SPX_GW_PICTURE_H

/* For SPX_X11_SUPPORTED. */
#include "SPxLibUtils/SPxCommon.h"

#ifndef _WIN32
#if SPX_X11_SUPPORTED
#include <X11/Xlib.h>
#endif
#endif
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/* Other headers required. */
#include "SPxLibUtils/SPxAreaObj.h"
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxGwObj.h"
#include "SPxLibUtils/SPxRendererObj.h"

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

/* Forward declarations. */
class SPxGwWidgetGroup;
class SPxGwWidget;
class SPxViewControl;
struct SPxGwPicturePriv;
#ifndef _WIN32
#ifdef SPX_X11_SUPPORTED
struct _GdkDisplay;
struct _GdkDrawable;
struct _GdkGC;
#endif
#endif

typedef struct
{
    SPxGwWidgetGroup *group;
    char handle[17];
    char name[129];
    int colour;
} SPxGwDxfLayer;

/*********************************************************************
 *
 *   Class definitions
 *
 *********************************************************************/

/* Define our class. */
class SPxGwPicture : public SPxGwObj, public SPxRendererObj, public SPxAreaObj
{
public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
#ifdef _WIN32
    SPxGwPicture(void);
#else
    SPxGwPicture(int isDoubleBuffered=FALSE);
#endif
    virtual ~SPxGwPicture(void);

#ifndef _WIN32
    /* Functions for backwards compatiblity with original X11 implementation. */
    virtual int IsDoubleBuffered(void) { return m_isDoubleBuffered; }

    virtual unsigned int GetDestinationDepth(void) {return m_depth;}
    virtual unsigned int GetMaxWidth(void) {return m_maxWidth;}
    virtual unsigned int GetMaxHeight(void) {return m_maxHeight;}

#ifdef SPX_X11_SUPPORTED
    virtual SPxErrorCode SetDestination(Display *x11Display,
                                        Drawable x11Drawable);
#endif

    virtual SPxErrorCode SetClipRect(unsigned int width, unsigned int height);
    virtual unsigned int GetClipRectWidth(void);
    virtual unsigned int GetClipRectHeight(void);

    virtual SPxErrorCode FillClipRect(UINT32 argb);
    virtual SPxErrorCode FillClipRect(UINT8 r, UINT8 g, UINT8 b, UINT8 a=255);
    virtual SPxErrorCode FillClipRect(const char *name);

    virtual SPxErrorCode SetBackgroundColour(UINT32 argb);
    virtual SPxErrorCode SetBackgroundColour(UINT8 r, UINT8 g, 
                                             UINT8 b, UINT8 a=0xFF);
    virtual SPxErrorCode SetBackgroundColour(const char *name);
    virtual UINT32 GetBackgroundColour(void);
    virtual UINT8 GetBackgroundColourA(void);
    virtual UINT8 GetBackgroundColourR(void);
    virtual UINT8 GetBackgroundColourG(void);
    virtual UINT8 GetBackgroundColourB(void);

    virtual SPxErrorCode SetClearBackgroundEnabled(int isEnabled);
    virtual int IsClearBackgroundEnabled(void);

    virtual SPxErrorCode SetAntiAliasEnabled(int isEnabled);
    virtual int IsAntiAliasEnabled(void) { return m_isAntiAliasEnabled; }

    virtual SPxErrorCode SetAlphaReplaceEnabled(int isEnabled);
    virtual int IsAlphaReplaceEnabled(void) { return m_isAlphaReplaceEnabled; }

    virtual SPxErrorCode Render(void);

    virtual SPxErrorCode Show(unsigned int x, unsigned int y,
                              unsigned int width, unsigned int height);
    virtual SPxErrorCode Show(void);
#endif

    /* Configuration. */
    virtual SPxErrorCode AddWidgetGroupAbove(SPxGwWidgetGroup *group,
                                             SPxGwWidgetGroup *above=NULL);

    virtual SPxErrorCode AddWidgetGroupBelow(SPxGwWidgetGroup *group,
                                             SPxGwWidgetGroup *below=NULL);

    virtual SPxErrorCode RemoveWidgetGroup(SPxGwWidgetGroup *group);

    virtual SPxErrorCode DeleteAllWidgetGroups(void);

    /* View control. */
    virtual SPxErrorCode SetViewControl(SPxViewControl *viewCtrl);
    virtual void RequestRedraw(void);
    virtual SPxErrorCode SetPulseEnabled(int enabled);
    virtual int IsPulseEnabled(void) const;

    /* Rendering. */
    virtual SPxErrorCode Render(SPxExtRenderer *renderer);

    /* SPxAreaObj interface. */
    virtual const char *GetItemName(void) const;

    virtual unsigned int GetNumTypes(void) const;
    virtual const char *GetTypeName(unsigned int typeIndex) const;
    virtual SPxAreaObj *CreateArea(unsigned int typeIndex);
    virtual SPxErrorCode DeleteAreas(void);

    virtual SPxErrorCode IterateObjects(SPxAreaObjFn fn, void *userArg) const;

    virtual int IsSaveSupported(void) const { return TRUE; }
    virtual SPxErrorCode Load(const char *name);
    virtual SPxErrorCode Save(const char *name) const;
    virtual const char *GetFullPath(void) const;
    virtual const char *GetShortPath(void) const;

    /* Named colours. */
    static SPxErrorCode AddNamedColour(const char *name, UINT32 argb);
    static SPxErrorCode AddNamedColour(const char *name, 
                                       UINT8 r, UINT8 g, 
                                       UINT8 b, UINT8 a=255);
    static SPxErrorCode GetNamedColour(const char *name, UINT32 *argbRtn);

    /* DXF access functions. */
    virtual SPxErrorCode SetDxfUnitsPerMetre(double unitsPerMetre);
    virtual double GetDxfUnitsPerMetre(void) const;
    virtual SPxErrorCode SetDxfOffsetMetres(double xOffset, double yOffset);
    virtual void GetDxfOffsetMetres(double *xOffset, double *yOffset) const;
    virtual SPxErrorCode SetDxfRotationDegs(double rotation);
    virtual double GetDxfRotationDegs(void) const;
    
private:
    /* Private variables. */
    SPxAutoPtr<SPxGwPicturePriv> m_p;	/* Private structure. */

#ifndef _WIN32
#ifdef SPX_X11_SUPPORTED
    /* Use opaque handles where possible so the header 
     * doesn't need to drag in gdk.h, cairo.h etc. 
     */
    struct _GdkDisplay *m_gdkDisplay;       /* Display. */
    struct _GdkDrawable *m_gdkDest;         /* Destination drawble. */
    struct _GdkGC *m_gdkDestGC;             /* Destination drawable GC. */
    struct _GdkDrawable *m_gdkBuffer;       /* Buffer drawable. */
    struct _GdkGC *m_gdkBufferGC;           /* Buffer drawable GC. */
#endif

    /* Destination (and buffer) attributes. */
    unsigned int m_depth;      /* Colour depth. */
    unsigned int m_maxWidth;   /* Maximum width. */
    unsigned int m_maxHeight;  /* Maximum height. */

    /* Renderer configuration. */
    UINT32 m_argbBackground;
    int m_isClearBackgroundEnabled;
    int m_isDoubleBuffered;
    int m_isAntiAliasEnabled;
    int m_isAlphaReplaceEnabled;
#endif

    SPxGwDxfLayer *m_dxfLayer;
    unsigned int m_dxfLayerCount;
    unsigned int m_dxfAngDir;
    double m_dxfUnitsPerMetre;
    double m_dxfOffsetXMetres;
    double m_dxfOffsetYMetres;
    double m_dxfRotationDegs;
    
    /* Private functions. */
    SPxGwWidgetGroup *getNextGroup(void);

    /* XML file importing. */
    SPxErrorCode loadXmlFile(FILE *handle, const char *filename);
    SPxErrorCode createXmlLayer(void *node, 
                                const char *units, 
                                const char *filename);
    SPxGwWidget *createXmlPolygon(void *node, SPxGwWidgetGroup *group);
    double getXmlNodeAttrDouble(void *node,
                                const char *attr,
                                double defaultValue=0.0);
    const char *getXmlNodeAttrText(void *node,
                                   const char *attr,
                                   const char *defaultValue="");
    UINT32 getXmlNodeAttrRGB(void *node,
                             const char *attr,
                             UINT32 defaultValue=0);
    int isXmlNodeAttrTrueOrFalse(void *node, 
                                 const char *attr,
                                 int defaultState=FALSE);

    /* DXF file importing. */
    SPxErrorCode loadDxfFile(FILE *fd, const char *filename);
    int getDxfCode(FILE *fd, int *code, char **value);
    SPxGwWidgetGroup *createDxfLayer(FILE *fd);
    SPxGwDxfLayer *getDxfLayer(const char *name);
    void applyDxfTransform(double *x, double *y);
    SPxGwWidget *createDxfLine(FILE *fd, unsigned int numPoints=0);
    SPxGwWidget *createDxfHatch(FILE *fd);
    SPxGwWidget *createDxfArc(FILE *fd);
    SPxGwWidget *createDxfText(FILE *fd);

}; /* SPxGwPicture */

#endif /* _SPX_GW_PICTURE_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/

