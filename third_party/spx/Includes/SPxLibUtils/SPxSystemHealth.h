/*********************************************************************
*
* (c) Copyright 2009 - 2017, Cambridge Pixel Ltd.
*
* File:  $RCSfile: SPxSystemHealth.h,v $
* ID: $Id: SPxSystemHealth.h,v 1.11 2017/03/08 14:49:06 rew Exp $
*
* Purpose:
*	Header file for SPxSystemHealth class.
*
*
* Revision Control:
*   08/03/17 v1.11   AGC	Health byte now atomic.
*
* Previous Changes:
*   23/02/17 1.10   REW	Add SPX_SYSTEM_HEALTH_BIT_SLAVE support.
*   02/11/16 1.9    REW	Add SPX_SYSTEM_HEALTH_SRC_SECONDARY_ERROR.
*   04/10/13 1.8    AGC	Simplify headers.
*   13/09/11 1.7    AGC	Fix icc warning by making GetSourceStatus() const.
*   07/09/11 1.6    AGC	Add source health handler function support.
*			Add AV_SETUP and AV_NO_DATA system health source bits.
*   07/10/10 1.5    REW	Remove duplicate DeleteAll() functions.
*   24/09/10 1.4    REW	Add using SPxObj::GetParameter().
*   15/09/10 1.3    REW	Make destructor virtual.
*   10/02/09 1.2    REW	Define access control for base class.
*   09/02/09 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_SYSTEMHEALTH_H
#define _SPX_SYSTEMHEALTH_H

/*
 * Other headers required.
 */

/* We need SPxLibUtils for common types. */
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxObj.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Definitions for the health byte bitmask. */
#define	SPX_SYSTEM_HEALTH_BIT_ERROR	0x01	/* Generic error */
#define	SPX_SYSTEM_HEALTH_BIT_LICENSE	0x02	/* Unlicensed */
#define	SPX_SYSTEM_HEALTH_BIT_OVERLOAD	0x04	/* Overload detected */
#define	SPX_SYSTEM_HEALTH_BIT_SLAVE	0x08	/* Operating in slave mode */

/* Definitions for the SourceStatus values (top byte of 16-bit value, the
 * bottom byte is source-specific).
 */
#define	SPX_SYSTEM_HEALTH_SRC_SETUP	    0x0100	/* Radar source - setup problem (e.g. net) */
#define	SPX_SYSTEM_HEALTH_SRC_NO_RTNS	    0x0200	/* Radar source - No returns detected */
#define	SPX_SYSTEM_HEALTH_SRC_NO_AZI	    0x0400	/* Radar source - No azimuth changes */
#define	SPX_SYSTEM_HEALTH_SRC_SECONDARY_ERROR 0x0800	/* Secondary data error */
#define SPX_SYSTEM_HEALTH_SRC_AV_SETUP	    0x1000	/* AV source/encoder - setup problem (e.g. net) */
#define SPX_SYSTEM_HEALTH_SRC_AV_NO_DATA    0x2000	/* AV source - No input data */
#define	SPX_SYSTEM_HEALTH_SRC_ERROR	    0x8000	/* Radar source - General error */


/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* System source health retrieval function. */
typedef UINT16 (*SPxSystemHealthSourceFn_t)(void *userArg);

/* Structure for storing source health retrieval callbacks. */
struct SPxSystemHealthSourceItem
{
    SPxSystemHealthSourceFn_t fn;
    void *userArg;
};

/*
 * Define the class.
 */
class SPxSystemHealth :public SPxObj
{
public:
    /*
     * Public functions.
     */
    /* Constructor/destructor. */
    SPxSystemHealth(void);
    virtual ~SPxSystemHealth(void);

    /* Add/remove custom source health retrieval handlers. */
    SPxErrorCode AddSourceHandler(SPxSystemHealthSourceFn_t fn, void* userArg);
    SPxErrorCode RemoveSourceHandler(SPxSystemHealthSourceFn_t fn, void* userArg);

    /* Retrieval functions. */
    UINT8 GetHealth(void)		{ return static_cast<UINT8>(m_health); }
    UINT16 GetSourceStatus(void) const;

    /* Set/latch functions. */
    void LatchOverload(void)	{ m_health |= SPX_SYSTEM_HEALTH_BIT_OVERLOAD;}
    void LatchLicense(void)	{ m_health |= SPX_SYSTEM_HEALTH_BIT_LICENSE;}
    void LatchError(void)	{ m_health |= SPX_SYSTEM_HEALTH_BIT_ERROR; }
    void LatchSlave(void)	{ m_health |= SPX_SYSTEM_HEALTH_BIT_SLAVE; }

    /* Reset functions. */
    void ClearOverload(void)	{ m_health &= ~SPX_SYSTEM_HEALTH_BIT_OVERLOAD;}
    void ClearLicense(void)	{ m_health &= ~SPX_SYSTEM_HEALTH_BIT_LICENSE;}
    void ClearError(void)	{ m_health &= ~SPX_SYSTEM_HEALTH_BIT_ERROR; }
    void ClearSlave(void)	{ m_health &= ~SPX_SYSTEM_HEALTH_BIT_SLAVE; }
    void ClearAll(void)		{ m_health = 0; }

    /* Generic parameter control. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    using SPxObj::GetParameter;
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

    /*
     * Public variables.
     */

    /*
     * Static functions for class.
     */
    static SPxSystemHealth *GetSystemHealthObject(void);
    static SPxErrorCode GlobalLatchOverload(void);
    static SPxErrorCode GlobalLatchLicense(void);
    static SPxErrorCode GlobalLatchError(void);

private:
    /*
     * Private functions.
     */

    /*
     * Private variables.
     */
    SPxAtomic<int> m_health;		/* Composite health byte */

    /*
     * Static variables for class.
     */

}; /* SPxSystemHealth */


/*********************************************************************
*
*   Macros
*
**********************************************************************/


/*********************************************************************
*
*   Variable definitions
*
**********************************************************************/

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_SYSTEMHEALTH_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
