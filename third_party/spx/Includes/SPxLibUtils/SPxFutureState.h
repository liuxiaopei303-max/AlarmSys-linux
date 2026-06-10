/*********************************************************************
*
* (c) Copyright 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxFutureState.h,v $
* ID: $Id: SPxFutureState.h,v 1.1 2014/11/07 14:42:47 rew Exp $
*
* Purpose:
*   Header for SPxFutureState object used internally by the
*   SPxPromise and SPxFuture objects to pass a value between them
*   from the promise object to the future object (and any copies).
*
* Revision Control:
*   07/11/14 v1.1    AGC	Initial version.
*
* Previous Changes:
**********************************************************************/

#ifndef _SPX_FUTURE_STATE_H
#define _SPX_FUTURE_STATE_H

/* Other headers required. */
#include "SPxLibUtils/SPxCondVar.h"
#include "SPxLibUtils/SPxCriticalSection.h"
#include "SPxLibUtils/SPxRefWrapper.h"
#include "SPxLibUtils/SPxTypes.h"

/* Generic state object. */
template<typename T>
struct SPxFutureState
{
    SPxFutureState() : cond(TRUE), mutex(TRUE, TRUE), ready(false), value() { }
    explicit SPxFutureState(typename SPxAddCrefIfNotRef<T>::type val) : cond(FALSE), mutex(TRUE), ready(true), value(val) { }

    mutable SPxCondVar cond; /* Event for signalling completion. */
    mutable SPxCriticalSection mutex;
    bool ready;		    /* Boolean for indicating complete state. */
    T value;		    /* Result value. */

    void wait() const
    {
	SPxAutoLock lock(mutex);
	if( ready )
	{
	    return;
	}
	cond.WaitForever(mutex);
    }
    SPxErrorCode wait_for(UINT32 timeoutMS) const
    {
	SPxAutoLock lock(mutex);
	if( ready )
	{
	    return SPX_NO_ERROR;
	}
	return cond.WaitTimedMsecs(mutex, timeoutMS);
    }
};

/* Specialisation that allows references to be stored. */
template<typename T>
struct SPxFutureState<T&>
{
    SPxFutureState() : cond(TRUE), mutex(TRUE, TRUE), ready(false), value() { }
    explicit SPxFutureState(typename SPxAddCrefIfNotRef<T>::type val) : cond(FALSE), mutex(TRUE), ready(true), value(val) { }

    mutable SPxCondVar cond; /* Event for signalling completion. */
    mutable SPxCriticalSection mutex;
    bool ready;		     /* Boolean for indicating complete state. */
    SPxRefWrapper<T> value;  /* Wrapper around reference to T. */

    void wait() const
    {
	SPxAutoLock lock(mutex);
	if( ready )
	{
	    return;
	}
	cond.WaitForever(mutex);
    }
    SPxErrorCode wait_for(UINT32 timeoutMS) const
    {
	SPxAutoLock lock(mutex);
	if( ready )
	{
	    return SPX_NO_ERROR;
	}
	return cond.WaitTimedMsecs(mutex, timeoutMS);
    }
};

/* Specialisation for when the result is void. */
template<>
struct SPxFutureState<void>
{
    SPxFutureState() : cond(TRUE), mutex(TRUE, TRUE), ready(false) { }

    mutable SPxCondVar cond; /* Event for signalling completion. */
    mutable SPxCriticalSection mutex;
    bool ready;		     /* Boolean for indicating complete state. */

    void wait() const
    {
	SPxAutoLock lock(mutex);
	if( ready )
	{
	    return;
	}
	cond.WaitForever(mutex);
    }
    SPxErrorCode wait_for(UINT32 timeoutMS) const
    {
	SPxAutoLock lock(mutex);
	if( ready )
	{
	    return SPX_NO_ERROR;
	}
	return cond.WaitTimedMsecs(mutex, timeoutMS);
    }
};

#endif /* _SPX_FUTURE_STATE_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
