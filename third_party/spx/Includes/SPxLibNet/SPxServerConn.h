#pragma once


/*
* Other headers required.
*/
/* For base class. */
#include "SPxLibUtils/SPxObj.h"
class SPxServerInterface;
class SPxThread;


class SPxServerConn :public SPxObj
{
public:
	SPxServerConn(int serverPort,
		const char *heartbeatAddress,
		int appId,
		int isKeepAliveEnabled);
	virtual ~SPxServerConn();
	int GetParameter(char *parameterName, char *valueBuf, int bufLen);
private:
	int m_serverPort;				/* Port number for server */
	char *m_heartbeatAddressString;		/* Address as a string */
	UINT32 m_lastHeartbeatTime;
	SPxServerInterface* m_ServerInterface;
	SPxThread *m_thread;
private:
	static void* StartAddress(SPxThread *thread);
	/* Thread entry point, not for calling by users. */
	void *ThreadFn(SPxThread *thread);
	static void SendHeartbeat(void * obj);
};


