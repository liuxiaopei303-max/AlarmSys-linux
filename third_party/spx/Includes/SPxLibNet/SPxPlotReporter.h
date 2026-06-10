/*********************************************************************
*
* (c) Copyright 2009, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxPlotReporter.h,v $
* ID: $Id: SPxPlotReporter.h,v 1.2 2009/06/29 14:56:32 rew Exp $
*
* Purpose:
*	Header for SPxPlotReporter class, used for sending plot reports
*	to the network.
*
*
* Revision Control:
*   29/06/09 v1.2    REW	Remove blob detector arg. Make handler public.
*
* Previous Changes:
*   25/06/09 1.1    REW	Initial Verison.
**********************************************************************/

#ifndef _SPX_PLOT_REPORTER_H
#define _SPX_PLOT_REPORTER_H

/*
 * Other headers required.
 */
/* We need the packet sending base class we are derived from. */
#include "SPxLibNet/SPxPacketSend.h"


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

/*
 * Define our class.
 */
class SPxPlotReporter :public SPxPacketSender
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxPlotReporter(void);
    virtual ~SPxPlotReporter();

    /* NB: use SPxObj::Get/SetActive() for overall state control,
     * and SPxPacketSender::GetSetAddress/Port() for destination control.
     */

    /* Sender ID. */
    void SetSenderID(UINT8 id)		{ m_senderID = id; }
    UINT8 GetSenderID(void)		{ return(m_senderID); }

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /*
     * Static functions.
     */
    /* This handler should be installed as a callback in the plot source. */
    static int StaticPlotReportHandler(void *invokingObject,
						void *userObject,
						void *arg);

private:
    /*
     * Private variables.
     */
    /* Our sender ID, if any. */
    UINT8 m_senderID;			/* Zero, or sender ID */

    /* Buffer for building reports. */
    unsigned char *m_buffer;		/* Buffer for reports */
    unsigned int m_bufferSizeBytes;	/* Size of buffer */

    /*
     * Private functions.
     */
    int plotReportHandler(struct SPxBlob_tag *blob);

}; /* SPxPlotReporter */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* SPX_PLOT_REPORTER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
