/*********************************************************************
*
* (c) Copyright 2007 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxLibWin.h,v $
* ID: $Id: SPxLibWin.h,v 1.76 2017/08/22 14:24:38 rew Exp $
*
* Purpose:
*   Top-level header for SPxLibWin library.
*
*   This should be the only header that needs to be included in
*   code that uses this library.  It includes all the other public
*   headers for the library.
*
* Revision Control:
*   22/08/17 v1.76   SP 	Add SPxRefPointRendererWin.h
*
* Previous Changes:
*   03/08/17 1.75   SP 	Add SPxJoystickWin.h.
*   29/06/17 1.74   SP 	Add SPxWinCheckboxTreeCtrl.h.
*   12/04/17 1.73   AGC	Add SPxComInit.h.
*			    Add SPxComPtr.h.
*   17/11/16 1.72   AGC	Add SPxWinCustomDlg.h.
*   16/09/16 1.71   SP 	Add SPxSystemTrayDialog.h.
*   14/07/16 1.70   SP 	Add SPxSQLBuilderDlgWin.h.
*   14/07/16 1.69   AGC	Add SPxWinRestartApp.h.
*   15/06/16 1.68   AGC	Add SPxCoverageDlgWin.h.
*   29/04/16 1.67   SP 	Add SPxLanguageDlgWin.h.
*   08/04/16 1.66   SP 	Add SPxSQLQueryDlgWin.h.
*   30/11/15 1.65   REW	Add SPxTextEntryDlgWin.h and Popup.
*   26/11/15 1.64   AJH	Add SPxP172EmuDlgWin.h
*   13/11/15 1.63   REW	Add SPxPlotRendererWin.h
*   09/11/15 1.62   AGC	Add SPxTableDlgWin.h
*   06/11/15 1.61   REW	Add SPxPimCtrlDlgWin.h and Popup.
*   22/07/15 1.60   REW	Add SPxAboutDlgWin.h
*   14/04/15 1.59   AGC	Add SPxParamDlgWin.h
*   26/03/15 1.58   SP 	Add SPxSerialCtrlDlgWin.
*   06/03/15 1.57   AGC	Add SPxProcCtrlPopupDlgWin.h
*   06/03/15 1.56   SP 	Add SPxLevelIndicatorWin.h
*   05/02/15 1.55   AGC	Add SPxWinD2DUtils.h
*   09/01/15 1.54   REW	Add SPxSrcNetCtrlDlgWin.h for AGC.
*   07/01/15 1.53   SP 	Add SPxRecordRendererWin.h
*   21/11/14 1.52   SP 	Add SPxTimelineRendererWin.h
*   22/09/14 1.51   REW	Add SPxSafetyNetRendererWin.h
*   20/06/14 1.50   AGC	Add SPxWinFileDialog.h.
*			Add SPxWinSplashDlg.h.
*   23/04/14 1.49   AGC	Add SPxWinMsgBox.h.
*			Add SPxWinTaskbarButton.h.
*   26/03/14 1.48   AGC	Add SPxProcCtrlDlgWin.h.
*   17/02/14 1.47   AGC	Add SPxCamCtrlDlgWin and SPxCamCtrlPopupDlgWin.
*			Add SPxCamCtrlAdvDlgWin.
*   21/01/14 1.46   SP 	Add SPxSeparatorWin.h.
*   15/01/14 1.45   SP 	Add SPxWinSliderCtrl.h.
*   15/01/14 1.44   REW	Add SPxFlightPlan dialog files.
*   22/11/13 1.43   AGC	Add SPxNetCtrlDlgWin.h.
*   04/11/13 1.42   SP 	Add SPxAlertRendererWin.h.
*                       Add SPxTiledMapDlgWin.h.
*                       Add SPxTiledMapPopupDlgWin.h.
*   21/10/13 1.41   AGC	Add SPxConfigObjDlgWin.h.
*			Add SPxHealthDlgWin.h.
*			Add SPxRendererDlgWin.h.
*			Add SPxServerDlgWin.h.
*			Add SPxTableLayoutWin.h.
*   07/10/13 1.40   AGC	Add SPxAreaEditorDlgWin.h.
*			Add SPxExtRendererWin.h.
*			Add SPx...RendererD2D.h.
*			Sort headers alphabetically.
*   04/10/13 1.39   AGC	Move functions to SPxWinInit.h.
*   05/09/13 1.38   SP 	Add SPxRasterMapRendererWin.h.
*   14/08/13 1.37   SP 	Add SPxThreatRendererWin.h.
*   01/08/13 1.36   SP 	Add SPxWinMenu.h.
*   25/07/13 1.35   AGC	Add SPxWinImageCtrl.h.
*   25/07/13 1.34   REW	Add SPxROCCtrlDlgWin and SPxROCCtrlPopupDlgWin.
*   12/12/12 1.33   REW	Add SPxP226CtrlDlgWin.h
*   23/11/12 1.32   AGC	Add SPxMouseControlWin.h.
*   09/11/12 1.31   AGC	Add SPxViewControlRendererWin.h.
*   26/10/12 1.30   AGC	Move MFC utilities to SPxWinMfcUtils.h.
*			Add SPxSim... re-usable dialogs.
*			Add SPxSrcFile... re-usable dialogs.
*			Add SPxTestGen... re-usable dialogs.
*   18/10/12 1.29   SP 	Add SPxP172Rc... headers.
*                       Add new combobox utility functions.
*                       Add SPxSetWindowText().
*   21/06/12 1.28   SP 	Add SPxTiledMapRendererWin.h.
*   15/05/12 1.27   REW	Move utils to SPxWin32Utils.h
*   04/04/12 1.26   AGC	Add SPxAutoDC.
*   29/03/12 1.25   AGC	Add SPxScenGenCtrlPopupDlgWin.
*			Add SPxScDestDisplayMixer.
*   21/02/12 1.24   REW	Re-include ENC headers.
*   13/02/12 1.23   REW	Comment out ENC headers until dependencies fixed.
*   08/02/12 1.22   SP 	Add SPxENCManagerWin and SPxENCRenderWin.
*			Add SPxOwnShipRendererWin.
*			Add SPxAreaRendererWin.
*			Add SPxWorldMapRendererWin.
*			Add SPxDDX_Slider.
*   13/01/12 1.21   AGC	Add SPxScDestDisplayWinRaw and SPxScFollowWinRaw.
*   23/12/11 1.20   REW	Add SPxADSBRendererWin.h
*   25/10/11 1.19   AGC	Add SPxScenGenCtrlDlgWin.h.
*   07/07/11 1.18   SP 	Add SPxGetMaxScreenSize().
*   26/05/11 1.17   SP 	Add SPX_OMIT_MFC protection around recent code.
*   25/05/11 1.16   SP 	Add SPxHPxAdvCtrlDlgWin.h and
*			SPxHPxAdvCtrlPopupDlgWin.h.
*			Add MFC utils prototypes.
*   12/05/11 1.15   SP 	Add SPxHPxCtrlDlgWin.h and
*                       SPxHPxCtrlPopupDlgWin.h.
*   26/04/11 1.14   SP 	Add SPxGraphicsRendererWin.h
*   11/03/11 1.13   SP 	Add SPxTrackRendererWin.h
*   02/03/11 1.12   REW	Add SPxAISRendererWin.h
*   17/12/10 1.11   AGC	Rename SPxWinToggleBtn to SPxWinButton.
*   13/12/10 1.10   AGC	Add SPxWinToggleBtn.
*   08/12/10 1.9    AGC	Add SPxWinSpinCtrl, SPxWinEditCtrl and 
*			SPxWinNumEditCtrl.
*   30/09/10 1.8    SP 	Include SPxBitmapWin if SPX_OMIT_MFC.
*   21/09/10 1.7    SP 	Add SPxFlushWinMsgQueue()
*   28/07/10 1.6    SP 	Add SPxBitmapWin.h, SPxGraphicsWindow.h 
*			and SPxRadarWindow.h
*   23/04/10 1.5    SP 	Add SPxIsWin64Bit global.
*   09/07/07 1.4    REW	Args to SPxLibWinInit().
*   19/06/07 1.3    DGJ	DestRender now DestDisplay.
*   13/06/07 1.2    DGJ	DestDisplay now DestRender, ScGeom now ScFollow
*   24/05/07 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_LIBWIN_H_
#define	_SPX_LIBWIN_H_

/* Include the public headers from this library. */

/* MFC based classes */
#ifndef SPX_OMIT_MFC
#include "SPxAboutDlgWin.h"
#include "SPxADSBRendererWin.h"
#include "SPxAISRendererWin.h"
#include "SPxAlertRendererWin.h"
#include "SPxAreaEditorDlgWin.h"
#include "SPxAreaRendererWin.h"
#include "SPxCamCtrlAdvDlgWin.h"
#include "SPxCamCtrlDlgWin.h"
#include "SPxCamCtrlPopupDlgWin.h"
#include "SPxConfigObjDlgWin.h"
#include "SPxCoverageDlgWin.h"
#include "SPxENCManagerWin.h"
#include "SPxENCRendererWin.h"
#include "SPxExtRendererD2D.h"
#include "SPxExtRendererWin.h"
#include "SPxFlightPlanDbDlgWin.h"
#include "SPxFlightPlanDbPopupDlgWin.h"
#include "SPxFlightPlanDlgWin.h"
#include "SPxFlightPlanPopupDlgWin.h"
#include "SPxGraphicsRendererD2D.h"
#include "SPxGraphicsRendererWin.h"
#include "SPxGraphicsWindow.h"
#include "SPxHealthDlgWin.h"
#include "SPxHPxAdvCtrlDlgWin.h"
#include "SPxHPxAdvCtrlPopupDlgWin.h"
#include "SPxHPxCtrlDlgWin.h"
#include "SPxHPxCtrlPopupDlgWin.h"
#include "SPxJoystickWin.h"
#include "SPxLanguageDlgWin.h"
#include "SPxLevelIndicatorWin.h"
#include "SPxMouseControlWin.h"
#include "SPxNetCtrlDlgWin.h"
#include "SPxOwnShipRendererD2D.h"
#include "SPxOwnShipRendererWin.h"
#include "SPxParamDlgWin.h"
#include "SPxP172EmuDlgWin.h"
#include "SPxP172RcDlgWin.h"
#include "SPxP172RcInterfaceWin.h"
#include "SPxP172RcPopupDlgWin.h"
#include "SPxP172RcTransportWin.h"
#include "SPxP226CtrlDlgWin.h"
#include "SPxPimCtrlDlgWin.h"
#include "SPxPimCtrlPopupDlgWin.h"
#include "SPxPlotRendererWin.h"
#include "SPxProcCtrlDlgWin.h"
#include "SPxProcCtrlPopupDlgWin.h"
#include "SPxROCCtrlDlgWin.h"
#include "SPxROCCtrlPopupDlgWin.h"
#include "SPxRadarWindow.h"
#include "SPxRasterMapRendererWin.h"
#include "SPxRecordRendererWin.h"
#include "SPxRendererDlgWin.h"
#include "SPxSafetyNetRendererWin.h"
#include "SPxScDestBitmapD2D.h"
#include "SPxScDestDisplayWin.h"
#include "SPxScFollowWin.h"
#include "SPxScenGenCtrlDlgWin.h"
#include "SPxScenGenCtrlPopupDlgWin.h"
#include "SPxSerialCtrlDlgWin.h"
#include "SPxServerDlgWin.h"
#include "SPxSeparatorWin.h"
#include "SPxSimMotionCtrlDlgWin.h"
#include "SPxSimMotionCtrlPopupDlgWin.h"
#include "SPxSimMotionNewDlgWin.h"
#include "SPxSimMotionRendererD2D.h"
#include "SPxSimMotionRendererWin.h"
#include "SPxSimRadarCtrlDlgWin.h"
#include "SPxSimRadarCtrlPopupDlgWin.h"
#include "SPxSimTargetCtrlDlgWin.h"
#include "SPxSimTargetCtrlPopupDlgWin.h"
#include "SPxSimTerrainCtrlDlgWin.h"
#include "SPxSimTerrainCtrlPopupDlgWin.h"
#include "SPxSQLBuilderDlgWin.h"
#include "SPxSQLQueryDlgWin.h"
#include "SPxSrcFileCtrlDlgWin.h"
#include "SPxSrcFileCtrlPopupDlgWin.h"
#include "SPxSrcNetCtrlDlgWin.h"
#include "SPxSystemTrayDialog.h"
#include "SPxTableDlgWin.h"
#include "SPxTableLayoutWin.h"
#include "SPxTestGenCtrlDlgWin.h"
#include "SPxTestGenCtrlPopupDlgWin.h"
#include "SPxTextEntryDlgWin.h"
#include "SPxTextEntryPopupDlgWin.h"
#include "SPxThreatRendererWin.h"
#include "SPxTiledMapRendererD2D.h"
#include "SPxTiledMapRendererWin.h"
#include "SPxTiledMapDlgWin.h"
#include "SPxTiledMapPopupDlgWin.h"
#include "SPxTimelineRendererWin.h"
#include "SPxTrackRendererD2D.h"
#include "SPxTrackRendererWin.h"
#include "SPxRefPointRendererWin.h"
#include "SPxViewControlRendererD2D.h"
#include "SPxViewControlRendererWin.h"
#include "SPxWinAscanWindow.h"
#include "SPxWinButton.h"
#include "SPxWinCheckboxTreeCtrl.h"
#include "SPxWinCustomDlg.h"
#include "SPxWinEditCtrl.h"
#include "SPxWinFileDialog.h"
#include "SPxWinImageCtrl.h"
#include "SPxWinMenu.h"
#include "SPxWinMsgBox.h"
#include "SPxWinNumEditCtrl.h"
#include "SPxWinSliderCtrl.h"
#include "SPxWinSpinCtrl.h"
#include "SPxWinSplashDlg.h"
#include "SPxWinTaskbarButton.h"
#include "SPxWorldMapRendererD2D.h"
#include "SPxWorldMapRendererWin.h"
#endif

/* Non-MFC classes */
#include "SPxAutoDC.h"
#include "SPxBitmapWin.h"
#include "SPxComInit.h"
#include "SPxComPtr.h"
#include "SPxScDestDisplayWinRaw.h"
#include "SPxScDestDisplayMixer.h"
#include "SPxScFollowWinRaw.h"
#include "SPxWin32Utils.h"
#include "SPxWinD2DUtils.h"
#include "SPxWinInit.h"
#include "SPxWinRestartApp.h"

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#ifndef SPX_OMIT_MFC

#include "SPxWinMfcUtils.h"

#endif /* NOT SPX_OMIT_MFC */

/*********************************************************************
*
*   Public Variables
*
**********************************************************************/

#endif /* _SPX_LIBWIN_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
