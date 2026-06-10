/*********************************************************************
*
* (c) Copyright 2014, 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxThreadPool.h,v $
* ID: $Id: SPxThreadPool.h,v 1.3 2017/02/21 15:20:31 rew Exp $
*
* Purpose:
*   Header for SPxThreadPool class that allows tasks to be submitted
*   and run on one of a number of threads.
*
* Revision Control:
*   21/02/17 v1.3    AGC	Add ClearQueue() function.
*
* Previous Changes:
*   23/01/17 1.2    AGC	Add GetNumTasks() function.
*   07/11/14 1.1    AGC	Initial version.
**********************************************************************/
#ifndef _SPX_THREAD_POOL_H
#define _SPX_THREAD_POOL_H

/* Other headers required. */
#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxSharedPtr.h"
#include "SPxLibUtils/SPxThreads.h"
#include "SPxLibUtils/SPxObj.h"

/* Forward declarations. */
class SPxThread;

/* Define our class. */
class SPxThreadPool : public SPxObj
{
public:
    /* Type of task. */
    typedef void (*Task)(void *userArg);

    /* Construction and destruction. */
    explicit SPxThreadPool(unsigned int maxThreads=4, UINT32 minThreadLifeMS=30000);
    virtual ~SPxThreadPool();

    /* Submit a new task to be run. */
    void Submit(Task task, void *userArg);
    
    void ClearQueue(void);

    unsigned int GetNumTasks(void) const;
    
private:
    struct ThreadData;
    struct impl;
    SPxAutoPtr<impl> m_p;

    static void *threadFnWrapper(SPxThread *thread);
    void *threadFn(SPxThread *thread, SPxSharedPtr<ThreadData> *data);
    bool getTask(ThreadData *data, Task &task, void *&userArg);
};

#endif /* _SPX_THREAD_POOL_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
