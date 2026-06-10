/*********************************************************************
*
* (c) Copyright 2011 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxADSBUtils.h,v $
* ID: $Id: SPxADSBUtils.h,v 1.3 2014/10/23 11:17:56 rew Exp $
*
* Purpose:
*	Header for SPxADSB utility functions.
*
*
* Revision Control:
*   23/10/14 v1.3    AGC	Use double type for feet argument.
*
* Previous Changes:
*   22/10/14 1.2    AGC	Add encode functions.
*   15/10/14 1.1    REW	Initial Version, from part of SPxADSBDecoder.
**********************************************************************/

#ifndef _SPX_ADSB_UTILS_H
#define _SPX_ADSB_UTILS_H

/*
 * Other headers required.
 */
/* We need the definition of UINT16. */
#include "SPxLibUtils/SPxTypes.h"


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

/*
 * Extern the public functions.
 */
extern int SPxADSBModeCToFeet(UINT16 modeC);
extern UINT16 SPxADSBModeCFromFeet(double feet);
extern int SPxADSBGillhamToFeet(unsigned int gillhamValue);
extern unsigned int SPxADSBGillhamFromFeet(double feet);

#endif /* SPX_ADSB_UTILS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
