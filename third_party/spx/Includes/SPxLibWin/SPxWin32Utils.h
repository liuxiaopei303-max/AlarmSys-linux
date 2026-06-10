/*********************************************************************
*
* (c) Copyright 2007 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxWin32Utils.h,v $
* ID: $Id: SPxWin32Utils.h,v 1.10 2017/06/22 14:24:30 rew Exp $
*
* Purpose:
*   Header for Windows utility functions.
*
*
* Revision Control:
*   22/06/17 v1.10   AGC	Add monitor option to SPxFullScreenToggle().
*
* Previous Changes:
*   20/05/16 1.9    AGC	Add long overload for SPxGetDpiScaledSize().
*   11/03/16 1.8    SP 	Add some DPI scaling functions.
*   02/03/15 1.7    SP 	Add SPxBGRtoARGB().
*   04/10/13 1.6    SP 	Add SPxRotateImage().
*   30/07/13 1.5    AGC	Add SPxWinLibrary for loading/unloading DLL.
*   06/06/13 1.4    AGC	Improve full-screen efficiency.
*   24/05/13 1.3    AGC	Add SPxFullScreenToggle().
*   23/11/12 1.2    SP 	Add SPxCOLORREFtoARGB() and SPxRGBtoCOLORREF().
*   15/05/12 1.1    REW	Initial Version.
**********************************************************************/

#ifndef _SPX_WIN32UTILS_H_
#define	_SPX_WIN32UTILS_H_

/* For SPxErrorCode. */
#include "SPxLibUtils/SPxError.h"

/* For SPX_T16. */
#include "SPxLibUtils/SPxWideString.h"

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Structure for storing fullscreen state. */
struct SPxFullScreenState
{
    int fullscreen;
    UINT showCmd;
    RECT size;
    RECT normSize;
    DWORD style;
    DWORD exStyle;
};

/* Automatically load/free a DLL. */
struct SPxWinLibrary
{
    SPxWinLibrary(const char *lib)
	: m_hmodule(::LoadLibrary(SPX_T16(lib))) {}
    ~SPxWinLibrary(void)
    {
	if( m_hmodule )
	{
	    ::FreeLibrary(m_hmodule);
	}
    }
    operator HMODULE()
    {
	return m_hmodule;
    }
    HMODULE m_hmodule;
};

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/

extern unsigned int SPxFlushWinMsgQueue(void);

extern void SPxGetMaxScreenSize(unsigned int *wRtn, 
                                unsigned int *hRtn);

extern UINT32 SPxCOLORREFtoARGB(COLORREF cr, UINT8 alpha=255);

extern COLORREF SPxRGBtoCOLORREF(UINT32 rgb);

extern SPxErrorCode SPxFullScreenToggle(HWND window,
                                        SPxFullScreenState *state,
                                        HMONITOR monitor=NULL);

extern SPxErrorCode SPxRotateImage(HDC dstHdc,
                                   HDC srcHdc, 
                                   unsigned int srcWidth, 
                                   unsigned int srcHeight, 
                                   double rotationDegs,
                                   int centreX,
                                   int centreY,
                                   int translateX=0,
                                   int translateY=0);

extern UINT32 SPxBGRtoARGB(COLORREF bgr);

extern double SPxGetDpiScale(HWND win=NULL);

extern int SPxGetDpiScaledSize(int size);

extern long SPxGetDpiScaledSize(long size);

extern unsigned int SPxGetDpiScaledSize(unsigned int size);

extern float SPxGetDpiScaledSize(float size);

extern double SPxGetDpiScaledSize(double size);

#endif /* _SPX_WIN32UTILS_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
