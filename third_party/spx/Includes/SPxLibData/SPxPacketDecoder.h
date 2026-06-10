/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxPacketDecoder.h,v $
* ID: $Id: SPxPacketDecoder.h,v 1.22 2017/07/13 14:12:52 rew Exp $
*
* Purpose:
*	Header for SPxPacketDecoder class.
*
*	This is a generic class that may be used to decode an SPx
*	packet stream. The SPxPacketDecoderNet and SPxPacketDecoderFile
*	classes implement decoding of network streams and file streams.
*
*	This class parses the messages given to it and invokes 
*	callback functions according to the packet types found.
*	The file is heavily based on parts of the old SPxPacketDecoder
*	class from SPxLibNet (that is now SPxPacketDecoderNet).
*
*
* Revision Control:
*   13/07/17 v1.22   AGC	Improvements to handler removal.
*
* Previous Changes:
*   03/10/16 1.21   AGC	Add channel filter to NMEA decode.
*   27/09/16 1.20   AGC	Add channel filter.
*   16/09/16 1.19   AGC	Add fromAddr argument to DecodeBuffer().
*   03/11/15 1.18   AGC	Packet decoder callback data now const.
*   03/11/15 1.17   SP 	Add peekNetPacket().
*   29/06/15 1.16   AGC	Add GetChannelIndices().
*   23/06/15 1.15   AGC	Add device manufacturer.
*			Support serial data decoding.
*   05/06/15 1.14   SP 	Add peekRecordPacket().
*   11/05/15 1.13   SP 	Update current channel index from packets
*                       that have a channel index field.
*   27/04/15 1.12   AGC	Fix handler removal.
*   26/03/15 1.11   AGC	Support mutiple handlers for each packet type.
*   20/02/15 1.10   SP 	Rename handleChannelPacket() to handleChanSelectPacket().
*   16/02/15 1.9    SP 	Add handleChannelPacket().
*   05/02/15 1.8    SP 	Add initial support for channel index.
*   13/03/14 1.7    AGC	Support processing packets from within handlers.
*   20/12/13 1.6    REW	Change SetDirectHandler() to InstallDirectHandler()
*   15/11/13 1.5    SP 	Add DecodeBuffer() and GetPacketDeficit().
*   04/10/13 1.4    AGC	Move C++ headers to source.
*   15/05/13 1.3    AGC	Report bad message errors every 10 seconds.
*   15/03/12 1.2    REW	Support direct access to packets.
*   06/04/11 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_PACKET_DECODER_H
#define _SPX_PACKET_DECODER_H

/*
 * Other headers required.
 */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibUtils/SPxCriticalSection.h"


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

enum SPxDeviceManufacturer_t
{
    SPX_DEV_MANUF_NONE = 0,
    SPX_DEV_MANUF_SPX = 1,
    SPX_DEV_MANUF_RG = 2,
    SPX_DEV_MANUF_KAL = 3
};

/* Need to forward-declare the class for use in the handler function type. */
class SPxPacketDecoder;

/* Define the type of the callback packet handler functions. */
typedef void (*SPxPacketDecoderFn_t)(SPxPacketDecoder *,	/* this */
				     void *,			/* userArg */
				     UINT32,			/* packetType*/
				     struct sockaddr_in *,	/* from/NULL */
				     struct SPxTime_tag *,	/* time/NULL */
				     const unsigned char *,	/* payload */
				     unsigned int);		/* numBytes */

/*
 * Define our class.
 */
class SPxPacketDecoder : public SPxObj
{
public:
    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxPacketDecoder(void);
    virtual ~SPxPacketDecoder(void);

    /* Configuration functions. */
    SPxErrorCode SetManufacturer(SPxDeviceManufacturer_t manufacturer);
    SPxDeviceManufacturer_t GetManufacturer(void) const { return m_manufacturer; }

    /* Handler functions. */
    SPxErrorCode InstallHandler(SPxPacketDecoderFn_t fn,
				void *userArg,
				UINT32 packetType);
    SPxErrorCode RemoveHandler(SPxPacketDecoderFn_t fn,
			       void *userArg,
			       UINT32 packetType,
			       int waitIfInvoking = TRUE);
    SPxErrorCode RemoveHandler(SPxPacketDecoderFn_t fn,
			       void *userArg,
			       int waitIfInvoking = TRUE);

    /* Direct access handlers for pre-decoded data.
     * Packet type will be zero and there will be no timestamp when invoked.
     */
    SPxErrorCode InstallDirectHandler(SPxPacketDecoderFn_t fn, void *userArg);
    SPxErrorCode RemoveDirectHandler(SPxPacketDecoderFn_t fn, void *userArg,
				     int waitIfInvoking = TRUE);

    /* Decode data in a user-supplied buffer. */
    SPxErrorCode DecodeBuffer(const unsigned char *buf, unsigned int numBytes,
			      struct sockaddr_in *fromAddr=NULL)
    {
        return decodeAndInvoke(buf, numBytes, fromAddr);
    }

    /* Get channel index for the current packet. */
    virtual UINT8 GetChannelIndex(void) const { return m_channelIndex; }

    /* Get list of channel indices. */
    SPxErrorCode GetChannelIndices(unsigned char *indices,
	unsigned int *numIndices) const;

    /*
     * Public static functions.
     */

    static SPxErrorCode GetPacketDeficit(unsigned char *buf, 
                                         unsigned int numBytes,
                                         int *deficitBytes);

protected:
    /*
     * Protected functions.
     */
    SPxErrorCode decodeAndInvoke(const unsigned char *buf,
                                 unsigned int numBytes,
                                 struct sockaddr_in *fromAddr,
                                 int mutexAlreadyLocked=FALSE,
				 int channelFilter=-1);

    SPxErrorCode decodePacketType(UINT32 packetType, 
				  struct sockaddr_in *fromAddr,
				  struct SPxTime_tag *timePtr, 
				  const unsigned char *buf,
				  unsigned int numBytes, 
				  int mutexAlreadyLocked,
				  int channelFilter=-1);

    /* Set channel index for the next packet. */
    virtual void setChannelIndex(UINT8 index);

    void resetChannelIndices(void);

private:
    /*
     * Private variables.
     */
    struct impl;
    SPxAutoPtr<impl> m_p;
    mutable SPxCriticalSection m_mutex;
    SPxDeviceManufacturer_t m_manufacturer;
    UINT32 m_lastBadMsgTime;		/* Last time a bad message error was reported. */
    UINT32 m_badMsgErrIntervalMsecs;	/* Minimum interval between bad message errors. */
    UINT8 m_channelIndex;		/* Channel index for next packet. */
    int m_channelIndices[256];		/* Channel indices. */

    /* KAL decoding fields. */
    unsigned char m_kalBuf[1024];
    unsigned int m_kalBufPresent;
    unsigned int m_kalBufDLEIndex;

    /*
     * Private functions.
     */
    int removeHandler(SPxPacketDecoderFn_t fn,
		      void *userArg,
		      UINT32 packetType);

    void handleChanSelectPacket(const unsigned char *payload,
                                unsigned int numBytes);
    
    void peekRadarConfigPacket(const unsigned char *payload,
                               unsigned int numBytes);

    void peekRadarReturnPacket(const unsigned char *payload,
                               unsigned int numBytes);

    void peekImageChunkPacket(const unsigned char *payload,
                              unsigned int numBytes);

    void peekRecordPacket(const unsigned char *payload,
                          unsigned int numBytes);

    void peekNetPacket(const unsigned char *payload,
                       unsigned int numBytes);

    /* Manufacturer decoding/unframing. */
    unsigned int decodeSPx(const unsigned char *buf,
                           unsigned int numBytes,
                           struct sockaddr_in *fromAddr,
                           int mutexAlreadyLocked,
			   int channelFilter);
    unsigned int decodeRG(const unsigned char *buf,
			  unsigned int numBytes,
			  struct sockaddr_in *fromAddr,
                          int mutexAlreadyLocked);
    unsigned int decodeKAL(const unsigned char *buf,
                           unsigned int numBytes,
                           struct sockaddr_in *fromAddr,
                           int mutexAlreadyLocked);

    /* Protocol deserialisation. */
    unsigned int decodeAISorNMEA(UINT32 packetType, 
			struct sockaddr_in *fromAddr,
			struct SPxTime_tag *timePtr,
			const unsigned char *buf, 
			unsigned int numBytes,
			int mutexAlreadyLocked,
			int channelFilter);

    /* Callback invocation. */
    int invokeDirect(struct sockaddr_in *fromAddr,
		     struct SPxTime_tag *timePtr,
		     const unsigned char *buf, 
		     unsigned int numBytes,
		     int mutexAlreadyLocked);
    SPxErrorCode invoke(UINT32 packetType, 
			struct sockaddr_in *fromAddr,
			struct SPxTime_tag *timePtr,
			const unsigned char *buf, 
			unsigned int numBytes,
			int mutexAlreadyLocked,
			int channelFilter=-1);

}; /* SPxPacketDecoder */

#endif /* _SPX_PACKET_DECODER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
