/*********************************************************************
 *
 * (c) Copyright 2015-2017, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxP313RadarController.h,v $
 * ID: $Id: SPxP313RadarController.h,v 1.3 2017/03/16 15:30:30 rew Exp $
 *
 * Purpose:
 *   Class for controlling Project P313 radar via serial interface.
 *
 *
 * Revision Control:
 *  16/03/17 v1.3    AJH    	Add SPxP313Status_t.
 *
 * Previous Changes:
 *  15/12/15 1.2    AJH	    Use SPxAutoPtr for m_port.
 *			    Add mutex protection for message sending.
 *  14/12/15 1.1    AJH     Initial version.
 **********************************************************************/

/* SPx headers. */
#include "SPxLibData/SPxSerialPort.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxTimer.h"

/* Definitions for the message contents. */
#define SPX_P313_CTRL1_PM_ON		0x08
#define SPX_P313_CTRL1_PM_TX		0x04
#define SPX_P313_CTRL1_RANGE_NM		0x00
#define SPX_P313_CTRL1_RANGE_KM		0x01
#define SPX_P313_CTRL1_RANGE_YD		0x02

#define	SPX_P313_CTRL2_SWEPT_GAIN	0x10
#define SPX_P313_CTRL2_TRANSMIT_ON	0x08
#define SPX_P313_CTRL2_AFC_ON		0x04
#define SPX_P313_CTRL2_LONG_PULSE	0x00
#define SPX_P313_CTRL2_MEDIUM_PULSE	0x01
#define SPX_P313_CTRL2_SHORT_PULSE	0x02

/* The number of blanking sectors supported. */
#define SPX_P313_NUM_SECTOR		16

/* The types of message supported. */
enum SPxP313MsgType_t
{
    SPX_P313_MSG_CONTROL,
    SPX_P313_MSG_LO_TUNE,
    SPX_P313_MSG_PM_TUNE,
    SPX_P313_MSG_XR_ADJ,
    SPX_P313_MSG_XT_ADJ,
    SPX_P313_MSG_TEST
};

/* The default baud rate and parity. */
#define SPX_P313_DEFAULT_BAUD	    76800
#define SPX_P313_DEFAULT_PARITY	    SPX_SERIAL_PARITY_EVEN

/* The type and contents of a message. */
struct SPxP313Data_t
{
    SPxP313MsgType_t messageType;
    unsigned int ctrlValue;
    unsigned int dataValue;
};

/* Data for a blanking sector. */
struct SPxP313Sector_t
{
    unsigned int active;
    double aziStart;
    double aziEnd;
};

/* The contents of decoded status information. */
struct SPxP313Status_t
{
    unsigned int config1;		/* Config Message 1 */
    unsigned int config2;		/* Config Message 2 */
    unsigned int status1;		/* Status Message 1 */
    unsigned int status2;		/* Status Message 2 */
    unsigned int error1;		/* Error Message 1 */
    unsigned int error2;		/* Error Message 2 */
    unsigned int testVal;		/* Test value */
    unsigned int tuneVal;		/* Tune value */
    unsigned int bist1;			/* BIST value 1 */
    unsigned int bist2;			/* BIST value 2 */
    unsigned int bist3;			/* BIST value 3 */
    unsigned int bist4;			/* BIST value 4 */
    unsigned int bist5;			/* BIST value 5 */
};

class SPxP313RadarController
{
public:
    /* Constructor and destructor. */
    SPxP313RadarController(void);
    ~SPxP313RadarController(void);

    /* Public methods. */
    SPxErrorCode Create(const char *name,
			unsigned int baud = SPX_P313_DEFAULT_BAUD,
			SPxSerialParity_t parity = SPX_P313_DEFAULT_PARITY);
    void SetMessage(const SPxP313Data_t &data);
    void SetInterval(unsigned int msec);
    void SetCurrAzimuth(double azimuth);
    SPxErrorCode SetBlankSector(unsigned int sectNum, double aziStart,
	double aziEnd);
    void SetLogFile(const char *logfile);
    void GetStatus(SPxP313Status_t *status) { *status = m_status; }

private:
    /* The serial port used for sending messages. */
    SPxAutoPtr<SPxSerialPort> m_port;

    /* The data structure for the message currently being sent. */
    SPxP313Data_t m_data;

    /* The received radar status. */
    SPxP313Status_t m_status;

    /* Protect access to the sending method. */
    SPxCriticalSection m_mutex;

    /* The interval between scheduled message transmissions, in msec. */
    unsigned int m_interval;

    /* The timer object for scheduled messages. */
    SPxTimer m_timer;

    /* The current azimuth from the radar. */
    double m_currAzimuth;

    /* Blanking sector definitions. */
    SPxP313Sector_t m_blankSector[SPX_P313_NUM_SECTOR];

    /* The debug log file name. */
    const char *m_logFile;

    /* Private functions. */
    unsigned int inBlankSector(void);
    void sendMessage(void);
    void handleTimer(void);
    unsigned int handleInput(const unsigned char *payload, unsigned int numBytes);
    void log(const char *format, ...);
    static unsigned int handleInputWrapper(SPxSerialPort *port, void *userArg, SPxTime_tag *timePtr, const unsigned char *payload, unsigned int numBytes);
    static void handleTimerWrapper(void *userArg);
};

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
