/*********************************************************************
*
* (c) Copyright 2012, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNetUtils.h,v $
* ID: $Id: SPxNetUtils.h,v 1.2 2012/11/09 12:02:40 rew Exp $
*
* Purpose:
*	Header for general network utility functions.
*
* Revision Control:
*   09/11/12 v1.2    AGC	Tidy line endings.
*
* Previous Changes:
*   26/10/12 1.1    AGC	Initial version.
* 
**********************************************************************/

#ifndef _SPX_NET_UTILS_H
#define _SPX_NET_UTILS_H

/*
 * Other headers required.
 */

/* For SPxErrorCode. */
#include "SPxLibUtils/SPxError.h"

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
*   Function prototypes
*
**********************************************************************/
#ifdef _WIN32
extern SPxErrorCode SPxDownloadFile(const char *url,
                                    unsigned char *buffer,
                                    unsigned int *bufferSizeBytes,
                                    unsigned int timeoutMsecs);
#else
extern SPxErrorCode SPxDownloadFile(const char *url,
                                    unsigned char *buffer,
                                    unsigned int *bufferSizeBytes,
                                    unsigned int timeoutMsecs,
                                    const char *headers=NULL,
                                    const char *post=NULL,
                                    unsigned int *statusCodeRet=NULL);
#endif




#endif /* _SPX_NET_UTILS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
