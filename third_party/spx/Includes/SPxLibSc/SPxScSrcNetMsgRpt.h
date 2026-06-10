/*********************************************************************
*
* (c) Copyright 2007, 2010, 2013, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxScSrcNetMsgRpt.h,v $
* ID: $Id: SPxScSrcNetMsgRpt.h,v 1.3 2013/11/18 17:02:34 rew Exp $
*
* Purpose:
*	Header file for SPxScSourceNetMsgRpt class, which can be
*	used to direct generic asynchronous messages through the
*	UDP message port on the SPxScSourceNet object.
*
*
* Revision Control:
*   18/11/13 v1.3    AGC	Fix clang warning.
*
* Previous Changes:
*   15/09/10 1.2    REW	Make destructor virtual.
*   03/09/07 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_SC_SRCNET_MSGRPT_H
#define _SPX_SC_SRCNET_MSGRPT_H

/*
 * Other headers required.
 */
#include "SPxLibSc/SPxScSourceNet.h"
#include "SPxLibUtils/SPxMsgRpt.h"


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

/* Forward-declare the class we need in case headers are in wrong order. */
class SPxScSourceNet;

/*
 * Define the class itself, derived from generic message reporter.
 */
class SPxScSourceNetMsgRpt :public SPxMessageReporter
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructors and destructor. */
    SPxScSourceNetMsgRpt();
    virtual ~SPxScSourceNetMsgRpt()	{ return; }

    /* Initialisation function. */
    int Create(SPxScSourceNet *srcNet);

    /* Overwrite the base-class Report function. */
    void Report(const char *msg);

private:
    /*
     * Private variables.
     */
    /* Handle of source-net object we send messages via. */
    SPxScSourceNet *m_srcNet;

    /*
     * Private functions.
     */
}; /* SPxScSourceNetMsgRpt */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_SC_SRCNET_MSGRPT_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
