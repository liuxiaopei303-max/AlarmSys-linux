/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxRendererD2D.h,v $
* ID: $Id: SPxRendererD2D.h,v 1.19 2017/08/31 15:36:19 rew Exp $
*
* Purpose:
*   Header for SPxRendererD2D class which supports rendering
*   of graphics in a MS Direct2D Windows application.
*
*
* Revision Control:
*   31/08/17 v1.19   AGC	Add floating point overload for renderArc().
*				Add floating point overload for renderSector().
*
* Previous Changes:
*   01/08/17 1.18   AGC	Add floating point overload for renderText().
*   22/06/17 1.17   AGC	Add floating point overload for renderLine().
*   19/10/16 1.16   AGC	Add floating point overload for renderPoly().
*   24/06/16 1.15   AGC	Cache text layouts.
*   07/01/16 1.14   AGC	Add floating point overload for renderEllipse().
*   04/12/15 1.13   REW	Support mouseOverBitmap in SPxSoftButtons.
*   12/06/15 1.12   AGC	Support user specified soft button sizes.
*   14/04/15 1.11   AGC	Support ID2D1DeviceContext targets.
*			Support perspective version of renderImage().
*   09/02/15 1.10   AGC	Support automatic soft button management.
*   11/12/14 1.9    AGC	Move Direct2D support macros to SPxWinD2DUtils.
*			SetDestination no longer requires factory.
*   24/11/14 1.8    SP 	Add setClipRectangle().
*   18/07/14 1.7    AGC	Add GetDeviceScale() function.
*   08/10/13 1.6    AGC	Remove createPopupMenu() function.
*   07/10/13 1.5    AGC	Support new forms for renderArc(),
*			renderSector() and renderSectorGradient().
*   27/09/13 1.4    AGC	Fix compilation errors.
*   27/09/13 1.3    AGC	Fix warning on VS2005.
*			Add support for ellipses and sectors.
*   18/09/13 1.2    AGC	Fix build errors on VS2005.
*   17/09/13 1.1    AGC	Initial version.
*
**********************************************************************/

#ifndef SPX_RENDERER_D2D_H
#define SPX_RENDERER_D2D_H

/*
 * Other headers required.
 */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxRenderer.h"
#include "SPxLibWin/SPxComPtr.h"
#include "SPxLibWin/SPxWinD2DUtils.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Need to forward-declare other classes in case headers 
 * are in wrong order. 
*/
class SPxRenderer;
class SPxSoftButton;
struct ID2D1Factory;
struct ID2D1RenderTarget;
struct ID2D1DeviceContext;
struct ID2D1Effect;
struct ID2D1SolidColorBrush;
struct ID2D1StrokeStyle;
struct IDWriteFactory;
struct IDWriteTextLayout;
struct IWICImagingFactory;
struct D2D_POINT_2F;
struct D2D_SIZE_F;
struct D2D_RECT_F;

/*
 * Define our class, derived from the base renderer object.
 */
class SPxRendererD2D : virtual public SPxRenderer
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxRendererD2D(void);
    virtual ~SPxRendererD2D(void);
    static int IsSupported(void);

    /* Set/get destination to render to */
    SPxErrorCode SetDestination(ID2D1RenderTarget *target);
    SPxErrorCode SetDestination(ID2D1DeviceContext *target);
    SPxErrorCode SetDestination(int target);
#ifdef SPX_SUPPORT_D2D
    ID2D1Factory *GetFactory(void) { return m_factory; }
    ID2D1RenderTarget *GetDestination(void) { return m_target; }
    IWICImagingFactory *GetImaging(void) { return m_wicFactory; }
#endif

    virtual SPxErrorCode SetTextRenderingMode(SPxTextRenderingMode_t mode);

    virtual int IsPerspectiveSupported(void) const;
    virtual float GetDeviceScale(void) const;

protected:
    /*
     * Protected functions.
     */

    /* The following rendering functions implement functions
     * defined in the base class.
     */
    virtual SPxErrorCode startRendering(void);
    virtual SPxErrorCode renderPoly(UINT32 colour,
                                    unsigned int numPoints,
                                    SPxRendererXY_t *points,
                                    unsigned int filled);
    virtual SPxErrorCode renderPoly(UINT32 colour,
                                    unsigned int numPoints,
                                    SPxRendererXYD_t *points,
                                    unsigned int filled);
    virtual SPxErrorCode renderPolyGradient(UINT32 colour[2],
                                            unsigned int numPoints,
                                            SPxRendererXY_t *points);
    virtual SPxErrorCode renderLine(UINT32 colour,
                                    SPxRendererXY_t *start,
                                    SPxRendererXY_t *end);
    virtual SPxErrorCode renderLine(UINT32 colour,
                                    SPxRendererXYD_t *start,
                                    SPxRendererXYD_t *end);
    virtual SPxErrorCode renderArc(UINT32 colour,
                                   SPxRendererXY_t *centre,
                                   unsigned int radiusX,
                                   unsigned int radiusY,
                                   double startDegs,
                                   double sweepDegs,
                                   unsigned int filled=FALSE);
    virtual SPxErrorCode renderArc(UINT32 colour,
                                   SPxRendererXYD_t *centre,
                                   double radiusX,
                                   double radiusY,
                                   double startDegs,
                                   double sweepDegs,
                                   unsigned int filled=FALSE);
    virtual SPxErrorCode renderEllipse(UINT32 colour,
                                       SPxRendererXY_t *topLeft,
                                       SPxRendererXY_t *bottomRight);
    virtual SPxErrorCode renderEllipse(UINT32 colour,
                                       SPxRendererXYD_t *topLeft,
                                       SPxRendererXYD_t *bottomRight);
    virtual SPxErrorCode renderPoint(UINT32 colour,
                                     SPxRendererXY_t *point);
    virtual SPxErrorCode renderText(UINT32 colour,
                                    SPxRendererXY_t *point,
                                    const char *text);
    virtual SPxErrorCode renderText(UINT32 colour,
                                    SPxRendererXYD_t *point,
                                    const char *text);
    virtual SPxErrorCode getTextExtents(const char *text,
                                        double *widthRtn,
                                        double *heightRtn);
    SPxErrorCode renderImage(UINT32 colour,
                             SPxImage *image,
                             SPxRendererXY_t *topLeft,
                             SPxRendererXY_t *bottomRight,
                             double rotDegs);
    virtual SPxErrorCode renderImage(UINT32 colour,
                                     SPxImage *image,
                                     const SPxRendererXY_t *topLeft,
                                     const SPxRendererXY_t *bottomRight,
                                     const SPxRendererXYD_t bitmapPoints[4]);
    virtual SPxErrorCode renderSector(UINT32 colour,
                                      SPxRendererXY_t *centre,
                                      unsigned int innerRadius,
                                      unsigned int outerRadius,
                                      double startDegs,
                                      double sweepDegs,
                                      unsigned int filled);
    virtual SPxErrorCode renderSector(UINT32 colour,
                                      SPxRendererXYD_t *centre,
                                      double innerRadius,
                                      double outerRadius,
                                      double startDegs,
                                      double sweepDegs,
                                      unsigned int filled);
    virtual SPxErrorCode renderSectorGradient(UINT32 colour[2],
                                              SPxRendererXY_t *centre,
                                              unsigned int innerRadius,
                                              unsigned int outerRadius,
                                              double startDegs,
                                              double sweepDegs);
    virtual SPxErrorCode setClipRectangle(SPxRendererXY_t *topLeft,
                                          SPxRendererXY_t *bottomRight);
    virtual SPxErrorCode endRendering(void);
 
    /* Protected functions. */
#ifdef SPX_SUPPORT_D2D
    D2D_POINT_2F pxToDips(D2D_POINT_2F point);
    D2D_SIZE_F pxToDips(D2D_SIZE_F size);
    D2D_RECT_F pxToDips(D2D_RECT_F rect);
#endif

private:
    /* Private variables. */
    
    /* Handle of the device context to use */
#ifdef SPX_SUPPORT_D2D
    struct impl;
    SPxAutoPtr<impl> m_p;
    SPxComPtr<IDWriteFactory> m_writeFactory;
    SPxComPtr<IWICImagingFactory> m_wicFactory;
    SPxComPtr<ID2D1Factory> m_factory;
    SPxComPtr<ID2D1RenderTarget> m_target;
#ifdef SPX_SUPPORT_D2D_1_1
    SPxComPtr<ID2D1DeviceContext> m_deviceContext;
#endif
#endif

    /*
     * Private functions.
     */
    virtual SPxSoftButton *newSoftButton(const char *bitmap,
                                         int width, int height,
                                         const char *moBitmap,
                                         int moWidth, int moHeight);
    virtual SPxErrorCode renderArcShort(UINT32 colour,
                                        SPxRendererXYD_t *centre,
                                        double radiusX,
                                        double radiusY,
                                        double startDegs,
                                        double sweepDegs,
                                        unsigned int filled);
    template<typename T>
    SPxErrorCode renderPolyT(UINT32 colour,
                             unsigned int numPoints,
                             T *points,
                             unsigned int filled);
    ID2D1SolidColorBrush *getBrush(UINT32 colour);
    ID2D1StrokeStyle *getStroke(void);
    IDWriteTextLayout *getTextLayout(const char *text);
    static void setSoftButtonDest(SPxSoftButton *btn, void *userArg);

}; /* SPxRendererD2D */

/*********************************************************************
*
* Functions
*
**********************************************************************/

#endif /* SPX_RENDERER_D2D_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
