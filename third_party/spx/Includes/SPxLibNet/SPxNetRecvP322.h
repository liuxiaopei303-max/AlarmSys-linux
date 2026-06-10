/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNetRecvP322.h,v $
* ID: $Id: SPxNetRecvP322.h,v 1.1 2016/02/01 14:02:50 rew Exp $
*
* Purpose:
*	Header for project-322 network receiving module.
*
*
* Revision Control:
*   29/01/16 v1.1    REW	Initial Version.
*
* Previous Changes:
**********************************************************************/

#ifndef _SPX_NETRECVP322_H
#define _SPX_NETRECVP322_H

/*
 * Other headers required.
 */
/* We need the base class header (which includes several others as well). */
#include "SPxLibNet/SPxNetRecv.h"


/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Default values. */
#define	SPX_NET_RECV_P322_DEFAULT_DISCO_ADDR	"239.255.123.1"
#define	SPX_NET_RECV_P322_DEFAULT_DISCO_PORT	12345


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
 * Define our project-specific network receiver class, derived from the
 * standard one.
 */
class SPxNetworkReceiveP322 :public SPxNetworkReceive
{
public:
    /*
     * Public fields.
     */

    /*
     * Public functions.
     */
    /* Constructor, destructor etc. */
    SPxNetworkReceiveP322(SPxRIB *buffer);
    virtual ~SPxNetworkReceiveP322();

    /* Overwrite the base class functions we want to handle ourselves. */
    virtual int ThreadSocketChangedFn(void);
    virtual int ThreadReadDataFn(unsigned char *buf,
				unsigned int bufSizeBytes,
				unsigned int timeoutMsecs);
    virtual int ThreadHandleDataFn(unsigned char *data,
				unsigned int dataSizeBytes);

    /* Configuration functions. */
    SPxErrorCode SetUseDiscovery(int useDiscovery);
    int GetUseDiscovery(void) const { return(m_useDiscovery); }
    SPxErrorCode SetSerialNumber(unsigned int serialNumber);
    unsigned int GetSerialNumber(void) const { return(m_serialNumber); }
    SPxErrorCode SetForce8BitOutput(int force);
    int GetForce8BitOutput(void) const { return(m_force8BitOutput); }

    /* Status functions.  */

    /* Generic parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:
    /*
     * Private fields.
     */
    /* Configuration options. */
    int m_useDiscovery;			/* Do we auto-discover address? */
    unsigned int m_serialNumber;	/* Serial number to discover */
    int m_force8BitOutput;		/* Use 8-bit output even if 12 in? */

    /* Video and status sockets etc. */
    SOCKET_TYPE m_videoSocket;			/* Socket handle */
    struct sockaddr_in m_videoSocketAddress;	/* Address structure */
    SOCKET_TYPE m_statusSocket;			/* Socket handle */
    struct sockaddr_in m_statusSocketAddress;	/* Address structure */

    /* Buffer for receiving packets. */
    unsigned char *m_recvBuf;			/* Buffer */
    unsigned int m_recvBufSizeBytes;		/* Allocated size of buffer */

    /* Buffer for unpacking data. */
    unsigned char *m_decodeBuf;			/* Buffer */
    unsigned int m_decodeBufSizeBytes;		/* Allocated size of buffer */

    /* PRF estimation. */
    UINT32 m_prfRefTimeMsecs;			/* Reference time */
    unsigned int m_prfSpokesSinceLastRef;	/* Counter */
    double m_prfEstimateHz;			/* Estimated PRF */
    unsigned int m_prfUsecsPerSpoke;		/* Inter-spoke timing */

    /*
     * Private functions.
     */
    SPxErrorCode setupOtherSockets(UINT32 now,
			    const char *videoAddr, unsigned int videoPort,
			    const char *statusAddr, unsigned int statusPort);
    SPxErrorCode closeOtherSockets(void);

    /* Data handling. */
    SPxErrorCode handleDiscoveryPacket(UINT32 now, const unsigned char *data,
					unsigned int dataSizeBytes);
    SPxErrorCode handleStatusPacket(UINT32 now, const unsigned char *data,
					unsigned int dataSizeBytes);
    SPxErrorCode handleVideoPacket(UINT32 now, const unsigned char *data,
					unsigned int dataSizeBytes);
    SPxErrorCode extractSamples(const unsigned char *data,
					unsigned int numPackedBytes,
					unsigned int numSamples,
					unsigned int bitsPerSample,
					unsigned char *decodeBuf,
					unsigned int decodeBufSizeBytes,
					unsigned char packing);
}; /* SPxNetworkReceiveP322 class */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_NETRECVP322_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
