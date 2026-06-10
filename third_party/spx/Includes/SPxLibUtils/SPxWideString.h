/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxWideString.h,v $
* ID: $Id: SPxWideString.h,v 1.12 2017/07/18 14:33:12 rew Exp $
*
* Purpose:
*   Header containing classes for converting to/from wide strings
*   on Windows.
*
* Revision Control:
*   18/07/17 v1.12   AGC	Fix conversion when truncation required.
*
* Previous Changes:
*   09/05/17 1.11   AGC	Fix cppcheck warnings.
*   07/11/16 1.10   AGC	Allow UTF-8 or ACP strings internally.
*			Simplify SPxTBase and improve layout.
*   21/05/15 1.9    AGC	Add pound character.
*   20/02/15 1.8    AGC	Add SPxT8toT16 and SPxT16toT8 classes for non-Unicode builds.
*   01/09/14 1.7    AGC	VS2015 format string changes.
*   04/07/14 1.6    AGC	Remove need for m_short flag.
*   15/01/14 1.5    AGC	Add optionally included CString constructors.
*			Move SPxGetCharacter() here from SPxSysUtils.h.
*			Fix broken truncated UTF-8 strings when converting.
*   08/11/13 1.4    AGC	Fix cppcheck warnings.
*   27/09/13 1.3    AGC	Add SPX_CHAR type.
*   01/08/13 1.2    AGC	Test for Unicode using UNICODE or _UNICODE.
*   30/07/13 1.1    AGC	Initial version.
*
**********************************************************************/
#ifndef SPX_WIDE_STRING_H
#define SPX_WIDE_STRING_H

/*
 * Other headers required.
 */

#include <cstring>  /* For std::memset, std::strlen. */
#include "SPxLibUtils/SPxCommon.h" /* For windows.h. */
#include "SPxLibUtils/SPxTypes.h" /* For SPX_FUNCTION_DELETE. */

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

#ifdef _WIN32
typedef TCHAR SPX_CHAR;
#else
typedef char SPX_CHAR;
#endif

/* SPxTAtoT16 and SPxT16toTA should only be available under Windows. */
#ifdef _WIN32

/* Base class for string conversions. */
template<typename I, typename O>
class SPxTBase
{
public:
    /* Public functions. */
    SPxTBase(void) : m_ostr(NULL)
    {
	std::memset(m_shortOstr, 0, sizeof(m_shortOstr));
    }
    SPxTBase(const O *str) : m_ostr(str)
    {
	std::memset(m_shortOstr, 0, sizeof(m_shortOstr));

	/* Mark that m_ostr should not be freed. */
	m_shortOstr[0] = TRUE;
    }
    virtual ~SPxTBase(void)
    {
	if(m_ostr && !m_shortOstr[0])
	{
	    delete[] m_ostr;
	}
	m_ostr = NULL;
    }
    operator const O*(void) const
    {	
	return m_ostr ? m_ostr : m_shortOstr;
    }

protected:
    /* Protected functions. */
    void DoConversion(const I *str, int codePage)
    {
	int numInChars = -1;

	/* Find required length for output string, try UTF-8. */
	SetLastError(0);
	std::size_t len = Convert(codePage, str, NULL, 0, numInChars, (codePage == CP_UTF8));
	if(codePage == CP_UTF8)
	{
	    if(GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
	    {
		numInChars = TruncateConvert(codePage, str, TRUE, len);
	    }
	    if(GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
	    {
		/* Conversion failed: do the conversion again,
		 * but treating the input as local code page.
		 */
		codePage = CP_ACP;
		len = Convert(codePage, str, NULL, 0, numInChars, FALSE);
	    }
	}

	/* Use short output string if possible. */
	O *ostr = m_shortOstr;
	if( len > (31 - (sizeof(O*) / sizeof(O))) )
	{
	    /* Create dynamic long string if required. */
	    ostr = new O[len + 1];
	    m_ostr = ostr;
	}
	
	/* Perform actual conversion. */
	Convert(codePage, str, ostr, len, numInChars, FALSE);

	/* Null terminate. */
	ostr[len] = O();
    }

private:
    /* Private variables. */
    O m_shortOstr[32 - (sizeof(O*) / sizeof(O))];	/* Short output string. */
    const O *m_ostr;				/* Long output string. */

    /* Private functions (disable copying). */
    SPxTBase(const SPxTBase&) SPX_FUNCTION_DELETE;
    SPxTBase& operator=(const SPxTBase&) SPX_FUNCTION_DELETE;

    /* Function to do conversion. */
    virtual int Convert(int codePage, const I *str, O *ostr,
			std::size_t numChars, int numInChars, int flags)=0;
    virtual int TruncateConvert(int /*codePage*/, const I * /*str*/, int /*flags*/, std::size_t& /*len*/)
    {
	return -1;
    }

}; /* SPxTBase */

/* Translate ACP/UTF-8 -> UTF-16. */
class SPxTAtoT16 : public SPxTBase<char, wchar_t>
{
public:
    /* Public functions. */
    SPxTAtoT16(const char *str, int codePage=CP_UTF8) { DoConversion(str, codePage); }
#ifdef __CSTRINGT_H__
    SPxTAtoT16(const CStringA &str, int codePage=CP_UTF8) { DoConversion(static_cast<LPCSTR>(str), codePage); }
#endif
    SPxTAtoT16(const wchar_t *str) : SPxTBase(str) {}
#ifdef __CSTRINGT_H__
    SPxTAtoT16(const CStringW &str) : SPxTBase(static_cast<LPCWSTR>(str)) {}
#endif
    virtual ~SPxTAtoT16(){}

private:
    virtual int Convert(int codePage, const char *str, wchar_t *ostr,
			std::size_t numChars, int numInChars, int errFlag) SPX_VIRTUAL_OVERRIDE
    {
	return ::MultiByteToWideChar(codePage, errFlag ? MB_ERR_INVALID_CHARS : 0,
	    str, numInChars, ostr, static_cast<int>(numChars));
    }

    virtual int TruncateConvert(int codePage, const char *str, int errFlag, std::size_t& len) SPX_VIRTUAL_OVERRIDE
    {
	/* Remove trailing bytes from input string to remove part
	 * of a multi-byte character that has been left due to
	 * truncation.
	 */
	int numInChars = static_cast<int>(std::strlen(str));
	if(str[numInChars - 1] < 0)
	{
	    /* Find the first character (going backwards from end of string),
	    * that is a valid first byte in a multi-byte character.
	    * (0 -> 127) are single byte characters.
	    * (-128 -> -65) are trailing bytes in multi-byte characters.
	    * (-64 -> -33) are first byte of 2-byte sequences.
	    * (-32 -> -17) are first byte of 3-byte sequences.
	    * (-16 -> -1) are first byte of 4-byte sequences.
	    */
	    while((numInChars > 0) && ((str[numInChars - 1] < -64) || (0 <= str[numInChars - 1])))
	    {
		--numInChars;
	    }

	    /* Move before that part multi-byte character. */
	    --numInChars;
	    
	    SetLastError(0);
	    len = Convert(codePage, str, NULL, 0, numInChars, errFlag);

	    return numInChars;
	}

	return -1;
    }

}; /* SPxTAtoT16 */

/* Translate UTF-16 -> ACP/UTF-8. */
class SPxT16toTA : public SPxTBase<wchar_t, char>
{
public:
    /* Public functions. */
    SPxT16toTA(const wchar_t *str, int codePage=CP_UTF8) { DoConversion(str, codePage); }
#ifdef __CSTRINGT_H__
    SPxT16toTA(const CStringW &str, int codePage=CP_UTF8) { DoConversion(static_cast<LPCWSTR>(str), codePage); }
#endif
    SPxT16toTA(const char *str) : SPxTBase(str) {}
#ifdef __CSTRINGT_H__
    SPxT16toTA(const CStringA &str) : SPxTBase(static_cast<LPCSTR>(str)) {}
#endif
    virtual ~SPxT16toTA(){}

private:
    virtual int Convert(int codePage, const wchar_t *str, char *ostr,
	std::size_t numChars, int numInChars, int errFlag) SPX_VIRTUAL_OVERRIDE
    {
	return ::WideCharToMultiByte(codePage, errFlag ? 0x80 : 0,
				     str, numInChars, ostr, static_cast<int>(numChars), NULL, NULL);
    }

}; /* SPxTAtoT16 */

/* Convenient name for classes above. */
#define SPX_TW(val) ((const wchar_t*)SPxTAtoT16(val))
#define SPX_TA(val) ((const char*)SPxT16toTA(val))

#endif

/* Text UTF-8 <--> UTF-16 transformations. */
#if defined(__BORLANDC__) || defined(_UNICODE) || defined(UNICODE)

/* Translate UTF-8 -> UTF-16. */
class SPxT8toT16 : public SPxTAtoT16
{
public:
    /* Public functions. */
    SPxT8toT16(const char *str) : SPxTAtoT16(str) {}
    SPxT8toT16(const wchar_t *wstr) : SPxTAtoT16(wstr) {}
    virtual ~SPxT8toT16(){}

}; /* SPxT8toT16 */

/* Translate UTF-16 -> UTF-8 */
class SPxT16toT8 : public SPxT16toTA
{
public:
    SPxT16toT8(const wchar_t *wstr) : SPxT16toTA(wstr) {}
    SPxT16toT8(const char *str) : SPxT16toTA(str) {}
    virtual ~SPxT16toT8(){}

}; /* SPxT16toT8 */

/* Convenient names for classes above. */
#define SPX_T16(val) ((const TCHAR*)SPxT8toT16(val))
#define SPX_T8(val) ((const char*)SPxT16toT8(val))
#else

#ifdef _WIN32
/* Define SPxT8toT16/SPxT16toT8 classes that can be 
 * used in non-unicode builds so that generically transformed
 * strings can be stored without having to use CString.
 */
typedef SPxT16toTA SPxT8toT16;
typedef SPxTAtoT16 SPxT16toT8;

/* Convert to UTF-16 and back again, so that UTF-8 and local code page strings
 * area all converted to local code page as best as is possible.
 */
#define SPX_T16(val) ((const TCHAR*)SPxT16toTA(SPxTAtoT16(val), CP_ACP))
#define SPX_T8(val) val /* Do nothing. */

#else

#define SPX_T16(val) val /* Do nothing. */
#define SPX_T8(val) val	/* Do nothing. */

#endif

#endif

/* Characters outside normal ASCII range.
 * For use with SPxGetCharacter() function.
 */
enum SPxCharacter_t
{
    SPX_CHAR_DEGS,
    SPX_CHAR_SUPER2,
    SPX_CHAR_MICRO,
    SPX_CHAR_SIGMA_LOWER,
    SPX_CHAR_COPYRIGHT,
    SPX_CHAR_POUND
};

/*********************************************************************
*
*   Functions
*
**********************************************************************/

/* Get a character in the current internal string format (ACP or UTF-8). */
extern const char *SPxGetCharacter(SPxCharacter_t c);

#endif /* SPX_WIDE_STRING_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
