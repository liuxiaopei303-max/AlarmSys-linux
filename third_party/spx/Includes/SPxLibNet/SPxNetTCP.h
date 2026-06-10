/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNetTCP.h,v $
* ID: $Id: SPxNetTCP.h,v 1.16 2017/02/22 12:25:55 rew Exp $
*
* Purpose:
*	Header of SPxNetTCP class - An class for handling a TCP
*	connection with in-built asynchronous support.
*
* Revision Control:
*   22/02/17 v1.16   AGC	Add SetPortNumber() function.
*
* Previous Changes:
*   23/01/17 1.15   AGC	Add more functions for testing.
*   18/01/17 1.14   AGC	Support continue of existing Connect().
*   11/01/17 1.13   AGC	Support client disconnect callback.
*   12/12/16 1.12   AGC	Add functions for testing.
*   07/10/16 1.11   AGC	Support changing the port.
*   23/09/16 1.10   AGC	Support OS assigned TCP port.
*   20/04/16 1.9    AGC	Support reporting all connection failures.
*   17/02/16 1.8    AJH	Add sock to completion fn parameters.
*   11/02/16 1.7    AJH	Renamed to SPxNetTCP; removed std headers.
*   20/01/16 1.6    AGC	Support receiving minimum number of bytes.
*   22/10/15 1.5    AGC	Support for handling multiple clients.
*   06/10/15 1.4    AGC	Support callback function received data.
*   19/07/13 1.3    AGC	Fix issues under Windows.
*			Support retrieve locally bound address.
*   25/04/13 1.2    AGC	Support synchronous commands.
*			Improve error reporting from Connect failures.
*   02/08/12 1.1    AGC	Initial Version.
**********************************************************************/
#ifndef _SPX_NET_TCP_H
#define _SPX_NET_TCP_H

/*
 * Other headers required.
 */

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SOCKET_TYPE. */
#include "SPxLibUtils/SPxCommon.h"

/* For SPX_SOCK_INVALID. */
#include "SPxLibSc/SPxScNet.h"

/* For base class. */
#include "SPxLibUtils/SPxObj.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward declarations. */
class SPxNetTCP;
struct SPxNetTCPActionData;
class SPxThread;
struct timeval;

/* Supported actions. */
enum SPxNetTCPAction
{
    SPX_NET_TCP_ACTION_NONE=0,
    SPX_NET_TCP_ACTION_CONNECT=1,
    SPX_NET_TCP_ACTION_DISCONNECT=2,
    SPX_NET_TCP_ACTION_SEND=3,
    SPX_NET_TCP_ACTION_RECEIVE=4,
    SPX_NET_TCP_ACTION_SET_PORT=5
};

typedef void (*SPxNetTCPCompletionFn)(SPxNetTCP *tcp,
				      SPxErrorCode errorCode,
				      SPxNetTCPAction action,
				      void* userArg,
				      const unsigned char* buffer,
				      unsigned int bufLen,
				      UINT32 fromAddr,
				      SOCKET_TYPE sock);
typedef void (*SPxNetTCPDisconnectFn)(SPxNetTCP *tcp,
				      void *userArg,
				      SOCKET_TYPE sock);

/*
 * Define our class.
 */
class SPxNetTCP : public SPxObj
{
public:
    /* Public functions. */
    explicit SPxNetTCP(int supportAsync=TRUE, int port=0);
    virtual ~SPxNetTCP(void);
    void Stop(void);
    void SetPort(int port);
    void SetPortNumber(int port) { SetPort(port); }
    UINT16 GetPort(void) const;
    void Connect(SPxNetTCPCompletionFn fn, void *userArg,
		 const char *addr, UINT16 port, const char *ifAddr=NULL,
		 int reportAllFailures=FALSE, int allowRetry=FALSE);
    SPxErrorCode Connect(const char *addr, UINT16 port, const char *ifAddr=NULL, int retry=TRUE);
    void Disconnect(SPxNetTCPCompletionFn fn, void *userArg);
    SPxErrorCode Disconnect(void);
    int IsConnected(void) const;
    UINT32 GetIfAddr(void) const;
    void Send(SPxNetTCPCompletionFn fn, void *userArg,
	      const unsigned char *buffer, int bufLen,
	      SOCKET_TYPE sock=SPX_SOCK_INVALID);
    SPxErrorCode Send(const unsigned char *buffer, int bufLen,
		      SOCKET_TYPE sock=SPX_SOCK_INVALID);
    void Recv(SPxNetTCPCompletionFn fn, void *userArg,
	      UINT32 maxNumBytes, UINT32 timeoutMsecs, int recvAll=FALSE,
	      const unsigned char *recvdBuf=NULL, unsigned int recvdBytes=0);
    int Recv(unsigned char *recvBuffer, int recvBufLen, UINT32 timeoutMsecs);
    void SendRecv(SPxNetTCPCompletionFn fn, void *userArg,
		  const unsigned char *buffer, int bufLen,
		  UINT32 maxNumBytes, UINT32 timeoutMsecs);
    int SendRecv(const unsigned char *buffer, int bufLen,
		  unsigned char *recvBuffer, int recvBufLen,
		  UINT32 timeoutMsecs);

    SPxErrorCode SetDefaultRecv(SPxNetTCPCompletionFn fn, void *userArg,
				UINT32 maxNumBytes);
    SPxErrorCode SetDisconnectHandler(SPxNetTCPDisconnectFn fn, void *userArg);

    int IsQueueEmpty(void) const;

protected:
    static const int SPX_FD_SETSIZE = 64;

    struct SPxFDSet
    {
	int fd_count;
	SOCKET_TYPE fd_array[SPX_FD_SETSIZE];
    };

    /* Network functions that may be overriden for testing. */
    virtual int connect(SOCKET_TYPE s, const sockaddr *name, int nameLen);
    virtual int select(SPxFDSet *readfds, timeval *timeout);
    virtual int send(SOCKET_TYPE s, const char *buf, int len, int flags);
    virtual int recv(SOCKET_TYPE s, char *buf, int len, int flags);
    virtual int listen(SOCKET_TYPE s, int backlog);
    virtual SOCKET_TYPE accept(SOCKET_TYPE s, sockaddr *addr, SOCKLEN_TYPE *addrLen);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Private functions. */
    static void *threadFnWrapper(SPxThread *thread);
    void *threadFn(SPxThread *thread);
    void closeServerSockets(void);
    void tryAccept(void);
    int getAction(SPxNetTCPAction& action, SPxNetTCPActionData& actionData);
    void execAction(SPxNetTCPAction& action, SPxNetTCPActionData& actionData);
    void handleConnect(SPxNetTCPActionData& action, int async = TRUE);
    void handleDisconnect(SPxNetTCPActionData& action);
    void handleSend(SPxNetTCPActionData& action);
    int handleReceive(SPxNetTCPActionData& action, int all=FALSE);
    void handleSetPort(SPxNetTCPActionData& action);
    void reportError(const SPxNetTCPActionData& action, SPxErrorCode errorCode);
    int readSock(SPxNetTCPActionData& action, SOCKET_TYPE sock);
    static void sendComplete(SPxNetTCP *tcp, SPxErrorCode errorCode,
	SPxNetTCPAction action, void* userArg, const unsigned char* buffer,
	unsigned int bufLen, UINT32 fromAddr, SOCKET_TYPE sock);

    static void FDZero(SPxFDSet *fds);
    static void FDSet(SOCKET_TYPE sock, SPxFDSet *fds);
    static int FDIsSet(SOCKET_TYPE sock, const SPxFDSet *fds);

}; /* SPxNetTCP */

#endif /* _SPX_NET_TCP_H */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
