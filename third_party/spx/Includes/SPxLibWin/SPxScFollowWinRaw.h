/*********************************************************************
*
* (c) Copyright 2007 - 2016, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxScFollowWinRaw.h,v $
* ID: $Id: SPxScFollowWinRaw.h,v 1.4 2016/03/03 10:59:08 rew Exp $
*
* Purpose:
*   Header for class to track window geometries.
*
* Revision Control:
*   03/03/16 v1.4    AGC	Fix clang warnings.
*
* Previous Changes:
*   04/10/13 1.3    AGC	Simplify headers.
*   13/07/12 1.2    AGC	Support sub-areas.
*   13/01/12 1.1    AGC	Create from SPxScFollowWin 1.12 removing MFC.
**********************************************************************/

#ifndef _SPX_SC_FOLLOW_WIN32_H
#define _SPX_SC_FOLLOW_WIN32_H

#include "SPxLibUtils/SPxScFollow.h"

class SPxThread;
class SPxScDestDisplayWinRaw;

/* Class used to track a MS application window that will contain scan
 * converted radar video 
 */

// Define the default poll period in ms
#define SPX_DEFAULT_WIN_FOLLOW_POLL_PERIOD_MS 250

/* The maximum number of slave windows. */
#define SPX_MAX_NUM_SLAVE_WINS 5

class SPxScFollowWinRaw :public SPxScFollow
{
    /* Private variables. */
    SPxThread *m_thread;		// The thread that the code runs in
    HWND m_win;			// The window being tracked (the control);
    HWND m_winSlave[SPX_MAX_NUM_SLAVE_WINS];
					// The (optional) windows that gets moved with the control
    SPxScDestDisplayWinRaw *m_dd[SPX_FOLLOW_WIN_MAX];
					// List of SPxScDestDisplayWins
    static int m_pollPeriod;		// The poll period in ms.
    BOOL m_visibility;			// Is the radar window currently visible.
    int m_justAddedSlaveWindow;
    int m_numDDbeingManaged;
    int m_subAreaX, m_subAreaY, m_subAreaW, m_subAreaH; // Sub-area.
    int m_subAreaEnabled;
    int m_subAreaSizeChanged;
    int m_subAreaPosChanged;

    /* Private functions. */
    void CommonConstruct(SPxSc *sc, HWND win, SPxScDestDisplayWinRaw *destDisplay);
    static void *pollGeometryWrapper(SPxThread *thread);
    void *pollGeometry(SPxThread *thread);
    
public:
    /* Public functions. */
    SPxScFollowWinRaw(SPxSc *sc, HWND win);
    SPxScFollowWinRaw(SPxSc *sc, HWND win, SPxScDestDisplayWinRaw *destDisplay);
    virtual ~SPxScFollowWinRaw(void);

    void ClearJustAddedSlaveWindow() {m_justAddedSlaveWindow = 0;}
    BOOL GetJustAddedSlaveWindow(void) {return m_justAddedSlaveWindow;}
    SPxThread *GetThread(void) {return m_thread;}
        void SetVisibility(BOOL newVis) {m_visibility = newVis;}
    BOOL GetVisibility(void) {return m_visibility;}
    HWND GetHwnd(void) {return m_win;}
    void SetWin(HWND win) {m_win = win;}
    void SetSlaveWindow(int index, HWND win);
    
    void SetSubArea(int x, int y, int w, int h);
    void EnableSubArea(int enable);

    HWND GetSlaveHwnd(int i) {if (i < 0 || i >= SPX_MAX_NUM_SLAVE_WINS) return NULL; else return m_winSlave[i];}
    static int GetPollPeriod(void) {return m_pollPeriod;}
    static void SetPollPeriod(int pollPeriodMS) {m_pollPeriod = pollPeriodMS;}
    virtual SPxErrorCode AddSC(SPxSc *sc) { return AddSC(sc, 0); }
    SPxErrorCode AddSC(SPxSc *sc, SPxScDestDisplayWinRaw *destDisplayWin);
    int GetNumDDmanaged(void) {return m_numDDbeingManaged;}
    SPxScDestDisplayWinRaw *GetDDRaw(int i) {if (i < 0 || i >= SPX_FOLLOW_WIN_MAX) return NULL; else return m_dd[i];}

};

#endif /* _SPX_SC_FOLLOW_WIN32_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
