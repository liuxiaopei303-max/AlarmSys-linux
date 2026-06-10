/*********************************************************************
*
* (c) Copyright 2012 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSimMotion.h,v $
* ID: $Id: SPxSimMotion.h,v 1.28 2017/04/12 09:05:33 rew Exp $
*
* Purpose:
*	Header for SPxSimMotion object which is a simulated motion
*	for simulated target and radar motion within the 
*	SPxRadarSimulator radar source.
*
*
* Revision Control:
*   12/04/17 v1.28   AGC	Support undo of max turn change.
*
* Previous Changes:
*   09/03/17 1.27   AGC	Improve metres-based platform motions.
*   31/01/17 1.26   AGC	Support multiple platforms.
*   23/08/16 1.25   AGC	Improve ordering of motions.
*			Mutex improvements.
*   22/01/16 1.24   AGC	Add GetNextSegment()/GetPrevSegment() overloads.
*   07/11/14 1.23   AGC	Support event handlers.
*   18/07/14 1.22   AGC	Use correct forward declared type for SPxLatLong_t.
*   02/06/14 1.21   AGC	Support child motions.
*   19/05/14 1.20   AGC	Fix target visibility when hidden due to motion.
*   16/05/14 1.19   AGC	Support remote control.
*   10/04/14 1.18   AGC	Add GetSegment() function for testing.
*   24/02/14 1.17   AGC	Rename and reorder functions for consistency.
*   04/10/13 1.16   AGC	Simplify headers.
*   05/09/13 1.15   AGC	Allow passing platform lat/long to RecalcSegmentTimes().
*			Add wrapped flag to Update() to detect end of motion.
*   30/07/13 1.14   AGC	Improved speed change handling.
*   25/07/13 1.13   AGC	Prevent motion modifications on loading.
*   14/06/13 1.12   AGC	Preserve motion shape on speed change.
*   06/06/13 1.11   AGC	Support multiple simulation files.
*   26/04/13 1.10   AGC	Support choosing position for new segments.
*   28/03/13 1.9    AGC	Move load motion here from SPxRadarSimulator.
*   08/03/13 1.8    AGC	Support max turn rate.
*   04/03/13 1.7    AGC	Add GetNextSegment() function.
*			Support undo motion create/delete.
*   12/02/13 1.6    AGC	Prevent duplicated motion names.
*   25/01/13 1.5    AGC	Add index to segment iteration.
*			Support reversing end mode.
*   21/12/12 1.4    AGC	Signal motion changes.
*   23/11/12 1.3    AGC	Add GetPrevSegment() function.
*   09/11/12 1.2    AGC	Start state now a property of a motion.
*			End action replaces RestartAtEnd.
*   26/10/12 1.1    AGC	Initial version.
**********************************************************************/
#ifndef _SPX_SIM_MOTION_H
#define _SPX_SIM_MOTION_H

/*
 * Other headers required.
 */

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPxErrorCode. */
#include "SPxLibUtils/SPxError.h"

/* For SPxObjPtr. */
#include "SPxLibUtils/SPxObjPtr.h"

/* For SPxSimSegAccel. */
#include "SPxLibData/SPxSimSegment.h"

/* For SPxSimTargetVis. */
#include "SPxLibData/SPxSimTarget.h"

/*
 * Types
 */

/* Forward declarations. */
class SPxRadarSimulator;
class SPxSimSegment;
class SPxSimRadar;
class SPxSimTarget;
struct SPxSimState;
struct SPxLatLong_tag;
struct SPxRadarSimEventParams;

enum SPxSimMotionEndAction
{
    SPX_SIM_MOTION_END_ACTION_STOP = 0,
    SPX_SIM_MOTION_END_ACTION_HIDE = 1,
    SPX_SIM_MOTION_END_ACTION_RESTART = 2,
    SPX_SIM_MOTION_END_ACTION_REVERSE = 3
};

/* Callback types. */
typedef SPxErrorCode (*SPxSimMotionSegmentFn_t)(unsigned int index, SPxSimSegment *segment, void *userArg);

/* Define our class. */
class SPxSimMotion : public SPxObj
{
public:
    /* Public types. */
    typedef SPxErrorCode(*EventHandler)(UINT32 flags);

    /* Public functions. */
    explicit SPxSimMotion(SPxRadarSimulator *radarSim);
    virtual ~SPxSimMotion(void);

    /* Motion name. */
    SPxErrorCode SetName(const char *name);
    SPxErrorCode GetName(char *buffer, unsigned int bufLen,
	int mutexAlreadyLocked=FALSE) const;

    /* Start/end state of motion. */
    SPxErrorCode SetStartState(const SPxSimState *startState);
    SPxErrorCode GetStartState(SPxSimState *startState, int setLatLong=FALSE,
	int mutexAlreadyLocked=FALSE,
	const SPxLatLong_tag *initPlatLatLong=NULL);
    SPxErrorCode GetEndState(SPxSimState *endState, 
	SPxSimSegment *beforeSeg=NULL);
    double GetTotalTimeSecs(void) const;

    /* Child motions. */
    SPxErrorCode AddMotion(SPxSimMotion *motion, SPxSimSegment *nextSegment=NULL);
    SPxErrorCode RemoveMotion(SPxSimMotion *motion,
	int mutexAlreadyLocked=FALSE, const SPxLatLong_tag *initPlatLatLong=NULL,
	int delEndPosSeg=TRUE);
    int IsMotionAddable(SPxSimMotion *motion, int mutexAlreadyLocked=FALSE) const;
    int IsParentMotion(const SPxSimMotion * const motion, int mutexAlreadyLocked=FALSE,
	const SPxSimMotion * const skipMotion=NULL) const;
    SPxSimMotion *GetChildMotion(SPxSimSegment *segment, int mutexAlreadyLocked=FALSE) const;

    /* Segments. */
    unsigned int GetNumSegments(void) const;
    SPxSimSegment *GetSegment(unsigned int index) const;
    SPxSimSegment *GetNextSegment(SPxSimSegment *seg, int mutexAlreadyLocked=FALSE) const;
    SPxSimSegment *GetPrevSegment(SPxSimSegment *seg, int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode GetNextSegment(SPxSimSegment *seg, SPxObjPtr<SPxSimSegment>& nextSeg) const;
    SPxErrorCode GetPrevSegment(SPxSimSegment *seg, SPxObjPtr<SPxSimSegment>& prevSeg) const;
    SPxErrorCode IterateSegments(SPxSimMotionSegmentFn_t fn, void *userArg);
    SPxErrorCode GetSegmentStartState(const SPxSimSegment * const seg,
	SPxSimState *startState, int mutexAlreadyLocked=FALSE) const;
    SPxErrorCode GetSegmentEndState(const SPxSimSegment * const seg,
	SPxSimState *endState, int mutexAlreadyLocked=FALSE) const;
    double GetSegmentTimeSecs(const SPxSimSegment * const seg, int mutexAlreadyLocked=FALSE) const;

    /* End action. */
    SPxErrorCode SetEndAction(SPxSimMotionEndAction endAction);
    SPxErrorCode GetEndAction(SPxSimMotionEndAction *endAction) const;

    /* Max turn rate. */
    SPxErrorCode SetMaxTurnType(SPxSimSegAccel maxTurnType);
    SPxSimSegAccel GetMaxTurnType(void) const;
    SPxErrorCode SetMaxTurnG(double maxTurnG);
    double GetMaxTurnG(void) const;
    SPxErrorCode SetMaxTurnDegs(double maxTurnDegs);
    double GetMaxTurnDegs(void) const;

    /* Get parent radar simulator. */
    SPxErrorCode GetRadarSim(SPxRadarSimulator **radarSim);

    /* Add/remove event callbacks. */
    template<typename F, typename O>
    SPxErrorCode AddEventCallback(F fn, O *obj)
    {
	return m_fns.Add(fn, obj);
    }
    template<typename F, typename O>
    SPxErrorCode RemoveEventCallback(F fn, O *obj)
    {
	return m_fns.Remove(fn, obj);
    }

    /* Generic parameter assignment. */
    virtual int SetParameter(char *name, char *value);
    virtual int GetParameter(char *name, char *valueBuf, int bufLen);

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;
    SPxEventHandler<EventHandler> m_fns;

    /* Called by SPxRadarSimulator. */
    SPxErrorCode RemoveRadarSim(void);
    void ApplyDesc(const char *buffer);
    const char *GetDesc(void) const;
    void SetFile(const char *filename);
    int WasFromFile(const char *filename);
    SPxErrorCode ApplyDelayed(const SPxLatLong_tag *initPlatLatLong);

    /* Called by SPxSimPlatform. */
    SPxErrorCode GetStartStatePlat(SPxSimState *startState, int setLatLong,
				   const SPxLatLong_tag *initLatLong);

    /* Called by SPxSimSegment. */
    SPxErrorCode AddSegment(SPxSimSegment *segment, SPxSimSegment *nextSeg,
	SPxSimSegment *prevSeg=NULL, SPxSimMotion *descMotion=NULL, int mutexAlreadyLocked=FALSE,
	const SPxLatLong_tag *initPlatLatLong=NULL, int swapStartStates=TRUE);
    SPxErrorCode RemoveSegment(const SPxSimSegment *segment);
    SPxErrorCode MoveSegment(SPxSimSegment *segment,
	unsigned int *segList, unsigned int numSegs);
    SPxErrorCode MoveSegment(SPxSimSegment *segment, int move);
    unsigned int *GetSegList(unsigned int *numSegments);
    SPxErrorCode RecalcSegmentTimes(int mutexLocked=FALSE,
	const SPxLatLong_tag *initPlatLatLong=NULL);
    SPxErrorCode SignalEvent(UINT32 flags, const SPxRadarSimEventParams *params=NULL);
    
    /* Called by SPxSimPlatform/SPxSimRadar/SPxSimTarget. */
    virtual SPxErrorCode Update(double currSecs, double intervalSecs, 
	SPxSimState *state, const SPxLatLong_tag *initPlatLatLong,
	SPxSimTargetVis *visibleRtn=NULL, int *wrappedRtn=NULL);

    /* Private functions. */
    SPxErrorCode setName(const char *name, int genMismatchWarnings=TRUE, int genEvent=TRUE);
    SPxErrorCode addDelayedMotion(const char *name,
	const SPxLatLong_tag *initPlatLatLong=NULL);
    SPxErrorCode addParentMotion(SPxSimMotion *motion);
    SPxErrorCode removeParentMotion(SPxSimMotion *motion);
    SPxErrorCode updateEndPosSeg(SPxSimMotion *motion, const SPxLatLong_tag *endPos);
    SPxErrorCode recalcSegmentTimes(const SPxLatLong_tag *initPlatLatLong);
    SPxSimMotion *findMotion(const char *name);
    SPxErrorCode addUndoMaxTurn(const char *desc);
    static SPxErrorCode undoDelete(void *userObj1, void *userObj2, void *userArg);
    SPxErrorCode getStartStateInternal(SPxSimState *startState, int setLatLong,
				       const SPxLatLong_tag *initPlatLatLong);
    double getMaxTurnG(double speedMps) const;
    double getMaxTurnDegs(double speedMps) const;
    SPxErrorCode setSegmentParameter(const char *param, const char *value);
    SPxErrorCode getSegmentParameter(const char *param, char *valueBuf, int bufLen);
    SPxErrorCode splitSegmentParameter(const char *param,
	unsigned int *index, char *paramBuf, int bufLen);
    static SPxErrorCode insertChildSeg(unsigned int index, SPxSimSegment *segment, void *userArg);
    static SPxErrorCode undoChangeMaxTurn(void *userObj1, void * /*userObj2*/,
					  void *userArg);

    friend class SPxRadarSimulator;
    friend class SPxSimPlatform;
    friend class SPxSimSegment;
    friend class SPxSimRadar;
    friend class SPxSimTarget;

}; /* SPxSimMotion */

#endif /* _SPX_SIM_MOTION_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
