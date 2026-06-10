/*********************************************************************
*
* (c) Copyright 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCameraFake.h,v $
* ID: $Id: SPxCameraFake.h,v 1.2 2017/02/22 12:26:36 rew Exp $
*
* Purpose:
*   Header for SPxCameraFake class.
*
*   This object may be communicated with Pelco-D messages over a
*   TCP socket, and will respond appropriately.
*
* Revision Control:
*   22/02/17 v1.2    AGC	Rename Set/GetPort() to Set/GetPortNumber().
*
* Previous Changes:
*   23/01/17 1.1    AGC	Initial version.
**********************************************************************/

#ifndef SPX_CAMERA_FAKE_H_
#define SPX_CAMERA_FAKE_H_

/* Library headers. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibNet/SPxNetTCP.h"

/* Forward declare any other classes we need. */
class SPxNetTCP;

class SPxCameraFake : public SPxObj
{
public:
    SPxCameraFake(void);
    virtual ~SPxCameraFake(void);

    SPxErrorCode SetPortNumber(int port);
    int GetPortNumber(void) const;

    SPxErrorCode SetAbsFocusSupported(int enable);
    int GetAbsFocusSupported(void) const;

    void SetPan(float panDegs);
    void SetTilt(float tiltDegs);
    void SetZoom(float zoomPos);
    void SetFocus(float focusPos);

protected:
    /* Protected functions for testing. */
    explicit SPxCameraFake(SPxNetTCP *tcp);

private:
    struct impl;
    SPxAutoPtr<impl> m_p;

    static void netMesgHandler(SPxNetTCP *tcp,
			       SPxErrorCode errorCode,
			       SPxNetTCPAction action,
			       void* userArg,
			       const unsigned char* buffer,
			       unsigned int bufLen,
			       UINT32 fromAddr, SOCKET_TYPE sock);
    void handleData(SOCKET_TYPE sock,
		    const unsigned char* buffer,
		    unsigned int bufLen);
    unsigned int handleMesg(SOCKET_TYPE sock,
			    const unsigned char* buffer,
			    unsigned int bufLen);
    void handlePelcoDMesg(SOCKET_TYPE sock,
			  const unsigned char* buffer,
			  unsigned int bufLen);
    void handlePelcoDExtMesg(SOCKET_TYPE sock,
			     const unsigned char* buffer,
			     unsigned int bufLen);
    void sendPelcoDGenResponse(SOCKET_TYPE sock,
			       UINT8 camAddress);
    void sendPelcoDExtResponse(SOCKET_TYPE sock,
			       UINT8 camAddress,
			       UINT16 opcode,
			       UINT16 value);
    void sendResponse(SOCKET_TYPE sock,
		      const unsigned char *data,
		      unsigned int numBytes);
    void timerFn(void);

}; /* SPxCameraFake */

#endif /* SPX_CAMERA_FAKE_H_ */

/*********************************************************************
*
* End of file
*
**********************************************************************/
