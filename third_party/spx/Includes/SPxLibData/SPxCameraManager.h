/*********************************************************************
 *
 * (c) Copyright 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxCameraManager.h,v $
 * ID: $Id: SPxCameraManager.h,v 1.4 2017/02/21 15:15:14 rew Exp $
 *
 * Purpose:
 *   SPx camera manager class header.
 *
 * Revision Control:
 *   21/02/17 v1.4    AGC	Support camera information retrieval.
 *
 * Previous Changes:
 *   24/01/17 1.3    AGC	Improvements to maintain command interval. 
 *   23/01/17 1.2    AGC	Modifications for testing.
 *   18/01/17 1.1    AGC	Initial version.
 *
 *********************************************************************/

#ifndef SPX_CAMERA_MANAGER_H_
#define SPX_CAMERA_MANAGER_H_

/* Other headers required. */
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibNet/SPxNetTCP.h"

/* Forward declare any classes required. */
class SPxNetTCP;
class SPxSerialPort;
class SPxThread;
class SPxCamCtrlPelco;

class SPxCameraManager : public SPxObj
{
public:
    typedef void(*CameraFn)(UINT8 camIndex, void *userArg);

    /* Constructor and destructor. */
    SPxCameraManager(void);
    virtual ~SPxCameraManager(void);

    /* Set/get network port for receiving messages. */
    SPxErrorCode SetPortNumber(int port);
    UINT16 GetPortNumber(void) const;

    /* Set timeout for releasing control of camera. */
    SPxErrorCode SetControlTimeoutMS(unsigned int timeoutMS);
    unsigned int GetControlTimeoutMS(void) const;

    /* Set interval for summary debug messages. */
    SPxErrorCode SetSummaryIntervalMS(unsigned int intervalMS);
    unsigned int GetSummaryIntervalMS(void) const;

    /* Add/remove cameras. */
    SPxErrorCode AddCamera(UINT8 camIndex);
    SPxErrorCode RemoveCamera(UINT8 camIndex);

    /* Get camera information. */
    unsigned int GetNumCameras(void) const;
    SPxErrorCode IterateCameras(CameraFn fn, void *userArg) const;
    SPxCamCtrlPelco *GetCameraControl(UINT8 camIndex);

    /* Status messages and updates. */
    SPxErrorCode CheckStatus(void);
    unsigned int GetNumTotalClients(void) const;
    unsigned int GetNumClients(UINT8 camIndex) const;
    
    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

    void SetVerbosity(unsigned int verbosity);
    void SetLogFile(std::FILE *logFile);

protected:
    /* Protected functions that may be called by derived testing classes. */
    explicit SPxCameraManager(SPxNetTCP *netTCP);
    SPxErrorCode AddCamera(UINT8 camIndex, SPxCamCtrlPelco *camCtrl);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    
    /* Private functions. */
    void initialise(void);

    SPxErrorCode addCamera(UINT8 camIndex, SPxCamCtrlPelco *camCtrl);

    void handleData(const unsigned char* buffer,
		    unsigned int bufLen,
		    UINT32 fromAddr,
		    SOCKET_TYPE sock);
    
    unsigned int handleMesg(const unsigned char *data,
			    unsigned int numBytes,
			    UINT32 fromAddr,
			    SOCKET_TYPE sock);

    void handlePelcoDMesg(const unsigned char *data,
                          unsigned int numBytes,
			  UINT32 fromAddr,
			  SOCKET_TYPE sock);

    void handlePelcoDExtMesg(const unsigned char *data,
                             unsigned int numBytes,
			     UINT32 fromAddr,
			     SOCKET_TYPE sock);

    void handlePelcoDQueryMesg(const unsigned char *data,
			       unsigned int numBytes,
			       UINT32 fromAddr,
			       SOCKET_TYPE sock);

    void sendPelcoDGenResponse(UINT8 addr, SOCKET_TYPE sock);

    void sendPelcoDExtResponse(UINT8 addr, 
                               UINT16 opcode, 
                               UINT16 value,
			       SOCKET_TYPE sock);

    void sendResponse(const unsigned char *data,
		      unsigned int numBytes,
		      SOCKET_TYPE sock);
    
    int canConnectionHaveControl(UINT8 camAddress,
				 UINT32 fromAddr,
				 SOCKET_TYPE sock) const;
    void connectionTakeControl(UINT8 camAddress,
			       UINT32 fromAddr,
			       SOCKET_TYPE sock);
    
    void netDisconnect(SPxNetTCP *tcp, SOCKET_TYPE sock);

    void outputSummary(void) const;
    
    void threadFn(SPxThread *thread, UINT8 camIndex);

    /*
     * Private static functions.
     */
    
    static void netMesgHandler(SPxNetTCP *tcp,
			       SPxErrorCode errorCode,
			       SPxNetTCPAction action,
			       void* userArg,
			       const unsigned char* buffer,
			       unsigned int bufLen,
			       UINT32 fromAddr,
			       SOCKET_TYPE sock);

    static void netDisconnectHandler(SPxNetTCP *tcp,
				     void *userArg,
				     SOCKET_TYPE sock);

    static int isPelcoDExtQueryCommand(UINT16 command);

    static void *threadFnWrapper(SPxThread *thread);

}; /* SPxCameraManager */

#endif /* SPX_CAMERA_MANAGER_H_ */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
