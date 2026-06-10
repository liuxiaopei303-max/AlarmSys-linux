/*********************************************************************
*
* (c) Copyright 2010 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxSysUtils.h,v $
* ID: $Id: SPxSysUtils.h,v 1.40 2017/05/09 13:23:46 rew Exp $
*
* Purpose:
*   Header file for miscellaneous system utilities.
*
*
* Revision Control:
*   09/05/17 v1.40   AGC	Fix cppcheck warnings.
*
* Previous Changes:
*   03/10/16 1.39   AGC	Support GetFullDirname/GetShortDirname() with user-provided buffer.
*   09/09/16 1.38   AGC	Fix Unicode calls to SPxGetTotalCpuLoad().
*   11/08/16 1.37   AJH	Add SPxGetDiskFreeSpaceMB().
*   05/05/16 1.36   AJH	Add filter to SPxListDirectory.
*   22/04/15 1.35   AGC	SPxRename/SPxRemove now return SPxErrorCode.
*   14/04/15 1.34   SP 	Add optional filter arguments to SPxDeleteDir.
*   17/03/15 1.33   AGC	Add SPxCopyFile() and SPxCopyTree().
*			Add SPxDirSizeBytes().
*   02/03/15 1.32   SP 	Add optional time args to SPxIsDirectory().
*                       Add extra time arg to SPxIsFile().
*   14/10/14 1.31   AGC	Add SPxGetFileExtension().
*   29/08/14 1.30   SP 	Add SPxIsDirectory().
*   01/05/14 1.29   SP 	Return file age in SPxIsFile().
*                       Add SPxListDirectory().
*   10/03/14 1.28   SP 	Add SPxGetProcInstanceCount().
*   05/03/14 1.27   AGC	Add SPxFtell/SPxFseek.
*   15/01/14 1.26   AGC	Add SPxGetLastModified().
*			Move SPxGetCharacter() to SPxWideString.h.
*   07/10/13 1.25   AGC	Use NULL for default argument to SPxQueryStartProc().
*   04/10/13 1.24   AGC	Simplify headers.
*   04/10/13 1.23   SP 	Replace SPxStat() with SPxIsFile().
*   30/07/13 1.22   AGC	Move SPX_T8/SPX_T16 to SPxWideString header.
*   25/04/13 1.21   AGC	Add SPxRename/SPxStat/SPxRemove.
*   08/04/13 1.20   AGC	Make SPxDirname/SPxBasename re-entrant.
*   08/03/13 1.19   AGC	Add SPX_CHAR_MICRO.
*   21/02/13 1.18   SP 	Add SPxPath::Basename and SPxBasename functions.
*   13/02/13 1.17   AGC	Create classes for UTF8<->UTF16 conversion.
*   14/01/13 1.16   AGC	Fix #227: Only include atl header when using Unicode.
*   23/11/12 1.15   AGC	Support Unicode under Windows.
*   26/10/12 1.14   AGC	Add SPxMkdir() function.
*			Add SPxDeleteDir() function.
*   01/10/12 1.13   AGC	Reduce header dependencies.
*   09/12/11 1.12   AGC	Move SPxEstimateLinuxCPULoad() here from SPxFrameXSupport.
*   17/11/11 1.11   AGC	Add private copy-constructor and assignment operator.
*   14/11/11 1.10   AGC	Include required SPxCommon.h header.
*   02/11/11 1.9    AGC	Prevent accidental assignment of string to SPxPath.
*   14/07/11 1.8    SP 	Increase SPxQueryStartRDC() default
*                       timeout from 500ms to 1000ms.
*   27/06/11 1.7    SP 	Add SPxQueryStartProc() and
*                       SPxQueryStartRDC() (TODO 81).
*   24/05/11 1.6    AGC	Add SPxGetTotalCpuLoad().
*   12/05/11 1.5    AGC	Make GetFullPath(), GetFullDirname() etc const.
*   22/12/10 1.4    AGC	Add Reset() function.
*			Add fileAbsToRel() and prependCurrDirIfRel()
*			functions to SPxPath class.
*   17/12/10 1.3    AGC Add Assign() function and tidy currConfigPath.
*   16/12/10 1.2    AGC	Add SPxPath object.
*   08/11/10 1.1    AGC	Initial version.
**********************************************************************/

#ifndef _SPX_SYSUTILS_H
#define _SPX_SYSUTILS_H

/*
 * Other headers required.
 */

/* For FILE. */
#include <stdio.h>

/* For SPxAutoPtr. */
#include "SPxLibUtils/SPxAutoPtr.h"

/* For SPxErrorCode. */
#include "SPxLibUtils/SPxError.h"

/* For TRUE/FALSE etc. */
#include "SPxLibUtils/SPxTypes.h"

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

/* Forward declarations. */
struct SPxTime_tag;

/* Used by SPxListDirectory. */
typedef struct
{
    char fullname[FILENAME_MAX]; /* Path and basename. */
    char basename[FILENAME_MAX]; /* Basename. */
    
} SPxFileInfo_t;

/*
 * SPx Path class
 */
class SPxPath
{
public:
    /*
     * Public functions.
     */

    /* Construction and Destruction. */
    SPxPath(void);
    explicit SPxPath(const char *path, const char *relFromEnvVar=NULL);
    ~SPxPath(void);

    /* Set paths in one SPxPath from another. */
    void Assign(const SPxPath &path);

    /* Reset all paths to empty strings. */
    void Reset(void);

    /* Find and open a file. */
    FILE *FindFile(const char *path, 
		   const char *mode, 
		   const char *envVarFolder=NULL,
		   int closeFile=FALSE);

    /* Query functions. */
    const char *GetFullPath(void) const;
    const char *GetFullDirname(char *buf=NULL, int bufLen=0) const;
    const char *GetShortPath(void) const;
    const char *GetShortDirname(char *buf=NULL, int bufLen=0) const;
    const char *GetFilename(void) const;
    const char *GetRelFromEnvVar(void) const;

    /*
     * Static functions.
     */
    static char *GetSPxEnv(void);
    static const char *Dirname(const char *path, char *buf, int bufLen);
    static const char *Basename(const char *path, char *buf, int bufLen);

    static FILE *FindFileStatic(const char *path,
				const char *mode, 
				const char *envVarFolder,
				char *foundPath=NULL,
				int foundPathLen=0);
private:
    /*
     * Private fields.
     */
    struct impl;
    SPxAutoPtr<impl> m_p;    /* Private structure. */

    /*
     * Private functions.
     */
    void CommonInit(const char *path, const char *relFromEnvVar);
    SPxPath(const SPxPath&) SPX_FUNCTION_DELETE;
    SPxPath& operator=(const SPxPath&) SPX_FUNCTION_DELETE;

};

/*********************************************************************
*
*   Macros
*
**********************************************************************/

/*********************************************************************
*
*   Variable definitions
*
**********************************************************************/

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

extern char *SPxGetSPxEnv(void);

extern const char *SPxDirname(const char *path, char *buf, int bufLen);
extern const char *SPxBasename(const char *path, char *buf, int bufLen);
SPxErrorCode SPxGetFileExtension(const char *path, char *buf, int bufLen);

extern FILE *SPxFopen(const char *path, const char *mode);
extern SPxErrorCode SPxMkdir(const char *path);
extern SPxErrorCode SPxDeleteDir(const char *dirName,
                                 const char *dirPrefixFilter=NULL,
                                 const char *dirSuffixFilter=NULL,
                                 unsigned int dirAgeSecs=0,
                                 const char *filePrefixFilter=NULL,
                                 const char *fileSuffixFilter=NULL,
                                 unsigned int fileAgeSecs=0,
                                 int deleteEmptyDirs=TRUE,
                                 int *allDeletedRtn=NULL);
extern SPxErrorCode SPxRename(const char *oldPath, const char *newPath);
extern int SPxIsFile(const char *path, 
                     SPxTime_tag *modTime=NULL,
                     SPxTime_tag *createTime=NULL);
extern int SPxIsDirectory(const char *path, 
                          SPxTime_tag *modeTime=NULL,
                          SPxTime_tag *createTime=NULL);
extern int SPxGetLastModified(const char *path, SPxTime_tag *time);
extern SPxErrorCode SPxRemove(const char *path);
extern INT64 SPxFtell(FILE *file);
extern int SPxFseek(FILE *file, INT64 offset, int origin);
extern SPxErrorCode SPxCopyFile(const char *src, const char *dst, int overwrite);
extern SPxErrorCode SPxCopyTree(const char *srcDir, const char *dstDir, int overwrite);
extern SPxErrorCode SPxDirSizeBytes(const char *dir, UINT64 *numBytes);

extern FILE *SPxFindFile(const char *path, const char *mode, 
			 const char *envVarFolder=NULL);

extern SPxErrorCode SPxListDirectory(const char *searchDir,
                                     SPxFileInfo_t **filesRtn,
                                     unsigned int *numFiles,
                                     SPxFileInfo_t **dirsRtn,
                                     unsigned int *numDirs,
				     const char *filter=NULL);

extern SPxErrorCode SPxGetDiskFreeSpaceMB(const char *path,
					  unsigned int *space);

extern SPxErrorCode SPxGetTotalCpuLoad(double *percentage);

#ifndef _WIN32
/* Legacy CPU load function that was used by FrameX. */
extern int SPxEstimateLinuxCPUload(void);
#endif

extern SPxErrorCode SPxQueryStartProc(const char *procName, 
                                      const char *envVarDir=NULL,
                                      const char **procArgv=NULL,
                                      unsigned int waitMSecs=0,
                                      int *procStartedRtn=NULL);

extern SPxErrorCode SPxQueryStartRDC(const char *filename="spxrdc",
                                     const char **argv=NULL, 
                                     unsigned int waitMSecs=1000);

extern unsigned int SPxGetProcInstanceCount(const char *procName);

#endif /* _SPX_SYSUTILS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
