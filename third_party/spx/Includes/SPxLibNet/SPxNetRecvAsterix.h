/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNetRecvAsterix.h,v $
* ID: $Id: SPxNetRecvAsterix.h,v 1.13 2017/05/09 13:21:55 rew Exp $
*
* Purpose:
*	Header for ASTERIX Cat-240 network receiving object.
*
* Revision Control:
*   09/05/17 v1.13   AGC	Add GetLastVideoIsCellDurNano() function.
*
* Previous Changes:
*   06/05/16 1.12   SP 	Support filtering of summary using SP data.
*   08/04/16 1.11   SP 	Support SP field in summary message.
*   26/11/15 1.10   AGC	Azimuth reference control moved to SPxRadarSource.
*   15/09/14 1.9    SP 	Support azimuth input mode.
*   04/10/13 1.8    AGC	Simplify headers.
*   25/04/13 1.7    AGC	Use new SPxCompress class.
*   12/12/12 1.6    AGC	Add NorthUp/AziRefModeEnabled parameters.
*   09/11/12 1.5    SP 	Improve video message index checking.
*   11/09/12 1.4    SP 	Improve out-of-sequence message handling.
*   07/09/12 1.3    SP 	Support receipt of fragmented returns.
*   14/11/11 1.2    SP 	Add additional data retrieval functions.
*                       Add summary message callback.
*                       Rename some items and tidy.
*   02/11/11 1.1    SP 	Initial Version.
*
**********************************************************************/

#ifndef _SPX_NET_RECV_ASTERIX_H
#define _SPX_NET_RECV_ASTERIX_H

/*
 * Other headers required.
 */

/* For SPxReturnHeader. */
#include "SPxLibData/SPxRib.h"

/* We need the base class header (which includes several others as well). */
#include "SPxLibNet/SPxNetRecv.h"


/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Maximum length of meta text string that can be received in
 * a Video Summary Message, including space for a null 
 * terminator (if the received string is not already terminated). 
 */
#define SPX_CAT240_MAX_SUMMARY_TEXT_BYTES 256

/* Maximum number of Special Purpose Field bytes supported. 
 * This is an arbitrary value and can be increased in the future
 * if needed.
 */
#define SPX_CAT240_MAX_SP_DATA_BYTES 8

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
class SPxCompress;

/*
 * Define our project-specific network receiver class, derived from the
 * standard one.
 */
class SPxNetworkReceiveAsterix : public SPxNetworkReceive
{
public:
    /*
     * Public types.
     */

    /* Azimuth reference mode. */
    typedef enum
    {
        /* Use END_AZ field to set azimuth position. */
        AZI_INPUT_REF_MODE_END = 0,

        /* Use START_AZ field to set azimuth position. */
        AZI_INPUT_REF_MODE_START = 1

    } AziInputRefMode_t;

    /*
     * Public fields.
     */

    /*
     * Public functions.
     */

    /* Constructor, destructor etc. */
    SPxNetworkReceiveAsterix(SPxRIB *buffer);
    virtual ~SPxNetworkReceiveAsterix();

    /* Configuration functions. */
    SPxErrorCode SetExpectedSIC(UINT8 sic) 
    { 
        m_expectedSIC = sic; 
        return SPX_NO_ERROR;
    }
    UINT8 GetExpectedSIC(void) { return m_expectedSIC; }

    SPxErrorCode SetExpectedSAC(UINT8 sac) 
    { 
        m_expectedSAC = sac; 
        return SPX_NO_ERROR;
    }
    UINT8 GetExpectedSAC(void) { return m_expectedSAC; }

    SPxErrorCode SetAziInputRefMode(AziInputRefMode_t mode);
    AziInputRefMode_t GetAziInputRefMode(void) { return m_aziInputRefMode; }

    SPxErrorCode SetSummarySpFilter(UINT8 mask, UINT8 match);
    UINT8 GetSummarySpFiterMask(void) { return m_spFilterMask; }
    UINT8 GetSummarySpFilterMatch(void) { return m_spFilterMatch; }

    /* Data retrieval (any message type). */
    SPxErrorCode GetLastSAC(UINT8 *sacRtn, 
                            UINT32 expiryMsecs=0);

    SPxErrorCode GetLastSIC(UINT8 *sicRtn, 
                            UINT32 expiryMsecs=0);

    /* Data retrieval (Video Summary Message). */
    SPxErrorCode GetLastSummaryText(char *bufPtr, 
                                    unsigned int bufSizeBytes,
                                    UINT32 expiryMsecs=0);

    SPxErrorCode GetLastSummaryTimeOfDayMsecs(UINT32 *timeOfDayRtn, 
                                              UINT32 expiryMsecs=0);

    SPxErrorCode GetLastSummarySpData(UINT8 *bufPtr,
                                      unsigned int bufSizeBytes,
                                      unsigned int *dataSizeBytes,
                                      UINT32 expiryMsecs=0);

    /* Data retrieval (Video Message). */
    SPxErrorCode GetLastVideoMesgIndex(UINT32 *mesgIndexRtn, 
                                       UINT32 expiryMsecs=0);
  
    SPxErrorCode GetLastVideoTimeOfDayMsecs(UINT32 *timeOfDayRtn, 
                                            UINT32 expiryMsecs=0);

    SPxErrorCode GetLastVideoIsCellDurNano(int *isCellDurNano);


    /* Generic parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

    /* Install callback that will be invoked for each Video Summary
     * message that is received.
     */
    SPxErrorCode InstallSummaryFn(void (*fn)(SPxNetworkReceiveAsterix *obj,
                                             void *arg,
                                             UINT8 sac, UINT8 sic,
                                             const char *summaryText),
                                  void *arg)
    {
	m_fnSummaryArg = arg;
	m_fnSummary = fn;
	return SPX_NO_ERROR;
    }

protected:

    /*
     * Protected functions.
     */

    /* Overwrite the base class functions we want to handle ourselves. */
    int ThreadHandleDataFn(unsigned char *data,
                           unsigned int dataSizeBytes);   

 
    /*
     * Protected fields.
     */

private:

    /*
     * Private fields.
     */

    /* Configurable items. */
    UINT8 m_expectedSAC;               /* Used to filter on SAC if != 0. */
    UINT8 m_expectedSIC;               /* Used to filter on SIC if != 0. */ 
    AziInputRefMode_t m_aziInputRefMode; /* Azimuth input ref mode. */
    UINT8 m_spFilterMask;              /* Mask value for SP data filtering. */
    UINT8 m_spFilterMatch;             /* Match value for SP data filtering. */

    /* Message timestamps. */
    UINT32 m_lastValidMesgTimeMsecs;   /* Time of last valid mesg. */
    UINT32 m_lastValidSummaryTimeMsecs;/* Time of last valid summary msg. */
    UINT32 m_lastValidVideoTimeMsecs;  /* Time of last valid video msg. */
    int m_isLastMesgTimeSet;           /* True if mesg timestamp set. */
    int m_isLastValidSummaryTimeSet;   /* True if valid summary time set. */
    int m_isLastValidVideoTimeSet;     /* True if valid video time set. */

    /* Received message data. */
    char *m_summaryText;               /* Buffer used to store meta text. */
    SPxCriticalSection m_summaryTextMutex; /* Control access to text buf. */
    UINT8 m_lastSAC;                   /* Received System Area Code. */
    UINT8 m_lastSIC;                   /* Received System Ident Code. */
    UINT32 m_lastSummaryTimeOfDayMsecs;/* Received summary msg time-of-day. */
    UINT32 m_lastVideoTimeOfDayMsecs;  /* Received video msg time-of-day. */
    int m_isSummaryTimeOfDayPresent;   /* True if last summary msg had tod. */
    int m_isVideoTimeOfDayPresent;     /* True if last video msg had tod. */
    UINT8 m_summarySpData[SPX_CAT240_MAX_SP_DATA_BYTES]; /* Summary SP data. */
    SPxCriticalSection m_summarySpDataMutex; /* Summary SP data mutex. */
    unsigned int m_summaryNumSpDataBytes; /* Number of summary SP bytes. */

    /* Message reconstruction. */
    UINT8 *m_videoBuf;                 /* Buffer used to rebuild message. */
    UINT32 m_videoBufSizeBytes;        /* Video buffer size. */
    SPxReturnHeader m_rtnHdr;          /* Return header to write to RIB. */
    UINT16 m_rtnStartAzi;              /* Return start azimuth. */
    UINT32 m_rtnCellDur;               /* Return cell duration is ns or fs. */
    int m_rtnIsCellDurNano;            /* Is m_rtnCellDur in nano secs? */
    double m_rtnCellDurMetres;         /* Return cell duration in metres. */
    UINT32 m_rtnFragCount;             /* Return fragment count. */

    /* Video message index checking. */
    UINT32 m_lastVideoMesgIndex;       /* Index number from last valid msg. */
    UINT32 m_maxVideoMesgIndexJump;    /* Max index jump allowed. */
    UINT32 m_numFragsDiscarded;        /* Number of fragments discarded. */
    UINT32 m_fragDiscardLimit;         /* Fragment discard limit. */

    /* Max number of cells calculation. */
    UINT16 m_maxNumCells;              /* Return length high water mark. */
    UINT16 m_mncOverInterval;          /* Used to calculate max num cells. */
    UINT32 m_mncIntervalMsecs;         /* Interval for above in millisecs. */
    UINT32 m_mncIntervalStartMsecs;    /* Interval start time. */

    /* Min start cell calculation. */
    UINT32 m_minStartCell;             /* Return start low water mark. */
    UINT32 m_mscOverInterval;          /* Used to calculate min start cell. */
    UINT32 m_mscIntervalMsecs;         /* Interval for above in millisecs. */
    UINT32 m_mscIntervalStartMsecs;    /* Interval start time. */

    /* Return interval calculation. */
    UINT32 m_numReturns;               /* Number of returns in interval. */
    UINT32 m_rtnIntervalStartMsecs;    /* Interval start time in millisecs.*/
    UINT32 m_averageRtnIntervalUsecs;  /* Average interval in microsecs. */

    /* ZLIB decompression. */
    SPxCompress *m_compress;

    /* Summary message callback. */
    void *m_fnSummaryArg;
    void (*m_fnSummary)(SPxNetworkReceiveAsterix *obj,
                        void *arg,
                        UINT8 sac, UINT8 sic,
                        const char *summaryText);
    
    /*
     * Private functions.
     */

    SPxErrorCode recvSummaryMesg(UINT8 *data,
                                 unsigned int dataSizeBytes,
                                 UINT8 *srcPtr,
                                 unsigned int mesgBytes,
                                 UINT8 fspec1,
                                 UINT8 fspec2,
                                 UINT8 sac,
                                 UINT8 sic);

    SPxErrorCode recvVideoMesg(UINT8 *data,
                               unsigned int dataSizeBytes,
                               UINT8 *srcPtr,
                               unsigned int mesgBytes,
                               UINT8 fspec1,
                               UINT8 fspec2,
                               UINT8 sac,
                               UINT8 sic);

    void updateMaxNumCells(UINT16 numVideoCells);

    void updateMinStartCell(UINT32 startCell);

    int checkVideoMesgIndex(UINT32 mesgIndex);

}; /* SPxNetworkReceiveAsterix class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_NET_RECV_ASTERIX_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
