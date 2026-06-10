/*********************************************************************
*
* (c) Copyright 2009 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxNavData.h,v $
* ID: $Id: SPxNavData.h,v 1.53 2017/06/22 14:20:31 rew Exp $
*
* Purpose:
*	Header for SPxNavData objects which support navigation data
*	interfaces on moving platform systems.
*
*
* Revision Control:
*   22/06/17 v1.53   AGC	Add SetAltitude().
*
* Previous Changes:
*   16/05/17 1.52   DGJ	Additional fields for heading update rate
*   16/05/17 1.51   AGC	Overhaul velocity over water support.
*			Support VDR message.
*   12/04/17 1.50   AGC	Make ReadMoreData() protected for testing.
*   23/01/17 1.49   AGC	Add version of SetLatLong() with timestamp for testing.
*   23/01/17 1.48   AGC	Improve mutex behaviour.
*   04/08/16 1.47   SP 	Add GetHeadingExclusive().
*   28/07/16 1.46   SP 	Add optional time arg to SetNMEA().
*   16/06/16 1.45   SP 	Support logging to SQL database.
*   08/04/16 1.44   AGC	Add BuildTHS().
*   09/03/16 1.43   REW	Support heading (compass) correction.
*   03/03/16 1.42   DGJ	Support for platform position updates
*   26/11/15 1.41   AGC	Add SetMagneticVariation().
*   09/11/15 1.40   AGC	Add SetVelocity().
*			Add ExtrapolateLatLong().
*   29/09/15 1.39   REW	Add BuildTTM().
*   26/08/15 1.38   AGC	Add constructor for testing.
*			Add interface address for network receipt.
*			Separate timestamp for magnetic variation.
*   22/04/15 1.37   AGC	Remove limit on number of radars.
*			Support devices (same as radars).
*			Support altitude offset for devices/radars.
*   26/03/15 1.36   SP 	Fix last change.
*   17/03/15 1.35   SP 	Fix bug in IsDestNetwork().
*   06/03/15 1.34   AGC	Make GetHeading/GetVelocity() const.
*   26/01/15 1.33   SP 	Add GetPacketSender() function.
*   18/08/14 1.32   DGJ Add support for heading change rate
*   01/04/14 1.31   SP 	Add SetPitchRoll().
*   27/03/14 1.30   SP 	Add BuildXDR().
*                       Don't enable XDR by default.
*   26/03/14 1.29   SP 	Support XDR sentence for pitch and roll.
*   13/12/13 1.28   SP 	Add SetHeading() and HeadingDegs config param support.
*   13/12/13 1.27   AGC	Add support for building many more sentences.
*			Add decode flag to SetNMEA().
*   04/10/13 1.26   AGC	Simplify headers.
*   27/09/13 1.25   SP 	Add SPX_NAV_DATA_REF_PLATFORM.
*                       Add SetLatLong().
*                       Add IsSourceSerial(), etc.
*   09/09/13 1.24   AGC	Extract altitude from GGA/GNS sentences.
*			Add GetAltitude() function.
*   24/05/13 1.23   AGC	Add optional fix char to generated RMC.
*			Support interface config on dest network.
*   10/04/13 1.22   AGC	Complete support for ZDA,GLL,GNS,DTM,VBW,VTG.
*   08/04/13 1.21   AGC	Use SPxNMEADecoder.
*   08/03/13 1.20   AGC	Add SetRawInput().
*   12/02/13 1.19   AGC	Increase max number of radars to 8.
*   25/01/13 1.18   AGC	Support raw output.
*   12/12/12 1.17   AGC	Add SetOutputOnly().
*			Add SetLogFile().
*   26/10/12 1.16   AGC	Add SetLive() function.
*   14/05/12 1.15   SP 	Make CloseSource() public.
*   13/04/12 1.14   REW	Keep timestamps for certain sentences.
*			Support GGA and VHW sentences.
*   16/03/12 1.13   REW	Support NMEA checksums.
*   08/03/12 1.12   SP 	Support radar offset from platform.
*   28/02/12 1.11   REW	Support GetHeading() from HDT/HDG/HDM sentences.
*			Add BuildHDx() and BuildRMC() functions.
*   08/02/12 1.10   SP 	Add SetSourceNetwork() with UINT32 addr.
*   06/04/11 1.9    AGC	Use new SPxPacketDecoderNet class.
*   12/11/10 1.8    REW	Add GetGpsTimeDate() and GetMagneticVariation().
*   08/11/10 1.7    SP 	Add closeSource(), plus some get functions.
*			Use const for addr strings.
*   28/10/10 1.6    AGC	Add SetDestNetwork() that takes UINT32 addr.
*   15/10/10 1.5    REW	Add Get/SetOverrideCourse().
*   19/03/09 1.4    REW	Add GetMotion().
*   13/03/09 1.3    REW	Add network source/dest support.
*   27/02/09 1.2    REW	Add navigation fields, timestamps etc.
*   26/02/09 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_NAVDATA_H
#define _SPX_NAVDATA_H

/*
 * Other headers required.
 */

/* We need SPxCallbackListFn_t. */
#include "SPxLibUtils/SPxCallbackList.h"

/* We need SPxCriticalSection. */
#include "SPxLibUtils/SPxCriticalSection.h"

/* We need SPxErrorCode enumeration. */
#include "SPxLibUtils/SPxError.h"

/* We need SPxTime_t. */
#include "SPxLibUtils/SPxTime.h"

/* We need the NMEA decoder. */
#include "SPxLibNet/SPxNMEADecoder.h"

/* We need generic network definitions. */
#include "SPxLibSc/SPxScNet.h"

/* We need our base classes. */
#include "SPxLibUtils/SPxObj.h"
#include "SPxLibNet/SPxNetAddr.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Debug flags. */
#define	SPX_NAVDATA_DEBUG_RECV		0x00000001	/* Any receipt */
#define	SPX_NAVDATA_DEBUG_MSGS		0x00000002	/* Whole msgs */
#define	SPX_NAVDATA_DEBUG_DECODES	0x00000004	/* Decoded msgs */
#define	SPX_NAVDATA_DEBUG_UPDATES	0x00000008	/* Nav updates */
#define	SPX_NAVDATA_DEBUG_VERBOSE	0x80000000	/* Verbose */

/* This may be passed in place of a radar/device index.
 * The function will use/return the platform position.
 */
#define SPX_NAV_DATA_REF_PLATFORM       ((unsigned int)-1)

/* NMEA navigation messages to handle. */
#define SPX_NAVDATA_NMEA_RMC		0x00000001
#define SPX_NAVDATA_NMEA_THS		0x00000002
#define SPX_NAVDATA_NMEA_HDT		0x00000004
#define SPX_NAVDATA_NMEA_HDM		0x00000008
#define SPX_NAVDATA_NMEA_HDG		0x00000010
#define SPX_NAVDATA_NMEA_GGA		0x00000020
#define SPX_NAVDATA_NMEA_VHW		0x00000040
#define SPX_NAVDATA_NMEA_ZDA		0x00000080
#define SPX_NAVDATA_NMEA_GLL		0x00000100
#define SPX_NAVDATA_NMEA_GNS		0x00000200
#define SPX_NAVDATA_NMEA_DTM		0x00000400
#define SPX_NAVDATA_NMEA_VBW		0x00000800
#define SPX_NAVDATA_NMEA_VTG		0x00001000
#define SPX_NAVDATA_NMEA_XDR            0x00002000
#define SPX_NAVDATA_NMEA_VDR		0x00004000

/* Default NMEA navigation messages to handle. */
#define SPX_NAVDATA_NMEA_DEFAULT	(SPX_NAVDATA_NMEA_RMC	\
                     | SPX_NAVDATA_NMEA_THS	\
                     | SPX_NAVDATA_NMEA_HDT	\
                     | SPX_NAVDATA_NMEA_HDM	\
                     | SPX_NAVDATA_NMEA_HDG	\
                     | SPX_NAVDATA_NMEA_GGA	\
                     | SPX_NAVDATA_NMEA_VHW)


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

/* Forward-declare other classes we use. */
class SPxCallbackList;
class SPxRadarReplay;
class SPxPacketDecoder;
class SPxPacketSender;
class SPxThread;
class SPxSQLDatabase;

/* SPxNavData device specific information. */
struct SPxNavDataDeviceInfo
{
    /* The following are platform referenced. */
    double xOffsetMetres;   /* X offset from platform in metres. */
    double yOffsetMetres;   /* Y offset from platform in metres. */
    double altOffsetMetres;	/* Altitude offset from platform in metres. */
};

/*
 * Define our class, derived from the standard SPx base class.
 */
class SPxNavData :public SPxObj
{
public:
    /*
     * Public types.
     */
    typedef void (*DeviceFn)(unsigned int deviceIndex,
    const SPxNavDataDeviceInfo *deviceInfo, void *userArg);

    /*
     * Public functions.
     */
    /* Constructor, destructor etc. */
    SPxNavData();
    virtual ~SPxNavData();

    /* Source control. */
    SPxErrorCode SetSourceSerial(const char *name=NULL,
                                 unsigned int baud=4800);
    const char *GetSourceSerialName(void) { return m_serialPortName; }
    unsigned int GetSourceSerialBaud(void) {return m_serialBaud; }
    SPxErrorCode SetSourceNetwork(const char *addr=NULL, int port=0, const char *ifAddr=NULL);
    SPxErrorCode SetSourceNetwork(UINT32 addr, int port=0, const char *ifAddr=NULL);
    UINT32 GetSourceNetworkAddr(void);
    int GetSourceNetworkPort(void);
    UINT32 GetSourceNetworkIfAddr(void);
    const char *GetSourceNetworkIfAddrString(void);
    void CloseSource(void);
    void SetRawInput(int rawInput);
    int GetRawInput(void);
    int IsSourceSerial(void)
    {
        return m_serialPortOpen ? TRUE : FALSE;
    }
    int IsSourceNetwork(void)
    {
        return ((m_sock != SPX_SOCK_INVALID) && m_packetDecoder) ? TRUE:FALSE;
    }

    /* Destination control. */
    SPxErrorCode SetDestNetwork(const char *addr=NULL, int port=0, const char *ifAddr=NULL);
    SPxErrorCode SetDestNetwork(UINT32 addr, int port=0, const char *ifAddr=NULL);
    UINT32 GetDestNetworkAddr(void);
    int GetDestNetworkPort(void);
    UINT32 GetDestNetworkIfAddr(void);
    const char *GetDestNetworkIfAddrString(void);
    void SetDestNetworkEnabled(int isEnabled);
    int GetDestNetworkEnabled(void);
    void SetOutputOnly(int isOutputOnly);
    int GetOutputOnly(void);
    void SetRawOutput(int rawOutput);
    int GetRawOutput(void);
    int IsDestNetwork(void)
    {
        return (m_packetSender) ? TRUE:FALSE;
    }

    /* Callback support. */
    SPxErrorCode AddCallback(SPxCallbackListFn_t fn, void *userObj)
    {
    return( (SPxErrorCode)m_callbackList->AddCallback(fn, userObj) );
    }
    SPxErrorCode RemoveCallback(SPxCallbackListFn_t fn, void *userObj)
    {
    return( (SPxErrorCode)m_callbackList->RemoveCallback(fn, userObj) );
    }

    /* Data setting interface. */
    SPxErrorCode SetNMEA(const char *nmea,
                         int live=TRUE,
                         int decode=TRUE,
                         const SPxTime_t *time=NULL);

    /* Use live data immediately. */
    SPxErrorCode SetLive(void);

    /* Set/get types of NMEA message to handle. */
    SPxErrorCode SetNMEAMessages(UINT32 nmeaMessages);
    UINT32 GetNMEAMessages(void);

    SPxErrorCode ExtrapolateLatLong(double intervalSecs);

    /* Overrides for certain values. */
    SPxErrorCode SetOverrideCourse(double degrees)
    {
    /* Negative value means override turned off.*/
    while( degrees >= 360.0 )	{ degrees -= 360.0; }
    m_overrideCourseDegs = degrees;
    return(SPX_NO_ERROR);
    }
    double GetOverrideCourse(void)	{ return(m_overrideCourseDegs); }

    /* Data retrieval interface.  Most functions can optionally provide
     * the time associated with the requested nav data via the timestampPtr
     * location.  They return SPX_NO_ERROR on success or an error code
     * otherwise (e.g. SPX_ERR_NAV_DATA_EXPIRED if they give a value but
     * it may not be up to date).  The GetHeading() function should provide
     * a heading relative to true north, but may revert to the course instead
     * if heading is not available explicitly.  The courseNotHeadingFlag
     * will be set to TRUE in that instance, if a pointer is given.
     */
    char GetLastGpsStatusChar(void)	{ return(m_lastGpsStatusChar); }
    SPxErrorCode SetHeading(double headingTrueDegs, int setCourse=TRUE);
    SPxErrorCode GetHeading(double *headingTrueDegsPtr,
                int *courseNotHeadingFlagPtr=NULL,
                SPxTime_t *timestampPtr=NULL) const;
    SPxErrorCode GetHeadingExclusive(double *headingTrueDegsPtr,
                         SPxTime_t *timestampPtr=NULL,
                     int isMutexAlreadyLocked=FALSE) const;
    SPxErrorCode SetHeadingCorrection(double headingCorrectionDegs);
    SPxErrorCode GetHeadingCorrection(double *headingCorrectionDegsPtr) const;
    SPxErrorCode GetHeadingChangeRate(double *headingChangeRatePtr);

    SPxErrorCode SetVelocity(double speedMps, double courseDegs,
                 int setTimestamp=TRUE);
    SPxErrorCode GetVelocity(double *speedMpsPtr, double *courseDegsPtr,
                 SPxTime_t *timestampPtr=NULL,
                 int isMutexAlreadyLocked=FALSE) const;
    SPxErrorCode SetVelocityOverWater(double speedMps, double courseDegs);
    SPxErrorCode GetVelocityOverWater(double *speedMpsPtr, double *courseDegsPtr,
                      SPxTime_t *timestampPtr=NULL,
                      int isMutexAlreadyLocked=FALSE) const;
    SPxErrorCode SetLatLong(double latDegs, double longDegs);
    SPxErrorCode GetLatLong(double *latDegsPtr, double *longDegsPtr,
                            SPxTime_t *timestampPtr=NULL,
                int isMutexAlreadyLocked=FALSE);
    SPxErrorCode GetRadarLatLong(double *latDegsPtr, double *longDegsPtr,
                                 SPxTime_t *timestampPtr=NULL,
                                 unsigned int radarIndex=0);
    SPxErrorCode GetDeviceLatLong(double *latDegsPtr, double *longDegsPtr,
                                 SPxTime_t *timestampPtr=NULL,
                                 unsigned int deviceIndex=0);
    SPxErrorCode SetMagneticVariation(double magVarDegs);
    SPxErrorCode GetMagneticVariation(double *magVarPtr,
                      SPxTime_t *timestampPtr=NULL,
                      int isMutexAlreadyLocked=FALSE);
    SPxErrorCode SetAltitude(double altMetres);
    SPxErrorCode GetAltitude(double *altMetresPtr,
                 double *geoidSepMetresPtr,
                 SPxTime_t *timestampPtr=NULL,
                 int isMutexAlreadyLocked=FALSE);
    SPxErrorCode GetRadarAltitude(double *altMetresPtr,
                  double *geoidSepMetresPtr,
                  SPxTime_t *timestampPtr=NULL,
                  unsigned int radarIndex=0);
    SPxErrorCode GetDeviceAltitude(double *altMetresPtr,
                   double *geoidSepMetresPtr,
                   SPxTime_t *timestampPtr=NULL,
                   unsigned int deviceIndex=0);
    SPxErrorCode GetMotion(double secs, double *xMetresPtr,double *yMetresPtr);
    SPxErrorCode GetGpsTimeDate(double *gpsTimePtr, unsigned int *gpsDatePtr,
                SPxTime_t *timestampPtr=NULL);
    SPxErrorCode SetPitchRoll(double pitchDegs,
                              double rollDegs);
    SPxErrorCode GetPitchRoll(double *pitchDegsPtr,
                              double *rollDegsPtr,
                              SPxTime_t *timestampPtr=NULL);

    /* Status.  */
    double GetSecsSinceLastNMEA(void)
    {
    SPxTime_t now;
    SPxTimeGetEpoch(&now);
    return( SPxTimeGetDiffEpoch(&m_timeOfLastNMEA, &now) );
    }

    /* Radar offset relative to platform. */
    SPxErrorCode SetRadarOffsetMetres(double xMetres,
                                      double yMetres,
                                      unsigned int radarIndex=0);

    SPxErrorCode GetRadarOffsetMetres(double *xMetres,
                                      double *yMetres,
                                      unsigned int radarIndex=0);

    /*  Radar offset relative to true north. */
    SPxErrorCode GetRadarOffsetMetresNorth(double *xMetres,
                                           double *yMetres,
                                           unsigned int radarIndex=0);

    /* Radar altitude relative to platform. */
    SPxErrorCode SetRadarOffsetAltitudeMetres(double altMetres,
                          unsigned int radarIndex=0);

    SPxErrorCode GetRadarOffsetAltitudeMetres(double *altMetres,
                          unsigned int radarIndex=0);

    /* Device offset relative to platform. */
    SPxErrorCode SetDeviceOffsetMetres(double xMetres,
                                       double yMetres,
                                       unsigned int deviceIndex=0);

    SPxErrorCode GetDeviceOffsetMetres(double *xMetres,
                                       double *yMetres,
                                       unsigned int deviceIndex=0);

    /*  Device offset relative to true north. */
    SPxErrorCode GetDeviceOffsetMetresNorth(double *xMetres,
                                            double *yMetres,
                                            unsigned int deviceIndex=0);

    /* Device altitude relative to platform. */
    SPxErrorCode SetDeviceOffsetAltitudeMetres(double altMetres,
                           unsigned int deviceIndex=0);

    SPxErrorCode GetDeviceOffsetAltitudeMetres(double *altMetres,
                           unsigned int deviceIndex=0);

    SPxErrorCode IterateDevices(DeviceFn fn, void *userArg);

    /* Generic parameter control. */
    int SetParameter(char *parameterName, char *parameterValue);
    int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /* Get the platform position. */
    void GetPlatformPosition(double *x, double *y);
    double GetCourseSpeedFixRate(void);

    /* Get rate of arrival of heading messages (Hz) */
    double GetHeadingRate(void);

    /* Debug. */
    SPxErrorCode SetLogFile(const char *filename);
    const char *GetLogFile(void) { return(m_logFilename ? m_logFilename : ""); }
    void SetDebug(UINT32 flags)	{ m_debug = flags; }
    UINT32 GetDebug(void)	{ return(m_debug); }

    /* Thread entry point, not for calling by users. */
    void *ThreadFn(SPxThread *thread);

    /* Access to the network packer sender. */
    SPxPacketSender *GetPacketSender(void) { return m_packetSender; }

    /* Install an optional SQL database. */
    SPxErrorCode SetSQLDatabase(SPxSQLDatabase *sqlDB, UINT8 channelIndex=0);
    SPxSQLDatabase *GetSQLDatabase(void) { return m_sqlDB; }

    /*
     * Static functions for class.
     */
    static SPxNavData *GetNavDataObject(void);	/* Get global obj, if any */

    /* Convenience functions to build NMEA sentences. */
    static SPxErrorCode BuildRMC(char *sentence, size_t sizeBytes,
                 double latDegs, double longDegs,
                 double speedMps, double courseDegs,
                 double magVarDegs = 0.0,
                 const SPxTime_t *timestamp = NULL,
                 int isVoid = FALSE, char fixChar = 0);
    static SPxErrorCode BuildTHS(char *sentence, size_t sizeBytes,
                 double headingDegs, char fixChar);
    static SPxErrorCode BuildHDG(char *sentence, size_t sizeBytes,
                 double headingDegs, double deviationDegs = 0.0,
                 double magVarDegs = 0.0);
    static SPxErrorCode BuildHDx(char *sentence, size_t sizeBytes,
                 double headingDegs, int headingIsMagnetic);
    static SPxErrorCode BuildGGA(char *sentence, size_t sizeBytes,
                 double latDegs, double longDegs,
                 double altitudeMetres = 0.0,
                 const SPxTime_t *timestamp = NULL);
    static SPxErrorCode BuildGLL(char *sentence, size_t sizeBytes,
                 double latDegs, double longDegs,
                 const SPxTime_t *timestamp = NULL,
                 int isVoid = FALSE, char fixChar = 0);
    static SPxErrorCode BuildGNS(char *sentence, size_t sizeBytes,
                 double latDegs, double longDegs,
                 double altitudeMetres = 0.0,
                 const SPxTime_t *timestamp = NULL);
    static SPxErrorCode BuildVBW(char *sentence, size_t sizeBytes,
                 int isWaterValid, int isGroundValid,
                 double longWaterSpeedKnots, double transWaterSpeedKnots,
                 double longGroundSpeedKnots, double transGroundSpeedKnots);
    static SPxErrorCode BuildVHW(char *sentence, size_t sizeBytes,
                 double speedMps, double headingDegs,
                 double magVarDegs = 0.0);
    static SPxErrorCode BuildVDR(char *sentence, size_t sizeBytes,
                 double setMps, double driftDegs,
                 double magVarDegs = 0.0);
    static SPxErrorCode BuildVTG(char *sentence, size_t sizeBytes,
                 double speedMps, double headingDegs,
                 double magVarDegs = 0.0, char fixChar = 0);
    static SPxErrorCode BuildXDR(char *sentence, size_t sizeBytes,
                                 double pitchDegs, double rollDegs);
    static SPxErrorCode BuildTTM(char *sentence, size_t sizeBytes,
                const SPxTime_t *timestamp,
                unsigned int id,
                double rangeMetres, double azimuthDegs,
                double speedMps, double courseDegs,
                double cpaMetres, double cpaSeconds,
                int isProvisional, int isLost, int isManual);

    /* Convenience functions for checksum handling (return -1 on error). */
    static int CalcNMEAChecksum(const char *sentence);
    static int ReadNMEAChecksum(const char *sentence);

protected:
    /* Testing functions. */
    explicit SPxNavData(SPxPacketDecoder *packetDecoder);
    void Shutdown(void);
    SPxErrorCode SetLatLong(double latDegs, double longDegs, const SPxTime_t *timestamp);
    virtual int ReadMoreData(char *buf, unsigned int bufSize);

protected:
    /*
     * Private fields.
     */
    /* General control. */
    UINT32 m_debug;				/* Debug flags */
    char *m_logFilename;			/* Log file name. */
    FILE *m_logFile;				/* Log file. */
    mutable SPxCriticalSection m_mutex;		/* Mutex */

    /* Platform position. This is relative to an arbitrary (0,0) reference
     * and is updated on each RMC message using speed and course for the
     * interval. A client can query changes in the position to get a measure
     * of the platform position change over an interval.
     */
    double m_xPosition, m_yPosition;
    UINT32 m_lastDeltaPositionTime;
    double m_averagePeriodOfSpeedCourseFixesMS;

    /* Filter for heading change rate */
    double m_headingChangeRateFilter;
    int m_headingUpdatesWithNoChange;

    /* Serial port input. */
    char *m_serialPortName;			/* Name of serial port */
    int m_serialPortOpen;			/* Is port open? */
#ifdef _WIN32
    HANDLE m_serialPort;			/* Handle of serial port */
#else
    int m_serialPort;				/* FD of serial port */
#endif
    unsigned int m_serialBaud;

    /* Network input. */
    SOCKET_TYPE m_sock;				/* Input socket */
    struct sockaddr_in m_sockAddress;		/* Socket address */
    char *m_sockBuf;				/* For reading from socket */
    unsigned int m_sockBufSize;			/* Size of buffer */
    char *m_sockIfAddrString;			/* Socket interface address */
    struct sockaddr_in m_sockIfAddr;
    SPxPacketDecoder *m_packetDecoderExt;	/* External packet decoder object */
    SPxPacketDecoder *m_packetDecoder;		/* Packet decoder object */
    SPxAtomic<int> m_rawInput;			/* True if raw input */

    /* Network output. */
    SPxPacketSender *m_packetSender;		/* Packet sender object */
    int m_destNetEnabled;			/* True if enabled */
    SPxAtomic<int> m_outputOnly;		/* True if output only. */
    int m_rawOutput;				/* True if raw output. */

    /* Callback support. */
    SPxCallbackList *m_callbackList;		/* List of callbacks */

    /* Worker thread. */
    SPxThread *m_thread;			/* Worker thread */

    /* Timestamps etc. */
    SPxTime_t m_timeOfLastNMEA;			/* Time of last NMEA msg */
    SPxTime_t m_timeOfLastNMEAReplay;		/* Time of last NMEA replay */
    SPxTime_t m_timeOfLastRMC;			/* Time of last RMC sentence*/
    SPxTime_t m_timeOfLastTHS;			/* Time of last THS sentence*/

    /* Overrides. */
    double m_overrideCourseDegs;		/* Negative for disabled */

    /* Last known updates. */
    char m_lastGpsStatusChar;			/* 'A'ctive or 'V'oid */
    double m_headingDegs;			/* Heading in degrees */
    //double m_headingChangeRateDegsSec;		/* Heading change rate */
    double m_headingChangeRateFilteredDegsSec;	/* Filtered heading change rate */
    double m_previousHeadingUsedForChange;
    SPxTime_t m_timeForPreviousHeadingChange;
    double m_headingMessageRate;		/* Rate of arrival of heading messages (Hz) */
    SPxTime_t m_headingTimestamp;		/* Time of the value */
    double m_headingCorrectionDegs;		/* Correction to apply */
    double m_speedMps;				/* Speed in metres per sec */
    double m_courseDegs;			/* Course in degrees */
    SPxTime_t m_speedCourseTimestamp;		/* Time of the fix */
    double m_speedOverWaterMps;			/* Speed over water metres per sec */
    double m_courseOverWaterDegs;		/* Course over water in degrees */
    SPxTime_t m_speedCourseOverWaterTimestamp;	/* Time of the fix */
    double m_setMps;				/* Water speed in metres per sec */
    double m_driftDegs;				/* Water course in degrees */
    SPxTime_t m_setDriftTimestamp;		/* Time of the fix */
    double m_magneticVariationDegs;		/* Mag variation, +ve east */
    SPxTime_t m_magVarTimestamp;		/* Time of the fix */
    double m_latDegs;				/* Latitude, +ve north */
    double m_longDegs;				/* Longitude, +ve east */
    SPxTime_t m_latLongTimestamp;		/* Time of the fix */
    double m_altitudeMetres;			/* Altitude in metres */
    double m_geoidSepMetres;			/* Geoid seperation in metres */
    SPxTime_t m_altTimestamp;			/* Time of the fix */
    double m_gpsTime;				/* Time within day */
    unsigned int m_gpsDate;			/* Date as DDMMYY */
    double m_pitchDegs;                         /* Pitch in degrees. */
    double m_rollDegs;                          /* Roll in degrees. */
    SPxTime_t m_pitchRollTimestamp;             /* Time of last pitch/roll message. */

    /* NMEA parsing. */
    UINT32 m_nmeaMessages;			/* NMEA message types to handle. */
    SPxNMEADecoder m_nmeaDecoder;		/* NMEA parsing object. */

    /* Optional SQL database. */
    SPxSQLDatabase *m_sqlDB;
    UINT8 m_sqlDBChannelIndex;

    /* Device specific information. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /*
     * Private functions.
     */
    void commonConstructor(SPxPacketDecoder *packetDecoder);
    void rotateXY(double degs,
                  double x, double y,
                  double *xRtn, double *yRtn);
    SPxErrorCode handleRMC(void);
    SPxErrorCode handleHDx(void);
    SPxErrorCode handleGGAorGNS(void);
    SPxErrorCode handleVHW(void);
    SPxErrorCode handleZDA(void);
    SPxErrorCode handleGLL(void);
    SPxErrorCode handleDTM(void);
    SPxErrorCode handleVBW(void);
    SPxErrorCode handleVDR(void);
    SPxErrorCode handleVTG(void);
    SPxErrorCode handleXDR(void);
    SPxErrorCode getVelocity(double *speedMpsPtr, double *courseDegsPtr,
                 SPxTime_t *timestampPtr=NULL) const;
    SPxErrorCode getVelocityOverWater(double *speedMpsPtr, double *courseDegsPtr,
                      SPxTime_t *timestampPtr=NULL) const;
    SPxErrorCode getHeading(double *headingDegsPtr,
                int *courseNotHeadingFlagPtr=NULL,
                SPxTime_t *timestampPtr=NULL) const;
    SPxErrorCode getHeadingChangeRate(double *headingChangeRatePtr) const;
    SPxErrorCode getMotion(double secs, double *xMetresPtr, double *yMetresPtr);
    SPxErrorCode getDeviceOffsetMetresNorth(double *xMetres,
                        double *yMetres,
                        unsigned int deviceIndex);

    /* Handler for network packets. */
    static void netHandler(SPxPacketDecoder *decoder, void *navDataObj,
               UINT32 packetType, struct sockaddr_in *fromAddr,
               SPxTime_t *timestamp, const unsigned char *payload,
               unsigned int numBytes);

    /* Handler for raw network packets. */
    static void rawNetHandler(SPxPacketDecoder *decoder, void *navDataObj,
                  UINT32 packetType, struct sockaddr_in *fromAddr,
                  SPxTime_t *timestamp, const unsigned char *payload,
                  unsigned int numBytes);


}; /* SPxNavData class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* _SPX_NAVDATA_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
