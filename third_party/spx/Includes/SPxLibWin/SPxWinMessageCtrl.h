/*********************************************************************
*
* (c) Copyright 2014 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxWinMessageCtrl.h,v $
* ID: $Id: SPxWinMessageCtrl.h,v 1.5 2017/06/22 14:31:55 rew Exp $
*
* Purpose:
*	Header for a message window that can displays coloured
*	SPx errors and other arbitrary messages.
*
*
* Revision Control:
*   22/06/17 v1.5    SP 	Support log file.
*
* Previous Changes:
*   04/02/16 1.4    AGC	Support manually forwarding SPxErrors.
*   10/03/14 1.3    AGC	Add option for including date.
*			Add option for scrolling to bottom.
*   05/03/14 1.2    AGC	Add Clear() function.
*   17/02/14 1.1    AGC	Initial Version.
**********************************************************************/
#pragma once

/* For std::va_list. */
#include <cstdarg>

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/*
 * SPxWinMessageCtrl control
 */
class SPxWinMessageCtrl : public CRichEditCtrl
{
    DECLARE_DYNAMIC(SPxWinMessageCtrl)

public:
    /* Construction and desctruction. */
    SPxWinMessageCtrl(void);
    virtual ~SPxWinMessageCtrl(void);

    void SetShowErrors(int showErrors);
    void SetShowDate(int showDate);
    void SetScrollToBottom(int scrollToBottom);
    void SetMaxLines(int maxLines);
    SPxErrorCode SetLogFilePrefix(const char *prefix);

    void ReportMsgV(COLORREF colour, const char *msg, std::va_list ap);
    void ReportMsg(COLORREF colour, const char *msg, ...);
    void ReportMsg(const char *msg, ...);
    void ReportError(SPxErrorType errType, SPxErrorCode errorCode,
		     int arg1, int arg2, const char *arg3, const char *arg4);

    void Clear(void);

protected:
    /* Protected functions. */
    virtual void PreSubclassWindow(void);
    void OnTimer(UINT_PTR nIDEvent);
    DECLARE_MESSAGE_MAP()

private:
    /* Private variables. */
    struct impl;
    struct msgDesc;
    SPxAutoPtr<impl> m_p;

    /* Private functions. */
    static void errorFnWrapper(SPxErrorType errType, SPxErrorCode errorCode,
			       int arg1, int arg2, const char *arg3, const char *arg4,
			       void *userArg);
    void errorFn(SPxErrorType errType, SPxErrorCode errorCode,
		 int arg1, int arg2, const char *arg3, const char *arg4);
    void updateMessages(void);

}; /* class SPxWinMessageCtrl */

/*********************************************************************
*
* End of file
*
**********************************************************************/
