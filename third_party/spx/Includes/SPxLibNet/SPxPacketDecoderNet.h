/*********************************************************************
*
* (c) Copyright 2008 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxPacketDecoderNet.h,v $
* ID: $Id: SPxPacketDecoderNet.h,v 1.19 2017/03/08 14:43:29 rew Exp $
*
* Purpose:
*	Header for SPxPacketDecoderNet class.
*
*	This is a generic class that may be used to decode an SPx
*	packet stream received on the network.
*	It parses the messages given to it and invokes callback functions
*	according to the packet types found.
*
*
* Revision Control:
*   08/03/17 v1.19   AGC	Use atomic variables.
*
* Previous Changes:
*   11/01/17 1.18   AGC	Improve thread start/shutdown logic.
*   04/01/17 1.17   AGC	Add GetSocket().
*   23/09/16 1.16   AGC	Improve mutex behaviour.
*   02/09/16 1.15   AGC	Use atomic variable for change pending.
*   23/08/16 1.14   AGC	Make PollSocket() virtual.
*   23/06/15 1.13   AGC	Support TCP receipt.
*   26/01/15 1.12   SP 	Derive from SPxNetAddr.
*                       Make network Get...() functions const.
*   26/11/13 1.11   AGC	Remove unused m_invalidSocket.
*   04/10/13 1.10   AGC	Simplify headers.
*   24/05/13 1.9    AGC	Add IsDecodeThreadStarted().
*   14/11/11 1.8    AGC	Return error from updateAddress().
*			Use const char * for address params.
*   25/10/11 1.7    AGC	Add GetIfAddressString() function.
*   07/09/11 1.6    AGC	Add GetInitialised() function.
*   09/08/11 1.5    AGC Report sender address correctly. 
*   09/06/11 1.4    AGC	Add GetAddressString() function.
*   06/04/11 1.3    AGC	Rename class/file to SPxPacketDecoderNet.
*			Refactor to use new SPxPacketDecoder class.
*   04/10/10 1.2    AGC	Support running decoder in a thread.
*   16/09/08 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_PACKET_DECODER_NET_H
#define _SPX_PACKET_DECODER_NET_H

/*
 * Other headers required.
 */
/* We need SPxLibUtils for common types, events, errors etc. */
#include "SPxLibUtils/SPxAtomic.h"
#include "SPxLibUtils/SPxCriticalSection.h"

/* We need SPxPacketDecoder base classes. */
#include "SPxLibData/SPxPacketDecoder.h"
#include "SPxLibNet/SPxNetAddr.h"


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

/* Forward declarations. */
class SPxThread;

/*
 * Define our class.
 */
class SPxPacketDecoderNet :public SPxPacketDecoder, public SPxNetAddr
{
public:
    /*
     * Public variables.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxPacketDecoderNet(void);
    virtual ~SPxPacketDecoderNet(void);

    /* Address/port control. */
    SPxErrorCode SetAddress(const char *address, int port=0, const char *ifAddr=NULL);
    SPxErrorCode SetAddress(UINT32 address, int port=0, const char *ifAddr=NULL);
    SPxErrorCode SetPort(int port) { return SetPortNumber(port); }
    /* The SetPort function calls the SetPortNumber because the winspool
     * header defines SetPort to be either SetPortW or SetPortA which causes
     * issues when linking to a function called SetPort. This inline function
     * does not suffer from this problem.
     */
    SPxErrorCode SetPortNumber(int port);
    UINT32 GetAddress(void) const;
    const char *GetAddressString(void) const;
    int GetPort(void) const;
    UINT32 GetIfAddress(void) const;
    const char *GetIfAddressString(void) const;
    virtual int IsTCPSupported(void) const { return TRUE; }
    virtual SPxErrorCode SetTCP(int isTCP);
    virtual int IsTCP(void) const { return m_isTCP; }

    /* Decode thread functions. */
    SPxErrorCode StartDecodeThread(void);
    SPxErrorCode StopDecodeThread(void);
    int IsDecodeThreadStarted(void) const;
    
    /* Poll the socket for packets. */
    virtual SPxErrorCode PollSocket(void);

    /* Decoding functions. */
    SPxErrorCode DecodePackets(unsigned char *buf, unsigned int numBytes,
				struct sockaddr_in *fromAddr = NULL);

    SPxErrorCode GetInitialised(void) const { return m_isBoundOrConnected ? SPX_NO_ERROR : SPX_ERR_UNKNOWN; }

    SOCKET_TYPE GetSocket(void) { return m_socket; }

    /* Debug/log reporting. */
    void SetLogFile(FILE *logFile) { m_logFile = logFile; }

private:
    /*
     * Private variables.
     */
    /* Critical section. */
    mutable SPxCriticalSection m_mutex;
    SPxAtomic<int> m_changePending;

    /* Decode thread. */
    SPxThread *m_thread;
    SPxAtomic<int> m_threadRunning;

    unsigned char *m_buf;		/* Buffer for reading data */
    unsigned int m_bufSize;		/* Size of readBuf in bytes */
    unsigned int m_timeoutMsecs;	/* Time out for reading from socket. */

    /* Socket for receiving radar images etc. */
    int m_isTCP;			/* TCP or UDP? */
    SOCKET_TYPE m_socket;		/* Socket to receive via */
    struct sockaddr_in m_socketAddress;	/* Address structure */
    SPxAtomic<UINT32> m_addressCache;
    SPxAtomic<int> m_portCache;
    SPxAtomic<UINT32> m_ifAddressCache;
    SPxAtomic<char*> m_addressString;	/* String with dotted-IP address */
    SPxAtomic<char*> m_ifAddrString;		/* Interface address for multicast, or NULL */
    struct sockaddr_in m_interfaceAddr;	/* Used if ifAddrString not NULL */
    int m_port;				/* Port to receive from */ 
    SPxAtomic<int> m_isBoundOrConnected;
    
    /* Debug/log reporting. */
    FILE *m_logFile;			/* NULL or destination for debug */

    /*
     * Private functions.
     */
    static void* decodeThreadWrapper(SPxThread *thread);
    void* decodeThreadFn(SPxThread *thread);
    SPxErrorCode updateAddress(void);
    int threadReadDataFn(unsigned char *buf,
			 unsigned int bufSizeBytes,
			 unsigned int timeoutMsecs,
			 sockaddr_in *from);

}; /* SPxPacketDecoderNet */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* SPX_PACKET_DECODER_NET_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
