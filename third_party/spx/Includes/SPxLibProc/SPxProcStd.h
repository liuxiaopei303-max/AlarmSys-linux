/*********************************************************************
*
* (c) Copyright 2007 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxProcStd.h,v $
* ID: $Id: SPxProcStd.h,v 1.55 2017/07/05 13:32:57 rew Exp $
*
* Purpose:
*   Header for Standard Processes.
*
* Revision Control:
*   05/07/17 v1.56   REW	Add SPxProOSD.
*
* Previous Changes:
*   26/05/17 1.55   AGC	Allow explicit control of SPxProRecordReturn() time.
*   08/03/17 1.54   AGC	Avoid recording of errors causing license errors.
*   11/01/17 1.53   SP 	Pass run process to SPxProRecordUpdateInternals().
*   23/12/16 1.52   REW	Add SPxProRangeAdjust.
*   17/11/16 1.51   SP 	Add optional nav data arg to 
*			SPxProRecordUseNavData().
*   03/03/16 1.50   DGJ	Add SPxProMBT Process.
*   03/11/15 1.49   AGC	SPxProRecordPacketB() now takes void* payload.
*   26/03/15 1.48   AGC	Add overload of SPxProcRecordPacketB().
*   16/02/15 1.47   SP 	Add optional channel index argument to
*                       SPxProRecordPacketB() and  
*                       SPxProRecordReturn().
*   23/10/14 1.46   JP 	Add declaration of function to calculate
*                       the variance.
*   14/10/14 1.45   JP 	Add SPxProIntensityHistogram.
*   14/10/14 1.44   SP 	Add SPxProRecordReturn().
*   19/08/14 1.43   REW	Add SPxProProjection.
*   26/06/14 1.42   SP 	Add SPxProStats.
*   28/06/13 1.41   REW	Add SPxProROC.
*   22/02/13 1.40   REW	Add SPxProMonitor.
*   15/01/13 1.39   REW	Remove old/unused experimental code.
*   09/01/13 1.38   REW	Add SPxProBoost.
*   19/12/12 1.37   REW	Add SPxProGainOffset.
*   28/09/12 1.36   REW	Add SPxProIntegrateLoadFromFileNow().
*   02/05/12 1.35   REW	Add SPxProRecordUpdateInternals().
*   09/02/12 1.34   DGJ	Rename clutter removal to noise removal
*   01/02/12 1.33   DGJ	Add AdaptiveClutter process
*   04/10/11 1.32   REW	Add SPxProIntegrateLoadFromFile/SaveToFile().
*   18/08/11 1.31   REW	Add SPxProAziSpokes.
*   08/07/11 1.30   REW	Add SPxProRecordPacketB().
*   03/12/10 1.29   REW	Add SPxProRecordProbeLicense().
*   23/06/10 1.28   REW	Add SPxProSSI64.
*   03/11/09 1.27   SP 	Add SPxProDebug.
*   01/05/09 1.26   REW	Add LMM process
*   22/03/09 1.25   DGJ	Add LMB process
*   11/03/09 1.24   DGJ	Add SPxCalculateAverageIntoWorkspace2()
*   26/02/09 1.23   REW	Add SPxProRecordUseNavData().
*   17/10/08 1.22   REW	Add SPxProIntegrateHoldArea().
*   25/09/08 1.21   DGJ	Now support A scan under X11
*   23/06/08 1.20   DGJ	Add SPxProMHT
*   16/05/08 1.19   REW	Add SPxProBlobMerge
*   07/02/08 1.18   REW	Add SPxProHistory.
*   07/01/08 1.17   DGJ	Add SPxProVisualData and SPxProVisualInfo
*   04/01/08 1.16   REW	Add SPxProTracker.  Reorder alphabetically.
*   10/12/07 1.15   REW	Add SPxProRecord.
*   08/11/07 1.14   REW	Add SPxProRangeRings.
*   22/10/07 1.13   REW	Add SPxProSyncCombine and SPxProLUT.
*   08/10/07 1.12   REW	Add SPxProSpanDetect.
*   30/09/07 1.11   DGJ	New process API.  Add Ascan process for windows.
*   17/09/07 1.10   DGJ	Added SPxProAreaSelect, SPxProSSI, SPxProPimCopy
*   30/08/07 1.9    DGJ	Added SPxProToRIB, Integrate and Combine.
*   08/08/07 1.8    DGJ	Add SPxProRemap and matching Init().
*   25/07/07 1.7    REW	Add SPxProNetworkSend and matching Init().
*   27/06/07 1.6    DGJ	Add SPxProFilterNotch.
*   12/06/07 1.5    REW	Need more process headers.
*   11/06/07 1.4    REW	Add SPxProScanConv.
*   07/06/07 1.3    REW	Processes broken out to separate files.
*   24/05/07 1.2    REW	Added header, changed to SPx etc.
*   20/05/07 1.1    DGJ	Initial Version
**********************************************************************/

#ifndef _SPX_PROCSTD_H
#define _SPX_PROCSTD_H

/*
 * Other headers required.
 */
/* We need process manager support. */
#include "SPxLibData/SPxProcParam.h"
#include "SPxLibData/SPxProcess.h"
#include "SPxLibData/SPxProcMan.h"

/* Forward declarations. */
struct SPxPacketHeaderB_tag;
class SPxNavData;

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/*********************************************************************
*
*   Macros
*
**********************************************************************/

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/*********************************************************************
*
*   Variables
*
**********************************************************************/

/* Extern the handles of the well-known "standard" processes.
 * Will be NULL until initialised with one of the functions below.
 */
extern SPxProcess *SPxProAreaSelect;
extern SPxProcess *SPxProAverage;
extern SPxProcess *SPxProAziSpokes;
extern SPxProcess *SPxProBlobMerge;
extern SPxProcess *SPxProBoost;
extern SPxProcess *SPxProCombine;
extern SPxProcess *SPxProDebug;
extern SPxProcess *SPxProDynamicThreshold;
extern SPxProcess *SPxProFilterNotch;
extern SPxProcess *SPxProFTC;
extern SPxProcess *SPxProGainOffset;
extern SPxProcess *SPxProHistory;
extern SPxProcess *SPxProIntegrate;
extern SPxProcess *SPxProIntensityHistogram;
extern SPxProcess *SPxProLMB;
extern SPxProcess *SPxProLMM;
extern SPxProcess *SPxProLUT;
extern SPxProcess *SPxProMBT;
extern void SPxProMBTInit(void);
extern SPxProcess *SPxProMHT;
extern void SPxProMHTInit(void);
extern SPxProcess *SPxProMonitor;
extern SPxProcess *SPxProNull;
extern SPxProcess *SPxProOSD;
extern SPxProcess *SPxProPimCopy;
extern SPxProcess *SPxProProjection;
extern SPxProcess *SPxProRangeAdjust;
extern SPxProcess *SPxProRangeBlank;
extern SPxProcess *SPxProRangeRings;
extern SPxProcess *SPxProRemap;
extern SPxProcess *SPxProROC;
extern SPxProcess *SPxProScanConv;
extern SPxProcess *SPxProSectorBlank;
extern SPxProcess *SPxProSpanDetect;
extern SPxProcess *SPxProSSI;
extern SPxProcess *SPxProSSI64;
extern SPxProcess *SPxProSTC;
extern SPxProcess *SPxProStats;
extern SPxProcess *SPxProSyncCombine;
extern SPxProcess *SPxProThreshold;
extern SPxProcess *SPxProToRIB;
extern SPxProcess *SPxProTracker;
extern SPxProcess *SPxProVisualData;
extern SPxProcess *SPxProVisualInfo;
extern SPxProcess *SPxProAScan;
extern SPxProcess *SPxProAScanAux;
extern SPxProcess *SPxProAdaptiveNoiseRemoval;

extern SPxProcess *SPxProTrails;
extern SPxProcess *SPxProGaussianNoiseRemoval;
extern SPxProcess *SPxProResidualEstimate;
extern SPxProcess *SPxProErode;
extern SPxProcess *SPxProOTSU;//add for otsu
extern SPxProcess *SPxProClutterMap;
extern SPxProcess *SPxProPeak;
/* Extern the handles of the other well-known processes.  Again
 * these will be NULL until initialised, but are probably NOT initialised
 * by the standard init function.
 */
extern SPxProcess *SPxProNetworkSend;
extern SPxProcess *SPxProRecord;
extern SPxProcess *SPxProGenPGM;
extern SPxProcess *SPxProBlobCheck;
/*********************************************************************
*
*   Functions
*
**********************************************************************/

/* Extern the function to initialise all the standard processes. */
extern void SPxinitStdProcesses(void);

/* Extern the functions to initialise specific standard processes,
 * which are probably called by SPxinitStdProcesses() with the exception
 * of the MHT/MBT ones.
 */
extern void SPxProAreaSelectInit(void);
extern void SPxProAScanInit(void);
extern void SPxProAverageInit(void);
extern void SPxProAziSpokesInit(void);
extern void SPxProBlobMergeInit(void);
extern void SPxProBoostInit(void);
extern void SPxProCombineInit(void);
extern void SPxProDebugInit(void);
extern void SPxProDynamicThresholdInit(void);
extern void SPxProFilterNotchInit(void);
extern void SPxProFTCInit(void);
extern void SPxProGainOffsetInit(void);
extern void SPxProHistoryInit(void);
extern void SPxProIntegrateInit(void);
extern void SPxProIntensityHistogramInit(void);
extern void SPxProLMBInit(void);
extern void SPxProLMMInit(void);
extern void SPxProLUTInit(void);
extern void SPxProMBTInit(void);
extern void SPxProMHTInit(void);
extern void SPxProNullInit(void);
extern void SPxProMonitorInit(void);
extern void SPxProOSDInit(void);
extern void SPxProPimCopyInit(void);
extern void SPxProProjectionInit(void);
extern void SPxProRangeAdjustInit(void);
extern void SPxProRangeBlankInit(void);
extern void SPxProRangeRingsInit(void);
extern void SPxProRemapInit(void);
extern void SPxProROCInit(void);
extern void SPxProStatsInit(void);
extern void SPxProScanConvInit(void);
extern void SPxProSectorBlankInit(void);
extern void SPxProSpanDetectInit(void);
extern void SPxProSSIInit(void);
extern void SPxProSSI64Init(void);
extern void SPxProSTCInit(void);
extern void SPxProSyncCombineInit(void);
extern void SPxProThresholdInit(void);
extern void SPxProToRIBInit(void);
extern void SPxProTrackerInit(void);
extern void SPxProVisualDataInit(void);
extern void SPxProVisualInfoInit(void);
extern void SPxProAdaptiveNoiseRemovalInit(void);

extern void SPxProGaussianNoiseRemovalInit();
extern void SPxProErodeInit();
extern void SPxProResidualEstimateInit();
extern void SPxProOTSUInit(void);
extern void SPxProClutterMapInit();
extern void SPxProPeakInit();
extern void SPxProGenPGMInit();
/* Extern the functions to initialise specific processes,
 * which are probably NOT called by SPxinitStdProcesses().
 */
extern void SPxProNetworkSendInit(void);
extern void SPxProRecordInit(void);
extern void SPxProBlobCheckInit(void);

/* Extern the function used by some of the standard processes
 * internally (but probably not by other users).
 */
extern void SPxcalculateAverageIntoWorkspace(SPxRunProcess *rp, SPxPIM *pim,
					unsigned azimuth, int winWidth);

extern void SPxcalculateVarianceIntoWorkspace(SPxRunProcess *rp, SPxPIM *pim, 
				       UINT32 *dst, unsigned azimuth,
				       int winWidth);
/* Extern the additional functionality provided by some of the processes. */
extern void SPxProIntegrateHoldArea(SPxRunProcess *rp,
					double rngMetres, double aziDegs,
					double sizeMetres, double sizeDegs);
extern SPxErrorCode SPxProIntegrateSaveToFile(SPxRunProcess *rp,
					const char *filename);
extern SPxErrorCode SPxProIntegrateLoadFromFile(SPxRunProcess *rp,
					const char *filename);
extern SPxErrorCode SPxProIntegrateLoadFromFileNow(SPxRunProcess *rp,
					const char *filename);
extern SPxErrorCode SPxProRecordUseNavData(SPxRunProcess *rp, 
                                           SPxNavData *navData=NULL);
extern SPxErrorCode SPxProRecordPacketB(SPxRunProcess *rp,
					UINT16 packetType,
					const void *payload,
					unsigned int numBytes,
                                        UCHAR channelIndex=0);
extern SPxErrorCode SPxProRecordPacketB(SPxRunProcess *rp,
					const SPxPacketHeaderB_tag *packetHdr,
					const void *payloadHdr, unsigned int payloadHdrBytes,
					const void *payload, unsigned int payloadBytes,
					UCHAR channelIndex=0);
extern SPxErrorCode SPxProRecordReturn(SPxRunProcess *rp, 
                                       SPxReturn *rtn,
                                       UCHAR channelIndex=0,
				       SPxTime_tag *rtnTime=NULL);
extern SPxErrorCode SPxProRecordProbeLicense(unsigned int *graceSecsLeftPtr);
extern SPxErrorCode SPxProLMMRegisterBlob(SPxRunProcess *rp,
					struct SPxBlob_tag *blob);

/* Extern the internal functions for some of the processes (shouldn't be
 * used by applications).
 */
extern SPxErrorCode SPxProRecordUpdateInternals(SPxRunProcess *rp, void *ptr,
						int silent=FALSE);

#endif /* _SPX_PROCSTD_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
