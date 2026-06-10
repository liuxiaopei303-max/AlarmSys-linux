/*********************************************************************
 *
 * (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxChannelNetRaw.h,v $
 * ID: $Id: SPxChannelNetRaw.h,v 1.7 2017/08/03 13:42:43 rew Exp $
 *
 * Purpose:
 *   SPxChannelNetRaw class header.
 *
 * Revision Control:
 *   03/08/17 v1.7   SP 	Override SetSaveUpdateOnly().
 *
 * Previous Changes:
 *   12/04/17 1.6   AGC	Correct SPxThread forward declaration.
 *   22/01/16 1.5   SP 	Add fileID constructor arg.
 *   03/11/15 1.4   AGC	Packet decoder callback data now const.
 *   15/06/15 1.3   SP  Don't accept connections when disabled.
 *                      Release port for replay channels if no data.
 *   26/03/15 1.2   SP  Support output selection.
 *   06/03/15 1.1   SP  Initial version.
 *
 *********************************************************************/

#ifndef _SPX_CHANNEL_NET_RAW_H
#define _SPX_CHANNEL_NET_RAW_H

/* Other headers required. */
#include "SPxLibData/SPxChannel.h"
#include "SPxLibData/SPxCompress.h"

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
class SPxThread;

class SPxChannelNetRaw : public SPxChannel
{
public:

    /*
     * Public types.
     */

    /*
     * Public variables.
     */

    /*
     * Public functions.
     */

    /* Constructor and destructor. */
    SPxChannelNetRaw(SPxChannelDatabase *database, UINT8 id, UINT8 fileID);
    SPxErrorCode Create(Media_t inputMedia, Media_t outputMedias);
    virtual ~SPxChannelNetRaw(void);

    /* Live input config functions. */
    virtual void GetLiveInputInfo(char *buf, 
                                  unsigned int bufSizeBytes) const;
    virtual int IsLiveInputAvail(void) const;

    /* Output config functions. */
    virtual int IsOutputMediaAvail(Media_t media) const;
    virtual SPxErrorCode SetOutputEnabled(unsigned int index, int isEnabled);
    virtual int IsOutputEnabled(unsigned int index) const;
    virtual void GetOutputInfo(unsigned int index,
                               char *buf, 
                               unsigned int bufSizeBytes) const;
    virtual SPxNetAddr *GetOutputNetCtrlIf(unsigned int index);

    /* SPxNetAddr interface functions. */
    virtual SPxErrorCode SetAddress(const char *address, int port=0, 
                                    const char *ifAddr=NULL);
    virtual SPxErrorCode SetAddress(UINT32 address, int port=0, 
                                    const char *ifAddr=NULL);
    virtual UINT32 GetAddress(void) const;
    virtual const char *GetAddressString(void) const;
    virtual int GetPort(void) const;
    virtual UINT32 GetIfAddress(void) const;
    virtual const char *GetIfAddressString(void) const;
    
    /* Get packet sender. */
    virtual SPxPacketSender *GetPacketSender(void) { return m_packetSender; }

    /* Config load/save. */
    virtual SPxErrorCode SetStateFromConfig(void);
    virtual SPxErrorCode SetConfigFromState(void);
    virtual void SetSaveUpdateOnly(int updateOnly);

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Live input config functions. */
    virtual SPxErrorCode SetLiveInputEnabled(int isEnabled);
    virtual int IsLiveInputEnabled(void) const;

    /* Check and get channel status. */
    virtual SPxStatus_t CheckStatus(char *mesgBuf, unsigned int mesgBufLen);

    /* Replay packet handler. */
    virtual void HandleReplayPacket(UINT16 packetType,
                                    const unsigned char *data,
                                    unsigned int dataSizeBytes);

    /* Background processing function called from thread. */
    virtual UINT32 doBackgroundProcessing(void);

    /* Parameter handling. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private variables.
     */

    /* General. */
    int m_isLiveInputEnabled;            /* Is input enabled? */
    int m_isTCP;                         /* TRUE if TCP, FALSE if UDP. */
    
    /* TCP/UDP Input. */
    SPxCriticalSection m_srcMutex;       /* Source socket mutex. */
    mutable char m_srcAddrString[16];    /* Current input address. */
    mutable char m_srcIfAddrString[16];  /* Current input ifaddrs. */
    mutable int m_srcPort;               /* Current input port. */
    SOCKET_TYPE m_srcSock;		 /* Handle of socket */
    struct sockaddr_in m_srcSockAddr;	 /* Socket address structure */
    struct sockaddr_in m_srcFromAddr;	 /* Remote address structure */
    struct sockaddr_in m_srcIfAddr;	 /* Interface address structure */
    int m_isSrcBoundOrConnected;         /* Is socket bound or connected? */
    int m_isSrcMcastJoined;              /* Is UDP multicast group joined. */
    UINT8 *m_srcBuf;                     /* Buffer to read into. */
    unsigned int m_srcBufSizeBytes;      /* Buffer size. */
    SPxThread *m_srcThread;              /* Thread used to receive data. */
    int m_isUserOutputEnabled;           /* Has user enabled the output? */
   
    /* TCP/UDP output. */
    SPxPacketSender *m_packetSender;     /* Packet sender.*/

    /* Compression. */
    UINT8 *m_compBuf;                    /* Buffer to use for compression. */
    unsigned int m_compBufSizeBytes;     /* Buffer size. */
    SPxCompress m_compress;              /* Compression object. */
    
    /*
     * Private functions.
     */

    void doInputProcessing(void);
    void checkInputSocket(void);
    void joinOrLeaveMcast(int join);
    SPxErrorCode updateInputSocketAddress(const char *addr,
                                          int port,
                                          const char *ifAddr);
    int readInputSocket(void);
    void recordData(const UINT8 *data, unsigned int numBytes);
    void outputData(const UINT8 *data, unsigned int numBytes);

    /*
     * Private static functions.
     */

    static void *inputThreadHandler(SPxThread *thread);

}; /* SPxChannelNetRaw */

#endif /* _SPX_CHANNEL_NET_RAW_H */

/*********************************************************************
 *
 *      End of file
 *
 *********************************************************************/
