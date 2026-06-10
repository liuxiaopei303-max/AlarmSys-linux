/*********************************************************************
*
* (c) Copyright 2013 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxServerBase.h,v $
* ID: $Id: SPxServerBase.h,v 1.9 2016/02/08 14:38:35 rew Exp $
*
* Purpose:
*	Header for SPxServerBase object which encapsulates a
*	server with heartbeats and a remote parameter interface.
*
*
* Revision Control:
*   08/02/16 v1.9    AGC	Support for disabling client connections.
*
* Previous Changes:
*   21/09/15 1.8    AGC	Add GetServer().
*   26/06/14 1.7    AGC	Add GetAppBit().
*   20/06/14 1.6    AGC	Add HasLicenseExpired().
*   01/05/14 1.5    AGC	Add IsGracePeriodOver().
*   10/04/14 1.4    AGC	Support setting title.
*   13/12/13 1.3    AGC	Add grace period option to GetLicenseCaption().
*   08/11/13 1.2    AGC	Add Create() function.
*   21/10/13 1.1    AGC	Initial version.
**********************************************************************/
#ifndef _SPX_SERVER_H
#define _SPX_SERVER_H

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPX_SCNET_HB_ID_SERVER. */
#include "SPxLibSc/SPxScNet.h"

/* For base class. */
#include "SPxLibUtils/SPxObj.h"

/* Forward declarations. */
class SPxServerInterface;

/*
 * Types
 */

class SPxServerBase : public SPxObj
{
public:
    explicit SPxServerBase(const char *title, UINT32 projectCode,
	int port=4377, int id=SPX_SCNET_HB_ID_SERVER, UINT32 appBit=0,
	int controlAvailable=TRUE);
    virtual ~SPxServerBase(void);
    SPxErrorCode Create(void);
    SPxErrorCode Shutdown(void);

    const char *GetTitle(void) const;
    UINT32 GetProjectCode(void) const;
    int IsControlAvailable(void) const;
    int GetPort(void) const;
    int GetAppBit(void) const;
    int IsLicensed(void) const;
    virtual int IsGracePeriodOver(void) const;
    int HasLicenseExpired(void) const;
    SPxErrorCode GetLicenseDesc(char *buffer, unsigned int numBytes) const;
    SPxErrorCode GetLicenseCaption(char *buffer, unsigned int numBytes,
	UINT32 gracePeriodSecs=3600) const;
    SPxServerInterface *GetServer(void);

    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

private:
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Derived classes may override this function. */
    virtual void background(void);

    /* Private functions. */
    static void backgroundWrapper(void *userData);
    void backgroundInternal(void);
    static void timerWrapper(void *userArg);
    void timer(void);
};

#endif /* _SPX_SERVER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
