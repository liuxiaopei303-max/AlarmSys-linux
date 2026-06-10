/*********************************************************************
*
* (c) Copyright 2008 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxHPx100Source.h,v $
* ID: $Id: SPxHPx100Source.h,v 1.112 2017/06/22 14:58:56 rew Exp $
*
* Purpose:
*	Header for SPxHPx100Source objects, which support all radar
*	input cards based on the HPx-100 (including HPx-150), allowing
*	for some minor differences.
*
*
* Revision Control:
*   22/06/17 v1.112  AJH	Add m_sig1Azi* member variables.
*
* Previous Changes:
*   17/05/17 1.111  REW	Tweak watchdog register address.
*   17/05/17 1.110  REW	Add Get/SetRefreshWatchdog()
*   28/04/17 1.109  REW	Add shouldProcessData flag to processStream().
*   20/04/17 1.108  REW	Start HPx-346 support.
*   12/04/17 1.107  SP 	Add new functions to check SIG1 and SIG2 support.
*			Add SetDualModeAllowed() and IsDualModeAvailable().
*   06/03/17 1.106  REW	Fix HPx-400 backwards compatibility definition.
*   18/01/17 1.105  REW	Support SPX_HPX400_DUAL_REDUNDANT mode
*			and SPX_HPX400_VIDEO_AUTO option.
*   05/01/17 1.104  REW	Support per-stream alarm and data functions.
*   04/01/17 1.103  REW	Add SetSecondRIB().
*   22/12/16 1.102  SP 	Rename SPX_HPX400_DUAL_STREAM to SPX_HPX400_DUAL_RADAR.
*   19/12/16 1.101  REW	Support SetDualMode() and ISM_INPUT_AUTO.
*   08/12/16 1.100  REW	Support Get/SetLEDSource().
*   07/12/16 1.99   REW	Support open-collector input options.
*   17/11/16 1.98   REW	Support digital offset for HPx-400.
*   16/11/16 1.97   REW	Add getP313Parameter().
*   16/11/16 1.96   REW	Support SIG3 control and SIG1/2/3 thresholds.
*   02/11/16 1.95   REW	DumpState() supports second stream.
*   31/10/16 1.94   REW	Start breaking out dual stream settings.
*   25/10/16 1.93   REW	Add ISM configuration options.
*   13/10/16 1.92   REW	Start HPx-400 dual stream support.
*   07/10/16 1.91   REW	Add SPX_HPX400_ISM_BASE_ADDR/END_ADDR defs.
*   24/06/16 1.90   REW	Add ReadADCReg() and WriteADCReg().
*   03/06/16 1.89   REW	Add m_adcDacGain1/2/etc.
*   06/05/16 1.88   REW	Initial support for HPx-400.
*   09/12/15 1.87   REW	Support new P313 composite serial ACP/ARP mode.
*   28/04/15 1.86   REW	Add SetAziScaleDegs() etc.
*   23/09/14 1.85   REW	Add SetPackingOverride().
*   04/04/14 1.84   REW	Add SetACPAlarmMsecs(), SetARPAlarmMsecs()
*			and SetTRGAlarmMsecs().
*   14/11/13 1.83   REW	Add IsVideoChannelSupported().
*   08/11/13 1.82   REW	Add SPX_HPX200_VIDEO_COMBI_8A_71, 611 etc.
*   11/10/13 1.81   REW	Support larger buffers.
*   04/10/13 1.80   AGC	Simplify headers.
*   19/07/13 1.79   AGC	Add anticlockwise option.
*   14/06/13 1.78   REW	Bring GetInitialised() back from SPxHPxCommon.
*   13/06/13 1.77   REW	Move common API to SPxHPxCommon.h
*   11/02/13 1.76   REW	Change StartAutoGain() to StartAutoGainOffset()
*   07/02/13 1.75   REW	Add StartAutoGain().
*   22/01/13 1.74   REW	Add SPX_HPX200_AZI_INTERNAL.
*   03/09/12 1.73   REW	Add SPX_HPX200_FLASH_PAGESIZE_BYTES.
*   18/07/12 1.72   REW	Rename interleaved to high-res throughout.
*   21/06/12 1.71   REW	Remove calibration. Add CardRecords.
*			Add IsFlashSuported().
*   14/04/12 1.70   REW	Start interleaved video calibration.
*   27/03/12 1.69   REW	Add API for de-bounce control. Use const more.
*   22/03/12 1.68   REW	Start support for SPX_HPX200_VIDEO_INTERLEAVED.
*   13/02/12 1.67   REW	Use GAINA and GAINB registers in 12-bit mode.
*   09/02/12 1.66   REW	Add definitions for GAINA and GAINB regs.
*   08/09/11 1.65   REW	Add 12A_1111 and 8A_8B combi options.
*   16/08/11 1.64   REW	GetMaxGain() supports HPx-250.
*   10/08/11 1.63   REW	Add IsProgThresholdSupported().
*   22/07/11 1.62   REW	Add support for ACPBLANK and ARPBLANK regs.
*   22/07/11 1.61   REW	Add support for reading flash contents.
*   24/05/11 1.60   REW	Add functions for programmable thresholds.
*   18/05/11 1.59   REW	Add waitForDAC().
*			Add PROG options for TRG/AZI/ARP.
*   13/04/11 1.58   REW	Support PAGE register on HPx-200.
*   06/04/11 1.57   REW	Support SPIFLASH register on HPx-200.
*   04/04/11 1.56   REW	Support 12-bit analogue input options.
*   24/03/11 1.55   REW	Support Mean/SD statistics.
*   23/03/11 1.54   REW	Support SIG1/2 registers etc. on HPx-200.
*   09/03/11 1.53   REW	Support GPIO on HPx-200, SetPortOutEnable().
*   24/02/11 1.52   REW	Add getMinStartSamples().
*   17/02/11 1.51   REW	Fix SPX_HPX200_LUT_END_ADDR2 and LUT sizes.
*   11/02/11 1.50   REW	Add SPX_HPX200_REG_LINKS.
*   28/01/11 1.49   REW	Start HPx-200 support. See .cpp for changes.
*   24/01/11 1.48   AGC	Return correct offsets from GetParameter().
*   14/01/11 1.47   REW	Add support direct DAC control.
*   11/01/11 1.46   REW	Add support for end-range pulses.
*   22/12/10 1.45   AGC Add GetRequested{Start/End}RangeMetres(),
*			and GetLastOffsetA/B().
*   15/07/10 1.44   REW	Add ProbeBoard().
*   21/07/10 1.43   REW	Add lutFlags to LUTTest().
*   16/07/10 1.42   REW	Add DATE register.
*   21/06/10 1.41   REW	Maintain min/max trigger period for debug.
*   24/05/10 1.40   REW	Support manual bank swapping.
*   27/04/10 1.39   REW	Support SPX_HPX100_ARP_CMP.
*   23/03/10 1.38   REW	Add updateVariousClocks().
*   23/03/10 1.37   REW	Add SelectDigitalClock().
*   15/02/10 1.36   REW	Add SPX_HPX100_VIDEO_COMBI_4A_4B.
*   30/10/09 1.35   REW	Add GetCardSubType() from derived class.
*   30/10/09 1.34   REW	Add SetOffsetA/B() and SetVoltageRefA/B().
*   29/06/09 1.33   REW	Add GetPCIBusNumber() and GetPCISlotNumber().
*   25/06/09 1.32   REW	Add SPX_HPX100_VIDEO_NONE.
*   24/06/09 1.31   REW	Add auto-range mode, average PRF monitoring
*			and SPX_HPX100_DEBUG_PRF.
*   09/02/09 1.30   REW	Implement GetInitialised().
*   22/01/09 1.29   REW	Add more VIDEO_COMBI modes, 6+1+1 etc.
*   26/11/08 1.28   REW	Add control of PRF/Azi reports to clients.
*   13/11/08 1.27   REW	GetParameter() returns value in a string.
*   04/11/08 1.26   REW	Support SetAziInterpolation(), debug flags etc.
*   29/10/08 1.25   REW	Move PMC_DGR specifics out to derived object.
*   24/10/08 1.24   REW	Support noInit flag in OpenBoard()/CloseBoard()
*			Make DumpState() public (was dumpState()).
*			Add GetRegName(), GetLUTSize(), addr defs.
*   22/10/08 1.23   REW	Add GetCardName(), GetCardSubType(), GetTBI()
*			and handle PMC_DGR cards.
*   21/10/08 1.22   REW	Support programmable TPG.
*   21/10/08 1.21   REW	Support scanMode detection and override.
*   21/10/08 1.20   REW	Add SetStartRangeMetres() and RangeCorrection.
*   18/09/08 1.19   REW	Rename ACP definitions to AZI.
*   28/08/08 1.18   REW	Add GetCardType() and comments about HPx-150.
*   23/07/08 1.17   REW	Add SAMPLEK register.
*   18/07/08 1.16   REW	Support Get/SetParameter().
*   18/07/08 1.15   REW	Support alarm and data callbacks.
*   17/07/08 1.14   REW	Adjust DAC offset limits.
*   03/07/08 1.13   REW	Add SetGainA() and SetGainB() utilities.
*   05/06/08 1.12   REW	Add definitions for DAC offsets.
*			Add gain to default LUT functions.
*   02/06/08 1.11   REW	Add ACP/ARPTEST, TIMALTRG/ACP/ARP registers
*			and support for alarm/status functions.
*   28/05/08 1.10   REW	Add m_nsPerMainClk.
*   29/04/08 1.9    REW	Add dumpState().
*   29/04/08 1.8    REW	Add spokeNum to processData().
*   21/04/08 1.7    REW	Add control over spokes-per-interrupt.
*   10/04/08 1.6    REW	Add range/sampling control. Believe MAGBANK.
*			Add LUT-D MASK control.  Add GetDAC().
*   08/04/08 1.5    REW	Add DAC support.
*   07/04/08 1.4    REW	Keep stats on radar data etc.
*   04/04/08 1.3    REW	Add LUT functions. Two-level mutex. MemTest().
*   01/04/08 1.2    REW	New registers in FPGA V27.  Add control
*			over numSamples and numAzimuths.
*   19/03/08 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_HPX100_SOURCE_H
#define _SPX_HPX100_SOURCE_H

/*
 * Other headers required.
 */

#include "SPxLibUtils/SPxError.h"

/* We need the radar source base class header. */
#include "SPxLibData/SPxRadarSource.h"

/* We need the HPx common header. */
#include "SPxLibData/SPxHPxCommon.h"

/* We also need SPxRIB definitions. */
#include "SPxLibData/SPxRib.h"


/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Local-bus addresses, sizes etc. */
#define	SPX_HPX100_MEM_BASE_ADDR	0x00000000
#define	SPX_HPX100_MEM_SIZE_BYTES	0x00200000	/* Single page */
#define	SPX_HPX346_MEM_SIZE_BYTES	0x01000000	/* No paging */
#define	SPX_HPX100_LUT_BASE_ADDR	0x00200000
#define	SPX_HPX100_LUT_END_ADDR		0x00203FFF	/* HPx-1xx only */
#define	SPX_HPX100_REG_BASE_ADDR	0x00204000	/* Stream 0 */
#define	SPX_HPX100_REG_END_ADDR		0x002040FF
#define	SPX_HPX400_REG_BASE_ADDR_STR1	0x00205000	/* HPx-4xx Stream 1 */
#define	SPX_HPX400_REG_END_ADDR_STR1	0x002050FF
#define	SPX_HPX400_ISM_BASE_ADDR	0x00206000	/* HPx-4xx LUT-S only */
#define	SPX_HPX400_ISM_END_ADDR		0x00206FFF
#define	SPX_HPX200_LUT_BASE_ADDR2	0x00208000	/* Not HPx-1xx */
#define	SPX_HPX200_LUT_END_ADDR2	0x00210000
#define	SPX_HPX200_FLASH_SIZE_BYTES	SPX_HPX_FLASH_SIZE_BYTES /* Not 1xx */
#define	SPX_HPX200_FLASH_PAGESIZE_BYTES	256		/* Not HPx-1xx */

/* Register offsets from the base of the register location. */
#define	SPX_HPX100_REG_PERTRG		0x00
#define	SPX_HPX100_REG_DLYTRG		0x04
#define	SPX_HPX100_REG_NUMSAMPLE	0x08
#define	SPX_HPX100_REG_CLKADDIV		0x0c
#define	SPX_HPX100_REG_CONFIG		0x10
#define	SPX_HPX100_REG_IOPORT		0x14	/* Not HPx-150 */
#define	SPX_HPX100_REG_STATUS2		0x14	/* Card type 0x000D only */
#define	SPX_HPX100_REG_STATUS		0x18
#define	SPX_HPX100_REG_FRACFREQ		0x1c
#define	SPX_HPX100_REG_MASK		0x20
#define	SPX_HPX100_REG_VERSION		0x24
#define	SPX_HPX100_REG_DAC		0x28
#define	SPX_HPX100_REG_MAGBANK		0x2c
#define	SPX_HPX100_REG_OFFSET		0x30
#define	SPX_HPX100_REG_TRGTEST		0x34
#define	SPX_HPX100_REG_ACPTEST		0x38
#define	SPX_HPX100_REG_ARPTEST		0x3c
#define	SPX_HPX100_REG_TIMALTRG		0x40
#define	SPX_HPX100_REG_TIMALACP		0x44
#define	SPX_HPX100_REG_TIMALARP		0x48
#define	SPX_HPX100_REG_SAMPLEK		0x4c
#define	SPX_HPX100_REG_DATE		0x50
#define	SPX_HPX100_REG_TRGENDRNG	0x54
#define	SPX_HPX100_REG_TRGBLANK		0x58
#define	SPX_HPX100_REG_ACPBLANK		0x5C
#define	SPX_HPX100_REG_ARPBLANK		0x60
#define	SPX_HPX100_REG_GAINA		0x64
#define	SPX_HPX100_REG_GAINB		0x68
#define	SPX_HPX100_REG_ACPFREQ		0x6C
#define	SPX_HPX100_REG_FIFOCTRL		0x70
#define	SPX_HPX100_REG_FIFOERROR	0x74
#define	SPX_HPX100_REG_FIFODATA		0x78
#define	SPX_HPX100_REG_LASTREG		0x78	/* Keep maintained! */
#define	SPX_HPX200_REG_LINKS		0x80
#define	SPX_HPX200_REG_SIG1TIME		0x84
#define	SPX_HPX200_REG_SIG1AZ		0x88
#define	SPX_HPX200_REG_SIG2TIME		0x8c
#define	SPX_HPX200_REG_SIG2AZ		0x90
#define	SPX_HPX200_REG_SPIFLASH		0x94
#define	SPX_HPX200_REG_PAGE		0x98
#define	SPX_HPX200_REG_LASTREG		0x98	/* Keep maintained! */
#define	SPX_HPX400_REG_ADCSPICTRL	0xA0
#define	SPX_HPX400_REG_ADCSPIDATA	0xA4
#define	SPX_HPX400_REG_SIG3TIME		0xA8
#define	SPX_HPX400_REG_SIG3AZ		0xAC
#define	SPX_HPX346_REG_SOFTRESET	0xB0
#define	SPX_HPX346_REG_WATCHDOG		0xB4
#define	SPX_HPX400_REG_ADCRAW		0xF0
#define	SPX_HPX400_REG_LASTREG		0xF0	/* Keep maintained! */
#define	SPX_HPX346_REG_LASTREG		0xF0	/* Keep maintained! */


/*
 * Debug flags.
 */
#define	SPX_HPX100_DEBUG_STATE		0x00000001
#define	SPX_HPX100_DEBUG_DECODE		0x00000002
#define	SPX_HPX100_DEBUG_INTERRUPT	0x00000004
#define	SPX_HPX100_DEBUG_BANK		0x00000008
#define	SPX_HPX100_DEBUG_AZIMUTHS	0x00000010
#define	SPX_HPX100_DEBUG_RANGE		0x00000020
#define	SPX_HPX100_DEBUG_INT_RATE	0x00000040
#define	SPX_HPX100_DEBUG_ALARM		0x00000080
#define	SPX_HPX100_DEBUG_AZI_INTERP	0x00000100
#define	SPX_HPX100_DEBUG_PRF		0x00000200
#define	SPX_HPX100_DEBUG_SERIAL		0x00000400
/* Top 16-bits are in common base class. */
#define	SPX_HPX100_DEBUG_READ_REG	SPX_HPX_DEBUG_READ_REG
#define	SPX_HPX100_DEBUG_READ_MEM	SPX_HPX_DEBUG_READ_MEM
#define	SPX_HPX100_DEBUG_READ_LUT	SPX_HPX_DEBUG_READ_LUT
#define	SPX_HPX100_DEBUG_READ_FLASH	SPX_HPX_DEBUG_READ_FLASH
#define	SPX_HPX100_DEBUG_WRITE_REG	SPX_HPX_DEBUG_WRITE_REG
#define	SPX_HPX100_DEBUG_WRITE_MEM	SPX_HPX_DEBUG_WRITE_MEM
#define	SPX_HPX100_DEBUG_WRITE_LUT	SPX_HPX_DEBUG_WRITE_LUT
#define	SPX_HPX100_DEBUG_WRITE_FLASH	SPX_HPX_DEBUG_WRITE_FLASH
#define	SPX_HPX100_DEBUG_VERBOSE	SPX_HPX_DEBUG_VERBOSE

/*
 * Input channel options.  These bear no resemblance to register values!
 */
#define	SPX_HPX100_VIDEO_INVALID	0	/* Same as none */
#define	SPX_HPX100_VIDEO_NONE		0	/* None */
#define	SPX_HPX100_VIDEO_A		1	/* Analogue A */
#define	SPX_HPX100_VIDEO_B		2	/* Analogue B */
#define	SPX_HPX100_VIDEO_DIG		3	/* Digital (C) */
#define	SPX_HPX100_VIDEO_COMBI_HW	4	/* Combination, HighestWins */
#define	SPX_HPX100_VIDEO_COMBI_OR	5	/* Combination, OR */
#define	SPX_HPX100_VIDEO_COMBI_ADD	6	/* Combination, ADD */
#define	SPX_HPX100_VIDEO_COMBI_LUT	7	/* Combination, LUT_D */
#define	SPX_HPX100_VIDEO_COMBI_71	8	/* Combination, 7+1 */
#define	SPX_HPX100_VIDEO_COMBI_611	9	/* Combination, 6+1+1 */
#define	SPX_HPX100_VIDEO_COMBI_5111	10	/* Combination, 5+1+1+1 */
#define	SPX_HPX100_VIDEO_COMBI_41111	11	/* Combination, 4+1+1+1+1 */
#define	SPX_HPX100_VIDEO_COMBI_4A_4B	12	/* Top nibble A, bottom B */
#define	SPX_HPX200_VIDEO_A_12BIT	13	/* 12-bit A, not HPx-1xx */
#define	SPX_HPX200_VIDEO_B_12BIT	14	/* 12-bit B, not HPx-1xx */
#define	SPX_HPX200_VIDEO_COMBI_12A_1111	15	/* Top A, 4 digital */
#define	SPX_HPX200_VIDEO_COMBI_8A_8B	16	/* Top A, bottom B */
#define	SPX_HPX200_VIDEO_HIGH_RES	17	/* On HPx-200H variant only */
#define	SPX_HPX200_VIDEO_COMBI_8A_71	18	/* Top 8A, bottom 7B+1 */
#define	SPX_HPX200_VIDEO_COMBI_8A_611	19	/* Top 8A, bottom 6B+11 */
#define	SPX_HPX200_VIDEO_COMBI_8A_5111	20	/* Top 8A, bottom 5B+111 */
#define	SPX_HPX200_VIDEO_COMBI_8A_41111	21	/* Top 8A, bottom 4B+1111 */
#define	SPX_HPX400_VIDEO_AUTO_12BIT	98	/* A or B dual-redundant */
#define	SPX_HPX400_VIDEO_AUTO		99	/* A or B dual-redundant */

/*
 * Test Pattern Generator options.  These do not match register values!
 */
#define	SPX_HPX100_TPG_DISABLE		0	/* Off */
#define	SPX_HPX100_TPG_DIG_PULS_PULS	1	/* Digital, A=Pulse B=Pulse */
#define	SPX_HPX100_TPG_DIG_PULS_RAMP	2	/* Digital, A=Pulse B=Ramp */
#define	SPX_HPX100_TPG_DIG_RAMP_PULS	3	/* Digital, A=Ramp B=Pulse */
#define	SPX_HPX100_TPG_DIG_RAMP_RAMP	4	/* Digital, A=Ramp B=Ramp */
#define	SPX_HPX100_TPG_ANALOGUE_A	5	/* Analogue ramp into A */
#define	SPX_HPX100_TPG_ANALOGUE_B	6	/* Analogue ramp into B */
#define	SPX_HPX100_TPG_DAC		7	/* Direct value from DAC */

/*
 * Azimuth options.
 */
#define	SPX_HPX100_AZI_ACP		0	/* ACP-in via optocoupler */
#define	SPX_HPX100_AZI_422		1	/* ACP-422 */
#define	SPX_HPX100_AZI_PARALLEL		2	/* Parallel azimuth */
/* Not HPx-1xx... */
#define	SPX_HPX200_AZI_OPTO		SPX_HPX100_AZI_ACP
#define	SPX_HPX200_AZI_422		SPX_HPX100_AZI_422
#define	SPX_HPX200_AZI_PARALLEL		SPX_HPX100_AZI_PARALLEL
#define	SPX_HPX200_AZI_PROG		3	/* ACP-in via comparator */
#define	SPX_HPX200_AZI_INTERNAL		4	/* Internally generated */
/* Not HPx-1xx or HPx-2xx... */
#define	SPX_HPX400_AZI_OPTO		SPX_HPX200_AZI_OPTO
#define	SPX_HPX400_AZI_422		SPX_HPX200_AZI_422
#define	SPX_HPX400_AZI_PARALLEL		SPX_HPX200_AZI_PARALLEL
#define	SPX_HPX400_AZI_PROG		SPX_HPX200_AZI_PROG
#define	SPX_HPX400_AZI_INTERNAL		SPX_HPX200_AZI_INTERNAL
#define	SPX_HPX400_AZI_OC		5	/* Open-collector */
/* Backwards compatibility. */
#define	SPX_HPX100_ACP_ACP		SPX_HPX100_AZI_ACP
#define	SPX_HPX100_ACP_422		SPX_HPX100_AZI_422
#define	SPX_HPX100_ACP_PARALLEL		SPX_HPX100_AZI_PARALLEL

/*
 * ARP options.
 */
#define	SPX_HPX100_ARP_ARP		0	/* ARP-in via optocoupler */
#define	SPX_HPX100_ARP_422		1	/* ARP-422 */
#define	SPX_HPX100_ARP_CMP		2	/* Composite on ACP line */
/* Not HPx-1xx... */
#define	SPX_HPX200_ARP_OPTO		SPX_HPX100_ARP_ARP
#define	SPX_HPX200_ARP_422		SPX_HPX100_ARP_422
#define	SPX_HPX200_ARP_CMP		SPX_HPX100_ARP_CMP
#define	SPX_HPX200_ARP_PROG		3	/* ARP-in via comparator */
/* Not HPx-1xx or HPx-2xx... */
#define	SPX_HPX400_ARP_OPTO		SPX_HPX200_ARP_OPTO
#define	SPX_HPX400_ARP_422		SPX_HPX200_ARP_422
#define	SPX_HPX400_ARP_CMP		SPX_HPX200_ARP_CMP
#define	SPX_HPX400_ARP_PROG		SPX_HPX200_ARP_PROG
#define	SPX_HPX400_ARP_OC		4	/* Open-collector */

/*
 * Trigger options.
 *
 * NB: The CMPA and CMPB options are only supported on HPx-100.
 */
#define	SPX_HPX100_TRG_TRG		0	/* TRG-in via optocoupler */
#define	SPX_HPX100_TRG_422		1	/* TRG-422 */
#define	SPX_HPX100_TRG_CMPA		2	/* Composite on video A */
#define	SPX_HPX100_TRG_CMPB		3	/* Composite on video B */
/* Not HPx-1xx... */
#define	SPX_HPX200_TRG_OPTO		SPX_HPX100_TRG_TRG
#define	SPX_HPX200_TRG_422		SPX_HPX100_TRG_422
#define	SPX_HPX200_TRG_PROG		4	/* TRG-in via comparator */
/* Not HPx-1xx or HPx-2xx... */
#define	SPX_HPX400_TRG_OPTO		SPX_HPX200_TRG_OPTO
#define	SPX_HPX400_TRG_422		SPX_HPX200_TRG_422
#define	SPX_HPX400_TRG_PROG		SPX_HPX200_TRG_PROG
#define	SPX_HPX400_TRG_OC		5	/* Open-collector */


/*
 * SIG1/2 options for HPx-200, plus SIG3 for HPx-400.
 *
 * NB: The numerical values for these do NOT match those for TRG/ACP/ARP.
 */
#define	SPX_HPX200_SIG_PROG		0	/* Single-ended to comparator*/
#define	SPX_HPX200_SIG_422		1	/* RS-422 differential input */
/* HPx-400... */
#define	SPX_HPX400_SIG_PROG		SPX_HPX200_SIG_PROG
#define	SPX_HPX400_SIG_422		SPX_HPX200_SIG_422
#define	SPX_HPX400_SIG_OPTO		2	/* Opto-coupled */
#define	SPX_HPX400_SIG_OC		3	/* Open-collector */

/*
 * LUT definitions.
 */
/* HPx-100 LUT size. */
#define	SPX_HPX100_LUT_A_SIZEBYTES	1024	/* 10-bit input */
#define	SPX_HPX100_LUT_B_SIZEBYTES	1024	/* 10-bit input */
#define	SPX_HPX100_LUT_C_SIZEBYTES	256	/* 8-bit input */
#define	SPX_HPX100_LUT_D_SIZEBYTES	256	/* 8-bit input */

/* HPx-200 and later LUT size. */
#define	SPX_HPX200_LUT_A_SIZEBYTES	4096	/* 12-bit input  */
#define	SPX_HPX200_LUT_B_SIZEBYTES	4096	/* 12-bit input  */
#define	SPX_HPX200_LUT_C_SIZEBYTES	256	/* 8-bit input */
#define	SPX_HPX200_LUT_D_SIZEBYTES	256	/* 8-bit input */

/* Biggest of all the LUTs on the HPx series. */
#define	SPX_HPX200_BIGGEST_LUT_SIZEBYTES	SPX_HPX200_LUT_A_SIZEBYTES

/*
 * DAC definitions.  Note that the digital thresholds are not supported
 * on all board types (currently just build variant 102).
 */
/* HPx-100 definitions (also compatible with later cards). */
#define	SPX_HPX100_DAC_OFFSET_A		0	/* Analogue A offset */
#define	SPX_HPX100_DAC_VREF_A		1	/* Dig 0/1 threshold */
#define	SPX_HPX100_DAC_OFFSET_B		2	/* Analogue B offset */
#define	SPX_HPX100_DAC_VREF_B		3	/* Dig 2 threshold */

/* HPx-200/400 definitions. These do NOT equate to DAC channel numbers and
 * they are NOT all supported on all cards (some signals share channels).
 */
#define	SPX_HPX200_DAC_OFFSET_A		SPX_HPX100_DAC_OFFSET_A
#define	SPX_HPX200_DAC_VREF_A		SPX_HPX100_DAC_VREF_A
#define	SPX_HPX200_DAC_OFFSET_B		SPX_HPX100_DAC_OFFSET_B
#define	SPX_HPX200_DAC_VREF_B		SPX_HPX100_DAC_VREF_B
#define	SPX_HPX200_DAC_VREF_TRG		4
#define	SPX_HPX200_DAC_VREF_ACP		5
#define	SPX_HPX200_DAC_VREF_ARP		6
#define	SPX_HPX200_DAC_VREF_SIG12	7
#define	SPX_HPX400_DAC_VREF_SIG1	8
#define	SPX_HPX400_DAC_VREF_SIG2	9
#define	SPX_HPX400_DAC_VREF_SIG3	10

/* Min/max offsets for standard HPx board.  Note that these vary slightly
 * for non-standard cards (such as 124-102 where the MAX is higher) but
 * these limits should be sufficient for clipping GUI controls etc.
 */
#define	SPX_HPX100_DAC_OFFSET_MIN	(-8.06)
#define	SPX_HPX100_DAC_OFFSET_MAX	(7.8)

/* HPx-400 digitiser stream options.  See GetNumStreams(). */
#define	SPX_HPX400_STREAM_IDX_0		0	/* Digitiser 0 */
#define	SPX_HPX400_STREAM_IDX_1		1	/* Digitiser 1 */
#define	SPX_HPX400_MAX_NUM_STREAMS	2	/* HPx-400 only */

/* Input Signal Mapping LUT definitions. */
#define	SPX_HPX400_ISM_INPUT_AUTO	0x000	/* Automatic selection */
#define	SPX_HPX400_ISM_INPUT_TRG	0x001	/* External trigger */
#define	SPX_HPX400_ISM_INPUT_ACP	0x002	/* External ACP */
#define	SPX_HPX400_ISM_INPUT_ARP	0x004	/* External ARP */
#define	SPX_HPX400_ISM_INPUT_SIG1	0x008	/* External SIG1 */
#define	SPX_HPX400_ISM_INPUT_SIG2	0x010	/* External SIG2 */
#define	SPX_HPX400_ISM_INPUT_SIG3	0x020	/* External SIG3 (=DIG0) */
#define	SPX_HPX400_ISM_INPUT_SIG4	0x040	/* External SIG4 (=DIG1) */
#define	SPX_HPX400_ISM_INPUT_SIG5	0x080	/* External SIG5 (=DIG2) */
#define	SPX_HPX400_ISM_INPUT_SIG6	0x100	/* External SIG6 (=DIG3) */
#define	SPX_HPX400_ISM_INPUT_SIG7	0x200	/* External SIG7 (=DIG4) */
#define	SPX_HPX400_ISM_OUTPUT_0_TRG	0x001	/* Output stream 0 trigger */
#define	SPX_HPX400_ISM_OUTPUT_0_ACP	0x002	/* Output stream 0 ACP */
#define	SPX_HPX400_ISM_OUTPUT_0_ARP	0x004	/* Output stream 0 ARP */
#define	SPX_HPX400_ISM_OUTPUT_0_SIG1	0x008	/* Output stream 0 SIG1/EOR */
#define	SPX_HPX400_ISM_OUTPUT_0_SIG2	0x010	/* Output stream 0 SIG2/SHM */
#define	SPX_HPX400_ISM_OUTPUT_1_TRG	0x020	/* Output stream 1 trigger */
#define	SPX_HPX400_ISM_OUTPUT_1_ACP	0x040	/* Output stream 1 ACP */
#define	SPX_HPX400_ISM_OUTPUT_1_ARP	0x080	/* Output stream 1 ARP */
#define	SPX_HPX400_ISM_OUTPUT_1_SIG1	0x100	/* Output stream 1 SIG1/EOR */
#define	SPX_HPX400_ISM_OUTPUT_1_SIG2	0x200	/* Output stream 1 SIG2/SHM */

/* Dual-operation modes. */
#define	SPX_HPX400_DUAL_NONE		0	/* No dual-operation */
#define	SPX_HPX400_DUAL_RANGE		1	/* Dual-range grabbing */
#define	SPX_HPX400_DUAL_RADAR		2	/* Dual-radar grabbing */
#define	SPX_HPX400_DUAL_REDUNDANT	3	/* Dual-redundant inputs */
#define SPX_HPX400_NUM_DUAL_MODES	4	/* Number of dual modes. */


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

/* We need to forward declare the class so it can be used in the per-stream
 * structure.
 */
class SPxHPx100Source;

/* Define a structure that may be stored in sensorSpecificData and passed
 * through the RIB to processes etc., or accessed directly via API function.
 * Although care is taken to align the fields in this structure, note that
 * the structure itself may start in unaligned locations within other message
 * buffers (e.g. when read via a network stream), so take care.
 *
 * Unknown/unavailable fields should be set to 0.
 *
 * Limit this to SPX_BLOB_SENSORDATA_SIZE bytes (32 at the time of writing).
 */
typedef struct SPxP313SensorSpecificData_tag {
    /* Word 0. */
    UINT8 id;				/* SPX_BLOB_SENSORDATA_ID_EXTEND */
    UINT8 reserved0;			/* Reserved */
    UINT16 extendedID;			/* SPX_BLOB_SENSORDATA_EXT_P313 */

    /* Word 1. */
    UINT8 config1;			/* Config Message 1 */
    UINT8 config2;			/* Config Message 2 */
    UINT8 status1;			/* Status Message 1 */
    UINT8 status2;			/* Status Message 2 */

    /* Word 2. */
    UINT8 error1;			/* Error Message 1 */
    UINT8 error2;			/* Error Message 2 */
    UINT8 testVal;			/* Test value */
    UINT8 tuneVal;			/* Tune value */

    /* Word 3. */
    UINT8 bist1;			/* BIST value 1 */
    UINT8 bist2;			/* BIST value 2 */
    UINT8 bist3;			/* BIST value 3 */
    UINT8 bist4;			/* BIST value 4 */

    /* Word 4. */
    UINT8 bist5;			/* BIST value 5 */
    UINT8 reserved4a;
    UINT8 reserved4b;
    UINT8 reserved4c;

    /* Words 5 to 7. */
    UINT32 reserved5;
    UINT32 reserved6;
    UINT32 reserved7;
} SPxP313SensorSpecificData_t;


/* Define the structure holding our per-stream settings. */
typedef struct SPxHPx100Stream_tag {
    /* General info. */
    unsigned int idx;		/* Stream index (SPX_HPX400_STREAM_IDX_0/1) */

    /* Radar input buffer where data is written. */
    SPxRIB *m_rib;		/* RIB, or NULL for direct callback only */
    int m_ribFull;		/* Do we think RIB filled up? */
    int m_ribCount;		/* Incrementing count for written returns */

    /* Configuration options. */
    unsigned int m_videoChannel;	/* One of SPX_HPX100_VIDEO_... */
    unsigned int m_endRangePulseEnable;	/* Zero or non-zero */

    /* Resolution etc. */
    unsigned int m_numSamples;		/* Set by user */
    unsigned int m_numAzimuths;		/* Set by user */
    unsigned int m_measAzimuths;	/* Read from hardware */
    UINT32 m_measAzimuthsTime;		/* Timestamp for m_measAzimuths */
    unsigned int m_numBytesPerSample;	/* Inferred from mode */
    double m_metresPerSample;		/* Metres per effective sample */

    /* Interrupt rate etc. */
    unsigned int m_intsPerSecond;	/* Requested by user, or zero */
    unsigned int m_spokesPerInt;	/* Set, or calculated from rate */
    UINT32 m_lastIntAdjustTime;		/* Timestamp for last adjustment */

    /* Range. */
    double m_rangeCorrectionMetres;	/* Correction for trigger delays */
    double m_startRangeRequestedMetres;	/* Requested start range in metres */
    double m_endRangeRequestedMetres;	/* Requested end range in metres */
    double m_startRangeAchievedMetres;	/* Achieved start range in metres */
    double m_endRangeAchievedMetres;	/* Achieved end range in metres */
    double m_endRangeAutoFactor;	/* Multiplying factor for auto mode */
    double m_endRangeAutoRounding;	/* Rounding amount for auto mode */

    /* ISM mapping options. */
    unsigned int m_ismSrcTRG;		/* Source of stream's trigger */
    unsigned int m_ismSrcACP;		/* Source of stream's ACP */
    unsigned int m_ismSrcARP;		/* Source of stream's ARP */
    unsigned int m_ismSrcSIG1;		/* Source of stream's SIG1/EOR */
    unsigned int m_ismSrcSIG2;		/* Source of stream's SIG2/SHM */

    /* Register values. */
    UINT32 m_regDlytrg;			/* Value of DLYTRG register */
    UINT32 m_regConfig;			/* Value of CONFIG register */
    UINT32 m_regMask;			/* Value of LUT D MASK register */
    UINT32 m_regFracFreq;		/* Value of FRACFREQ register */
    UINT32 m_regNumsample;		/* Value of NUMSAMPLE register */
    UINT32 m_regFifoCtrl;		/* Value of FIFOCTRL register */
    UINT32 m_regClkaddiv;		/* Register value for ADC clock div */
    UINT32 m_regTrgblank;		/* Value of TRGBLANK register */
    UINT32 m_regAcpblank;		/* Value of ACPBLANK register */
    UINT32 m_regArpblank;		/* Value of ARPBLANK register */
    UINT32 m_regTrgAlarm;		/* Value of TIMALTRG register */
    UINT32 m_regAcpAlarm;		/* Value of TIMALACP register */
    UINT32 m_regArpAlarm;		/* Value of TIMALARP register */

    /* Instantaneously measured PRF. */
    double m_prf;			/* Last measured PRF in Hz */
    unsigned int m_prfMayBeLimited;	/* Measured PRF might be wrong */

    /* Average measured PRF. */
    double m_avgPrf;			/* Last used averaged PRF in Hz */
    double m_avgPrfInstant;		/* Instantaneous averaged PRF in Hz */
    double m_avgPrfChangePercent;	/* Threshold for change-detection */
    unsigned int m_avgPrfChangeCount;	/* Count of maybe change-detections */
    UINT32 m_avgPrfLastCalcTime;		/* Time of last average */
    UINT32 m_avgPrfUpdateMsecs;			/* How often to update */
    unsigned int m_avgPrfMinPertrgSinceCalc;	/* Smallest PERTRG value */
    unsigned int m_avgPrfMaxPertrgSinceCalc;	/* Biggest PERTRG value */
    unsigned int m_avgPrfTotalPertrgSinceCalc;	/* Total of PERTRG values */
    unsigned int m_avgPrfNumPertrgs;		/* Number of PERTRG values */

    /* Input status etc. */
    unsigned int m_numInterrupts;	/* Number of interrupts */
    UINT32 m_lastAlarms;		/* Last set of alarms known */
    unsigned int m_lastBank;		/* Last bank read from */

    /* Callback functions. */
    void (*m_fnAlarm)(SPxHPx100Source *, void *);	/* Alarm function */
    void *m_fnAlarmArg;			/* Corresponding user argument */
    void (*m_fnData)(SPxHPx100Source *, void *,		/* Data function */
			SPxReturnHeader *, unsigned char *);
    void *m_fnDataArg;			/* Corresponding user argument */

    /* Azimuth fields. */
    unsigned int m_lastAzimuth;		/* Last azimuth received from card */
    unsigned int m_lastAzimuth16Bits;	/* Last azi after adjustment */
    unsigned int m_azi16AtLastChange;	/* Azimuth value when last changed */
    unsigned int m_nsSinceLastAziChange; /* Time since last change */
    double m_azi16PerNs;		/* Rotation rate */
    int m_aziInterpolation;		/* Flag to enable/disable it */
    unsigned int m_sig1Azi16;		/* Last valid azimuth for SIG1. */
    int m_sig1AziValid;			/* Is SIG1 azimuth valid? */

    /* Azimuth scaling. */
    unsigned int m_aziScaleDegs;	/* 0 or 360 for no scaling */

    /* Scan mode and detection fields. */
    SPxScanMode m_scanMode;		/* Set, or detected, mode */
    int m_scanModeOverride;		/* TRUE if overridden */
    int m_possibleModeRandom;		/* Count for mode change */
    int m_possibleModeRotating;		/* Count for mode change */

    /* Anti-clockwise. */
    int m_antiClockwise;		/* TRUE if radar is mounted upside down
					 * or rotating anti-clockwise.
					 */

    /* Packing override, one of SPX_RIB_PACKING_... values. */
    UINT8 m_packingOverride;		/* Override mode, ..INVALID for auto */

    /* Reporting information. */
    UINT32 m_reportPRFMsecs;		/* Time between reports, or 0 */
    UINT32 m_reportPRFLastTime;		/* Time of last report */
    UINT32 m_reportAziMsecs;		/* Time between reports, or 0 */
    UINT32 m_reportAziLastTime;		/* Time of last report */

    /* Statistics on radar values etc. */
    unsigned int m_statsEnabled;	/* Should we maintain statistics */
    unsigned int m_statsMinValue;	/* Lowest radar value seen */
    unsigned int m_statsMaxValue;	/* Highest radar value seen */
    UINT64 m_statsGrandTotal;		/* Total of all samples */
    UINT64 m_statsNumSamples;		/* Number of samples in GrandTotal */
    UINT64 *m_statsSampleCounts;	/* Array of counts of samples */
    unsigned int m_statsSampleCountsSize; /* Array size, entries not bytes */

    /* P313 data, if appropriate. */
    SPxP313SensorSpecificData_t m_p313Data;	/* Structure of values */
} SPxHPx100Stream_t;



/*
 * Define our class, derived from generic radar sources.
 */
class SPxHPx100Source :public SPxRadarSource, public SPxHPxCommon
{
public:
    /*
     * Public fields.
     */

    /*
     * Public functions.
     */
    /* Constructor, destructor etc. */
    SPxHPx100Source(SPxRIB *buffer, SPxRIB *buffer2=NULL);
    virtual ~SPxHPx100Source();

    /* Set second RIB if not given in constructor. */
    SPxErrorCode SetSecondRIB(SPxRIB *buffer2);

    /* Board open/close functions.  (ProbeBoard() is in SPxHPxCommon) */
    SPxErrorCode OpenBoard(unsigned int idx=0, int noInit=FALSE);
    SPxErrorCode CloseBoard(int noStop=FALSE);

    /* Override the status function from the base source class. */
    virtual UINT8 GetStatusByte(void);

    /*
     * Configuration functions.
     */
    SPxErrorCode SetVideoChannel(unsigned int video,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetVideoChannel(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    SPxErrorCode SetDigitalEnable(unsigned int enable);
    unsigned int GetDigitalEnable(void) const	{ return(m_digitalEnable); }
    SPxErrorCode SelectDigitalClock(double MHz);
    double GetDigitalClock(void) const		{ return(m_digitalClockMHz); }

    /* Resolution etc. */
    SPxErrorCode SetNumSamples(unsigned int numSamples,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetNumSamples(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    SPxErrorCode SetNumAzimuths(unsigned int numAzimuths,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetNumAzimuths(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Interrupt rate control. */
    SPxErrorCode SetInterruptsPerSecond(unsigned int numInts,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetInterruptsPerSecondReq(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    SPxErrorCode SetSpokesPerInterrupt(unsigned int numSpokes,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetSpokesPerInterrupt(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Range. */
    SPxErrorCode SetStartRangeMetres(double startMetres,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SetEndRangeMetres(double endMetres,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    double GetStartRangeMetres(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    double GetEndRangeMetres(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    double GetRequestedStartRangeMetres(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    double GetRequestedEndRangeMetres(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Range correction (to allow for trigger delays, i.e. not at zero). */
    SPxErrorCode SetRangeCorrectionMetres(double correctionMetres,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    double GetRangeCorrectionMetres(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Automatic PRF-based end range selection (if end range set to 0). */
    SPxErrorCode SetEndRangeAutoFactor(double factor,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SetEndRangeAutoRounding(double metres,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    double GetEndRangeAutoFactor(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    double GetEndRangeAutoRounding(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Average PRF monitoring. */
    SPxErrorCode SetAvgPRFIntervalMsecs(UINT32 msecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    UINT32 GetAvgPRFIntervalMsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    SPxErrorCode SetAvgPRFChangeThresholdPercent(double percent,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    double GetAvgPRFChangeThresholdPercent(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    double GetAvgPRF(unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    double GetAvgPRFInstantaneous(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Scan mode. */
    SPxErrorCode SetScanMode(int scanMode,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxScanMode GetScanMode(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    int GetScanModeOverride(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Azimuth adjustments. */
    SPxErrorCode SetAziScaleDegs(unsigned int degrees,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetAziScaleDegs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Test pattern generator. */
    SPxErrorCode SetTPG(unsigned int tpg);	/* One of SPX_HPX100_TPG...*/
    SPxErrorCode SetTPGtrgFreq(double hz);
    SPxErrorCode SetTPGacpFreq(double hz);
    SPxErrorCode SetTPGarpFreq(unsigned int numAzis);
    unsigned int GetTPG(void) const		{ return(m_tpg); }
    double GetTPGtrgFreq(void);
    double GetTPGacpFreq(void);
    unsigned int GetTPGarpFreq(void);

    /* Control-signals. */
    SPxErrorCode SetAZI(unsigned int azi);
    SPxErrorCode SetACP(unsigned int azi)	{ return(SetAZI(azi)); }
    SPxErrorCode SetARP(unsigned int arp);
    SPxErrorCode SetTRG(unsigned int trg);
    unsigned int GetAZI(void) const		{ return(m_azi); }
    unsigned int GetACP(void) const		{ return(GetAZI()); }
    unsigned int GetARP(void) const		{ return(m_arp); }
    unsigned int GetTRG(void) const		{ return(m_trg); }
    SPxErrorCode SetACPInverted(unsigned int inverted);
    SPxErrorCode SetARPInverted(unsigned int inverted);
    SPxErrorCode SetTRGInverted(unsigned int inverted);
    unsigned int GetACPInverted(void) const	{ return(m_acpInv); }
    unsigned int GetARPInverted(void) const	{ return(m_arpInv); }
    unsigned int GetTRGInverted(void) const	{ return(m_trgInv); }

    /* De-bounce on the use of those signals by each stream. */
    SPxErrorCode SetACPDeBounceUsecs(unsigned int usecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SetARPDeBounceUsecs(unsigned int usecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SetTRGDeBounceUsecs(unsigned int usecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetACPDeBounceUsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    unsigned int GetARPDeBounceUsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    unsigned int GetTRGDeBounceUsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    SPxErrorCode SetACPAlarmMsecs(unsigned int msecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SetARPAlarmMsecs(unsigned int msecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SetTRGAlarmMsecs(unsigned int msecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetACPAlarmMsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    unsigned int GetARPAlarmMsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    unsigned int GetTRGAlarmMsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* Stop-bit (end-range) control. */
    SPxErrorCode SetEndRangePulseEnable(unsigned int enable,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetEndRangePulseEnable(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;

    /* SIG1/2 signals (HPx-200 and later) and SIG3 (HPx-400 and later). */
    SPxErrorCode SetSIG1Type(unsigned int sig);	/* SPX_HPX200_SIG_... */
    SPxErrorCode SetSIG2Type(unsigned int sig);	/* SPX_HPX200_SIG_... */
    SPxErrorCode SetSIG3Type(unsigned int sig);	/* SPX_HPX200_SIG_... */
    unsigned int GetSIG1Type(void) const	{ return(m_sig1Type); }
    unsigned int GetSIG2Type(void) const	{ return(m_sig2Type); }
    unsigned int GetSIG3Type(void) const	{ return(m_sig3Type); }
    SPxErrorCode SetSIG1Inverted(unsigned int inverted);	/* 0/1 */
    SPxErrorCode SetSIG2Inverted(unsigned int inverted);	/* 0/1 */
    SPxErrorCode SetSIG3Inverted(unsigned int inverted);	/* 0/1 */
    unsigned int GetSIG1Inverted(void) const	{ return(m_sig1Inv); }
    unsigned int GetSIG2Inverted(void) const	{ return(m_sig2Inv); }
    unsigned int GetSIG3Inverted(void) const	{ return(m_sig3Inv); }
    SPxErrorCode GetSIG1Azi(unsigned int *azi16Ptr,
			double *degsPtr,
			int *validPtr,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetSIG1Rng(double *secsPtr,
			unsigned int *samplesPtr,
			double *metresPtr,
			int *validPtr,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetSIG2Azi(unsigned int *azi16Ptr,
			double *degsPtr,
			int *validPtr,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetSIG2Rng(double *secsPtr,
			unsigned int *samplesPtr,
			double *metresPtr,
			int *validPtr,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetSIG3Azi(unsigned int *azi16Ptr,
			double *degsPtr,
			int *validPtr,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetSIG3Rng(double *secsPtr,
			unsigned int *samplesPtr,
			double *metresPtr,
			int *validPtr,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* LUT configuration. */
    SPxErrorCode SetLUTA(unsigned char *buf, unsigned int bufSizeBytes)
    {
	return(WriteLUT(0, buf, bufSizeBytes));
    }
    SPxErrorCode SetLUTB(unsigned char *buf, unsigned int bufSizeBytes)
    {
	return(WriteLUT(1, buf, bufSizeBytes));
    }
    SPxErrorCode SetLUTC(unsigned char *buf, unsigned int bufSizeBytes)
    {
	return(WriteLUT(2, buf, bufSizeBytes));
    }
    SPxErrorCode SetLUTD(unsigned char *buf, unsigned int bufSizeBytes,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0)
    {
	if( streamIdx == SPX_HPX400_STREAM_IDX_0 )
	{
	    return(WriteLUT(3, buf, bufSizeBytes));
	}
	else
	{
	    return(WriteLUT(4, buf, bufSizeBytes));
	}
    }
    SPxErrorCode BuildLUTDefault(unsigned char *buf,unsigned int bufSizeBytes,
				double gain=1.0);
    SPxErrorCode BuildLUTRamp(unsigned char *buf, unsigned int bufSizeBytes,
			    unsigned int startIdx, unsigned int stopIdx,
			    unsigned char startValue, unsigned char stopValue);
    unsigned char *GetLUTBuffer(void)		{ return(m_lutBuf); }
    void SetLUTWriteMask(unsigned char mask)	{ m_lutWriteMask = mask; }
    unsigned char GetLUTWriteMask(void)		{ return(m_lutWriteMask); }
    SPxErrorCode SetLUTDMask(unsigned char aMask, unsigned char bMask,
				unsigned char cMask, unsigned char digMode=0,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetLUTDMask(unsigned char *aPtr, unsigned char *bPtr,
				unsigned char *cPtr,
				unsigned char *digModePtr=NULL,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* Input Signal Mapping options (ISM LUT). */
    SPxErrorCode SetISMSrcTRG(unsigned int src, unsigned int streamIdx);
    SPxErrorCode SetISMSrcACP(unsigned int src, unsigned int streamIdx);
    SPxErrorCode SetISMSrcARP(unsigned int src, unsigned int streamIdx);
    SPxErrorCode SetISMSrcSIG1(unsigned int src, unsigned int streamIdx);
    SPxErrorCode SetISMSrcSIG2(unsigned int src, unsigned int streamIdx);
    unsigned int GetISMSrcTRG(unsigned int streamIdx) const;
    unsigned int GetISMSrcACP(unsigned int streamIdx) const;
    unsigned int GetISMSrcARP(unsigned int streamIdx) const;
    unsigned int GetISMSrcSIG1(unsigned int streamIdx) const;
    unsigned int GetISMSrcSIG2(unsigned int streamIdx) const;

    /* Dual-operation mode. */
    SPxErrorCode SetDualModeAllowed(unsigned int mode, int isAllowed);
    SPxErrorCode SetDualMode(unsigned int mode);
    unsigned int GetDualMode(void) const	{ return(m_dualMode); }

    /* Dual-redundant status. */
    unsigned int GetDualRedundantCurrentGroup(void) const
					{ return(m_dualRedCurrentGroup); }

    /* LED mapping option. */
    SPxErrorCode SetLEDSource(unsigned int streamIdx);
    unsigned int GetLEDSource(void) const	{ return(m_ledSource); }

    /* Upside down. */
    void SetAntiClockwise(int antiClockwise,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    int GetAntiClockwise(unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 

    /* Azimuth interpolation. */
    SPxErrorCode SetAziInterpolation(int enable,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    int GetAziInterpolation(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 

    /* Override for the RIB packing mode. */
    SPxErrorCode SetPackingOverride(UINT8 packing,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    UINT8 GetPackingOverride(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 

    /* Reporting to connected clients. */
    void SetReportPRFMsecs(UINT32 msecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    UINT32 GetReportPRFMsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 
    void SetReportAziMsecs(UINT32 msecs,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    UINT32 GetReportAziMsecs(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 

    /* Convenience functions for gain setting.  NB: See DOCUMENTATION! */
    SPxErrorCode SetGainA(double gain);
    SPxErrorCode SetGainB(double gain);
    double GetLastGainA(void) const		{ return(m_lastGainA); }
    double GetLastGainB(void) const		{ return(m_lastGainB); }
    double GetMaxGain(void) const
    {
	if( m_is2xx )			{ return(16.0); }
	else if( m_is346 )		{ return(16.0); }
	else if( m_is4xx )		{ return(16.0); }
	else				{ return(4.0); }
    }
    SPxErrorCode StartAutoGainOffset(void);
    int IsAutoGainOffsetFinished(void)
    {
	return((m_autoGainOffsetState == 0) ? TRUE : FALSE);
    }

    /* DAC (i.e. offset) setup, but see convenience functions SetOffset() etc.
     * below.  The SetDAC() function takes a voltage (equivalent to the front
     * end voltage) whereas the SetDACDirect() function takes a register value
     * and is really for internal use only.  The SetOffset() and SetVoltage()
     * functions are the ones that will be most useful to client applications.
     */
    SPxErrorCode SetDAC(unsigned int dac, double voltage);
    double GetDAC(unsigned int dac);
    SPxErrorCode SetDACDirect(unsigned int dac, int regLevel);
    int GetDACDirect(unsigned int dac);

    /* Convenience functions for applications to use instead of the DAC
     * access functions above.  These handle differences in different cards
     * too.
     */
    SPxErrorCode SetOffsetA(double voltage);
    double GetOffsetA(void);
    SPxErrorCode SetOffsetB(double voltage);
    double GetOffsetB(void);
    SPxErrorCode SetVoltageRefA(double voltage)
    {
	return( SetDAC(SPX_HPX100_DAC_VREF_A, voltage) );
    }
    double GetVoltageRefA(void)	{ return(GetDAC(SPX_HPX100_DAC_VREF_A)); }
    SPxErrorCode SetVoltageRefB(double voltage)
    {
	return( SetDAC(SPX_HPX100_DAC_VREF_B, voltage) );
    }
    double GetVoltageRefB(void)	{ return(GetDAC(SPX_HPX100_DAC_VREF_B)); }
    SPxErrorCode SetVoltageRefTRG(double voltage)
    {
	return( SetDAC(SPX_HPX200_DAC_VREF_TRG, voltage) );
    }
    double GetVoltageRefTRG(void) { return(GetDAC(SPX_HPX200_DAC_VREF_TRG)); }
    SPxErrorCode SetVoltageRefACP(double voltage)
    {
	return( SetDAC(SPX_HPX200_DAC_VREF_ACP, voltage) );
    }
    double GetVoltageRefACP(void) { return(GetDAC(SPX_HPX200_DAC_VREF_ACP)); }
    SPxErrorCode SetVoltageRefARP(double voltage)
    {
	return( SetDAC(SPX_HPX200_DAC_VREF_ARP, voltage) );
    }
    double GetVoltageRefARP(void) { return(GetDAC(SPX_HPX200_DAC_VREF_ARP)); }
    SPxErrorCode SetVoltageRefSIG12(double voltage);
    double GetVoltageRefSIG12(void) {return(GetDAC(SPX_HPX200_DAC_VREF_SIG12));}
    SPxErrorCode SetVoltageRefSIG1(double voltage);
    double GetVoltageRefSIG1(void) {return(GetDAC(SPX_HPX400_DAC_VREF_SIG1));}
    SPxErrorCode SetVoltageRefSIG2(double voltage);
    double GetVoltageRefSIG2(void) {return(GetDAC(SPX_HPX400_DAC_VREF_SIG2));}
    SPxErrorCode SetVoltageRefSIG3(double voltage);
    double GetVoltageRefSIG3(void) {return(GetDAC(SPX_HPX400_DAC_VREF_SIG3));}

    /* General purpose I/O.
     * NB: Not supported on HPx-150 cards.
     * On HPx-100, the input and output ports are separate physical
     * connections and both can be used at the same time.
     * On HPx-200/400, the input and output ports share the same physical
     * connections and need to be configured as either inputs or outputs.
     * They default to inputs, but the SetPortOutEnable() function can be
     * used to specify which bits should be outputs.  This does nothing on
     * HPx-100.
     */
    unsigned int GetPortIn(void);
    SPxErrorCode SetPortOut(unsigned int bitMask);
    unsigned int GetPortOut(void)		{ return(m_portOut); }
    SPxErrorCode SetPortOutEnable(unsigned int enableMask);
    unsigned int GetPortOutEnable(void)		{ return(m_portOutEnable); }

    /* Status retrieval. (Lots more are in SPxHPxCommon) */
    double GetPRF(unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 
    unsigned int GetLastAzimuth(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 
    unsigned int GetLastAzimuth16Bits(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 
    unsigned int GetMeasAzimuths(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const; 
    unsigned int GetNumInterrupts(unsigned int reset=0,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetBank(unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* SPxRadarSource base class status function. */
    virtual SPxErrorCode GetInitialised(void)
    {
	/* Initialised okay if card open. */
	return( m_boardOpenFinished ? SPX_NO_ERROR : SPX_ERR_HW_NOT_OPEN);
    }

    /* Signal status/alarms. */
    SPxErrorCode GetSignalStatus(unsigned int *trgPtr,
				unsigned int *acpPtr,
				unsigned int *arpPtr,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode GetAlarmStatus(unsigned int *trgPtr,
				unsigned int *acpPtr,
				unsigned int *arpPtr,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode ClearAlarms(unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode InstallAlarmFn(void (*fn)(SPxHPx100Source *, void *),
				void *arg,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode InstallDataFn(void (*fn)(SPxHPx100Source *, void *,
						SPxReturnHeader *,
						unsigned char *),
				void *arg,
				unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* Statistics. */
    void SetStatsEnable(unsigned int enable,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    unsigned int GetStatsEnable(
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0) const;
    void GetStatsMinMaxValues(unsigned char *minPtr, unsigned char *maxPtr,
			unsigned int reset=0,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    void GetStatsMinMaxValues(unsigned int *minPtr, unsigned int *maxPtr,
			unsigned int reset=0,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    void GetStatsMeanSD(double *meanPtr, double *sdPtr, unsigned int reset=0,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* Watchdog functions. */
    SPxErrorCode SetRefreshWatchdog(int enable);
    int GetRefreshWatchdog(void) const	{ return(m_refreshWatchdog); }

    /* Bank swapping (see GetBank() and IsManualBankSwapSupported() too). */
    SPxErrorCode SetBank(unsigned int bank,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    SPxErrorCode SwapBank(unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* P313 support. */
    SPxErrorCode GetP313SensorData(SPxP313SensorSpecificData_t *p313,
			int reset=FALSE,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);

    /* Utility functions to check if certain features are available.
     * NB: These return SPX_NO_ERROR if the feature is supported.
     */
    SPxErrorCode IsCompositeTRGSupported(void) const;
    SPxErrorCode IsCompositeARPSupported(unsigned int projCode=0) const;
    SPxErrorCode IsP313CompositeARPSupported(void) const
    {
	return(IsCompositeARPSupported(313));
    }
    SPxErrorCode IsManualBankSwapSupported(void) const;
    SPxErrorCode IsEndRangePulseSupported(void) const;
    SPxErrorCode IsProgThresholdSupported(void) const;
    SPxErrorCode IsOpenCollectorSupported(void) const;
    SPxErrorCode IsNonLutGainSupported(void) const;
    SPxErrorCode IsHighResVideoSupported(void) const;
    SPxErrorCode IsDeBounceTRGSupported(void) const;
    SPxErrorCode IsDeBounceACPSupported(void) const;
    SPxErrorCode IsDeBounceARPSupported(void) const;
    SPxErrorCode IsFlashSupported(void) const;
    SPxErrorCode IsInternalACPSupported(void) const;
    SPxErrorCode IsVideoChannelSupported(unsigned int video) const;
    SPxErrorCode IsISMSupported(void) const;
    SPxErrorCode IsSIG1Supported(void) const;
    SPxErrorCode IsSIG2Supported(void) const;
    SPxErrorCode IsSIG3Supported(void) const;
    SPxErrorCode IsSIG1TypeSupported(unsigned int sig,
					int withDedicatedThresh=FALSE) const;
    SPxErrorCode IsSIG2TypeSupported(unsigned int sig,
					int withDedicatedThresh=FALSE) const;
    SPxErrorCode IsSIG3TypeSupported(unsigned int sig) const;
    SPxErrorCode IsGPIOSupported(void) const;
    SPxErrorCode AreUserLinksSupported(void) const;
    SPxErrorCode IsDualModeAvailable(unsigned int mode) const;

    /* See how many streams are supported. */
    unsigned int GetNumStreams(void) const;

    /* Basic testing functions. */
    SPxErrorCode RegTest(void);
    SPxErrorCode MemTest(void);
    SPxErrorCode LUTTest(unsigned int lutFlags=0x1F);
    /* NB: FlashTest() is in common base class */

    /* Low-level hardware access functions (not for normal use). */
    /* NB: ReadReg() and WriteReg() are in common base class. */
    SPxErrorCode ReadMem(void *dst, unsigned int src, unsigned int nBytes,
							int useDrv=0);
    SPxErrorCode WriteMem(unsigned int dst, void *src, unsigned int nBytes,
							int useDrv=0);
    SPxErrorCode ReadLUT(unsigned int lutIdx, unsigned char *buf,
				unsigned int bufSizeBytes);
    SPxErrorCode WriteLUT(unsigned int lutIdx, const unsigned char *buf,
				unsigned int bufSizeBytes);
    unsigned int GetLUTSize(unsigned int lutIdx);
    /* NB: ReadFlash() and associated functions are in common base class. */

    /* Slightly higher-level ADC register access functions for HPx-400. */
    SPxErrorCode ReadADCReg(unsigned int reg, UINT8 *valPtr);
    SPxErrorCode WriteADCReg(unsigned int reg, UINT8 value);

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /* Thread function, public so can be called by static function,
     * but should never be called by users.
     */
    void *ThreadFunc(SPxThread *thread);

    /* Debug. */
    void DumpState(const char *reason = NULL,
			unsigned int streamIdx=SPX_HPX400_STREAM_IDX_0);
    const char *GetRegName(unsigned int reg);

protected:
    /*
     * Protected fields (used by derived classes).
     */

private:
    /*
     * Private fields.
     */
    /* Per-stream information. */
    SPxHPx100Stream_t m_streams[SPX_HPX400_MAX_NUM_STREAMS];

    /* Worker-thread. */
    SPxThread *m_thread;		/* Thread handle for object */

    /* Dual-operation mode. */
    unsigned int m_dualMode;		/* SPX_HPX400_DUAL_... */
    int m_isDualModeAllowed[SPX_HPX400_NUM_DUAL_MODES]; /* Is mode allowed? */

    /* Dual-redundant information. */
    UINT32 m_dualRedLastSwitchMsecs;	/* Time of last switchover */
    unsigned int m_dualRedCurrentGroup;	/* 0 or 1 */

    /* Callback functions. */

    /* Clock information. */
    unsigned int m_clkAdDivider;	/* Actual divider for ADC clock */
    unsigned int m_nsPerMainClk;	/* Nanoseconds per CLKMAIN cycle */
    unsigned int m_nsPerAdClk;		/* Nanoseconds per ADC clock cycle */
    double m_metresPerMainClk;		/* Metres per CLKMAIN cycle */
    double m_metresPerAdClk;		/* Metres per ADC clock cycle */

    /* Common register values. */
    UINT32 m_regTrgtest;		/* Value of TRGTEST register */
    UINT32 m_regAcptest;		/* Value of ACPTEST register */
    UINT32 m_regArptest;		/* Value of ARPTEST register */
    UINT32 m_regIoport;			/* Value of IOPORT writeable bits */
    UINT32 m_regSig1;			/* Value of SIG1TIME writeable bits */
    UINT32 m_regSig2;			/* Value of SIG2TIME writeable bits */
    UINT32 m_regSig3;			/* Value of SIG3TIME writeable bits */
    UINT32 m_regGainA;			/* Value of GAINA register */
    UINT32 m_regGainB;			/* Value of GAINB register */

    /* Input signal configuration options. */
    unsigned int m_tpg;			/* One of SPX_HPX100_TPG_... */
    unsigned int m_digitalEnable;	/* Zero or non-zero */
    double m_digitalClockMHz;		/* Zero, or external clock rate */
    unsigned int m_azi;			/* AZI mode, SPX_HPX100_ACP_... */
    unsigned int m_arp;			/* ARP mode, SPX_HPX100_ARP_... */
    unsigned int m_trg;			/* TRG mode, SPX_HPX100_TRG_... */
    unsigned int m_acpInv;		/* ACP inverted, TRUE or FALSE */
    unsigned int m_arpInv;		/* ARP inverted, TRUE or FALSE */
    unsigned int m_trgInv;		/* TRG inverted, TRUE or FALSE */
    unsigned int m_sig1Type;		/* SIG1 mode, SPX_HPX200_SIG_... */
    unsigned int m_sig1Inv;		/* SIG1 inverted, TRUE or FALSE */
    unsigned int m_sig2Type;		/* SIG2 mode, SPX_HPX200_SIG_... */
    unsigned int m_sig2Inv;		/* SIG2 inverted, TRUE or FALSE */
    unsigned int m_sig3Type;		/* SIG3 mode, SPX_HPX400_SIG_... */
    unsigned int m_sig3Inv;		/* SIG3 inverted, TRUE or FALSE */

    /* LED mapping. */
    unsigned int m_ledSource;		/* Which stream should LEDs show? */

    /* Last gain values used (may not still be valid if LUT changed). */
    double m_lastGainA;			/* Last gain written to LUT A */
    double m_lastGainB;			/* Last gain written to LUT B */
    double m_bvaGain;			/* B vs A gain */

    /* Auto gain/offset. */
    int m_autoGainOffsetState;		/* 0 inactive, >0 in progress */
    UINT32 m_autoGainOffsetTimeMsecs;	/* Start time */
    unsigned int m_autoGainStartAzi;	/* 16-bit azi */
    unsigned int m_autoGainOffsetMinValue; /* Minimum value seen */
    unsigned int m_autoGainOffsetMaxValue; /* Maximum value seen */

    /* DAC levels. */
    double m_dacVoltageAoffset;		/* Voltage for analogue A offset */
    double m_dacVoltageBoffset;		/* Voltage for analogue B offset */
    double m_dacVoltageAref;		/* Voltage for reference A */
    double m_dacVoltageBref;		/* Voltage for reference B */
    double m_dacVoltageTRGref;		/* Not HPx-1xx */
    double m_dacVoltageACPref;		/* Not HPx-1xx */
    double m_dacVoltageARPref;		/* Not HPx-1xx */
    double m_dacVoltageSIG12ref;	/* Not HPx-1xx */
    double m_dacVoltageSIG1ref;		/* Not HPx-1xx or 2xx */
    double m_dacVoltageSIG2ref;		/* Not HPx-1xx or 2xx */
    double m_dacVoltageSIG3ref;		/* Not HPx-1xx or 2xx */

    /* Internal values used in DAC calculations. */
    double m_dacVref;			/* DAC reference voltage */
    double m_dacVdd;			/* DAC supply voltage */
    unsigned int m_dacGainPin;		/* Is DAC gain pin set high? */
    double m_adcFloor;			/* ADC floor voltage */
    double m_adcSpan;			/* ADC range voltage */
    double m_adcPreGain1;		/* Gain stage 1 before ADC */
    double m_adcPreGain2;		/* Gain stage 2 before ADC */
    double m_adcDacGain1;		/* Gain stage for DAC before ADC */
    double m_adcDacOffset1;		/* Offset stage for DAC before ADC */
    double m_adcDacGain2;		/* Gain stage for DAC before ADC */
    int m_dacRegLevelAoffset;		/* Register level for A offset */
    int m_dacRegLevelBoffset;		/* Register level for B offset */
    int m_dacRegLevelAref;		/* Register level for A reference */
    int m_dacRegLevelBref;		/* Register level for B reference */
    int m_dacRegLevelTRGref;		/* Not HPx-1xx */
    int m_dacRegLevelACPref;		/* Not HPx-1xx */
    int m_dacRegLevelARPref;		/* Not HPx-1xx */
    int m_dacRegLevelSIG12ref;		/* Not HPx-1xx */
    int m_dacRegLevelSIG1ref;		/* Not HPx-1xx or 2xx */
    int m_dacRegLevelSIG2ref;		/* Not HPx-1xx or 2xx */
    int m_dacRegLevelSIG3ref;		/* Not HPx-1xx or 2xx */

    /* General purpose I/O bits. */
    unsigned int m_portOut;		/* Bitmask, not in register location */
    unsigned int m_portOutEnable;	/* Bitmask, not in register location */
    unsigned int m_portIn;		/* Bitmask. */

    /* Buffer for building LUT values (big enough for largest LUT). */
    unsigned char *m_lutBuf;		/* Not necessarily used by users */
    unsigned char m_lutWriteMask;	/* Bitmask applied to LUT writes */

    /* Memory buffer. */
    unsigned char *m_memBuf;		/* Buffer/copy of hardware mem */
    unsigned int m_memBufSize;		/* Size of buffer in bytes */

    /* Watchdog refresh option. */
    int m_refreshWatchdog;		/* Zero to allow watchdog to expire */

    /* HPx-346-specific values. */
#ifdef __arm
    unsigned int m_last346IntCount;	/* As read from FPGA driver. */
#endif

    /*
     * Private functions.
     */
    /* Stream handling. */
    SPxHPx100Stream_t *getStream(unsigned int streamIdx);
    const SPxHPx100Stream_t *getConstStream(unsigned int streamIdx) const;

    /* Status retrieval. */
    int isDigitising(SPxHPx100Stream_t *stream); /* Return TRUE if active */
    int needToSwitchLUTaccess(void);	/* Return TRUE if need to switch */
    int getMinStartSamples(void);	/* Depends on board/FPGA */
    SPxErrorCode waitForDAC(void);	/* Wait until okay to write to DAC */

    /* Common register setting functions. */
    SPxErrorCode updateConfigReg(SPxHPx100Stream_t *stream);
    SPxErrorCode adjustSpokesPerInt(SPxHPx100Stream_t *stream);
    SPxErrorCode calcIoportReg(void);
    SPxErrorCode setPage(SPxHPx100Stream_t *stream, unsigned int page);

    /* Range/timing calculations. */
    SPxErrorCode updateVariousClocks(void);
    SPxErrorCode calculateSamplingInfo(SPxHPx100Stream_t *stream);

    /* Input signal mapping support. */
    SPxErrorCode buildAndWriteISM(void);

    /* Processing. */
    int processStream(SPxHPx100Stream_t *stream, UINT32 now,
				int received, int shouldProcessData);
    int readAndProcessData(SPxHPx100Stream_t *stream, UINT32 now,
				unsigned int bank);
    int processData(SPxHPx100Stream_t *stream, UINT32 now,
				unsigned char *data,
				unsigned int numSamples,
				unsigned int rawAzi,
				unsigned int trgPeriod,
				unsigned int spokeNumInSector);
    unsigned int decodeAndCheckRawAzi(SPxHPx100Stream_t *stream, UINT32 now,
					unsigned int rawAzi,
					unsigned int trgPeriod);
    unsigned int getExpectedAzisPerRev(SPxHPx100Stream_t *stream);
    void checkAutoGainOffset(void);
    SPxErrorCode checkSerialData(SPxHPx100Stream_t *stream, UINT32 now);
    SPxErrorCode checkDualRedundant(UINT32 now);

    /* P313 support. */
    void resetP313Data(SPxHPx100Stream_t *stream);
    int getP313Parameter(char *parameterName, char *valueBuf, int bufLen,
				unsigned int streamIdx);
}; /* SPxHPx100Source class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_HPX100_SOURCE_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
