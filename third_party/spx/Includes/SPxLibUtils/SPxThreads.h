/*********************************************************************
*
* (c) Copyright 2007 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxThreads.h,v $
* ID: $Id: SPxThreads.h,v 1.12 2017/05/09 13:23:46 rew Exp $
*
* Purpose:
*   Header for SPx Thread wrapper class.
*
*   See usage notes in SPxThread.cpp file.
*
*
* Revision Control:
*   09/05/17 v1.12   AGC	Add SPxGetCurrentThreadID().
*				Add SPxIsCurrentThreadID().
*
* Previous Changes:
*   08/03/17 1.11   AGC	Use atomic varible for thread running flag.
*   02/09/16 1.10   AGC	Use atomic variable for stop requested.
*   07/11/14 1.9    AGC	Add Detach() function.
*   27/08/14 1.8    AGC	Add thread ID debugging.
*   18/11/13 1.7    AGC	Fix clang warning.
*   08/11/13 1.6    AGC	Fix cppcheck warnings.
*   17/11/11 1.5    AGC	Add private copy-constructor and assignment operator.
*   15/09/10 1.4    REW	Make destructor virtual.
*   22/09/07 1.3    DGJ	Add GetHandle()
*   08/06/07 1.2    REW	Linux implementation.
*   07/06/07 1.1    REW	Initial Version
**********************************************************************/

#ifndef _SPX_THREADS_H
#define _SPX_THREADS_H

/* Need common definitions */
#include "SPxLibUtils/SPxCommon.h"

#include "SPxLibUtils/SPxAtomic.h"

/* Need error codes. */
#include "SPxLibUtils/SPxError.h"

/* Need thread types/functions on linux. */
#ifndef _WIN32
#include <pthread.h>
#endif


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

#ifdef	_WIN32
typedef DWORD SPxThreadID_t;
#else
typedef pthread_t SPxThreadID_t;
#endif

/*
 * SPx Thread class
 */
class SPxThread
{
private:
	/*
	 * Private fields.
	 */
#ifdef	_WIN32
	HANDLE m_thread;	    /* Handle of thread */
	SPxThreadID_t m_threadId;   /* ID of thread */
#else
	SPxThreadID_t m_thread;	    /* Handle of thread (may not be scalar). */
#endif
	
	/* Is a thread currently active? */
	SPxAtomic<int> m_threadRunning;	
	
	/* Flag to say the thread should terminate. */
	SPxAtomic<int> m_stopRequested;

	/* User data to pass to thread function. */
	void *m_userArgs;

	/*
	 * Private functions.
	 */
	SPxThread(const SPxThread&) {}
	// cppcheck-suppress operatorEqVarError
	SPxThread& operator=(const SPxThread&) { return *this; }

public:
	/*
	 * Public fields.
	 */

	/*
	 * Public functions.
	 */
	/* Constructor and destructor. */
	SPxThread(void);
	virtual ~SPxThread(void);

	/* Creation. */
	SPxErrorCode StartThread(void *(*fn)(SPxThread *), void *userArgs);

	/* Function for use by the child to retrieve the user arguments
	 * given to it by the parent.
	 */
	void *GetUserArgs(void)
	{
	    return(m_userArgs);
	}

	/* Stopping.  First function called by parent to ask child
	 * to stop.  Second function used by child to see if parent
	 * has asked it to stop (returns non-zero if so, zero otherwise).
	 */
	SPxErrorCode RequestStop(void)
	{
	    m_stopRequested = 1;
	    return(SPX_NO_ERROR);
	}
	int IsStopRequested(void)
	{
	    return(m_stopRequested);
	}

	/* Waiting (parent waits for child completion). */
	SPxErrorCode WaitForThread(void);

	/* Detach from thread. */
	void Detach(void);

	/* Retrieve handle. */
#ifdef	_WIN32
	HANDLE GetHandle(void) { return m_thread; }	/* Handle of thread */
#else
	pthread_t GetHandle(void) { return m_thread; }	/* Handle of thread */
#endif
};

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

extern SPxThreadID_t SPxGetCurrentThreadID(void);
extern int SPxIsCurrentThreadID(SPxThreadID_t id);

#endif /* _SPX_THREADS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
