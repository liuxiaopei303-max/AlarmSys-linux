/*********************************************************************
*
* (c) Copyright 2011 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTimer.h,v $
* ID: $Id: SPxTimer.h,v 1.18 2017/05/09 13:23:46 rew Exp $
*
* Purpose:
*   Header file for object that allows a function to be run 
*   repeatably at a fixed intervals.
*
* Revision Control:
*   09/05/17 v1.18   AGC	Fix cppcheck warnings.
*
* Previous Changes:
*   24/04/17 1.17   AGC	Add Shutdown() function for fixing #449.
*   12/12/16 1.16   AGC	Support calling Schedule() from within handler.
*   21/09/16 1.15   AGC	Support use with external mutex.
*   10/07/15 1.14   AGC	Prevent crash if handler removes itself.
*   29/04/15 1.13   AGC	Allow external drivers.
*   27/02/15 1.12   AGC	Allow virtual override of static Schedule() function.
*   24/09/14 1.11   AGC	Correct rescheduling of member functions.
*   15/09/14 1.10   AGC	Support member functions.
*   04/07/14 1.9    AGC	Rename private structure.
*   04/10/13 1.8    AGC	Simplify headers.
*   01/10/12 1.7    AGC	Move implementation to cpp file.
*   08/05/12 1.6    AGC	Make SPxTimer destructor virtual.
*   30/04/12 1.5    AGC	Make interface functions virtual.
*   17/11/11 1.4    AGC	Add private copy constructor and assigment operator.
*   03/08/11 1.3    AGC	Use pointer to SPxTimerFunc in container.
*   27/07/11 1.2    AGC	Support Linux.
*			Support non-integer intervals.
*   13/07/11 1.1    AGC	Initial Version.
**********************************************************************/

#ifndef _SPX_TIMER_H
#define _SPX_TIMER_H

/*
 * Other headers required.
 */

#include "SPxLibUtils/SPxTypes.h"
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxError.h"
#include "SPxLibUtils/SPxEventHandler.h"
#include "SPxLibUtils/SPxSharedPtr.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Forward declarations. */
class SPxThread;

/* The types of delay available. */
enum SPxTimerDelay
{
    /* The time between the end of one execution of the function 
     * and the start of the next execution of the function is constant. 
     */
    SPX_TIMER_FIXED_DELAY = 0,
    /* Time time between the start of one execution of the funciton
     * and the start of the next execution of the function is constant.
     */
    SPX_TIMER_FIXED_RATE  = 1	
};

class SPxTimer
{
public:
    typedef void(*StaticFunc)(void*);
    typedef void(*MemFunc)(void);
    typedef SPxEventHandler<MemFunc> Handler;
    typedef SPxSharedPtr<Handler> SharedHandler;

    /* Public functions. */
    explicit SPxTimer(int createThread=TRUE, SPxCriticalSection *externalMutex=NULL);
    virtual ~SPxTimer(void);
    virtual SPxErrorCode Schedule(StaticFunc task, SPxTimerDelay delayType,
			  UINT32 initialDelayMsecs, double intervalMsecs,
			  void *userArg, int inFunc=FALSE)
    {
	return Schedule<StaticFunc, void>(task, delayType, initialDelayMsecs, intervalMsecs,
	    userArg, inFunc);
    }
    template<typename F, typename O>
    SPxErrorCode Schedule(F task, SPxTimerDelay delayType,
			  UINT32 initialDelayMsecs, double intervalMsecs,
			  O *obj, int inFunc=FALSE)
    {
	SharedHandler handler = SPxMakeShared<Handler>();
	handler->Add(task, obj);
	return add(handler, delayType, initialDelayMsecs, intervalMsecs, inFunc);
    }
    SPxErrorCode Unschedule(StaticFunc task, void *userArg, int inFunc=FALSE)
    {
	return Unschedule<StaticFunc, void>(task, userArg, inFunc);
    }
    template<typename F, typename O>
    SPxErrorCode Unschedule(F task, O *obj, int inFunc=FALSE)
    {
	Handler handler;
	handler.Add(task, obj);
	return remove(handler, inFunc);
    }
    virtual SPxErrorCode UnscheduleAll(int inFunc=FALSE);

    virtual SPxErrorCode Poll(unsigned int timeoutMsecs,
			      SPxCriticalSection *externalMutex=NULL);

    SPxErrorCode Shutdown(void);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Private functions. */
    static void *threadFnWrapper(SPxThread *thread);
    void *threadFn(SPxThread *thread);
    SPxErrorCode add(const SharedHandler &handler, SPxTimerDelay delayType,
	UINT32 initialDelayMsecs, double intervalMsecs, int inFunc);
    SPxErrorCode remove(const Handler &handler, int inFunc);

    /* Prevent copying. */
    SPxTimer(const SPxTimer&) SPX_FUNCTION_DELETE;
    SPxTimer& operator=(const SPxTimer&) SPX_FUNCTION_DELETE;

}; /* SPxTimer */

#endif /* _SPX_TIMER_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
