/*********************************************************************
*
* (c) Copyright 2007, 2009, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxMsgRpt.h,v $
* ID: $Id: SPxMsgRpt.h,v 1.2 2009/02/01 20:14:36 rew Exp $
*
* Purpose:
*	Header for SPx Message Reporter base class.
*
*
* Revision Control:
*   29/01/09 v1.2    DGJ	Move constructor and destructor into .cpp.
*
* Previous Changes:
*   03/09/07 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_MSGRPT_H
#define _SPX_MSGRPT_H

/* Need common definitions */
#include "SPxLibUtils/SPxCommon.h"
#include "SPxObj.h"

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

/*
 * SPx Message Reporter base class (virtual)
 */
class SPxMessageReporter : public SPxObj
{
public:
	/*
	 * Public functions.
	 */
	/* Constructor and destructor. */
	SPxMessageReporter(void);
	virtual ~SPxMessageReporter(void);

	/* Reporting function (pure virtual). */
	virtual void Report(const char *msg) = 0;

	/*
	 * Public fields.
	 */

private:
	/*
	 * Private fields.
	 */


	/*
	 * Private functions.
	 */
}; /* SPxMessageReporter class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_MSGRPT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
