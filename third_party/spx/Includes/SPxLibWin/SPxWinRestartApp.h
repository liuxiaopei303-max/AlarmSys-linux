/*********************************************************************
*
* (c) Copyright 2016 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxWinRestartApp.h,v $
* ID: $Id: SPxWinRestartApp.h,v 1.3 2017/01/11 12:22:04 rew Exp $
*
* Purpose:
*	Header for SPx class to restart a Windows application with
*	the previously used or new configuration file.
*
* Revision Control:
*   11/01/17 v1.3    SP 	Add closeAllResources(). 
*
* Previous Changes:
*   23/08/16 1.2    AGC	Add functions for testing.
*   14/07/16 1.1    AGC	Initial Version.
**********************************************************************/
#pragma once

#include "SPxLibUtils/SPxAutoPtr.h"
#include "SPxLibUtils/SPxWideString.h"

/* Class used to start tasks. */
class SPxWinRestartApp
{
public:
    /* Public functions. */
    explicit SPxWinRestartApp(void);
    virtual ~SPxWinRestartApp(void);

    void SetArgs(int argc, SPX_CHAR *argv[]);
    void SetCmdLine(const SPX_CHAR *cmdLine);
    void SetDefaultConfigFilename(const char *defaultConfigFilename);

    int HaveCmdLineConfigFilename(void) const;
    const char *GetConfigFilename(void) const;
    const char *GetExePath(void) const;
    void Restart(const char *extraCmdLineParams=NULL,
		 const char *configDomain=NULL);

    void BuildCmdLine(char *buffer, unsigned int bufLen,
		      const char *extraCmdLineParams=NULL,
		      const char *configDomain=NULL) const;

protected:
    /* Functions for testing. */
    virtual const char *GetConfigShortPath(const char *configDomain) const;

private:
    /* Private variables. */
    struct impl;
    SPxAutoPtr<impl> m_p;

    /* Private functions. */
    void closeAllResources(void);

}; /* SPxWinRestartApp */

/*********************************************************************
*
* End of file
*
**********************************************************************/
