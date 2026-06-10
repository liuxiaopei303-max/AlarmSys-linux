/*********************************************************************
*
* (c) Copyright 2015 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxAccessControl.h,v $
* ID: $Id: SPxAccessControl.h,v 1.2 2016/01/06 15:42:25 rew Exp $
*
* Purpose:
*	Header for SPxAccessControl class, which provides basic password
*	access to admin functions.
*
*
* Revision Control:
*   06/01/16 v1.2    REW	Rename maintainer to admin.
*
* Previous Changes:
*   22/12/15 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_ACCESS_CONTROL_H
#define _SPX_ACCESS_CONTROL_H

/*
 * Other headers required.
 */
/* For base class. */
#include "SPxLibUtils/SPxObj.h"

/* We need SPxLibUtils for common types, events, errors etc. */
#include "SPxLibUtils/SPxError.h"

/* Callback support. */
#include "SPxLibUtils/SPxCallbackList.h"

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Max length of password.  Must be multiple of 8. */
#define	SPX_ACCESS_CONTROL_MAXLEN	64	/* Max num characters */

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

/* Forward declare classes in case headers are in the wrong order. */
class SPxCallbackList;


/*
 * Define our class, derived from SPx base object.
 */
class SPxAccessControl :public SPxObj
{
public:
    /*
     * Public fields.
     */

    /*
     * Public functions.
     */
    /* Constructor, destructor etc. */
    SPxAccessControl(void);
    virtual ~SPxAccessControl();

    /* Password setting. */
    SPxErrorCode SetAdminPassword(const char *password, int hashed=FALSE);

    /* Lock/unlock actions. */
    SPxErrorCode Unlock(const char *password);
    SPxErrorCode Lock(void);

    /* Status retrieval. */
    int IsAdmin(void) const { return(m_isAdmin); }

    /* Event callbacks for lock/unlocks. */
    SPxErrorCode AddEventCallback(SPxCallbackListFn_t fn, void *userArg);
    SPxErrorCode RemoveEventCallback(SPxCallbackListFn_t fn, void *userArg);

    /* Generic parameter assignment. */
    virtual int SetParameter(char *parameterName, char *parameterValue);
    virtual int GetParameter(char *parameterName, char *valueBuf, int bufLen);

private:
    /*
     * Private fields.
     */
    /* Current status. */
    int m_isAdmin;			/* TRUE if admin unlocked */

    /* Password. */
    char *m_hashedPassword;		/* Encrypted password, or null/empty */

    /* Callback list for state changes. */
    SPxCallbackList *m_callbackList;

    /*
     * Private functions.
     */
    SPxErrorCode hashString(const char *str, char *buf, int bufLen);
}; /* SPxAccessControl class */


/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

#endif /* _SPX_ACCESS_CONTROL_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
