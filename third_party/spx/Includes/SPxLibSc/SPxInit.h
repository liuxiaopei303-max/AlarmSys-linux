/*********************************************************************
*
* (c) Copyright 2009, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxInit.h,v $
* ID: $Id: SPxInit.h,v 1.2 2013/10/04 15:31:08 rew Exp $
*
* Purpose:
*	Header for SPx init functions.
*
*
* Revision Control:
*   04/10/13 v1.2    AGC	Simplify headers.
*
* Previous Changes:
*   24/04/09 1.1    REW	Initial Version from SPx.h.
**********************************************************************/

/* Don't multiply include this. */
#ifndef _SPX_INIT_H
#define	_SPX_INIT_H

#include <stdio.h>

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

/* Main initialisation function for the libraries.  Calling this
 * function will in turn call the initialisation functions for each
 * individual sub-library.
 *
 * The side-effect of this is that the application will drag in many of
 * the sub-library modules.  If that is undesired, the application
 * should call the appropriate initialisation functions separately.
 */
extern int SPxInit(int verbose=0, FILE *logFile=NULL);
extern int SPxIsInitialised(void);

#endif /* _SPX_INIT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
