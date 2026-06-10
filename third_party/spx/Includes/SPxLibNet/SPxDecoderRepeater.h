/*********************************************************************
 *
 * (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxDecoderRepeater.h,v $
 * ID: $Id: SPxDecoderRepeater.h,v 1.20 2017/03/15 14:52:39 rew Exp $
 *
 * Purpose:
 *   Header for SPxDecoderRepeater class.
 *
 * Revision Control:
 *  15/03/17 v1.20   AGC	Improve behaviour with testing constructor.
 *
 * Previous Changes:
 *  27/09/16 1.19   AGC	Use atomics for src/dest type.
 *  04/08/16 1.18   SP 	Add SetLiveSrcEnabled().
 *  22/02/16 1.17   SP 	Add SetSrcTypeFixed().
 *  09/11/15 1.16   AGC	Add constructor overload for testing.
 *  03/11/15 1.15   AGC	Packet decoder callback data now const.
 *  23/06/15 1.14   AGC	Support TCP send/receive.
 *			Support device manufacturers.
 *  11/05/15 1.13   AGC	Support manual input of data.
 *  26/03/15 1.12   SP 	Make SetHandler() protected.
 *                      Add SetDirectHandler() for public use.
 *  26/01/15 1.11   SP 	Add GetPacketSenderNet().
 *  23/12/13 1.10   REW	Add GetPacketDecoderNet().
 *  03/05/13 1.9    AGC	Add GetSrcNetAddrString().
 *  19/04/13 1.8    REW	Support asterix derived class.
 *  20/03/13 1.7    SP 	Fix file replay not working when serial src selected. 
 *  25/01/13 1.6    AGC	Support raw network input/output. 
 *  14/01/13 1.5    AGC	Allow receipt of 2 packet types.
 *  02/08/12 1.4    AGC	Forward declare SPxRunProcess.
 *  23/12/11 1.3    REW	Add GetSerialPort().
 *  13/09/11 1.2    AGC	SPxSerialRecv renamed to SPxSerialPort.
 *  09/08/11 1.1    SP	Initial Version.
 **********************************************************************/

#ifndef _SPX_DECODER_REPEATER_H
#define _SPX_DECODER_REPEATER_H

/*
 * Other headers required.
 */

/* We need the various decoders and sources that we use. */
#include "SPxLibNet/SPxPacketSend.h"
#include "SPxLibNet/SPxPacketDecoderNet.h"
#include "SPxLibData/SPxSerialPort.h"
#include "SPxLibData/SPxRadarReplay.h"

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

/* Forward declare our class so it can be used below */
class SPxDecoderRepeater;
class SPxRunProcess;

/* Source type for messages */
typedef enum
{
    SPX_MESG_IO_TYPE_DISABLED = 0,
    SPX_MESG_IO_TYPE_NETWORK = 1,
    SPX_MESG_IO_TYPE_SERIAL = 2,
    SPX_MESG_IO_TYPE_NETWORK_RAW = 3

} SPxMesgIOType_t;

/*
 * Define our class.
 */
class SPxDecoderRepeater : public SPxObj
{
public:

    /*
     * Public functions.
     */

    /* Create and destroy */
    SPxDecoderRepeater(void);
    virtual ~SPxDecoderRepeater(void);
    virtual SPxErrorCode Create(UINT32 packetType, UINT32 packetType2=0);

    /* Handler function */
    virtual SPxErrorCode SetDirectHandler(SPxPacketDecoderFn_t fn, 
                                          void *userArg);

    /* Source control. */
    virtual SPxErrorCode SetSrcType(SPxMesgIOType_t srcType);
    virtual SPxMesgIOType_t GetSrcType(void) { return static_cast<SPxMesgIOType_t>(static_cast<int>(m_srcType)); }
    virtual void SetSrcTypeFixed(int isFixed);
    virtual SPxErrorCode SetSrcSerial(const char *name=NULL,
                                      unsigned int baud=4800);
    virtual const char *GetSrcSerialName(void);
    virtual unsigned int GetSrcSerialBaud(void);
    virtual SPxErrorCode SetSrcSerialManufacturer(SPxDeviceManufacturer_t manufacturer);
    virtual SPxDeviceManufacturer_t GetSrcSerialManufacturer(void);
    virtual SPxErrorCode SetSrcNetAddr(const char *addr,
                                       int port=0,
                                       const char *ifAddr=NULL);
    virtual SPxErrorCode SetSrcNetAddr(UINT32 addr,
                                       int port=0,
                                       const char *ifAddr=NULL);
    virtual SPxErrorCode SetSrcNetPort(int port);
    virtual SPxErrorCode SetSrcNetManufacturer(SPxDeviceManufacturer_t manufacturer);
    virtual SPxDeviceManufacturer_t GetSrcNetManufacturer(void);
    virtual SPxErrorCode SetSrcNetTCP(int isTCP);
    virtual UINT32 GetSrcNetAddr(void);
    virtual const char *GetSrcNetAddrString(void);
    virtual int GetSrcNetPort(void);
    virtual int IsSrcNetTCP(void) const;
    virtual SPxErrorCode SetLiveSrcEnabled(int isEnabled);
    virtual int IsLiveSrcEnabled(void) const { return m_isLiveSrcEnabled; }

    /* Destination control. */
    virtual SPxErrorCode SetDestType(SPxMesgIOType_t destType);
    virtual SPxMesgIOType_t GetDestType(void) { return static_cast<SPxMesgIOType_t>(static_cast<int>(m_destType)); }
    virtual SPxErrorCode SetDestNetAddr(const char *addr,
                                        int port=0,
                                        const char *ifAddr=NULL);
    virtual SPxErrorCode SetDestNetAddr(UINT32 addr,
                                        int port=0,
                                        const char *ifAddr=NULL);
    virtual SPxErrorCode SetDestNetPort(int port);
    virtual SPxErrorCode SetDestNetTCP(int isTCP);
    virtual UINT32 GetDestNetAddr(void);
    virtual int GetDestNetPort(void);
    virtual int IsDestNetTCP(void) const;

    /* Input from file replay. */
    virtual SPxErrorCode AddReplaySource(SPxRadarReplay *replaySource);

    /* Output to recording process. */
    virtual SPxErrorCode AddRecordingProcess(SPxRunProcess *recProc);

    /* Manual input. */
    void ManualInput(SPxTime_t *time,
                     const unsigned char *data,
                     unsigned int numBytes);

    /* Access to underlying packet decoder net class (required if adding
     * this to SPxNetworkRecord class).
     */
    SPxPacketDecoderNet *GetPacketDecoderNet(void) { return(m_netPacketDecoder); }

    /* Get network packet sender. */
    SPxPacketSender *GetPacketSenderNet(void) { return(m_netPacketSender); }

    /* Parameters set/get */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

protected:
    /* Constructor for testing. */
    explicit SPxDecoderRepeater(SPxPacketDecoderNet *netPacketDecoder);

    /* Let derived classes get access to some of the underlying objects. */
    SPxSerialPort *GetSerialPort(void)	{ return(m_serialRecv); }

    /* Let derived classes install message Handler function */
    virtual SPxErrorCode SetHandler(SPxPacketDecoderFn_t fn, void *userArg);

private:

    /*
     * Private types.
     */

    /* Used to store replay sources in a linked list */
    typedef struct _ReplaySource
    {
        SPxRadarReplay *obj;
        SPxPacketDecoder *decoder;
        _ReplaySource *next;

    } ReplaySource_t;

    /* Used to store recording processes in a linked list */
    typedef struct _RecProc
    {
        SPxRunProcess *obj;
        _RecProc *next;

    } RecProc_t;

    /*
     * Private variables.
     */

    /* Our message type(s) */
    UINT32 m_packetType;
    UINT32 m_packetType2;

    /* Source */
    SPxAtomic<int> m_srcType;
    int m_isSrcTypeFixed;
    SPxPacketDecoderNet *m_netPacketDecoderSys;
    SPxPacketDecoderNet *m_netPacketDecoder;
    SPxSerialPort *m_serialRecv;
    int m_serialSupported;
    SPxAtomic<int> m_isLiveSrcEnabled;


    /* Dest */
    SPxAtomic<int> m_destType;
    SPxPacketSender *m_netPacketSender;

    /* Callback functions */
    SPxPacketDecoderFn_t m_fn;
    void *m_fnUserArg;
    SPxPacketDecoderFn_t m_directFn;
    void *m_directFnUserArg;

    /* Linked list of replay sources */
    ReplaySource_t *m_replaySourceList;

    /* Linked list of recording processes */
    RecProc_t *m_recProcList;

    /*
     * Private functions.
     */
    void init(SPxPacketDecoderNet *netPacketDecoder);

    /* Message handler */
    void MesgHandler(SPxPacketDecoder *packetDecoder,
                     struct sockaddr_in *sockaddr,
                     SPxTime_t *time,
                     const unsigned char *data,
                     unsigned int numBytes);


    /*
     * Private static functions.
     */
    static void NetRawMesgHandler(SPxPacketDecoder *source,
				  void *userArg,
				  UINT32 packetType,
				  struct sockaddr_in *addr,
				  SPxTime_t *time,
				  const unsigned char *data,
				  unsigned int numBytes);

    static void NetMesgHandler(SPxPacketDecoder *source,
                               void *userArg,
                               UINT32 packetType,
                               struct sockaddr_in *addr,
                               SPxTime_t *time,
                               const unsigned char *data,
                               unsigned int numBytes);

    static void SerialMesgHandler(SPxSerialPort *source,
                                  void *userArg,
                                  SPxTime_t *time,
                                  const unsigned char *data,
                                  unsigned int numBytes);

    static void ReplayMesgHandler(SPxPacketDecoder *source,
                                  void *userArg,
                                  UINT32 packetType,
                                  struct sockaddr_in *sockAddr,
                                  SPxTime_t *time,
                                  const unsigned char *data,
                                  unsigned int numBytes);

}; /* SPxDecoderRepeater */

#endif /* _SPX_DECODER_REPEATER_H */

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
