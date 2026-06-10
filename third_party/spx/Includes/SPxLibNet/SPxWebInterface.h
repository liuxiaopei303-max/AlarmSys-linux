/*********************************************************************
 *
 * (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxWebInterface.h,v $
 * ID: $Id: SPxWebInterface.h,v 1.10 2017/06/06 16:03:55 rew Exp $
 *
 * Purpose:
 *   Used to add webserver and websocket functionality to an
 *   application, typically to implement a web interface.
 *
 * Revision Control:
 *   06/06/17 v1.10   AGC	Allow user arg for AddProtocol().
 *				Add RemoveProtocol().
 *
 * Previous Changes:
 *   08/03/17 1.9   AGC	Improve mutex locking.
 *   23/09/16 1.8   AJH	Add Shutdown().
 *   23/08/16 1.7   AGC	Fix lock-ups under Linux.
 *   11/05/16 1.6   AGC	Update to match changes in new libwebsockets version.
 *   20/04/16 1.5   AGC	Support adding protocols.
 *   12/02/16 1.4   SP 	Add Set/GetPortNumber().
 *   21/09/15 1.3   AGC	Forward declare SPxThread.
 *   26/08/15 1.2   SP  Many changes.
 *   29/07/15 1.1   SP  Initial version.
 *
 *********************************************************************/

#ifndef _SPX_WEB_INTERFACE_H
#define _SPX_WEB_INTERFACE_H

/* Library headers. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxError.h"

/* Forward declare any classed required. */
class SPxScSourceNet;
class SPxThread;

/* Forward declare data types required so that we do not need to
 * include the libwebsockets.h header file here. Note that we cannot
 * forward declare libwebsocket_callback_reasons here as it is an
 * enum. Instead we use int for this argument can cast it within
 * the function.
 */
struct lws_context;
struct lws;

/* SPxWebInterface dialog */
class SPxWebInterface : public SPxObj
{
public:
    typedef int (*ProtocolHandler)(lws *wsi,
				   int reason,
				   void *user, void *in,
				   size_t len);


    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Construction and destruction. */
    SPxWebInterface(void);
    SPxErrorCode AddProtocol(const char *name, ProtocolHandler fn,
			     unsigned int perSessionDataSize,
			     void *userArg);
    SPxErrorCode RemoveProtocol(const char *name);
    virtual SPxErrorCode Create(unsigned int port=8080);
    virtual ~SPxWebInterface(void);
    void Shutdown(void);

    /* Set/get application name. */
    virtual SPxErrorCode SetApplicationName(const char *name);
    virtual const char *GetApplicationName(void) { return m_appName ? m_appName : ""; }

    /* Set/get server interface to handle command messages. */
    virtual SPxErrorCode SetServerInterface(SPxScSourceNet *svrIf);
    virtual SPxScSourceNet *GetServerInterface(void) { return m_svrIf; }

    /* Set/get web server port. */
    virtual SPxErrorCode SetPortNumber(unsigned int port);
    virtual unsigned int GetPortNumber(void) { return m_port; }

protected:
    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Optionally override these functions in a derived class
     * to add functionality to the web server.
     */

    /* Called to handle an HTTP request. */
    virtual int httpHandler(lws *wsi,
                            int reason,
                            void *user, void *in,
                            size_t len);

    /* Called from get an HTTP item, e.g. a webpage or image. */
    virtual unsigned int getHttp(const char *urlExt,
                                 char *buf,
                                 unsigned int bufSize,
                                 char *mime,
                                 unsigned int mimeSize);

    /* Called from getHttp() to get text. */
    unsigned int getText(const char *text,
                         const char *fileExt,
                         char *buf,
                         unsigned int bufSize,
                         char *mime,
                         unsigned int mimeSize);

    /* Called from getHttp() to get binary data, e.g. an image. */
    unsigned int getData(const UINT8 *data,
                         unsigned int dataSize,
                         const char *fileExt,
                         char *buf,
                         unsigned int bufSize,
                         char *mime,
                         unsigned int mimeSize);

    /* Use this function to get the MIME type. */
    unsigned int getMime(const char *fileExt,
                         char *mime,
                         unsigned int mimeSize);

    /* Called from getHttp() to get the default web page. */
    virtual unsigned int getDefaultPage(const char *urlExt,
                                        char *buf,
                                        unsigned int bufSize,
                                        char *mime,
                                        unsigned int mimeSize);

    /* Called to handle an SPx command message. */
    virtual int commandHandler(lws *wsi,
                               int reason,
                               void *user, void *in,
                               size_t len);

    /* Called to handle a single command message. */
    virtual void handleCommand(const char *cmd, 
                               char *replyBuf, 
                               unsigned int replyBufSize);

    /* Parameter set/get. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private types.
     */

    struct impl;

    /*
     * Private variables.
     */

    /* General. */
    SPxAutoPtr<impl> m_p;
    SPxThread *m_thread;                    /* Web server thread. */
    unsigned int m_pollTimeoutMsecs;        /* Timeout when polling for messages. */
    unsigned int m_port;                    /* Port number to listen on. */
    unsigned int m_cmdBufSize;              /* Command buffer size. */
    SPxAutoPtr<char[]> m_cmdBuf;            /* Command buffer. */
    SPxScSourceNet *m_svrIf;                /* Server interface object. */
    char *m_appName;			    /* Application name. */
    SPxCriticalSection m_mutex;             /* Object mutex. */
    SPxAtomic<int> m_stateChanged;

    /*
     * Private functions.
     */

    unsigned int getFile(const char *urlExt,
                         char *buf,
                         unsigned int bufSize,
                         char *mime,
                         unsigned int mimeSize);
    
    /*
     * Private static functions.
     */

    static void *webServerHandler(SPxThread *thread);
    static int httpHandlerWrapper(lws *wsi,
                                  int reason,
                                  void *user, void *in,
                                  size_t len);
    static int commandHandlerWrapper(lws *wsi,
                                     int reason,
                                     void *user, void *in,
                                     size_t len);
};

#endif /* _SPX_WEB_INTERFACE_H */

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
