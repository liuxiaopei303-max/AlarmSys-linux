/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxHPxCommon.h,v $
* ID: $Id: SPxHPxCommon.h,v 1.13 2017/04/20 10:38:38 rew Exp $
*
* Purpose:
*	Header for common interface to all HPx cards, both input and output.
*
*
* Revision Control:
*   20/04/17 v1.13   REW	Initial HPx-346 support.
*
* Previous Changes:
*   08/03/17 1.12   AGC	Check for Windows fast start-up config.
*   02/11/16 1.11   REW	DumpState() supports second stream.
*   31/10/16 1.10   REW	Add streamIdx to ReadReg() and WriteReg().
*			Increase default mask to LUTTest().
*   20/05/16 1.9    REW	Add convenience flags for card types.
*   27/08/15 1.8    REW	Add strictID arg to FlashTest().
*   22/04/15 1.7    SP 	Add IsBoardOpen().
*   22/04/15 1.6    AGC	Add CheckCompatibility().
*   14/10/14 1.5    SP 	Add GetBoardIdx().
*   13/12/13 1.4    REW	Add const to the constructor arg.
*   04/10/13 1.3    AGC	Simplify headers.
*   14/06/13 1.2    REW	Move GetInitialised() back to HPx100 class.
*   14/06/13 1.1    REW	Initial Version from 100 header.
**********************************************************************/

#ifndef _SPX_HPX_COMMON_H
#define _SPX_HPX_COMMON_H

/*
 * Other headers required.
 */
/* We need SPxLibUtils for common types etc. */
#include "SPxLibUtils/SPxCommon.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxError.h"


/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Hardware sizes etc. */
#define	SPX_HPX_FLASH_SIZE_BYTES	(512 * 1024)	/* Not all cards */

/*
 * Common debug flags.  Bottom 16-bits are in derived classes only.
 */
#define	SPX_HPX_DEBUG_READ_REG		0x00010000
#define	SPX_HPX_DEBUG_READ_MEM		0x00020000
#define	SPX_HPX_DEBUG_READ_LUT		0x00040000
#define	SPX_HPX_DEBUG_READ_FLASH	0x00080000
#define	SPX_HPX_DEBUG_WRITE_REG		0x00100000
#define	SPX_HPX_DEBUG_WRITE_MEM		0x00200000
#define	SPX_HPX_DEBUG_WRITE_LUT		0x00400000
#define	SPX_HPX_DEBUG_WRITE_FLASH	0x00800000
#define	SPX_HPX_DEBUG_VERBOSE		0x80000000

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
 * Define our class.  This is not derived from SPxObj because the
 * hardware-specific classes that derive from this will also be deriving
 * from other classes that do use SPxObj, so it's easier to avoid multiple
 * inheritance issues this way.  Effectively this class is a bit like an
 * interface although it does implement some of the common bits.
 */
class SPxHPxCommon
{
public:
    /*
     * Public fields.
     */

    /*
     * Public functions.
     */
    /* Constructor, destructor etc. */
protected:
    SPxHPxCommon(UINT16 subDeviceId, const char *ioType);
public:
    virtual ~SPxHPxCommon();

    /* Board open/close functions. */
    virtual SPxErrorCode ProbeBoard(unsigned int idx=0);
    virtual int IsBoardOpen(void) { return m_boardOpen; }
    virtual SPxErrorCode OpenBoard(unsigned int idx=0, int noInit=FALSE) = 0;
    virtual SPxErrorCode CloseBoard(int noStop=FALSE) = 0;

    /*
     * Configuration functions.
     */

    /*
     * Status retrieval.
     */
    virtual const char *GetCardName(void) const	{ return(m_cardName); }
    virtual UINT32 GetCardType(void) const	{ return(m_cardType); }
    virtual UINT32 GetCardSubType(void) const	{ return(m_cardSubType); }
    virtual UINT32 GetFpgaVersion(void) const	{ return(m_fpgaVersion); }
    virtual const char *GetIoString(void) const	{ return(m_ioString); }
    virtual int GetPCIBusNumber(void) const;
    virtual int GetPCISlotNumber(void) const;
    virtual unsigned int GetBoardIdx(void) const { return m_boardIdx; }

    /* This will return NULL if the card record is not available. */
    virtual const struct SPxHPxCardRecord_tag *GetCardRecord(void) const
						{ return(m_cardRecord); }

    /* DMA threshold control. */
    virtual void SetDMAThreshold(unsigned int nBytes)
						{ m_dmaThreshold = nBytes; }
    virtual unsigned int GetDMAThreshold(void) const { return(m_dmaThreshold);}

    /* Some utility functions to check for support of common features. */
    virtual SPxErrorCode IsFlashSupported(void) const
    {
	/* Assume yes unless overridden by base class. */
	return(SPX_NO_ERROR);
    }

    /* Basic testing functions. */
    virtual SPxErrorCode RegTest(void) = 0;
    virtual SPxErrorCode MemTest(void) = 0;
    virtual SPxErrorCode LUTTest(unsigned int lutFlags=0x1F) = 0;
    virtual SPxErrorCode FlashTest(int strictID=FALSE);

    /* Low-level hardware access functions (not for normal use). */
    virtual SPxErrorCode ReadReg(unsigned int reg, UINT32 *valPtr,
					int useDrv=0,
					unsigned int streamIdx=0);
    virtual SPxErrorCode WriteReg(unsigned int reg, UINT32 value,
					int useDrv=0,
					unsigned int streamIdx=0);
    virtual SPxErrorCode ReadMem(void *dst, unsigned int src,
					unsigned int nBytes, int useDrv=0) = 0;
    virtual SPxErrorCode WriteMem(unsigned int dst, void *src,
					unsigned int nBytes, int useDrv=0) = 0;
    virtual SPxErrorCode ReadLUT(unsigned int lutIdx, unsigned char *buf,
					unsigned int bufSizeBytes) = 0;
    virtual SPxErrorCode WriteLUT(unsigned int lutIdx, const unsigned char *buf,
					unsigned int bufSizeBytes) = 0;
    virtual unsigned int GetLUTSize(unsigned int lutIdx) = 0;
    virtual unsigned char *GetLUTBuffer(void) = 0;
    virtual SPxErrorCode ReadFlash(void *dst, unsigned int src,
					unsigned int nBytes,
					int swMutexLocked=FALSE);

    /* Even lower-level flash interface functions. */
    virtual SPxErrorCode WriteFlashCommand(unsigned int numBits,
					UINT32 value,
					int csHighAtEnd = FALSE);
    virtual SPxErrorCode ReadFlashResponse(unsigned int numBits,
					UINT32 *valuePtr,
					int csHighAtEnd = FALSE);

    /* Debug. */
    virtual void DumpState(const char *reason=NULL, unsigned int streamIdx=0)=0;
    virtual const char *GetRegName(unsigned int reg) = 0;

    /*
     * Static public functions.
     */
    static void SetLogFile(FILE *logFile)	{ m_logFile = logFile; }
    static FILE *GetLogFile(void)		{ return(m_logFile); }
    static void SetDebug(UINT32 flags)		{ m_debug = flags; }
    static UINT32 GetDebug(void)		{ return(m_debug); }

protected:
    /*
     * Protected fields (used by derived classes).
     */
    /* Critical section protection.  Sw must always be locked before Hw. */
    SPxCriticalSection m_mutexHw;	/* Protect hardware accesses */
    SPxCriticalSection m_mutexSw;	/* Protect internal software state */

    /* Offsets to registers that differ according to card type. */
    UINT32 m_regOffsetBase;		/* Offset to base of registers */
    UINT32 m_regOffsetBase2;		/* Offset to base of 2nd register set */
    UINT32 m_regOffsetVersion;		/* Extra offset to Version register */
    UINT32 m_regOffsetSpiFlash;		/* Extra offset to SPI Flash register */
    UINT32 m_regOffsetLast;		/* Extra offset to highest register */

    /* Card status etc. */
    UINT32 m_fpgaVersion;		/* Version of FPGA */
    UINT32 m_cardType;			/* 0x100, 0x150, 0x00D, 0x200 etc. */
    UINT32 m_cardSubType;		/* From STATUS2[1..0] for 0x00D */
    int m_is1xx;			/* TRUE if 0x100, 0x150 */
    int m_is2xx;			/* TRUE if 0x200, 0x250 */
    int m_is3xx;			/* TRUE if 0x300 (not HPx-346) */
    int m_is346;			/* TRUE if 0x346 */
    int m_is4xx;			/* TRUE if 0x400, 0x450 */
    int m_isPMC;			/* TRUE if PMC */
    char m_cardName[16];		/* Name of card */
    struct SPxHPxCardRecord_tag *m_cardRecord;	/* Card record, or NULL */

    /* Board handle etc. */
    unsigned int m_boardOpen;		/* Is the board open? */
    unsigned int m_boardOpenFinished;	/* Have we finished doing the open? */
    unsigned int m_boardIdx;		/* Index of board in use */
    unsigned int m_boardEverOpened;	/* Has it ever been opened? */
    struct _PLX_DEVICE_KEY *m_plxKey;	/* Identifier for card */
    struct _PLX_DEVICE_OBJECT *m_plxObj;/* Handle for card */
#ifdef __arm
    int m_fdMemory;			/* Handle for memory device */
    int m_fdFpga;			/* Handle for FPGA device */
    void *m_memBase0;			/* Base of bank 0 memory */
    void *m_memBase1;			/* Base of bank 1 memory */
#endif

    /* Memory-mapped PCI windows. */
    void *m_bar2Base;			/* Virtual base address of BAR2 */
    UINT32 *m_regBase;			/* Virtual base of registers */
    UINT32 *m_regBase2;			/* Same for 2nd reg set, or NULL. */

    /* DMA control. */
    int m_dmaChanOpen;			/* Flag to say DMA channel open */
    struct _PLX_DMA_PROP *m_plxDmaProp;	/* DMA channel properties */
    struct _PLX_DMA_PARAMS *m_plxDmaParams; /* DMA parameters */
    unsigned int m_dmaThreshold;	/* Number of bytes */
    int m_dmaLocalAddrConst;		/* Does derived class use const addr?*/

    /* Interrupt handling. */
    struct _PLX_INTERRUPT *m_plxInt;	/* Interrupt description */
    struct _PLX_NOTIFY_OBJECT *m_plxNotify; /* Notification object */

    /* SDK versions, status, PCI information etc. */
    UINT32 m_apiVersion;		/* Version of PLX library */
    UINT32 m_drvVersion;		/* Version of PLX driver */
    int m_eepromOkay;			/* Is the EEPROM okay? */
    struct _PLX_PCI_BAR_PROP *m_plxBar2; /* PCI bar 2 information */

    /*
     * Static (i.e. per class, not per object) variables.
     */
    static FILE *m_logFile;		/* Destination for debug messages */
    static UINT32 m_debug;		/* Debug flags */

    /*
     * Protected functions.
     */
    SPxErrorCode OpenBoardCommon(unsigned int idx, int noInit);
    SPxErrorCode CloseBoardCommon(void);

    /* Compatibility testing functions. */
    SPxErrorCode CheckCompatibility(int reqMajorVer, int reqMinorVer,
				    int only64Bit, const char *msg);

private:
    /*
     * Private fields.
     */
    /* PCI sub-device ID. */
    const UINT16 m_subDeviceId;		/* 0x3334 or 0x3565 */

    /* Card input/output string. */
    char *m_ioString;			/* "input" or "output" */

    /*
     * Private functions.
     */
    void checkFastBoot(void);

}; /* SPxHPxCommon class */


/*
 * Define the structure used to store per-card records (little-endian).
 *
 * NB: This needs to be 256 bytes exactly, i.e. 64 words.
 *
 * Reserved fields should be set to 0.
 */
typedef struct SPxHPxCardRecord_tag {
    /* Word 0 (doubles as magic value). */
    UINT16 partNumberFamily;			/* E.g. 124, 161 or 231 */
    UINT16 partNumberCode;			/* The xxx in p/n 161-xxx */

    /* Word 1. */
    UINT16 serialNumber;			/* The xxx in s/n 161-xxx */
    UINT16 reserved06;

    /* Word 2. */
    UINT32 timestamp;				/* Timestamp */

    /* Word 3. */
    UCHAR pcb;					/* PCB version */
    UCHAR bom;					/* BOM version */
    UCHAR asn;					/* Assembly version */
    UCHAR reserved15;

    /* Word 4. */
    REAL32 bvaGain;				/* B vs A gain */

    /* Words 5 to 63. */
    UINT32 reserved20[59];
} SPxHPxCardRecord_t;


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_HPX_COMMON_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
