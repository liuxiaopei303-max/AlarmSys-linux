/*********************************************************************
 *
 * (c) Copyright 2013, Cambridge Pixel Ltd.
 *
 * File: $RCSfile: SPxAlertRendererWin.h,v $
 * ID: $Id: SPxAlertRendererWin.h,v 1.1 2013/11/05 09:26:27 rew Exp $
 *
 * Purpose:
 *  Header for alert buttons class under Windows.
 *
 * Revision Control:
 *  04/11/13 v1.1    SP 	Initial version.
 *
 * Previous Changes:
 *
 **********************************************************************/
#ifndef _SPX_ALERT_RENDERER_WIN_H
#define _SPX_ALERT_RENDERER_WIN_H

/*
 * Other headers required.
 */

/* For base class. */
#include "SPxLibUtils/SPxAlertRenderer.h"
#include "SPxLibWin/SPxRendererWin.h"
#include "SPxResource.h"

/*
 * Types.
 */

/* Define our class. */
class SPxAlertRendererWin : public SPxAlertRenderer, public SPxRendererWin
{
public:
    /* Public functions */
    SPxAlertRendererWin(void);
    virtual ~SPxAlertRendererWin(void);

    /* Add alerts. */
    SPxErrorCode AddAlert(const char *text, 
                          const char *prefix=NULL,                       
                          UINT32 prefixColour=0, 
                          int bitmap=IDB_SPX_WARNING);

protected:

private:

}; /* SPxAlertRendererWin */

#endif /* _SPX_ALERT_RENDERER_WIN_H */

/*********************************************************************
 *
 * End of file
 *
 **********************************************************************/
