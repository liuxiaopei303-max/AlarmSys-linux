/*********************************************************************
*
* (c) Copyright 2007 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxRadarSource.h,v $
* ID: $Id: SPxRadarSource.h,v 1.19 2017/03/08 14:43:07 rew Exp $
*
* Purpose:
*   Header for Radar Source class.
*
* Revision Control:
*   08/03/17 v1.19   AGC	Make Shutdown() virtual.
*
* Previous Changes:
*   23/09/16 1.18   AGC	Add Shutdown() function.
*   02/09/16 1.17   AGC	Improve mutex behaviour.
*   03/03/16 1.16   AGC	Make GetMsecsSinceLast...() functions virtual.
*   26/11/15 1.15   AGC	Support azimuth reference control.
*   29/07/15 1.14   SP 	Add Get/SetParameter() functions.
*   07/11/14 1.13   AGC	Make GetEnable() function const.
*   17/06/13 1.12   REW	Add GetSourceType().
*   06/06/13 1.11   AGC	Add enable callback.
*   25/04/12 1.10   REW	Support GetLastEndRange().
*   15/09/10 1.9    REW	Make destructor virtual.
*   30/03/09 1.8    REW	Add m_threadThinksEnabled.
*   09/02/09 1.7    REW	Add GetFirstEnabledSource(), registerReturn()
*			and status retrieval functions.
*   23/10/08 1.6    DGJ	Support GetNext()
*   24/08/07 1.5    DGJ	Add GetFirstSource(), GetNextSource(), Set/GetRIB().
*   29/06/07 1.4    DGJ	Derive from SPxObj
*   07/06/07 1.3    REW	Comments etc.
*   24/05/07 1.2    REW	Added header, changed to SPx etc.
*   03/05/07 1.1    DGJ	Initial Version
**********************************************************************/

#ifndef _SPX_RADARSOURCE_H
#define _SPX_RADARSOURCE_H

#include "SPxLibUtils/SPxCommon.h"	/* For common types */
#include "SPxLibUtils/SPxAtomic.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxObj.h"		/* For base class */
#include "SPxLibUtils/SPxTime.h"	/* For ticker functions */

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* A radar source can drive its output into a number of RIBs. The
 * constant MAX_RIBS_PER_SOURCE defines the supported number. 
 */
#define MAX_RIBS_PER_SOURCE 5

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

/* Forward define the SPxRIB class that we use. */
class SPxRIB;
class SPxRadarSource;

typedef void (*SPxRadarSourceEnableFn)(SPxRadarSource *source, void *userArg);

/*
 * Define the class.
 */
class SPxRadarSource :public SPxObj
{
private:
    /*
     * Private fields.
     */

    mutable SPxCriticalSection m_mutex;

    /* Flag to indicate activity - zero means source is inactive. */
    SPxAtomic<int> m_enable;

    /* Linked list information. */
    SPxRadarSource *m_next;
    SPxRadarSource *m_prev;

    /* List of radar input buffers this source feeds. */
    SPxRIB *m_ribs[MAX_RIBS_PER_SOURCE];
    int m_numRIBs;

    /* Status information on last return etc. */
    UINT32 m_lastReturnMsecs;		/* Time of last return */
    UINT16 m_lastReturnAzi;		/* Azimuth of last return */
    UINT32 m_lastAziChangeMsecs;	/* Time of last azimuth change */
    REAL32 m_lastNominalEndRange;	/* Nominal end range of last return */

    /* Enable/disable callback. */
    SPxRadarSourceEnableFn m_enableFn;
    void *m_enableUserArg;

    int m_aziRefModeEnabled;           /* Set the azimuth reference mode? */
    int m_northUp;                     /* Input data is north referenced? */

    /*
     * Private functions.
     */
    void callEnableFn(void);

protected:
    /* Protected functions */
    virtual void Shutdown(void);
    SPxErrorCode registerReturn(struct SPxReturnHeader_tag *hdr);

    /* Parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /* Protected fields */
    SPxAtomic<int> m_threadThinksEnabled;		/* What state is the thread using? */

    /* Source type. */
    UINT8 m_sourceType;			/* What sort of source is this? */

public:
    /*
     * Public fields.
     */

    /*
     * Public functions.
     */
    /* Constructor and destructor. */
    SPxRadarSource(SPxRIB *rib);
    virtual ~SPxRadarSource(void);
    static SPxRadarSource *GetFirstSource(void);
    static SPxRadarSource *GetFirstEnabledSource(void);

    /* Activity control. */
    void Enable(int state);
    int GetEnable(void) const
    {
	return m_enable;
    }

    SPxErrorCode SetAziRefModeEnabled(int enable);
    int GetAziRefModeEnabled(void) const { return m_aziRefModeEnabled; }

    SPxErrorCode SetNorthUp(int northUp);
    int GetNorthUp(void) const { return m_northUp; }

    /* Status retrieval. */
    virtual UINT32 GetMsecsSinceLastReturn(void);
    virtual UINT32 GetMsecsSinceLastAziChange(void);
    virtual SPxErrorCode GetInitialised(void)
    {
	/* Return SPX_NO_ERROR if all okay, or error code if not initialised
	 * correctly (e.g. no file, no hardware found, network connection
	 * failed etc.).
	 */
	return(SPX_NO_ERROR);
    }
    virtual UINT8 GetStatusByte(void)
    {
	/* Return a source-specific value, 0 if not known/implemented. */
	return(0);
    }
    virtual REAL32 GetLastEndRange(void);
    virtual UINT8 GetSourceType(void) const;

    void InstallEnableFn(SPxRadarSourceEnableFn fn, void *userArg)
    {
	m_enableFn = fn;
	m_enableUserArg = userArg;
    }

    /* Get the next source in the link */
    SPxRadarSource *GetNext(void) {return m_next;}

    /* Return the RIB associated with this source. The index
     * goes from 0 to the number of RIBs-1.
     */
    SPxRIB *GetRIB(int index);

    /* Return the number of RIBs. */
    int GetNumRIBs(void) {return m_numRIBs;}

    /* Add a rib associated with the radar source. */
    void AddRIB(SPxRIB* rib);
    SPxRadarSource *GetNextSource(void) {return m_next;}

}; /* SPxRadarSource */



/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/


#endif /* _SPX_RADARSOURCE_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
