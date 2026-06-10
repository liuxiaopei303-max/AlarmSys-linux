/*********************************************************************
*
* (c) Copyright 2007, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxCompress.h,v $
* ID: $Id: SPxCompress.h,v 1.4 2015/11/03 11:31:39 rew Exp $
*
* Purpose:
*	Generic compression definitions.
*
*
* Revision Control:
*   03/11/15 v1.4    AGC	Uncompress() now takes source data as const.
*
* Previous Changes:
*   25/04/13 1.3    AGC	Create class with custom allocater for ZLIB.
*   18/10/12 1.2    AGC	Do not include zlib.h on ARM.
*   10/12/07 1.1    REW	Initial Version, from SPxCompressORC.h.
**********************************************************************/

#ifndef _SPX_COMPRESS_H
#define _SPX_COMPRESS_H

/* Other headers. */
#include "SPxLibData/SPxRib.h"

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

/* Maximum number of memory items managed by the ZLIB compression class. */
#define SPX_COMPRESS_NUM_MEM_ITEMS  (5)

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Structure for managing ZLIB memory. */
struct SPxCompressMem
{
    int inUse;
    unsigned int size;
    void *ptr;
};

/* ZLIB compression class. */
class SPxCompress
{
public:
    /* Public functions. */
    SPxCompress(void);
    virtual ~SPxCompress(void);

    int Compress(unsigned char *dest, unsigned long *destLen,
		const unsigned char *source, unsigned long sourceLen);

    unsigned char *CompressGen(const SPxReturn *rtn,
			       unsigned char *destBuf,
			       unsigned char *destBuf2,
			       unsigned int destBufSizeBytes,
			       unsigned int *encodedSizeBytesPtr,
			       unsigned int *formatReqPtr,
			       unsigned int *formatUsedPtr,
			       FILE *logFile, int verbose);

    int Uncompress(unsigned char *dest, unsigned long *destLen,
			const unsigned char *source, unsigned long sourceLen);

private:
    /* Private variables. */
    SPxCompressMem m_memory[SPX_COMPRESS_NUM_MEM_ITEMS];

    /* Private functions. */
    static void *SPxAlloc(void *opaque, unsigned int items, unsigned int size);
    static void SPxFree(void *opaque, void *address);

}; /* SPxCompress */

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_COMPRESS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
