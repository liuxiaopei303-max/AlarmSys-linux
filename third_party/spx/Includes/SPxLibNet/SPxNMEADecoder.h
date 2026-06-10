/*********************************************************************
*
* (c) Copyright 2013, 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNMEADecoder.h,v $
* ID: $Id: SPxNMEADecoder.h,v 1.2 2016/10/31 15:27:59 rew Exp $
*
* Purpose:
*	Header for SPxNMEADecoder class which supports decoding
*	the component parts of NMEA sentences.
*
*
* Revision Control:
*   31/10/16 v1.2    AGC	Add Skip() function.
*
* Previous Changes:
*   08/04/13 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_NMEA_DECODER_H
#define _SPX_NMEA_DECODER_H

/*
 * Other headers required.
 */
/* For SPxErrorCode enumeration. */
#include "SPxLibUtils/SPxError.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Maximum number of parts to a single NMEA message. */
#define SPX_NMEA_DECODER_MAX_NMEA_PARTS	    (80)

/* Maximum length of a single NMEA message in bytes.
 * The limit according to the standard is 82 bytes
 * (not including a NULL terminator), but this byffer is larger
 * to be on the safe side.
 */
#define SPX_NMEA_DECODER_MAX_LEN	    (120)

/* Get the next part of an NMEA message from a GetPart...() function. */
#define SPX_NMEA_DECODER_NEXT_PART	    (-1)

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

/*
 * Define our class, NOT derived from SPxObj.
 */
class SPxNMEADecoder
{
public:
    /*
     * Public functions.
     */
    explicit SPxNMEADecoder(const char *nmea=NULL);
    virtual ~SPxNMEADecoder(void);

    SPxErrorCode Parse(const char *nmea);
    SPxErrorCode GetParseError(void) const { return m_parseError; }

    const char *GetNMEA(void) const { return m_nmea; }
    int GetNumParts(void) const { return m_numParts; }
    char GetStartDelimiter(void) const { return m_startDelim; }
    const char *GetTalker(void) const { return m_talker; }
    const char *GetSentenceType(void) const { return m_sentenceType; }
    int GetChecksum(void) const { return m_reqChecksum; }
    int GetChecksumRead(void) const { return m_checksum; }
    int IsChecksumValid(void) const { return (m_checksum == m_reqChecksum); }

    SPxErrorCode GetPartAsStr(const char **s, int part=SPX_NMEA_DECODER_NEXT_PART) const;
    SPxErrorCode GetPartAsChar(char *c, int part=SPX_NMEA_DECODER_NEXT_PART) const;
    SPxErrorCode GetPartAsInt(int *i, int part=SPX_NMEA_DECODER_NEXT_PART) const;
    SPxErrorCode GetPartAsUint(unsigned int *u, int part=SPX_NMEA_DECODER_NEXT_PART) const;
    SPxErrorCode GetPartAsReal(float *f, int part=SPX_NMEA_DECODER_NEXT_PART) const;
    SPxErrorCode GetPartAsDouble(double *d, int part=SPX_NMEA_DECODER_NEXT_PART) const;
    SPxErrorCode Skip(unsigned int numParts=1) const;

    /* Convenience functions for checksum handling (return -1 on error). */
    static int CalcChecksum(const char *nmea);
    static int ReadChecksum(const char *nmea);

private:
    /*
     * Private variables.
     */
    SPxErrorCode m_parseError;	    /* Most recently parse status. */
    int m_numParts;		    /* Number of parts to most recent message. */
    mutable int m_nextPart;	    /* Number of next part to retreive. */
    char m_nmea[SPX_NMEA_DECODER_MAX_LEN];  /* Last parsed message. */
    char m_msg[SPX_NMEA_DECODER_MAX_NMEA_PARTS][SPX_NMEA_DECODER_MAX_LEN];  /* Parts of message. */
    char m_startDelim;		    /* Start delimiter from last message. */
    char m_talker[3];		    /* Talker from last message. */
    char m_sentenceType[4];	    /* Sentence type from last message. */
    int m_checksum;		    /* Read checksum from last message. */
    int m_reqChecksum;		    /* Calculated checksum from last message. */

    /*
     * Private functions.
     */
    SPxErrorCode checkPart(int& part) const;

}; /* SPxNMEADecoder */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* _SPX_NMEA_DECODER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
