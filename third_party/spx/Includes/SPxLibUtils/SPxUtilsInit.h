/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxUtilsInit.h,v $
* ID: $Id: SPxUtilsInit.h,v 1.6 2016/10/31 15:28:51 rew Exp $
*
* Purpose:
*   Initialisation and license functions for SPxLibUtils.
*
* Revision Control:
*   31/10/16 v1.6    AGC	Add AV bits argument to SPxLicBuildReport().
*
* Previous Changes:
*   08/07/16 1.5    REW	Add MBT check.
*   07/01/15 1.4    AGC	Improve comments.
*   01/07/14 1.3    AGC	Add more options to SPxLicBuildReport().
*   26/06/14 1.2    AGC	Add SPxLicBuildReport() function.
*   04/10/13 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_UTILS_INIT_H_
#define _SPX_UTILS_INIT_H_

#include <stdio.h>

#include "SPxLibUtils/SPxTypes.h"

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

extern int SPxLibUtilsInit(int verbose=0, FILE *logFile=NULL);

extern int SPxLicInit(void);
extern void SPxLicReport(FILE *logFile);
extern UINT32 SPxLicGetID(int silent=FALSE);
extern int SPxLicBuildReport(char *buffer, int bufLen, UINT32 appBits=0,
			     UINT32 featureBits=0xFFFFFFFF,
			     UINT32 feature2Bits=0xFFFFFFFF,
			     int incLicNum=TRUE, int incLicExpiry=TRUE);

extern int SPxLicGetFrameworkStatus(void);

/* Convenience functions, return FALSE if not licensed, TRUE if licensed. */
extern int SPxLicIsPpiLicensed(void);
extern int SPxLicIsAScanLicensed(void);
extern int SPxLicIsBScanLicensed(void);
extern int SPxLicIsPlotExtLicensed(void);
extern int SPxLicIsTrackingLicensed(void);
extern int SPxLicIsMBTTrackingLicensed(void);
extern int SPxLicIsNetSourceLicensed(void);
extern int SPxLicIsHPxSourceLicensed(void);
extern int SPxLicIsDistributionLicensed(void);
extern int SPxLicIsRecordingLicensed(void);

#endif /* _SPX_UTILS_INIT_H_ */

/*********************************************************************
*
* End of file
*
**********************************************************************/
