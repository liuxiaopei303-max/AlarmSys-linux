/*********************************************************************
 *
 * (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxVidSimCameraMount.h,v $
 * ID: $Id: SPxVidSimCameraMount.h,v 1.12 2016/12/01 14:47:40 rew Exp $
 *
 * Purpose:
 *   SPx Video Simulator SPxVidSimCameraMount class header.
 *
 * Revision Control:
 *   31/10/16 v1.14  SP 	Support Z reference mode.
 *
 * Previous Changes:
 *   07/11/16 1.13  AJH Support presets.
 *   07/10/16 1.12  SP 	Move events to base class.
 *   18/05/16 1.11  AGC Support network camera control.
 *   04/11/15 1.10  SP  Simulate camera mount acceleration.
 *                      Rename slave to master.
 *   03/11/15 1.9   AGC Packet decoder callback data now const.
 *   04/09/15 1.8   SP  Return pan/tilt as read from master.
 *   26/08/15 1.5   SP  Add Create().
 *   06/08/15 1.4   SP  Support remote control.
 *   20/06/14 1.3   AGC Move C++ headers to source file.
 *   04/10/13 1.2   AGC Simplify headers.
 *   15/07/13 1.1   SP  Initial version.
 *
 *********************************************************************/

#ifndef _SPX_VID_SIM_CAMERA_MOUNT_H
#define _SPX_VID_SIM_CAMERA_MOUNT_H

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxVidSimMovable.h"
#include "SPxLibUtils/SPxVidSimCamera.h"
#include "SPxLibUtils/SPxTime.h"
#include "SPxLibData/SPxCamCtrlPelco.h"

/*********************************************************************
 *
 *  Constants
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Macros
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Type definitions
 *
 *********************************************************************/

/*********************************************************************
 *
 *   Class definitions
 *
 *********************************************************************/

/* Forward declare any classes required. */
class SPxVidSimPlatform;
class SPxVidSimScene;
class SPxTerrainDatabase;
class SPxSerialPort;
class SPxThread;

class SPxVidSimCameraMount : public SPxVidSimMovable
{
    /* Grant SPxVidSimPlatform private access. */
    friend class SPxVidSimPlatform;

public:

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxVidSimCameraMount(SPxVidSimPlatform *platform);
    virtual ~SPxVidSimCameraMount(void);
    virtual SPxErrorCode Initialise(void);

    /* Parent object. */
    virtual SPxVidSimPlatform *GetPlatform(void) { return m_platform; }

    /* Add a camera. */
    virtual SPxErrorCode AddCamera(SPxVidSimCamera *camera);

    /* Retrieve a camera. */
    virtual unsigned int GetNumCameras(void);
    virtual SPxVidSimCamera *GetCamera(unsigned int index);

    /* Camera mount control. */
    SPxErrorCode SetPitchDegs(double degs);
    SPxErrorCode SetPitchSpeed(double speed);
    SPxErrorCode SetYawDegs(double degs);
    SPxErrorCode SetYawSpeed(double speed);

    /* Camera mount position. */
    virtual SPxErrorCode SetZRefMode(ZRefMode_t mode);
    virtual ZRefMode_t GetZRefMode(void) { return m_zRefMode; }
    virtual double GetZMetres(int absoluteValue=TRUE);

    virtual void SetPreset(int preset) { m_preset = preset;  }
    virtual int GetPreset(void) { int preset = m_preset; m_preset = 0; return preset; }

    /* Configuration save and restore. */
    virtual SPxErrorCode SetStateFromConfig(void);
    virtual SPxErrorCode SetConfigFromState(void);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Set link to parent object. */
    virtual SPxErrorCode SetPlatformLink(SPxVidSimPlatform *platform);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Parent objects. */
    SPxVidSimPlatform *m_platform;
    SPxVidSimScene *m_scene;
    SPxTerrainDatabase *m_terrainDB;

    /* Camera control type. */
    SPxMesgIOType_t m_camCtrlType;

    /* Serial Pelco-D interface. */
    SPxSerialPort *m_serialPort;

    /* Network Pelco-D interface. */
    SPxNetTCP *m_tcp;

    /* Thread used to update camera position. */
    SPxThread *m_thread;

    /* General. */
    SPxCriticalSection m_mutex;
    ZRefMode_t m_zRefMode;

    /* Requested relative speed. */
    double m_relTiltSpeed;
    double m_relPanSpeed;
    double m_relZoomSpeed;

    /* Requested absolute position. */
    double m_absPanDegs;
    int m_isAbsPanSet;
    double m_absTiltDegs;
    int m_isAbsTiltSet;

    /* Maximum pan and tilt speed in degs per second. */
    double m_maxPanDegsPerSec;
    double m_maxTiltDegsPerSec;

    /* Maximum pan and tilt acceleration in degs per second per second. */
    double m_maxPanAccel;
    double m_maxTiltAccel;

    /* Last update times. */
    UINT32 m_lastPanUpdateMsecs;
    UINT32 m_lastTiltUpdateMsecs;

    /* Current pan and tilt speed. */
    double m_panDegsPerSec;
    double m_tiltDegsPerSec;

    /* Last preset. */
    int m_preset;

    /* Master camera controller. */
    SPxCamCtrlPelco *m_masterCam;

    /* Last master camera query positions. */
    double m_masterLastPanQueryDegs;
    double m_masterLastTiltQueryDegs;

    /* Last master camera query times. */
    UINT32 m_masterLastPanQueryMsecs;
    UINT32 m_masterLastTiltQueryMsecs;
    UINT32 m_masterLastZoomQueryMsecs;

    /* Flags set when master camera is stopped. */
    int m_masterIsPanStopped;
    int m_masterIsTiltStopped;
    
    /*
     * Private functions.
     */

    unsigned int handleMesg(SPxTime_t *time,
                            const unsigned char *data,
                            unsigned int numBytes,
			    SOCKET_TYPE sock);

    void handlePelcoDMesg(SPxTime_t *time,
                          const unsigned char *data,
                          unsigned int numBytes,
			  SOCKET_TYPE sock);

    void handlePelcoDExtMesg(SPxTime_t *time,
                             const unsigned char *data,
                             unsigned int numBytes,
			     SOCKET_TYPE sock);

    void sendPelcoDGenResponse(UINT8 addr, SOCKET_TYPE sock);

    void sendPelcoDExtResponse(UINT8 addr, 
                               UINT8 opcode, 
                               UINT16 value,
			       SOCKET_TYPE sock);

    void sendResponse(const unsigned char *data,
		      unsigned int numBytes,
		      SOCKET_TYPE sock);

    void updatePanTiltSpeed(void);
    void updatePanTiltPosition(void);
    double syncPanToMaster(void);
    double syncTiltToMaster(void);
    double syncZoomToMaster(void);
    void sendToMaster(const unsigned char *data, unsigned int numBytes);

    /*
     * Private static functions.
     */

    static unsigned int serialMesgHandler(SPxSerialPort *source,
                                          void *userArg,
                                          SPxTime_t *time,
                                          const unsigned char *data,
                                          unsigned int numBytes);

    static void netMesgHandler(SPxNetTCP *tcp,
			       SPxErrorCode errorCode,
			       SPxNetTCPAction action,
			       void* userArg,
			       const unsigned char* buffer,
			       unsigned int bufLen,
			       UINT32 fromAddr,
			       SOCKET_TYPE sock);

    static void *threadHandler(SPxThread *thread);

}; /* SPxVidSimCameraMount */

#endif /* _SPX_VID_SIM_CAMERA_MOUNT_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
