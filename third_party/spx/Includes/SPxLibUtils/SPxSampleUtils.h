/*********************************************************************
*
* (c) Copyright 2011 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSampleUtils.h,v $
* ID: $Id: SPxSampleUtils.h,v 1.4 2016/01/06 12:02:58 rew Exp $
*
* Purpose:
*   Header for SPx sample utility functions.
*
* Revision Control:
*   06/01/16 v1.4    AGC	Allow signed shifts in a number of functions.
*
* Previous Changes:
*   23/12/15 1.3    AGC	Add functions for setting/getting values in channels.
*   04/10/13 1.2    AGC	Simplify headers.
*   19/08/11 1.1    SP 	Initial version.
**********************************************************************/

#ifndef _SPX_SAMPLE_UTILS_H
#define _SPX_SAMPLE_UTILS_H

/* Need common definitions */
#include "SPxLibUtils/SPxTypes.h"

/* Need error codes. */
#include "SPxLibUtils/SPxError.h"

/* For SPxPackingChannel. */
#include "SPxLibData/SPxRib.h"


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


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

extern UINT32 SPxGetSampleChanValue(const void *srcPtr,
				    UCHAR packing,
				    unsigned int srcOffsetSamples=0,
				    SPxPackingChannel channel=SPX_PACKING_CHAN_A);

extern UINT32 SPxGetSampleChanValueOpt(const UINT8 *srcPtr,
				       int shift, unsigned int mask,
				       unsigned int srcOffsetSamples=0);

extern UINT32 SPxGetSampleChanValueOpt(const UINT16 *srcPtr,
				       int shift, unsigned int mask,
				       unsigned int srcOffsetSamples=0);

extern SPxErrorCode SPxCopySamples(void *dstPtr, 
                                   const void *srcPtr,
                                   unsigned int numSamples,
                                   unsigned int bytesPerSample,
                                   unsigned int dstOffsetSamples=0,
                                   unsigned int srcOffsetSamples=0);

extern SPxErrorCode SPxCopyChanSamples(void *dstPtr, 
                                       const void *srcPtr,
                                       unsigned int numSamples,
                                       UCHAR dstPacking,
				       UCHAR srcPacking,
                                       unsigned int dstOffsetSamples=0,
                                       unsigned int srcOffsetSamples=0,
				       SPxPackingChannel dstChannel=SPX_PACKING_CHAN_ALL,
				       SPxPackingChannel srcChannel=SPX_PACKING_CHAN_ALL);

extern SPxErrorCode SPxCopySubSamples(void *dstPtr, 
                                     const void *srcPtr,
                                     unsigned int dstBufLenSamples,
                                     unsigned int srcBufLenSamples,
                                     unsigned int bytesPerSample,
                                     unsigned int numSrcSamples=0);

extern SPxErrorCode SPxCopyTruncateSamples(void *dstPtr, 
                                           const void *srcPtr,
                                           unsigned int numSamples,
                                           unsigned int dstBytesPerSample,
                                           unsigned int srcBytesPerSample,
                                           unsigned int dstOffsetSamples=0,
                                           unsigned int srcOffsetSamples=0);

extern SPxErrorCode SPxClearSamples(void *dstPtr,
                                    unsigned int numSamples,
                                    unsigned int bytesPerSample,
                                    unsigned int dstOffsetSamples=0);

extern SPxErrorCode SPxClearChanSamples(void *dstPtr,
                                        unsigned int numSamples,
                                        UCHAR packing,
                                        unsigned int dstOffsetSamples=0,
				        SPxPackingChannel channel=SPX_PACKING_CHAN_ALL);

extern SPxErrorCode SPxAssertSamples(void *dstPtr,
                                     unsigned int numSamples,
                                     unsigned int bytesPerSample,
                                     unsigned int dstOffsetSamples=0);

extern SPxErrorCode SPxAssertChanSamples(void *dstPtr,
                                         unsigned int numSamples,
                                         UCHAR packing,
                                         unsigned int dstOffsetSamples=0,
				         SPxPackingChannel channel=SPX_PACKING_CHAN_ALL);

extern SPxErrorCode SPxSetSamples(void *dstPtr,
                                  UINT32 value,
                                  unsigned int numSamples,
                                  unsigned int bytesPerSample,
                                  unsigned int dstOffsetSamples=0);

extern SPxErrorCode SPxSetChanSamples(void *dstPtr,
                                      UINT32 value,
                                      unsigned int numSamples,
                                      UCHAR packing,
                                      unsigned int dstOffsetSamples=0,
				      SPxPackingChannel channel=SPX_PACKING_CHAN_ALL,
				      int highestWins=FALSE);

extern SPxErrorCode SPxSetChanSingleSample(UINT8 *dstPtr,
					   UINT8 value,
					   int shift, unsigned int mask, 
					   unsigned int dstOffsetSamples=0);

extern SPxErrorCode SPxSetChanSingleSample(UINT16 *dstPtr,
					   UINT16 value,
					   int shift, unsigned int mask,
					   unsigned int dstOffsetSamples=0);

extern SPxErrorCode SPxByteSwapSamples(void *bufPtr,
                                       unsigned int numSamples,
                                       unsigned int bytesPerSample);

#endif /* _SPX_SAMPLE_UTILS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
