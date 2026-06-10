/*********************************************************************
 *
 * (c) Copyright 2012 - 2016, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxP172RcInterfaceWin.h,v $
 * ID: $Id: SPxP172RcInterfaceWin.h,v 1.19 2016/05/11 13:54:42 rew Exp $
 *
 * Purpose:
 *   P172 radar control interface object. Used to handle the receipt
 *   and decoding of status and heartbeat messages.
 *
 * Revision Control:
 *   11/05/16 v1.19   SP	 Use correct deg symbol to support Unicode.
 *
 * Previous Changes:
 *   16/12/15 1.18   SP	 Correct comment.
 *   04/12/15 1.17   SP	 Support Status 0 Racon and TWS bits.
 *   27/11/15 1.16   SP  Support nav data. 
 *   26/11/15 1.15   SP  Rework to support Mk11 radar.
 *                       Deprecation of some function name and types. 
 *   28/10/15 1.14   SP  Add GetRDUSoftwareVersion().
 *                       Add GetRDUZmNumber().
 *   14/10/15 1.13   SP  Rename TxRxStatus12_t as RDUStatus12_t.
 *                       Support linking of status fields.
 *   23/09/15 1.12   SP  Rename some functions.
 *   21/09/15 1.11   SP  Changes to tidy up API.
 *   26/08/15 1.10   SP  Changes for SBS-900-4 as per KSD-4750 Issue 8.
 *                       Add IsAutoRpmAvailable().
 *                       Add TxRxStatus12_t.
 *   26/03/14 1.9    SP  Further changes for SxV support.
 *   17/02/14 1.8    SP  Add support for SxV series radars.
 *                       Derive from SPxP172RcTranslatorWin.
 *   19/06/13 1.7    SP  Add RDUStatus9AntennaPolHandler.
 *   06/06/13 1.6    SP  Add R7Status0TxReadyHandler().
 *                       Add R7Status4SigInHandler().
 *                       Add Is700TxRxControllable().
 *                       Support alarm hold-off.
 *   08/03/13 1.5    SP  Add EmergencyStop() and ResetEmergencyStop().
 *   01/03/13 1.4    SP  Add RDUStatus7PsuHandler().
 *                       Add OperatingModeHandler().
 *                       Add R8R9SxvStatus1TxPowerHandler().
 *   26/10/12 1.3    SP  Add RDUStatus6SigInAHandler() and
 *                       RDUStatus6SigInBHandler().
 *   26/10/12 1.2    SP  Further development.
 *   18/10/12 1.1    SP  Initial version.
 *
 *********************************************************************/

#pragma once

/*
 * Other headers required.
 */

/* The various classes that we use. */
#include "SPxLibWin/SPxP172RcTranslatorWin.h"

/*********************************************************************
 *
 *   Constants
 *
 *********************************************************************/

/* Number of transceiver A or B modules. */
#define SPX_P172_NUM_MODULES 6

/* Number of blanking sectors for SBS radars. */
#define SPX_P172_NUM_SECTORS 4

/* Number of blanking sectors for SxV radar. */
#define SPX_P172_SXV_NUM_SECTORS 3

/*
 * Number of fields in each status message.
 */

/* Heartbeat. */
#define SPX_P172_HEARTBEAT_MIN_FIELDS       5
#define SPX_P172_HEARTBEAT_NUM_FIELDS       9

/* SBS-700 transceiver status. */
#define SPX_P172_R7_STATUS0_MIN_FIELDS      8
#define SPX_P172_R7_STATUS0_NUM_FIELDS      8
#define SPX_P172_R7_STATUS1_MIN_FIELDS      9
#define SPX_P172_R7_STATUS1_NUM_FIELDS      9
#define SPX_P172_R7_STATUS3_MIN_FIELDS      5
#define SPX_P172_R7_STATUS3_NUM_FIELDS      5
#define SPX_P172_R7_STATUS4_MIN_FIELDS      24
#define SPX_P172_R7_STATUS4_NUM_FIELDS      24

/* SBS-700/800/900,SxV or Mk11 transceiver status. */
#define SPX_P172_STATUS0_MIN_FIELDS         8
#define SPX_P172_STATUS0_NUM_FIELDS         16
#define SPX_P172_STATUS1_MIN_FIELDS         11
#define SPX_P172_STATUS1_NUM_FIELDS         13
#define SPX_P172_STATUS2_MIN_FIELDS         6
#define SPX_P172_STATUS2_NUM_FIELDS         7
#define SPX_P172_STATUS3_MIN_FIELDS         4
#define SPX_P172_STATUS3_NUM_FIELDS         5
#define SPX_P172_STATUS4_MIN_FIELDS         3
#define SPX_P172_STATUS4_NUM_FIELDS         3
#define SPX_P172_STATUS5_MIN_FIELDS         5
#define SPX_P172_STATUS5_NUM_FIELDS         5
#define SPX_P172_STATUS8_MIN_FIELDS         3
#define SPX_P172_STATUS8_NUM_FIELDS         3
#define SPX_P172_STATUS9_MIN_FIELDS         4
#define SPX_P172_STATUS9_NUM_FIELDS         4
#define SPX_P172_STATUS10_MIN_FIELDS        8
#define SPX_P172_STATUS10_NUM_FIELDS        8
#define SPX_P172_STATUS11_MIN_FIELDS        2
#define SPX_P172_STATUS11_NUM_FIELDS        2

/* RDU status. */
#define SPX_P172_RDU_STATUS6_MIN_FIELDS     15
#define SPX_P172_RDU_STATUS6_NUM_FIELDS     15
#define SPX_P172_RDU_STATUS7_MIN_FIELDS     16
#define SPX_P172_RDU_STATUS7_NUM_FIELDS     16
#define SPX_P172_RDU_STATUS8_MIN_FIELDS     4
#define SPX_P172_RDU_STATUS8_NUM_FIELDS     4
#define SPX_P172_RDU_STATUS9_MIN_FIELDS     6
#define SPX_P172_RDU_STATUS9_NUM_FIELDS     6
#define SPX_P172_RDU_STATUS12_MIN_FIELDS    6
#define SPX_P172_RDU_STATUS12_NUM_FIELDS    6

/* Sector blanking/transmit status. */
#define SPX_P172_SECTOR_STATUS_MIN_FIELDS   5
#define SPX_P172_SECTOR_STATUS_NUM_FIELDS   5

/* Max number of fields in any status message. */
#define SPX_P172_MAX_NUM_FIELDS             24

/* Field compatibility flags. These flags are used in StatusField_t.
 * They are used to specify if a radar model is supported.
 */
#define SPX_P172_FIELD_COMPAT_R7            (1U << 0) /* SBS-700 */
#define SPX_P172_FIELD_COMPAT_R8            (1U << 1) /* SBS-800 */
#define SPX_P172_FIELD_COMPAT_R9            (1U << 2) /* SBS-900 */
#define SPX_P172_FIELD_COMPAT_SXV           (1U << 3) /* SxV/SCV */
#define SPX_P172_FIELD_COMPAT_MK11          (1U << 4) /* MK11 */

/* Field compatibility groups. */

/* SBS-800 and SBS-900 radars. */
#define SPX_P172_FIELD_COMPAT_R8R9          (SPX_P172_FIELD_COMPAT_R8 | \
                                             SPX_P172_FIELD_COMPAT_R9)

/* All SBS radars. */
#define SPX_P172_FIELD_COMPAT_SBS_ALL       (SPX_P172_FIELD_COMPAT_R7 | \
                                             SPX_P172_FIELD_COMPAT_R8R9)

/* All PCAN protocol radars. */
#define SPX_P172_FIELD_COMPAT_PCAN_ALL      (SPX_P172_FIELD_COMPAT_SXV | \
                                             SPX_P172_FIELD_COMPAT_MK11)

/* All radars. */
#define SPX_P172_FIELD_COMPAT_ALL           (SPX_P172_FIELD_COMPAT_SBS_ALL | \
                                             SPX_P172_FIELD_COMPAT_PCAN_ALL)

/* Deprecated constants included for backward compatibility. */
#define SPX_P172_R7R8R9_STATUS5_NUM_FIELDS      SPX_P172_STATUS5_NUM_FIELDS
#define SPX_P172_R8R9SXV_STATUS0_MIN_FIELDS     SPX_P172_STATUS0_MIN_FIELDS
#define SPX_P172_R8R9SXV_STATUS0_NUM_FIELDS     SPX_P172_STATUS0_NUM_FIELDS
#define SPX_P172_R8R9SXV_STATUS1_NUM_FIELDS     SPX_P172_STATUS1_NUM_FIELDS
#define SPX_P172_R8R9SXV_STATUS3_MIN_FIELDS     SPX_P172_STATUS3_MIN_FIELDS
#define SPX_P172_R8R9SXV_STATUS3_NUM_FIELDS     SPX_P172_STATUS3_NUM_FIELDS
#define SPX_P172_R8R9_STATUS2_NUM_FIELDS        SPX_P172_STATUS2_NUM_FIELDS
#define SPX_P172_R8R9_STATUS4_NUM_FIELDS        SPX_P172_STATUS4_NUM_FIELDS
#define SPX_P172_R9_STATUS10_NUM_FIELDS         SPX_P172_STATUS10_NUM_FIELDS
#define SPX_P172_SXV_STATUS8_NUM_FIELDS         SPX_P172_STATUS8_NUM_FIELDS
#define SPX_P172_SXV_STATUS9_NUM_FIELDS         SPX_P172_STATUS9_NUM_FIELDS
#define SPX_P172_SXV_STATUS11_NUM_FIELDS        SPX_P172_STATUS11_NUM_FIELDS

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

/* Forward declare any classes required. */
class SPxNavData;

/*
 * Define our class.
 */
class SPxP172RcInterfaceWin : public SPxP172RcTranslatorWin
{
public:

    /*
     * Public types.
     */

    /* Status field handler function. */
    typedef BOOL (*FieldHandlerFn_t)(SPxP172RcInterfaceWin *obj,
                                     const char *unitName,
                                     void *fieldPtr,
                                     char *newValue);

    /* User mode. */
    typedef enum
    {
        USER_MODE_OPERATOR = 0,    /* Operator mode. */
        USER_MODE_MAINTAINER = 1,  /* Maintainer mode. */
        USER_MODE_TEST = 2,        /* Test mode. */

    } UserMode_t;

    /* Display mode. */
    typedef enum
    {
        DISPLAY_MODE_SERVICE,  /* Service display. */
        DISPLAY_MODE_REMOTE    /* Remote display. */

    } DisplayMode_t;

    /* Radar units. */
    typedef enum
    {
        UNIT_RDU,
        UNIT_TXA,
        UNIT_TXB

    } Unit_t;

    /* Status field control values for internal use. */
    typedef struct
    {
        /* Control values for internal use. */
        UINT32 compatFlags;                 /* Compatibility flags. */
        FieldHandlerFn_t hdlrFn;            /* Field handler function. */
        BOOL ignoreChangeEvents;            /* Ignore non-alarm changes? */
        const char *alarmValue1;            /* Alarm if this value. */
        const char *alarmValue2;            /* Alarm if this value. */
        double lowerLimit;                  /* Alarm if value < limit. */
        double upperLimit;                  /* Alarm if value > limit. */
        unsigned int precision;             /* Num decimal places to use for display. */

    } StatusFieldCtrl_t;

    /* Status field. */
    typedef struct StatusField_tag
    {
        const char *name;                   /* Static field name. */
        const char *units;                  /* Static units name to append. */
        char value[SPX_P172_MAX_FIELD_LEN]; /* Field value string. */
        BOOL isSet;                         /* Has value been set? */
        BOOL isAlarm;                       /* Is value showing error? */
        UINT32 lastUpdateTimeMs;            /* Last update time. */
        const StatusField_tag *linkedField; /* Link to an associated field. */
        StatusFieldCtrl_t ctrl;             /* Internal field control values. */

    } StatusField_t;

    /* Radar state. */
    typedef enum
    {
        /* This is the initial state. */
        RADAR_STATE_UNKNOWN = 0,

        /* Radar is in default state. */
        RADAR_STATE_DEFAULT,

        /* Radar is connected and configured. */
        RADAR_STATE_OK,

        /* Error condition. */
        RADAR_STATE_COMFAIL,

    } RadarState_t;

    /* Heartbeat message data. */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_HEARTBEAT_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *systemType;
        const StatusField_t *systemState;
        const StatusField_t *band;
        const StatusField_t *antennaPolOptions;
        const StatusField_t *minFreq;
        const StatusField_t *maxFreq;
        const StatusField_t *antennaRpmOptions;
        const StatusField_t *minFreqS; /* SBS-900 only. */
        const StatusField_t *maxFreqS; /* SBS-900 only. */

    } Heartbeat_t;

    /* SBS-700 transceiver operating mode status (0). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_R7_STATUS0_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *txRxMode;
        const StatusField_t *txReady;
        const StatusField_t *runMode;
        const StatusField_t *pulseLength;
        const StatusField_t *monitor;
        const StatusField_t *mute;
        const StatusField_t *jitterState;

    } R7TxRxStatus0_t;

    /* SBS-700 transceiver pulse status (1). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_R7_STATUS1_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *vspWidthNs;
        const StatusField_t *vspPrfHz;
        const StatusField_t *spWidthNs;
        const StatusField_t *spPrfHz;
        const StatusField_t *mpWidthNs;
        const StatusField_t *mpPrfHz;
        const StatusField_t *lpWidthNs;
        const StatusField_t *lpPrfHz;

    } R7TxRxStatus1_t;

    /* SBS-700 transceiver version status (3). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_R7_STATUS3_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *adaptorZmNumber;
        const StatusField_t *adaptorVersion;
        const StatusField_t *mk7ZmNumber;
        const StatusField_t *mk7Version;

    } R7TxRxStatus3_t;

    /* SBS-700 transceiver BITE status (4). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_R7_STATUS4_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *temp;
        const StatusField_t *rpmFromHl;
        const StatusField_t *azimuthsPerHl;
        const StatusField_t *adaptorSyncIn;
        const StatusField_t *adaptorVideoIn;
        const StatusField_t *modulatorNeg15v;
        const StatusField_t *modulator5v;
        const StatusField_t *modulator15v;
        const StatusField_t *modulator27v;
        const StatusField_t *modulatorHv;
        const StatusField_t *modulator27vCurrent;
        const StatusField_t *modulatorHvCurrent;
        const StatusField_t *adaptor27v;
        const StatusField_t *adaptor18v;
        const StatusField_t *adaptor15v;
        const StatusField_t *adaptorNeg15v;
        const StatusField_t *adaptor12v;
        const StatusField_t *adaptorNeg12v;
        const StatusField_t *adaptor5v;
        const StatusField_t *adaptor3_3v;
        const StatusField_t *adaptor2_5v;
        const StatusField_t *heaterInVolts;
        const StatusField_t *heaterOutVolts;

    } R7TxRxStatus4_t;

    /* Transceiver operating mode status (0). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS0_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *runMode;
        const StatusField_t *mute;
        const StatusField_t *range;
        const StatusField_t *freqNum;
        const StatusField_t *chirp;
        const StatusField_t *power;
        const StatusField_t *antennaRpm;

        /* Shortcuts to additional SxV/Mk11 status fields. */
        const StatusField_t *rpiMode;
        const StatusField_t *videoRange;
        const StatusField_t *engBuild;
        const StatusField_t *enhancedPulses;
        const StatusField_t *testByte;
        const StatusField_t *sartMode;
        const StatusField_t *raconMode;
        const StatusField_t *twsMode;

    } TxRxStatus0_t;

    /* Transceiver alarm status (1). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS1_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *rxSensitivity;
        const StatusField_t *vswr;
        const StatusField_t *txPower;
        const StatusField_t *ploLock;
        const StatusField_t *synthesizerLock;
        const StatusField_t *overTemp;
        const StatusField_t *turnInfoLost;
        const StatusField_t *hlNotDetected;
        const StatusField_t *azimuthStatus;
        const StatusField_t *fpga2Failed;
        const StatusField_t *fibreLinkFailed;
        const StatusField_t *ethernetFailed;

    } TxRxStatus1_t;

    /* Transceiver gain/sea/rain status (2). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS2_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *gain;
        const StatusField_t *sea;
        const StatusField_t *rain;
        const StatusField_t *ir;
        const StatusField_t *dopplerFilterMode;
        const StatusField_t *autoSeaEnabled;

    } TxRxStatus2_t;

    /* Transceiver version status (3). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS3_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *zmNumber;
        const StatusField_t *fpgaVersion;
        const StatusField_t *softwareVersion;

        /* Shortcuts to additional SxV status fields. */
        StatusField_t *customerId;

    } TxRxStatus3_t;

    /* Transceiver temperature status (4). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS4_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *fpga1Temp;
        const StatusField_t *fpga2Temp;

    } TxRxStatus4_t;

    /* Transceiver on-time status (5). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS5_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *onHours;
        const StatusField_t *onMinutes;
        const StatusField_t *runHours;
        const StatusField_t *runMinutes;

    } TxRxStatus5_t;

    /* Transceiver core temperatures status (8). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS8_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *fpga1Temp;
        const StatusField_t *fpga2Temp;

    } TxRxStatus8_t;

    /* SxV transceiver gyro COG SOG status (9). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS9_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *gyro;
        const StatusField_t *cog;
        const StatusField_t *sog;

    } TxRxStatus9_t;

    /* Transceiver fan & communication status (10). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS10_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *fan1;
        const StatusField_t *fan2;
        const StatusField_t *fan3;
        const StatusField_t *fan4;
        const StatusField_t *fan5;
        const StatusField_t *fan6;
        const StatusField_t *rduComs;

    } TxRxStatus10_t;

    /* Transceiver gyro COG SOG status (11). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_STATUS11_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *hls;

    } TxRxStatus11_t;

    /* RDU common status (6). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_RDU_STATUS6_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *rduMode;
        const StatusField_t *txInUse;
        const StatusField_t *antennaRpm;
        const StatusField_t *azimuths;
        const StatusField_t *syncInA;
        const StatusField_t *syncInB;
        const StatusField_t *videoInA;
        const StatusField_t *videoInB;
        const StatusField_t *txComsA;
        const StatusField_t *txComsB;
        const StatusField_t *sclStatus;
        const StatusField_t *doorSwitch;
        const StatusField_t *platformSwitch;
        const StatusField_t *waveguideSwitch;

    } RDUStatus6_t;

    /* RDU BITE status (7). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_RDU_STATUS7_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *psuA;
        const StatusField_t *psuB;
        const StatusField_t *moduleIdA[SPX_P172_NUM_MODULES];
        const StatusField_t *moduleIdB[SPX_P172_NUM_MODULES];
        const StatusField_t *variant;

    } RDUStatus7_t;

    /* RDU version status (8). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_RDU_STATUS8_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *zmNumber;
        const StatusField_t *fpgaVersion;
        const StatusField_t *softwareVersion;

    } RDUStatus8_t;

    /* Antenna & waveguide status (RDU only) (9). */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_RDU_STATUS9_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *statusType;
        const StatusField_t *waveguideAirDryer;
        const StatusField_t *enhancedEncoder;
        const StatusField_t *antennaPol;
        const StatusField_t *oilLevelSensor;
        const StatusField_t *oilTempSensor;

    } RDUStatus9_t;

    /* Tranceiver down-mast slave processor status (12). Although
     * this is described as SBS-800/900 in the documentation
     * it seems more sensible to add it as an RDU status.
     */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_RDU_STATUS12_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. Note that some shortcuts
         * are not used and will be NULL. At present only
         * A1, B1, A2, B2 and A6 will be set by the message
         * received from the radar.
         */
        const StatusField_t *statusType;
        const StatusField_t *moduleTempA[SPX_P172_NUM_MODULES];
        const StatusField_t *moduleTempB[SPX_P172_NUM_MODULES];

    } RDUStatus12_t;

    /* Sector status. */
    typedef struct
    {
        /* Status fields. */
        StatusField_t fields[SPX_P172_SECTOR_STATUS_NUM_FIELDS];
        unsigned int numFields;

        /* Shortcuts to status fields. */
        const StatusField_t *num;
        const StatusField_t *startDegs;
        const StatusField_t *stopDegs;
        const StatusField_t *state;
        const StatusField_t *band;

    } SectorStatus_t;

    /* Transceiver status. */
    typedef struct
    {
        /* SBS-700 specific status. */
        R7TxRxStatus0_t r7s0;
        R7TxRxStatus1_t r7s1;
        R7TxRxStatus3_t r7s3;
        R7TxRxStatus4_t r7s4;
        StatusField_t r7TuneIndicator;

        /* SBS-700/800/900, SxV or Mk11 status. */
        TxRxStatus0_t s0;
        TxRxStatus1_t s1;
        TxRxStatus2_t s2;
        TxRxStatus3_t s3;
        TxRxStatus4_t s4;
        TxRxStatus5_t s5;
        TxRxStatus8_t s8;
        TxRxStatus9_t s9;
        TxRxStatus10_t s10;
        TxRxStatus11_t s11;

        /* Sector status. */
        SectorStatus_t blankingSectors[SPX_P172_NUM_SECTORS];
        SectorStatus_t txSectors[SPX_P172_NUM_SECTORS];

    } TxRxStatus_t;

    /* RDU status. */
    typedef struct
    {
        RDUStatus6_t s6;
        RDUStatus7_t s7;
        RDUStatus8_t s8;
        RDUStatus9_t s9;
        RDUStatus12_t s12;

    } RDUStatus_t;

    /* Deprecated types included for backward compatibility. */
    typedef TxRxStatus5_t R7R8R9SxvTxRxStatus5_t;
    typedef TxRxStatus0_t R8R9SxvTxRxStatus0_t;
    typedef TxRxStatus1_t R8R9SxvTxRxStatus1_t;
    typedef TxRxStatus3_t R8R9SxvTxRxStatus3_t;
    typedef TxRxStatus2_t R8R9TxRxStatus2_t;
    typedef TxRxStatus4_t R8R9TxRxStatus4_t;
    typedef TxRxStatus10_t R9TxRxStatus10_t;
    typedef TxRxStatus8_t SxvTxRxStatus8_t;
    typedef TxRxStatus9_t SxvTxRxStatus9_t;
    typedef TxRxStatus11_t SxvTxRxStatus11_t;
    
    /*
     * Public functions.
     */

    /* Create and destroy */
    SPxP172RcInterfaceWin(void);
    virtual ~SPxP172RcInterfaceWin(void);

    /* Nav data. */
    virtual SPxErrorCode SetNavData(SPxNavData *navData);
    virtual SPxNavData *GetNavData(void) { return m_navData; }

    /* Radar control. */
    virtual SPxErrorCode OpenConnection(void);
    virtual SPxErrorCode CloseConnection(void);
    virtual void SetUserMode(UserMode_t mode) { m_userMode = mode; }
    virtual UserMode_t GetUserMode(void) { return m_userMode; }
    virtual BOOL IsMaintainerMode(void) { return (GetUserMode() >= USER_MODE_MAINTAINER); }
    virtual BOOL IsTestMode(void) { return (GetUserMode() >= USER_MODE_TEST); }
    virtual void SetDisplayMode(DisplayMode_t type) { m_displayMode = type; }
    virtual DisplayMode_t GetDisplayMode(void) { return m_displayMode; }
    virtual void SetOfflineMode(BOOL isEnabled) { m_isOfflineMode = isEnabled; }
    virtual BOOL IsOfflineMode(void) { return m_isOfflineMode; }
    virtual void SetConfigSent(void);
    virtual BOOL IsConfigSent(void) { return m_isConfigSent; }
    virtual SPxErrorCode EmergencyStop(void);
    virtual SPxErrorCode ResetEmergencyStop(void);
    virtual BOOL IsEmergencyStopLatched(void) { return m_isEmergencyStop; }
    virtual SPxErrorCode StartAlarmHoldOff(UINT32 msecs);
    virtual BOOL IsAlarmHoldOffActive(void);

    /* Radar information. */
    virtual RadarState_t GetRadarState(void);
    virtual BOOL Is700Series(void);
    virtual BOOL Is800Series(void);
    virtual BOOL Is900Series(void);
    virtual BOOL IsSBSSeries(void);
    virtual BOOL IsSxvSeries(void);
    virtual BOOL IsMk11Series(void);
    virtual BOOL IsSystemTypeKnown(void);
    virtual BOOL IsRDUFitted(void);
    virtual BOOL IsXBandFitted(void);
    virtual BOOL IsSBandFitted(void);
    virtual BOOL IsDualBandFitted(void);
    virtual BOOL IsDualRedundantFitted(void);
    virtual BOOL IsXBandAvailable(void);
    virtual BOOL IsSBandAvailable(void);
    virtual BOOL IsTxRxAActive(void);
    virtual BOOL IsTxRxBActive(void);
    virtual BOOL IsTransmitting(BOOL activeOrX);
    virtual BOOL IsActiveDisplay(void);
    virtual BOOL IsControllable(void);
    virtual BOOL Is700TxRxControllable(BOOL active);
    virtual void SetAutoRpmAvailable(BOOL isAvail);
    virtual BOOL IsAutoRpmAvailable(void);
    virtual BOOL IsRDUStatus12Available(void);
    virtual unsigned int GetRDUSoftwareVersion(unsigned int *minorRtn);
    virtual unsigned int GetRDUZmNumber(void);

    /* Heartbeat and status. */
    virtual const Heartbeat_t *GetHeartbeat(void);
    virtual const R7TxRxStatus0_t *GetR7TxRxStatus0(BOOL active);
    virtual const R7TxRxStatus1_t *GetR7TxRxStatus1(BOOL active);
    virtual const R7TxRxStatus3_t *GetR7TxRxStatus3(BOOL active);
    virtual const R7TxRxStatus4_t *GetR7TxRxStatus4(BOOL active);
    virtual const StatusField_t *GetR7TuneIndicator(void);
    virtual const TxRxStatus0_t *GetTxRxStatus0(BOOL activeOrX);
    virtual const TxRxStatus1_t *GetTxRxStatus1(BOOL activeOrX);
    virtual const TxRxStatus3_t *GetTxRxStatus3(BOOL activeOrX);
    virtual const TxRxStatus2_t *GetTxRxStatus2(BOOL activeOrX);
    virtual const TxRxStatus4_t *GetTxRxStatus4(BOOL activeOrX);
    virtual const TxRxStatus5_t *GetTxRxStatus5(BOOL activeOrX);
    virtual const TxRxStatus8_t *GetTxRxStatus8(BOOL activeOrX);
    virtual const TxRxStatus9_t *GetTxRxStatus9(BOOL activeOrX);
    virtual const TxRxStatus10_t *GetTxRxStatus10(void);
    virtual const TxRxStatus11_t *GetTxRxStatus11(BOOL activeOrX);
    virtual const RDUStatus6_t *GetRDUStatus6(void);
    virtual const RDUStatus7_t *GetRDUStatus7(void);
    virtual const RDUStatus8_t *GetRDUStatus8(void);
    virtual const RDUStatus9_t *GetRDUStatus9(void);
    virtual const RDUStatus12_t *GetRDUStatus12(void);

    /* Sector status. */
    virtual const SectorStatus_t *GetBlankingSectorStatus(unsigned int sectorNum,
                                                          BOOL xBand);

    virtual const SectorStatus_t *GetTxSectorStatus(unsigned int sectorNum,
                                                    BOOL xBand);

    /* Message polling and background processing. */
    virtual SPxErrorCode PollForMesgs(void);

    /* Navigation messages. */
    virtual SPxErrorCode SendNavMessages(void);

    /* Backup and restore. */
    virtual SPxErrorCode StartUnitBackup(Unit_t unit, const char *filename);
    virtual SPxErrorCode RestoreUnit(Unit_t unit, const char *filename);

    /* Get field text and status. */
    virtual BOOL GetFieldText(const StatusField_t *field,
                              char *buf, unsigned int bufLen,
                              const char *notSetValue=NULL);

    /* Deprecated functions included for backward compatibility. */
    virtual const TxRxStatus5_t *GetR7R8R9SxvTxRxStatus5(BOOL activeOrX) 
    { 
        return GetTxRxStatus5(activeOrX); 
    }
    virtual const TxRxStatus0_t *GetR8R9SxvTxRxStatus0(BOOL activeOrX)   
    { 
        return GetTxRxStatus0(activeOrX); 
    }
    virtual const TxRxStatus1_t *GetR8R9SxvTxRxStatus1(BOOL activeOrX)   
    { 
        return GetTxRxStatus1(activeOrX); 
    }
    virtual const TxRxStatus3_t *GetR8R9SxvTxRxStatus3 (BOOL activeOrX)  
    { 
        return GetTxRxStatus3(activeOrX); 
    }
    virtual const TxRxStatus2_t *GetR8R9TxRxStatus2(BOOL activeOrX)      
    { 
        return GetTxRxStatus2(activeOrX); 
    }
    virtual const TxRxStatus4_t *GetR8R9TxRxStatus4(BOOL activeOrX)      
    { 
        return GetTxRxStatus4(activeOrX); 
    }
    virtual const TxRxStatus10_t *GetR9TxRxStatus10 (void)               
    { 
        return GetTxRxStatus10(); 
    }
    virtual const TxRxStatus8_t *GetSxvTxRxStatus8(BOOL activeOrX)       
    { 
        return GetTxRxStatus8(activeOrX); 
    }
    virtual const TxRxStatus9_t *GetSxvTxRxStatus9(BOOL activeOrX)       
    { 
        return GetTxRxStatus9(activeOrX); 
    }
    virtual const TxRxStatus11_t *GetSxvTxRxStatus11(BOOL activeOrX)     
    { 
        return GetTxRxStatus11(activeOrX); 
    }
    virtual const SectorStatus_t *GetSectorStatus(unsigned int sectorNum, BOOL xBand)     
    {
        return GetBlankingSectorStatus(sectorNum, xBand);
    }
    virtual void SetMaintainerMode(BOOL isEnabled) { SetUserMode(USER_MODE_MAINTAINER); }

protected:

    /*
     * Protected variables.
     */

    /*
     * Protected functions.
     */

    /* Message handling. */
    virtual const char *HandleMesg(const char *sentence);

    /* Parameters set/get */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:

    /*
     * Private types.
     */

    typedef enum
    {
        /* Temperature normal. */
        TXRX_TEMP_OK,

        /* Temperature >= 80 degrees C. */
        TXRX_TEMP_CAUTION,

        /* Temperature >= 90 degrees C. */
        TXRX_TEMP_WARNING

    } TxRxTempState_t;

    /*
     * Private functions.
     */

    /* Message handling. */
    void HandleHeartbeat(const char *sentence);
    void HandleBlankingSectorStatus(const char *sentence);
    void HandleTxSectorStatus(const char *sentence);
    void HandleR7TuneIndicator(const char *sentence);
    void HandleR7TxRxStatus0(int statusType, const char *sentence);
    void HandleR7TxRxStatus1(int statusType, const char *sentence);
    void HandleR7TxRxStatus3(int statusType, const char *sentence);
    void HandleR7TxRxStatus4(int statusType, const char *sentence);
    void HandleTxRxStatus0(int statusType, const char *sentence);
    void HandleTxRxStatus1(int statusType, const char *sentence);
    void HandleTxRxStatus2(int statusType, const char *sentence);
    void HandleTxRxStatus3(int statusType, const char *sentence);
    void HandleTxRxStatus4(int statusType, const char *sentence);
    void HandleTxRxStatus5(int statusType, const char *sentence);
    void HandleTxRxStatus8(int statusType, const char *sentence);
    void HandleTxRxStatus9(int statusType, const char *sentence);
    void HandleTxRxStatus10(int statusType, const char *sentence);
    void HandleTxRxStatus11(int statusType, const char *sentence);
    void HandleRDUStatus6(const char *sentence);
    void HandleRDUStatus7(const char *sentence);
    void HandleRDUStatus8(const char *sentence);
    void HandleRDUStatus9(const char *sentence);
    void HandleRDUStatus12(const char *sentence);
    void HandleStatus(const char *unitName, const char *sentence,
                      StatusField_t *fields, unsigned int minNumFields,
                      unsigned int maxNumFields);
    void HandleBackupData(const char *sentence);

    /* Misc. */
    void CheckExpireHeartbeat(void);
    void CheckExpireStatus(void);
    void CheckExpireStatusField(StatusField_t *field);
    BOOL CheckShowLinkedField(const StatusField_t *parentField);
    void Reset(void);
    void DoBackgroundProc(void);
    void RequestStatusUpdate(void);

    /*
     * Private static functions.
     */

    /* Field update handlers. */
    static BOOL HeartbeatSystemTypeHandler(SPxP172RcInterfaceWin *obj,
                                           const char *unitName,
                                           void *fieldPtr,
                                           char *newValue);

    static BOOL HeartbeatSystemStateHandler(SPxP172RcInterfaceWin *obj,
                                            const char *unitName,
                                            void *fieldPtr,
                                            char *newValue);

    static BOOL R7Status0TxReadyHandler(SPxP172RcInterfaceWin *obj,
                                        const char *unitName,
                                        void *fieldPtr,
                                        char *newValue);

    static BOOL R7Status4OverTempHandler(SPxP172RcInterfaceWin *obj,
                                         const char *unitName,
                                         void *fieldPtr,
                                         char *newValue);

    static BOOL R7Status4SigInHandler(SPxP172RcInterfaceWin *obj,
                                      const char *unitName,
                                      void *fieldPtr,
                                      char *newValue);

    static BOOL R7Status4HeaterVoltsHandler(SPxP172RcInterfaceWin *obj,
                                            const char *unitName,
                                            void *fieldPtr,
                                            char *newValue);

    static BOOL Status1FieldHandler(SPxP172RcInterfaceWin *obj,
                                    const char *unitName,
                                    void *fieldPtr,
                                    char *newValue);

    static BOOL Status1TxPowerHandler(SPxP172RcInterfaceWin *obj,
                                      const char *unitName,
                                      void *fieldPtr,
                                      char *newValue);

    static BOOL Status1OverTempHandler(SPxP172RcInterfaceWin *obj,
                                       const char *unitName,
                                       void *fieldPtr,
                                       char *newValue);

    static BOOL Status1AzimuthHandler(SPxP172RcInterfaceWin *obj,
                                      const char *unitName,
                                      void *fieldPtr,
                                      char *newValue);

    static BOOL Status0RunModeHandler(SPxP172RcInterfaceWin *obj,
                                      const char *unitName,
                                      void *fieldPtr,
                                      char *newValue);

    static BOOL RDUStatus6ModeHandler(SPxP172RcInterfaceWin *obj,
                                      const char *unitName,
                                      void *fieldPtr,
                                      char *newValue);

    static BOOL RDUStatus6SigInAHandler(SPxP172RcInterfaceWin *obj,
                                        const char *unitName,
                                        void *fieldPtr,
                                        char *newValue);

    static BOOL RDUStatus6SigInBHandler(SPxP172RcInterfaceWin *obj,
                                        const char *unitName,
                                        void *fieldPtr,
                                        char *newValue);

    static BOOL RDUStatus7PsuHandler(SPxP172RcInterfaceWin *obj,
                                     const char *unitName,
                                     void *fieldPtr,
                                     char *newValue);

    static BOOL RDUStatus7ModuleIdHandler(SPxP172RcInterfaceWin *obj,
                                          const char *unitName,
                                          void *fieldPtr,
                                          char *newValue);

    static BOOL RDUStatus9AntennaPolHandler(SPxP172RcInterfaceWin *obj,
                                            const char *unitName,
                                            void *fieldPtr,
                                            char *newValue);

    static BOOL RDUStatus12ModuleTempHandler(SPxP172RcInterfaceWin *obj,
                                             const char *unitName,
                                             void *fieldPtr,
                                             char *newValue);

    static BOOL SectorStateHandler(SPxP172RcInterfaceWin *obj,
                                   const char *unitName,
                                   void *fieldPtr,
                                   char *newValue);

    static BOOL AntennaSpeedHandler(SPxP172RcInterfaceWin *obj,
                                    const char *unitName,
                                    void *fieldPtr,
                                    char *newValue);

    static BOOL OperatingModeHandler(SPxP172RcInterfaceWin *obj,
                                     const char *unitName,
                                     void *fieldPtr,
                                     char *newValue);

    /*
     * Private variables.
     */

    /* Radar state. */
    RadarState_t m_radarState;       /* Current radar state. */
    UINT32 m_radarStateChangeTimeMs; /* Last state change time. */
    BOOL m_isConfigSent;             /* Radar config sent? */
    BOOL m_isDisconnectPending;      /* About to disconnect if TRUE. */
    DisplayMode_t m_displayMode;     /* Display mode. */
    BOOL m_isOfflineMode;            /* Offline mode? */
    TxRxTempState_t m_r7TxRxTempState; /* SBS-700 temperature state. */
    BOOL m_isTxRxStopPending;        /* About to stop transceiver if TRUE. */
    UserMode_t m_userMode;           /* Operator, maintainer or test mode. */
    BOOL m_isEmergencyStop;          /* Is radar held in emergency stop. */
    UINT32 m_isAlarmHoldOffStartMs;  /* Alarm hold-off start time. */
    unsigned int m_alarmHoldOffMs;   /* Alarm hold-off period in millisecs. */
    BOOL m_isAutoRpmAvailable;       /* Is auto RPM mode available. */

    /* Heartbeats. */
    UINT32 m_lastTxHeartbeatTimeMs;  /* Last transmitted heartbeat time. */
    Heartbeat_t m_heartbeat;         /* Last received heartbeat. */
    BOOL m_isRxHeartbeatOk;          /* Received heartbeat status. */

    /* Status. */
    UINT32 m_lastStatusReqTimeMs;    /* Last status request time. */
    TxRxStatus_t m_txRxStatusActX;   /* Active or X-band transceiver. */
    TxRxStatus_t m_txRxStatusResS;   /* Reserve or S-band transceiver. */
    RDUStatus_t m_RDUStatus;         /* RDU. */

    /* Backup. */
    BOOL m_isBackupInProgress;       /* Is backup in progress? */
    FILE *m_backupFileHandle;        /* Backup file to write to. */
    const char *m_backupFilename;    /* Backup file name. */
    const char *m_backupUnitName;    /* Backup unit name. */
    UINT32 m_backupStartTimeMs;      /* Start timestamp. */

    /* Nav data. */
    SPxNavData *m_navData;           /* Optional nav data object. */
    UINT32 m_lastSogCogMesgTimeMs;   /* Last SOG/COG mesg time. */
    UINT32 m_lastGyroMesgTimeMs;     /* Last GYRO mesg time. */

    /* Labels. */
    CStringA m_degLabel;             /* Degrees. */
    CStringA m_degCLabel;            /* Degrees C. */

}; /* class SPxP172RcInterfaceWin */

/*********************************************************************
 *
 * End of file
 *
 *********************************************************************/
