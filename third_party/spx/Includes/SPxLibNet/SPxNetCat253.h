/*********************************************************************
*
* (c) Copyright 2013 - 2015, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNetCat253.h,v $
* ID: $Id: SPxNetCat253.h,v 1.5 2015/12/16 16:10:05 rew Exp $
*
* Purpose:
*	Header for ASTERIX Cat-253 base class.
*
* Revision Control:
*   16/12/15 v1.5    AJH 	WriteMesgToSendBuf() takes offset into buffer.
*
* Previous Changes:
*   27/01/14 1.4    SP 	Make Message Sequence ID optional.
*   24/10/13 1.3    SP 	Support I253/050 - Message Sequence ID.
*   04/10/13 1.2    AGC	Simplify headers.
*   08/04/13 1.1    SP 	Initial Version.
*
**********************************************************************/

#ifndef _SPX_NET_CAT_253_H
#define _SPX_NET_CAT_253_H

/*
 * Other headers required.
 */

/* We need SPxLibUtils for common types, callbacks, errors etc. */
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxError.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Maximum length of application data (not including null termination). */
#define SPX_CAT253_MAX_APP_DATA_BYTES 255

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

/* Forward declarations. */
struct SPxTime_tag;

class SPxNetCat253
{
public:
    /*
     * Public fields.
     */

    /*
     * Public functions.
     */

    /* Constructor, destructor etc. */
    SPxNetCat253(void);
    virtual ~SPxNetCat253();

    /* Configuration functions. */
    virtual SPxErrorCode SetSourceSIC(UINT8 sic) 
    { 
        m_srcSIC = sic; 
        return SPX_NO_ERROR; 
    }
    virtual UINT8 GetSourceSIC(void) { return m_srcSIC; }

    virtual SPxErrorCode SetSourceSAC(UINT8 sac) 
    { 
        m_srcSAC = sac; 
        return SPX_NO_ERROR; 
    }
    virtual UINT8 GetSourceSAC(void) { return m_srcSAC; }

    virtual SPxErrorCode SetSourceLocalID(UINT8 localID) 
    { 
        m_srcLocalID = localID; 
        return SPX_NO_ERROR; 
    }
    virtual UINT8 GetSourceLocalID(void) { return m_srcLocalID; }

    virtual SPxErrorCode SetDestSIC(UINT8 sic) 
    { 
        m_dstSIC = sic; 
        return SPX_NO_ERROR; 
    }
    virtual UINT8 GetDestSIC(void) { return m_dstSIC; }

    virtual SPxErrorCode SetDestSAC(UINT8 sac) 
    { 
        m_dstSAC = sac; 
        return SPX_NO_ERROR; 
    }
    virtual UINT8 GetDestSAC(void) { return m_dstSAC; }

    virtual SPxErrorCode SetDestLocalID(UINT8 localID) 
    { 
        m_dstLocalID = localID; 
        return SPX_NO_ERROR; 
    }
    virtual UINT8 GetDestLocalID(void) { return m_dstLocalID; }

    virtual SPxErrorCode SetFilterOnSACEnabled(int isEnabled)
    {
        m_filterOnSACEnabled = isEnabled;
        return SPX_NO_ERROR; 
    }
    virtual int IsFilterOnSACEnabled(void) 
    { 
        return m_filterOnSACEnabled; 
    }

    virtual SPxErrorCode SetFilterOnSICEnabled(int isEnabled)
    {
        m_filterOnSICEnabled = isEnabled;
        return SPX_NO_ERROR; 
    }
    virtual int IsFilterOnSICEnabled(void) 
    { 
        return m_filterOnSICEnabled; 
    }

    virtual SPxErrorCode SetFilterOnLocalIDEnabled(int isEnabled)
    {
        m_filterOnLocalIDEnabled = isEnabled;
        return SPX_NO_ERROR; 
    }
    virtual int IsFilterOnLocalIDEnabled(void) 
    { 
        return m_filterOnLocalIDEnabled; 
    }

    virtual SPxErrorCode SetIncludeTimeOfDay(int include)
    {
        m_includeTimeOfDay = include;
        return SPX_NO_ERROR; 
    }
    virtual int IsTimeOfDayIncluded(void) { return m_includeTimeOfDay; }

    virtual SPxErrorCode SetIncludeSequenceID(int include)
    {
        m_includeSequenceID = include;
        return SPX_NO_ERROR; 
    }
    virtual int IsSequenceIDIncluded(void) { return m_includeSequenceID; }

    virtual SPxErrorCode SetNullTermAppDataEnabled(int isEnabled)
    {
        m_nullTermAppDataEnabled = isEnabled;
        return SPX_NO_ERROR; 
    }
    virtual int IsNullTermAppDataEnabled(void) 
    { 
        return m_nullTermAppDataEnabled; 
    }

    virtual SPxErrorCode SetAutoPollMesgs(int isEnabled)
    {
        m_autoPollMesgs = isEnabled;
        return SPX_NO_ERROR; 
    }
    virtual int IsAutoPollMesgsEnabled(void) 
    { 
        return m_autoPollMesgs; 
    }
        
    /* Received data. */
    virtual SPxErrorCode GetLastSource(UINT8 *sacRtn,
                                       UINT8 *sicRtn,
                                       UINT8 *localIdRtn,
                                       UINT32 expiryMsecs=0);

    virtual SPxErrorCode GetLastDest(UINT8 *sacRtn,
                                     UINT8 *sicRtn,
                                     UINT8 *localIdRtn,
                                     UINT32 expiryMsecs=0);

    virtual SPxErrorCode GetLastTimeOfDayMsecs(UINT32 *timeOfDayRtn, 
                                               UINT32 expiryMsecs=0);

    /* Install callback that will be invoked when message is received. */
    virtual SPxErrorCode SetMesgHandler(void (*fn)(SPxNetCat253 *obj,
                                                   void *arg,
                                                   const UINT8 *appData,
                                                   unsigned int numAppDataBytes),
                                        void *arg)
    {
	m_fnAppDataArg = arg;
	m_fnAppData = fn;
	return SPX_NO_ERROR;
    }

protected:

    /*
     * Protected fields.
     */

    /* Send buffer. */
    UINT8 *m_sendBuf;                   /* Send buffer. */
    unsigned int m_sendBufBytes;        /* Send buffer size. */
    SPxCriticalSection m_sendMutex;     /* Send mutex. */

    /* Receive buffer. */
    UINT8 *m_recvBuf;                   /* Recv buffer. */
    unsigned int m_recvBufBytes;        /* Recv buffer size. */
    unsigned int m_recvDataBytes;       /* Amount of data in recv buffer. */
    SPxCriticalSection m_recvMutex;     /* Recv mutex. */    

    /* Application data buffer. */
    UINT8 *m_appDataBuf;                /* Application data buffer. */
    unsigned int m_appDataBufBytes;     /* Application data buffer size. */
    unsigned int m_lastAppDataBytes;    /* Size of last app data recv'd. */

    /*
     * Protected functions.
     */

    SPxErrorCode WriteMesgToSendBuf(UINT8 *data, 
                                    unsigned int numDataBytes,
				    unsigned int bufOffset,
                                    unsigned int *numMesgBytesRtn,
                                    UINT32 debug=0,
                                    FILE *logFile=NULL);

    unsigned int CopyDataToRecvBuf(unsigned char *data,
                                   unsigned int numBytes,
                                   int allOrNothing);

    void ProcessRecvData(UINT32 debug=0,
                         FILE *logFile=NULL);

    unsigned int HandleMesg(unsigned char *data,
                            unsigned int numBytes,
                            UINT32 debug=0,
                            FILE *logFile=NULL);
    
private:

    /*
     * Private fields.
     */

    /* Configurable items. */
    int m_includeTimeOfDay;        /* Include Time of Day in sent mesg? */
    int m_includeSequenceID;       /* Include Sequence ID in sent mesg? */
    UINT8 m_srcSAC;                /* Source System Area Code. */
    UINT8 m_srcSIC;                /* Source System Identification Code.*/
    UINT8 m_srcLocalID;            /* Source Local ID. */
    UINT8 m_dstSAC;                /* Dest System Area Code. */
    UINT8 m_dstSIC;                /* Dest System Identification Code. */
    UINT8 m_dstLocalID;            /* Dest Local ID. */
    int m_filterOnSACEnabled;      /* Filter received messages using SAC. */
    int m_filterOnSICEnabled;      /* Filter received messages using SIC. */
    int m_filterOnLocalIDEnabled;  /* Filter received msgs using Local ID. */
    int m_nullTermAppDataEnabled;  /* Enable to null terminate data. */
    int m_autoPollMesgs;           /* Enable to poll for mesgs in thread. */

    /* Send data. */
    UINT16 m_sequenceID;            /* Next qequence number to send. */

    /* Received data. */
    UINT8 m_lastSrcSAC;             /* Received source System Area Code. */
    UINT8 m_lastSrcSIC;             /* Received source System Ident Code. */
    UINT8 m_lastSrcLocalID;         /* Received source Local ID. */
    UINT8 m_lastDstSAC;             /* Received dest System Area Code. */
    UINT8 m_lastDstSIC;             /* Received dest System Ident Code. */
    UINT8 m_lastDstLocalID;         /* Received dest Local ID. */
    UINT32 m_lastTimeOfDayMsecs;    /* Received time-of-day. */
    int m_isTimeOfDaySet;           /* True if last mesg had time-of-day. */
    UINT16 m_lastSequenceID;        /* Last sequence number received. */

    /* Message timestamps. */
    UINT32 m_lastRxMesgTimeMsecs;   /* Time of last received mesg. */
    int m_isLastRxMesgTimeSet;      /* True if received mesg timestamp set. */
    UINT32 m_lastTxMesgTimeMsecs;   /* Time of last sent mesg. */
    UINT32 m_lastLogFileFlushMsecs; /* Last log file flush. */

    /* Application message callback. */
    void *m_fnAppDataArg;
    void (*m_fnAppData)(SPxNetCat253 *obj,
                        void *arg,
                        const UINT8 *appData,
                        unsigned int numBytes);

    /*
     * Private functions.
     */

    UINT32 get128thSecsSinceMidnight(SPxTime_tag *timestamp);
    void checkSequenceID(UINT16 sequenceID, UINT32 debug, FILE *logFile);

    /*
     * Private static functions.
     */

}; /* SPxNetCat253 class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_NET_CAT_253_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
