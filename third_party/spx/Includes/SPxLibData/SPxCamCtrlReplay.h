/*********************************************************************
*
* (c) Copyright 2015, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCamCtrlReplay.h,v $
* ID: $Id: SPxCamCtrlReplay.h,v 1.4 2016/04/08 13:24:23 rew Exp $
*
* Purpose:
*	Header for SPxCamCtrlReplay class - a class used to
*	replay recorded camera state.
*
* Revision Control:
*   08/04/16 v1.4    AGC	Add new forms of GetSerialName/Baud().
*
* Previous Changes:
*   26/11/15 1.3    AGC	Support camera speed replay.
*   09/11/15 1.2    AGC	Override base class serial functions.
*   03/11/15 1.1    AGC	Initial version.
*
**********************************************************************/
#ifndef SPX_CAM_CTRL_REPLAY_H_
#define SPX_CAM_CTRL_REPLAY_H_

/* For base class. */
#include "SPxLibData/SPxCamCtrlBase.h"

/* Forward declarations. */
class SPxRadarReplay;
class SPxPacketDecoder;

class SPxCamCtrlReplay : public SPxCamCtrlBase
{
public:
    /* Construction and destruction. */
    SPxCamCtrlReplay(void);
    virtual ~SPxCamCtrlReplay(void);

    virtual SPxErrorCode SetSerialName(const char *name);
    virtual SPxErrorCode GetSerialName(char *name, unsigned int bufLen) const;
    virtual const char *GetSerialName(void) const;
    virtual SPxErrorCode SetSerialBaud(unsigned int baud);
    virtual SPxErrorCode GetSerialBaud(unsigned int *baud) const;
    virtual unsigned int GetSerialBaud(void) const;

    virtual int IsAbsPanSupported(void) const { return TRUE; }
    virtual SPxErrorCode SetPan(float /*panAngleDegs*/) { return SPX_NO_ERROR; }
    virtual SPxErrorCode GetPan(float *panAngleDegs) const;

    virtual int IsAbsTiltSupported(void) const { return TRUE; }
    virtual SPxErrorCode SetTilt(float /*tiltAngleDegs*/) { return SPX_NO_ERROR; }
    virtual SPxErrorCode GetTilt(float *tiltAngleDegs) const;

    virtual int IsAbsZoomSupported(void) const { return TRUE; }
    virtual SPxErrorCode SetZoom(float /*fovDegs*/) { return SPX_NO_ERROR; }
    virtual SPxErrorCode GetZoom(float *fovDegs) const;
    virtual SPxErrorCode SetZoomRange(float minFovDegs, float maxFovDegs);
    virtual SPxErrorCode GetZoomRange(float *minFovDegs, float *maxFovDegs) const;

    float GetPanSpeedDegsPerSec(void) const;
    float GetTiltSpeedDegsPerSec(void) const;

    /* Input from file replay. */
    SPxErrorCode AddReplaySource(UCHAR channelIndex, SPxRadarReplay *replaySource);
    SPxErrorCode RemoveReplaySource(SPxRadarReplay *replaySource);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Private functions. */
    static void camPosFnWrapper(SPxPacketDecoder *pktDecoder, void *userArg, 
	UINT32 packetType, struct sockaddr_in *from, 
	struct SPxTime_tag *time, const unsigned char *payload, 
	    unsigned int numBytes);
    void camPosFn(const unsigned char *payload, unsigned int numBytes);
    static int replaySourceDeleteFn(void *invokingObject,
				    void *userObject,
				    void *arg);

}; /* SPxCamCtrlReplay */

#endif /* SPX_CAM_CTRL_REPLAY_H_ */

/*********************************************************************
*
*   End of File
*
**********************************************************************/
