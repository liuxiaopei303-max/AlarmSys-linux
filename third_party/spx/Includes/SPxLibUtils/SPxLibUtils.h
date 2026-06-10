/*********************************************************************
*
* (c) Copyright 2007 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxLibUtils.h,v $
* ID: $Id: SPxLibUtils.h,v 1.74 2017/08/22 14:24:01 rew Exp $
*
* Purpose:
*   Top-level header for SPxLibUtils library.
*
*   This should be the only header that needs to be included in
*   code that uses this library.  It includes all the other public
*   headers for the library, specifying the full (relative) pathname
*   to the headers, i.e. it expects the headers to be in an SPxLibData
*   subdirectory.
*
* Revision Control:
*   22/08/17 v1.74   SP 	Add SPxRefPoint.h and SPxRefPointRenderer.h.
*
* Previous Changes:
*   31/10/16 1.73   AGC	Add SPxCameraDatabase.h.
*   23/09/16 1.72   REW	Add SPxLogFile.h.
*   15/06/16 1.71   AGC	Add SPxTiledMapSrcCoverage.h.
*   20/04/16 1.70   SP 	Move SPxSQLDatabase.h to SPxLibData.h.
*   08/04/16 1.69   SP 	Add SPxSQLDatabase.h
*   22/12/15 1.68   REW	Add SPxAccessControl.h
*   16/11/15 1.67   SP 	Add SPxTimelineSession.h.
*   13/11/15 1.66   REW	Add SPxPlotRenderer.
*   27/08/15 1.65   AGC	Add SPxScreenshot.
*   15/07/15 1.64   SP 	Add SPxGate.
*   29/06/15 1.63   AGC	Add SPxJoystickCamCtrl.
*   14/04/15 1.62   AGC	Add SPxPIDFilter.
*   16/02/15 1.61   SP 	Add SPxTimelineRecorder and 
*                       SPxTimelineReplay.
*   07/01/15 1.60   SP 	Add SPxRecordRenderer.
*   21/11/14 1.59   SP 	Add SPxTimelineRenderer 
*                       Add SPxTimelineDatabase.
*   06/11/14 1.58   SP 	Remove SPxEventTemplate headers.
*                       Add SPxStamp headers.
*   14/10/14 1.57   SP 	Move SPxChannel.h and SPxChannelDatabase.h 
*                       to SPxLibData.
*   24/09/14 1.56   SP 	Add SPxChannel.h and SPxChannelDatabase.h.
*                       Add SPxEventTemplate.h and SPxEventTemplateDatabase.h.
*   04/07/14 1.55   AGC	Add SPxInstall.h.
*   01/05/14 1.54   AGC	Add SPxCircularBuffer.h.
*   10/03/14 1.53   AGC	Add SPxAutoFile.h.
*   02/12/13 1.52   SP 	Add SPxSound.h.
*   26/11/13 1.51   AGC	Add SPxSharedPtr.h.
*			Add SPxAtomic.h.
*   22/11/13 1.50   AGC	Add SPxSize.h, SPxPos.h, SPxArea.h, and
*			SPxFraction.h from AV library.
*   15/11/13 1.49   SP 	Add SPxFileServer.h.
*   11/11/13 1.48   AGC	Remove SPxUndo.h to prevent warning from SPxLibDN.
*   04/11/13 1.47   SP 	Add SPxAlertRenderer.h
*   07/10/13 1.46   AGC	Add SPxAreaRenderer.h.
*			Add SPxExtRenderer.h.
*			Add SPxGw...h from SPxLibX11.
*			Add SPxUndo.h.
*   04/10/13 1.45   AGC	Move functions to SPxUtilsInit.h.
*   14/08/13 1.44   REW	Add SPxCamera.h
*   30/07/13 1.43   AGC	Add SPxWideString.h.
*   26/04/13 1.42   SP 	Omit OpenGL files on unsupported platforms.
*   25/04/13 1.41   SP 	Add SPxGL.h.
*   08/04/13 1.40   AGC	Add SPxTiledMapSrcContour.h.
*   21/12/12 1.39   AGC	Add SPxPopupMenu.h.
*			Add SPxObjPtr.h.
*			Add SPxTrackRendererBase.h.
*   23/11/12 1.38   AGC	Add SPxMouseControl.h.
*			Add SPxLicGetFrameworkStatus().
*   09/11/12 1.37   AGC	Add SPxTiledMapSrc.h.
*			Add SPxTiledMapSrcTerrain.h.
*			Add SPxViewControlRenderer.h.
*			Remove SPxTerrainRenderer.h.
*   26/10/12 1.36   AGC	Add SPxTerrainDatabase.h.
*			Add SPxTerrainRenderer.h.
*   21/08/12 1.35   REW	Add SPxTiledMap.h for SP.
*   02/08/12 1.34   AGC	Add SPxByteStream.h.
*   06/06/12 1.33   SP 	Add SPxTiledMapDatabase.h.
*			Add SPxTiledMapRenderer.h
*   19/04/12 1.32   AGC	Add SPxAutoPtr.h.
*   29/03/12 1.31   AGC	Add SPxAutoLock.h.
*			Add SPxMatrix.h.
*			Add SPxSema.h.
*   20/02/12 1.30   SP 	Add SPxLatLongUtils.h.
*   08/02/12 1.29   SP 	Add SPxViewControl.h.
*			Add SPxOwnShipRenderer.h
*			Add SPxGraphicsRenderer.h
*			Add SPxAreaRenderer.h
*			Add SPxWorldMapRenderer.h
*   14/11/11 1.28   AGC	Add SPxImage.h.
*   09/11/11 1.27   REW	Add SPxLicIs...Licensed() functions.
*   22/09/11 1.26   AGC	Add SPxMaths.h.
*   19/08/11 1.25   SP 	Add SPxSampleUtils.h.
*   27/07/11 1.24   AGC	Add SPxTimer.h.
*   07/07/11 1.23   AGC	Add SPxColConvert.h.
*   27/06/11 1.22   AGC	Add SPxAutoMutex.h, SPxRateCounter.h.
*   26/04/11 1.21   SP 	Add SPxSymbol.h.
*   06/04/11 1.20   SP 	Add SPxRenderer.h
*   24/01/11 1.19   AGC	Add SPxLogTime.h
*   22/12/10 1.18   AGC	Add silent parameter to SPxLicGetID().
*   08/11/10 1.17   AGC	Add SPxLegacyParams.h and SPxSysUtils.h
*   05/11/10 1.16   REW	Add SPxWorldMap.h
*   02/10/09 1.15   SP 	Add SPxLatLongUTM.h
*   09/02/09 1.14   REW	Add SPxSystemHealth.h
*   10/07/08 1.13   REW	Add SPxLicGetID().
*   25/03/08 1.12   DGJ	Add SPxVisualServer.h
*   30/01/08 1.11   REW	Add SPxLicReport().
*   29/01/08 1.10   REW	Add SPxLicInit().
*   07/01/08 1.9    REW	Add SPxCallbackList.h
*   25/10/07 1.8    REW	SPxStringComp.h renamed to SPxStrings.h
*   12/10/07 1.7    DGJ	Add SPxStringComp.h
*   03/09/07 1.6    REW	Add SPxMsgRpt.h header
*   09/07/07 1.5    REW	Args to SPxLibUtilsInit().
*   29/06/07 1.4    DGJ	Add SPxObj.h include
*   08/06/07 1.3    REW	Add SPxByteSwap.h header.
*   07/06/07 1.2    REW	Headers are included from subdirectory.
*   24/05/07 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_LIBUTILS_H_
#define	_SPX_LIBUTILS_H_

/* Include all the public headers from this library, assuming they
 * are in a subdirectory of one of the directories in the include
 * path (i.e. so you don't need to add each subdirectory to the include
 * path, just the parent of them all).
 */
#include "SPxLibUtils/SPxUtilsInit.h"
#include "SPxLibUtils/SPxAccessControl.h"
#include "SPxLibUtils/SPxAlertRenderer.h"
#include "SPxLibUtils/SPxArea.h"
#include "SPxLibUtils/SPxAreaRenderer.h"
#include "SPxLibUtils/SPxAtomic.h"
#include "SPxLibUtils/SPxAutoFile.h"
#include "SPxLibUtils/SPxAutoLock.h"
#include "SPxLibUtils/SPxAutoMutex.h"
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxByteStream.h"
#include "SPxLibUtils/SPxByteSwap.h"
#include "SPxLibUtils/SPxCallbackList.h"
#include "SPxLibUtils/SPxCamera.h"
#include "SPxLibUtils/SPxCameraDatabase.h"
#include "SPxLibUtils/SPxCircularBuffer.h"
#include "SPxLibUtils/SPxColConvert.h"
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxEvents.h"
#include "SPxLibUtils/SPxExtRenderer.h"
#include "SPxLibUtils/SPxFileServer.h"
#include "SPxLibUtils/SPxFraction.h"
#include "SPxLibUtils/SPxGate.h"
#include "SPxLibUtils/SPxGraphicsRenderer.h"
#include "SPxLibUtils/SPxGwArc.h"
#include "SPxLibUtils/SPxGwBitmap.h"
#include "SPxLibUtils/SPxGwImage.h"
#include "SPxLibUtils/SPxGwImageBuffer.h"
#include "SPxLibUtils/SPxGwLine.h"
#include "SPxLibUtils/SPxGwObj.h"
#include "SPxLibUtils/SPxGwPicture.h"
#include "SPxLibUtils/SPxGwPoly.h"
#include "SPxLibUtils/SPxGwRect.h"
#include "SPxLibUtils/SPxGwSector.h"
#include "SPxLibUtils/SPxGwShape.h"
#include "SPxLibUtils/SPxGwSymbol.h"
#include "SPxLibUtils/SPxGwText.h"
#include "SPxLibUtils/SPxGwWidget.h"
#include "SPxLibUtils/SPxGwWidgetGroup.h"
//#include "SPxLibUtils/SPxImage.h"
#include "SPxLibUtils/SPxInstall.h"
#include "SPxLibUtils/SPxJoystickCamCtrl.h"
#include "SPxLibUtils/SPxLatLongUTM.h"
#include "SPxLibUtils/SPxLatLongUtils.h"
#include "SPxLibUtils/SPxLegacyParams.h"
#include "SPxLibUtils/SPxLogFile.h"
#include "SPxLibUtils/SPxLogTime.h"
#include "SPxLibUtils/SPxMaths.h"
#include "SPxLibUtils/SPxMatrix.h"
#include "SPxLibUtils/SPxMouseControl.h"
#include "SPxLibUtils/SPxMsgRpt.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxObjPtr.h"
#include "SPxLibUtils/SPxOwnShipRenderer.h"
#include "SPxLibUtils/SPxParams.h"
#include "SPxLibUtils/SPxPIDFilter.h"
#include "SPxLibUtils/SPxPlotRenderer.h"
#include "SPxLibUtils/SPxPopupMenu.h"
#include "SPxLibUtils/SPxPos.h"
#include "SPxLibUtils/SPxRateCounter.h"
#include "SPxLibUtils/SPxRecordRenderer.h"
#include "SPxLibUtils/SPxRefPoint.h"
#include "SPxLibUtils/SPxRefPointRenderer.h"
#include "SPxLibUtils/SPxTimelineRecorder.h"
#include "SPxLibUtils/SPxRenderer.h"
#include "SPxLibUtils/SPxTimelineReplay.h"
#include "SPxLibUtils/SPxSampleUtils.h"
#include "SPxLibUtils/SPxScreenshot.h"
#include "SPxLibUtils/SPxSema.h"
#include "SPxLibUtils/SPxSharedPtr.h"
#include "SPxLibUtils/SPxSize.h"
#include "SPxLibUtils/SPxSound.h"
#include "SPxLibUtils/SPxStamp.h"
#include "SPxLibUtils/SPxStampDatabase.h"
#include "SPxLibUtils/SPxStrings.h"
#include "SPxLibUtils/SPxSymbol.h"
#include "SPxLibUtils/SPxSysUtils.h"
#include "SPxLibUtils/SPxSystemHealth.h"
#include "SPxLibUtils/SPxTerrainDatabase.h"
#include "SPxLibUtils/SPxThreads.h"
#include "SPxLibUtils/SPxTiledMap.h"
#include "SPxLibUtils/SPxTiledMapDatabase.h"
#include "SPxLibUtils/SPxTiledMapRenderer.h"
#include "SPxLibUtils/SPxTiledMapSrc.h"
#include "SPxLibUtils/SPxTiledMapSrcContour.h"
#include "SPxLibUtils/SPxTiledMapSrcCoverage.h"
#include "SPxLibUtils/SPxTiledMapSrcTerrain.h"
#include "SPxLibUtils/SPxTime.h"
#include "SPxLibUtils/SPxTimelineDatabase.h"
#include "SPxLibUtils/SPxTimelineRenderer.h"
#include "SPxLibUtils/SPxTimelineSession.h"
#include "SPxLibUtils/SPxTimer.h"
#include "SPxLibUtils/SPxTrackRendererBase.h"
#include "SPxLibUtils/SPxViewControl.h"
#include "SPxLibUtils/SPxViewControlRenderer.h"
#include "SPxLibUtils/SPxVisualServer.h"
#include "SPxLibUtils/SPxWideString.h"
#include "SPxLibUtils/SPxWorldMap.h"
#include "SPxLibUtils/SPxWorldMapRenderer.h"

/* Omit these files on platforms that do not support OpenGL. */
#if !defined(__sun) && !defined(__arm) && !defined(__QNX__)
#include "SPxLibUtils/SPxGL.h"
#endif

#endif /* _SPX_LIBUTILS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
