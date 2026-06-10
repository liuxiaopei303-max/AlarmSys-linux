/*********************************************************************
*
* (c) Copyright 2013, 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxLibQt.h,v $
* ID: $Id: SPxLibQt.h,v 1.8 2015/04/22 14:36:35 rew Exp $
*
* Purpose:
*   Top-level header for SPxLibQt library.
*
*   This should be the only header that needs to be included in
*   code that uses this library.  It includes all the other public
*   headers for the library.
*
* Revision Control:
*   22/04/15 v1.8   JP 	Add SPxRasterMapRendererQt.
*
* Previous Changes:
*   08/04/15 1.7   JP 	Add SPxOwnShipRendererQt, SPxAISRendererQt
*			SPxExtRendererQt, SPxWorldMapRendererQt.
*   05/02/15 1.6   JP 	Add SPxPopupMenuQt.
*   26/01/15 1.5   JP 	Add Softbutton and mousecontrol.
*   07/01/15 1.4   JP 	Add renderers.
*   06/06/13 1.3   SP  	Add SPxQGLRadarVideoItem.
*			Add SPxQGLRadarWidget.
*			Fix Qt5 issues with QVariant.
*   03/05/13 1.2   SP  	Add SPxQRadarWidget.
*   25/04/13 1.1   SP 	Initial version.
*
**********************************************************************/

#ifndef _SPX_LIBQT_H_
#define	_SPX_LIBQT_H_

/* Including this after SPx.h, which defines bool and many other
 * common types, causes all sorts of problems. Therefore this header
 * is included first and remaining Qt headers are included after SPx.h.
 */
#include <QVariant>

/* We need the main SPx header first (for all the utility classes etc). */
#ifdef _WIN32
#include "SPxNoMFC.h"
#else
#include "SPx.h"
#endif

/* Include all the public headers from this library, assuming they
 * are in a subdirectory of one of the directories in the include
 * path (i.e. so you don't need to add each subdirectory to the include
 * path, just the parent of them all).
 */
#include "SPxLibQt/SPxAISRendererQt.h"
#include "SPxLibQt/SPxButtonBoxQt.h"
#include "SPxLibQt/SPxExtRendererQt.h"
#include "SPxLibQt/SPxGraphicsRendererQt.h"
#include "SPxLibQt/SPxMouseControlQt.h"
#include "SPxLibQt/SPxOwnShipRendererQt.h"
#include "SPxLibQt/SPxPopupMenuQt.h"
#include "./SPxLibQt/SPxQGLRadarWidget.h"
#include "./SPxLibQt/SPxQRadarWidget.h"
#include "./SPxLibQt/SPxQGLRadarVideoItem.h"
#include "./SPxLibQt/SPxQRadarVideoItem.h"
#include "./SPxLibQt/SPxRasterMapRendererQt.h"
#include "./SPxLibQt/SPxRendererQt.h"
#include "./SPxLibQt/SPxSoftButtonQt.h"
#include "./SPxLibQt/SPxTiledMapRendererQt.h"
#include "./SPxLibQt/SPxTrackRendererQt.h"
#include "./SPxLibQt/SPxViewControlRendererQt.h"
#include "./SPxLibQt/SPxWorldMapRendererQt.h"
#include "./SPxLibQt/SPxPlotRendererQt.h"//add
#include "./SPxLibQt/SPxMHTtrackRendererQT.h"//add
#include "./SPxLibQt/SPxADSBRendererQt.h"//add

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_LIBQT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
