/*********************************************************************
*
* (c) Copyright 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSound.h,v $
* ID: $Id: SPxSound.h,v 1.1 2013/12/02 16:40:59 rew Exp $
*
* Purpose:
*   Functions to play basic sounds.
*
* Revision Control:
*   02/12/13 v1.1   SP  	Initial version.
*
* Previous Changes:
*
**********************************************************************/

#ifndef _SPX_SOUND_H
#define _SPX_SOUND_H

/*********************************************************************
*
*   Headers 
*
**********************************************************************/

/* Need error types. */
#include "SPxLibUtils/SPxError.h"

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

/*
 * Extern the functions we provide.
 */

/* Enable / disable GUI control sounds by default. */
extern void SPxSoundSetGuiDefaultEnabled(int isEnabled);
extern int SPxSoundIsGuiDefaultEnabled(void);

/* Sound playing functions. */
extern SPxErrorCode SPxSoundPlayClick(void);

/* Windows-only functions. */
#ifdef _WIN32
SPxErrorCode SPxSoundPlayResource(int resID, int noStop=0);
#endif

#endif /* _SPX_SOUND_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
