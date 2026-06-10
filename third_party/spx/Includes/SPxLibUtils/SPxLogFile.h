/*********************************************************************
*
* (c) Copyright 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxLogFile.h,v $
* ID: $Id: SPxLogFile.h,v 1.3 2016/10/10 12:49:17 rew Exp $
*
* Purpose:
*	Header file for SPxLogFile functionality.
*
*
* Revision Control:
*   10/10/16 v1.3    REW	ID is optional in writeToFile().
*				Rename LogOwnPosition() to LogPosition() and
*				add 'whichPosition'.
*
* Previous Changes:
*   30/09/16 1.2    REW	Support refLatLong in LogTrack().
*   23/09/16 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_LOGFILE_H
#define _SPX_LOGFILE_H

/*
 * Other headers required.
 */
#include "SPxLibUtils/SPxObj.h"			/* For base class */
#include "SPxLibUtils/SPxTypes.h"		/* For SPxUnits... */

/*********************************************************************
*
*   Constants
*
**********************************************************************/


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward-declare classes we refer to, so header file order doesn't matter. */
class SPxCriticalSection;
class SPxRadarTrack;
class SPxNavData;

/*
 * Define our class.
 */
class SPxLogFile :public SPxObj
{
public:
    /*
     * Public functions.
     */
    /* Constructor/destructor. */
    SPxLogFile(const char *appName=NULL);
    virtual ~SPxLogFile(void);

    /* Generic parameter control. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /* Options. */
    SPxErrorCode SetUnitsDist(SPxUnitsDist_t option);
    SPxUnitsDist_t GetUnitsDist(void) const { return(m_unitsDist); }
    SPxErrorCode SetUnitsSpeed(SPxUnitsSpeed_t option);
    SPxUnitsSpeed_t GetUnitsSpeed(void) const { return(m_unitsSpeed); }
    SPxErrorCode SetFormatLatLong(SPxFormatLatLong_t option);
    SPxFormatLatLong_t GetFormatLatLong(void) const { return(m_formatLatLong);}

    /* Functions for logging particular things. */
    SPxErrorCode LogMsg(struct SPxTime_tag *epochTime,
				const char *msg,
				struct SPxLatLong_tag *latLong=NULL);
    SPxErrorCode LogTrack(struct SPxTime_tag *epochTime,
				SPxRadarTrack *track,
				struct SPxLatLong_tag *refLatLong=NULL,
				const char *reason=NULL);
    SPxErrorCode LogPosition(struct SPxTime_tag *epochTime,
				SPxNavData *navData,
				const char *whichPosition=NULL);
    SPxErrorCode LogPosition(struct SPxTime_tag *epochTime,
				struct SPxLatLong_tag *latLong,
				const char *whichPosition=NULL);

    /*
     * Static functions for class.
     */

private:
    /*
     * Private variables.
     */
    /* Protection mutex. */
    SPxCriticalSection *m_mutex;		/* Safety mutex */

    /* Application name. */
    char *m_appName;			/* Copy of application name, or NULL */

    /* File information. */
    FILE *m_file;			/* The file, or NULL if not open. */
    char *m_filename;			/* Name of the file */

    /* Options. */
    SPxUnitsDist_t m_unitsDist;		/* Units to report */
    SPxUnitsSpeed_t m_unitsSpeed;	/* Units to report */
    SPxFormatLatLong_t m_formatLatLong;	/* Formatting to use */

    /* Flags. */
    int m_pendingBanner;		/* Banner line is pending. */
    int m_pendingHeadings;		/* Headings line is pending. */

    /*
     * Private functions.
     */
    SPxErrorCode writeToFile(struct SPxTime_tag *epochTime,
			    const char *entryType,
			    int idValid, unsigned int id,
			    struct SPxLatLong_tag *latLong,
			    int posValid, double rangeMetres, double aziDegs,
			    int velValid, double speedMps, double courseDegs,
			    const char *comment);


}; /* SPxLogFile */


#endif /* _SPX_LOGFILE_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
