/*********************************************************************
*
* (c) Copyright 2013 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxTypes.h,v $
* ID: $Id: SPxTypes.h,v 1.37 2017/06/29 13:30:26 rew Exp $
*
* Purpose:
*   Common types across all SPx libraries.
*
* Revision Control:
*   29/06/17 v1.37   AGC	Add SPxEmpty().
*
* Previous Changes:
*   26/06/17 1.36   SP 	Rename last entry in SPxFormatLatLong_t.
*   26/05/17 1.35   AGC	Use SPX_FUNCTION_DELETE on SPxNullPtr.
*   10/05/17 1.34   AGC	Correct Visual Studio deleted function support detection.
*   02/05/17 1.33   AGC	Add SPX_FUNCTION_DELETE/DEFAULT.
*   08/03/17 1.32   AGC	Define TRUE as 1 so it has integer not bool type.
*   31/01/17 1.31   AGC	Support height based units conversion.
*   13/01/17 1.30   REW	Add UTM and MGRS options for lat/long display.
*   01/11/16 1.29   AGC	Add SPX_VIRTUAL_OVERRIDE/FINAL.
*   31/10/16 1.28   REW	Add SPX_RETURN_VAL_IF_NULL macro.
*   23/09/16 1.27   REW	Add SPxUnitsDist_t, SPxUnitsSpeed_ and
*			SPxFormatLatLong_t.
*   23/08/16 1.26   AGC	Add SPxIsSame.
*   08/07/16 1.25   AGC	Add SPxLength.
*			Add SPxBegin/SPxEnd etc.
*			Add SAL annotations to SPxArrayLen/SPxLength.
*   18/05/16 1.24   AGC	Add SPxAlign/SPxAlignof macros.
*   26/11/15 1.23   AGC	Add SPxNullPtr.
*   09/11/15 1.22   AGC	Add REAL64.
*   12/05/15 1.21   AGC	Fix test for empty() function.
*   11/05/15 1.20   AGC	Support SPxData() on objects without empty() function.
*   05/05/15 1.19   SP 	Add overloads for status functions.
*   06/03/15 1.18   AGC	Improve SPX_STATIC_ASSERT_ALWAYS.
*			Add SPxAddConst.
*   26/01/15 1.17   SP 	Reorder status values.
*   07/01/15 1.16   SP 	Add SPxGetStatusText().
*   11/12/14 1.15   SP 	Add SPxGetStatusARGB().
*   11/12/14 1.14   AGC	Avoid warning from SPxSizeof.
*   21/11/14 1.13   SP 	Reorder status values.
*			Rename SPX_STATUS_DISABLED to SPX_STATUS_NEUTRAL.
*			Rename SPX_STATUS_UNKNOWN to SPX_STATUS_DISABLED.
*   07/11/14 1.12   AGC	Add SPxAddRef and others.
*   22/10/14 1.11   AGC	Make SPxSizeof/SPxArrayLen inline.
*			Add SPxData() functions.
*   09/09/14 1.10   AGC	Add SPX_RETURN_IF_NULL2/3/4 macros.
*   18/08/14 1.9    SP 	Add SPxStatus_t.
*   05/03/14 1.8    AGC	Add SPX_FILE_FORMAT_WEBM.
*		            Reorder SPxSizeof to fix compiler issue.
*   24/02/14 1.7    AGC	Add SPxIsArray.
*   17/02/14 1.6    SP 	Add SPX_NM_PER_METRE.
*   15/01/14 1.5    AGC	Fix icc remarks.
*   15/01/14 1.4    AGC	Add SPxSizeof and SPxArrayLen functions.
*   08/11/13 1.3    AGC	Add SPX_STATIC_ASSERT_ALWAYS macro.
*   04/11/13 1.2    AGC	Add SPX_METADATA_OPT_SCALE.
*   04/10/13 1.1    AGC	Initial version - types from SPxCommon.h.
**********************************************************************/

#ifndef _SPX_TYPES_H
#define _SPX_TYPES_H

#if defined(_WIN32) && (_MSC_VER >= 1900)
#define SPX_TYPES_HAS_SAL
#include <sal.h>
#endif

/*********************************************************************
*
*   Constants
*
**********************************************************************/

/* Make sure we have TRUE and FALSE. */
#ifndef	FALSE
#define	FALSE	0
#endif
#ifndef TRUE
#define	TRUE	1
#endif

/* Define the speed of light in metres per second. */
#define	SPX_SPEED_OF_LIGHT_MPS	299792458.0

/* Define the number of metres per nautical mile. */
#define	SPX_METRES_PER_NM	1852.0
#define SPX_NM_PER_METRE        (1.0 / 1852.0)

/* Define the number of metres per second per 1 knot (1NM/hour) and vv. */
#define	SPX_MPS_PER_KNOT	(SPX_METRES_PER_NM / (60.0 * 60.0))
#define	SPX_KNOTS_PER_MPS	(60.0 * 60.0 / SPX_METRES_PER_NM)

/* Define the number of metres per second per 1 km/h and vv. */
#define SPX_MPS_PER_KPH		(1000.0 / (60.0 * 60.0))
#define SPX_KPH_PER_MPS		(60.0 * 60.0 / 1000.0)

/* Define the number of feet per metres and vice versa. */
#define	SPX_METRES_PER_FOOT	(12.0 * 2.54 * 0.01)
#define	SPX_FEET_PER_METRE	(100.0 / (2.54 * 12.0))

/* Maximum value for various integer field sizes. */
#define SPX_UINT8_MAXVAL           255u
#define SPX_UINT12_MAXVAL          4095u
#define SPX_UINT16_MAXVAL          65535u

/* Assert an expression is true at compile time. */
template<bool> struct SPxStaticAssert;
template<> struct SPxStaticAssert<true>{};
#define SPX_STATIC_ASSERT(expr, msg)					\
struct msg								\
{									\
    SPxStaticAssert<static_cast<bool>((expr))> SPxStaticAssert__##msg;	\
}

/* "type" must be a template type. */
#define SPX_STATIC_ASSERT_ALWAYS(type, msg)				\
    SPX_STATIC_ASSERT(sizeof(type) == -1, msg)

/*********************************************************************
*
*   Enumerations
*
**********************************************************************/

enum SPxFileFormat_t
{
    SPX_FILE_FORMAT_WINDOWS_BMP	= 0,
    SPX_FILE_FORMAT_JPEG	= 1,
    SPX_FILE_FORMAT_PNG		= 2,
    SPX_FILE_FORMAT_WEBM	= 3
};

enum SPxMetadata_t
{
    SPX_METADATA_STRING	    = 0,
    SPX_METADATA_BINARY	    = 1
};

enum SPxMetadataOptions_t
{
    SPX_METADATA_OPT_NONE	    = 0x00,
    SPX_METADATA_OPT_FORCE_DISPLAY  = 0x01, /* Metadata must be displayed. */
    SPX_METADATA_OPT_FIXED	    = 0x02, /* Metadata cannot be edited. */
    SPX_METADATA_OPT_SCALE	    = 0x04  /* Scale metadata based on window size. */
};

/* Generic status types. */
enum SPxStatus_t
{
    /* Status types in ascending level of severity. This
     * allows the maximum value to be used when combining
     * multiple status values to determine the overall status.
     *
     * IMPORTANT: SPxGetStatus...() functions in SPxUtilsInit.cpp
     * must be updated if new status values are added here.
     */
    SPX_STATUS_DISABLED = 0,	/* Source/object disabled. */
    SPX_STATUS_OK       = 1,	/* Operating with no issues. */
    SPX_STATUS_NEUTRAL  = 2,	/* Status unknown or pending. */
    SPX_STATUS_WARNING  = 3,	/* Operating with some issues. */
    SPX_STATUS_ERROR    = 4	/* Not operating due to major issues. */
};

/* Distance units, e.g. for display. */
enum SPxUnitsDist_t
{
    SPX_UNITS_METRES		= 0,
    SPX_UNITS_KILOMETRES	= 1,
    SPX_UNITS_NAUTICAL_MILES	= 2,
    SPX_UNITS_FEET		= 3,
    SPX_UNITS_DIST_MAX /* MUST BE LAST ENTRY */
};

/* Speed units, e.g. for display. */
enum SPxUnitsSpeed_t
{
    SPX_UNITS_MPS		= 0,
    SPX_UNITS_KMPH		= 1,
    SPX_UNITS_KNOTS		= 2,
    SPX_UNITS_SPEED_MAX /* MUST BE LAST ENTRY */
};

/* Lat/long and other position display formats. */
enum SPxFormatLatLong_t
{
    SPX_FORMAT_LL_DEGS			= 0,
    SPX_FORMAT_LL_DEGS_MINS		= 1,
    SPX_FORMAT_LL_DEGS_MINS_SECS	= 2,
    SPX_FORMAT_LL_UTM			= 3,	/* Not all uses support this */
    SPX_FORMAT_LL_MGRS			= 4,	/* Not all uses support this */
    SPX_FORMAT_LL_MAX /* MUST BE LAST ENTRY */
};

/* Deprecated type. */
#define SPX_FORMAT_LL_BEARING_MAX SPX_FORMAT_LL_MAX

/*********************************************************************
*
*   Type definitions
*
**********************************************************************/

/* Define the common fixed-size types we use throughout.
 * They also need to be checked at runtime
 * by one of the Init functions to make sure their sizes are
 * correct.
 */
typedef unsigned long long UINT64;
typedef unsigned int UINT32;
typedef unsigned short UINT16;
typedef unsigned char UINT8;
typedef long long INT64;
#ifndef XMD_H /* Xmd.h defines INT32 as long. */
#ifdef __sun
typedef long INT32;   /* Needs "long" on Solaris (clash with Xmd.h) */
#else
typedef int INT32;
#endif
#endif
typedef short INT16;
typedef signed char INT8;	/* Needs "signed" for windows */
typedef unsigned char UCHAR;	
typedef float REAL32;
typedef double REAL64;

/* Check sizes of types. */
SPX_STATIC_ASSERT(sizeof(UINT64) == 8, UINT64_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(UINT32) == 4, UINT32_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(UINT16) == 2, UINT16_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(UINT8) == 1, UINT8_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(INT64) == 8, INT64_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(INT32) == 4, INT32_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(INT16) == 2, INT16_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(INT8) == 1, INT8_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(UCHAR) == 1, UCHAR_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(REAL32) == 4, REAL32_is_incorrect_size);
SPX_STATIC_ASSERT(sizeof(REAL64) == 8, REAL64_is_incorrect_size);

/* Structure for a point */
typedef struct SPxPoint_tag
{
    double x;
    double y;

} SPxPoint;

/*********************************************************************
*
*   Macros
*
**********************************************************************/

/* If we don't already have MIN/MAX macros, define them now. */
#ifndef MIN
#define MIN(a,b) (((a)<(b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b)) ? (a) : (b))
#endif 

/* Macro that converts an angle in degrees into a 16-bit azimuth number. */
#define SPX_DEGREES_TO_16BITAZIMUTH(x) ((int)(x * 65536 / 360.0))
 

/* Assert/Confirm - now does nothing. */
#define CONFIRM(x) 

/* Create a common definition for the current function name. */
#if defined(WIN32)
#define SPX_FUNCTION_NAME __FUNCTION__
#elif defined(__BORLAND__)
#define SPX_FUNCTION_NAME __FUNC__
#else
#define SPX_FUNCTION_NAME __func__
#endif

/* Query alignment requirements for type. */
#ifdef _WIN32
#define SPxAlignof(a) __alignof(a)
#else
#ifdef __GNUC__
#define SPxAlignof(a) __alignof__(a)
#else
#define SPxAlignof(a) 1
#endif
#endif

/* Specify alignment requirements for variable. */
#ifdef _WIN32
#define SPxAlign(a) __declspec(align(a))
#else
#ifdef __GNUC__
#define SPxAlign(a)  __attribute__((__aligned__(a)))
#else
#define SPxAlign(a)
#endif
#endif

/* C++11 final/override identifiers. */
#ifndef SPX_NATIVE_OVERRIDE
#if defined(_WIN32)
#if (_MSC_VER >= 1700)
#define SPX_NATIVE_OVERRIDE
#endif /* _MSC_VER */
#else
#if __cplusplus >= 201103L
#if defined(__INTEL_COMPILER)
#if (__GNUC__ >= 14)
#define SPX_NATIVE_OVERRIDE
#endif
#elif defined(__clang__)
#if (__clang_major__ > 3) || ((__clang_major__ == 3) && (__clang_minor__ >= 2))
#define SPX_NATIVE_OVERRIDE
#endif
#elif defined(__GNUC__)
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 7))
#define SPX_NATIVE_OVERRIDE
#endif
#endif /* Linux variants */
#endif /* C++ version check */
#endif /* Windows/Linux */
#endif /* SPX_NATIVE_OVERRIDE */
#ifdef SPX_NATIVE_OVERRIDE
#define SPX_VIRTUAL_OVERRIDE override
#define SPX_VIRTUAL_FINAL final
#else
#define SPX_VIRTUAL_OVERRIDE
#define SPX_VIRTUAL_FINAL
#endif

/* C++11 deleted and default functions. */
#ifndef SPX_NATIVE_FUNCTION_DELETED
#if defined(_WIN32)
#if (_MSC_VER >= 1800)
#define SPX_NATIVE_FUNCTION_DELETED
#endif /* _MSC_VER */
#else
#if __cplusplus >= 201103L
#if defined(__INTEL_COMPILER)
#if (__GNUC__ >= 12)
#define SPX_NATIVE_FUNCTION_DELETED
#endif
#elif defined(__clang__)
#if (__clang_major__ > 2) || ((__clang_major__ == 2) && (__clang_minor__ >= 9))
#define SPX_NATIVE_FUNCTION_DELETED
#endif
#elif defined(__GNUC__)
#if (__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 4))
#define SPX_NATIVE_FUNCTION_DELETED
#endif
#endif /* Linux variants */
#endif /* C++ version check */
#endif /* Windows/Linux */
#endif /* SPX_NATIVE_FUNCTION_DELETED */
#ifdef SPX_NATIVE_FUNCTION_DELETED
#define SPX_FUNCTION_DELETE = delete
#define SPX_FUNCTION_DEFAULT = default
#else
#define SPX_FUNCTION_DELETE
#define SPX_FUNCTION_DEFAULT
#endif

/* Validate argument as non-NULL. */
#define SPX_RETURN_IF_NULL(arg) {				\
    if( !(arg) )						\
    {								\
	SPxError(SPX_ERR_USER, SPX_ERR_BAD_ARGUMENT, 0, 0,	\
	SPX_FUNCTION_NAME, "NULL " #arg " argument");		\
	return SPX_ERR_BAD_ARGUMENT;				\
    }								\
}

#define SPX_RETURN_IF_NULL2(arg1, arg2) {			\
    SPX_RETURN_IF_NULL(arg1);					\
    SPX_RETURN_IF_NULL(arg2);					\
}

#define SPX_RETURN_IF_NULL3(arg1, arg2, arg3) {			\
    SPX_RETURN_IF_NULL2(arg1, arg2);				\
    SPX_RETURN_IF_NULL(arg3);					\
}

#define SPX_RETURN_IF_NULL4(arg1, arg2, arg3, arg4) {		\
    SPX_RETURN_IF_NULL2(arg1, arg2);				\
    SPX_RETURN_IF_NULL2(arg3, arg4);				\
}

/* Similar to the above, but specifying the return value. */
#define SPX_RETURN_VAL_IF_NULL(arg, val) {			\
    if( !(arg) )						\
    {								\
	SPxError(SPX_ERR_USER, SPX_ERR_BAD_ARGUMENT, 0, 0,	\
	SPX_FUNCTION_NAME, "NULL " #arg " argument");		\
	return val;						\
    }								\
}


/* Validate error code and return on error. */
#define SPX_RETURN_IF_FAIL(call) {				\
    SPxErrorCode spxerr__ = static_cast<SPxErrorCode>(call);	\
    if(SPX_NO_ERROR != spxerr__) {				\
	return spxerr__;					\
    }								\
}

/*********************************************************************
*
*   More type definitions (that depend on macros above)
*
**********************************************************************/

/* Mimic C++11 nullptr. */
const class SPxNullPtr_t
{
public:
    template<class T>
    inline operator T*() const { return 0; }

    template<class C, class T>
    inline operator T C::*() const { return 0; }

private:
    void operator&() const SPX_FUNCTION_DELETE;

} SPxNullPtr = {};

/*********************************************************************
*
*   Function prototypes
*
**********************************************************************/
template<typename T, T v>
struct SPxIntegralConstant
{
    static const T value = v;
    typedef T value_type;
};
template<bool B> struct SPxBoolConstant : public SPxIntegralConstant<bool, B> {};
typedef SPxBoolConstant<true> SPxTrueType;
typedef SPxBoolConstant<false> SPxFalseType;
template<typename T> struct SPxAddRef { typedef T& type; };
template<typename T> struct SPxAddRef<T&> { typedef T& type; };
template<typename T> struct SPxAddCref { typedef T const & type; };
template<typename T> struct SPxAddCref<T&> { typedef T const & type; };
template<typename T> struct SPxAddCref<T const &> { typedef T const & type; };
template<typename T> struct SPxAddCrefIfNotRef { typedef T const & type; };
template<typename T> struct SPxAddCrefIfNotRef<T&> { typedef T& type; };
template<typename T> struct SPxAddCrefIfNotRef<T const &> { typedef T const & type; };
template<typename T> struct SPxAddConst { typedef const T type; };
template<typename T> struct SPxAddConst<T&> { typedef T const type; };
template<typename T> struct SPxRemoveConst { typedef T type; };
template<typename T> struct SPxRemoveConst<const T> { typedef T type; };
template<typename T> struct SPxRemoveVolatile { typedef T type; };
template<typename T> struct SPxRemoveVolatile<volatile T> { typedef T type; };
template<typename T> struct SPxRemoveCV { typedef typename SPxRemoveVolatile<typename SPxRemoveConst<T>::type>::type type; };
template<typename T> struct SPxRemoveRef { typedef T type; };
template<typename T> struct SPxRemoveRef<T&> { typedef T type; };
template<typename T> struct SPxIsPointerHelper { static const bool value = false; };
template<typename T> struct SPxIsPointerHelper<T*> { static const bool value = true; };
template<typename T> struct SPxIsPointer : public SPxIsPointerHelper<typename SPxRemoveCV<T>::type> {};
template<typename T> struct SPxIsArray { static const bool value = false; };
template<typename T> struct SPxIsArray<T[]> { static const bool value = true; };
template<typename T, unsigned int N> struct SPxIsArray<T[N]> { static const bool value = true; };
template<typename T, typename U> struct SPxIsSame : public SPxFalseType {};
template<typename T> struct SPxIsSame<T, T> : public SPxTrueType {};

template<typename T>
struct SPxHasEmptyMethod
{
    template<typename U, bool (U::*)() const> struct SFINAE {};
    template<typename U> static char Test(SFINAE<U, &U::empty>*);
    template<typename U> static int Test(...);
    static const bool value = sizeof(Test<T>(0)) == sizeof(char);
};

template<typename T>
inline unsigned int SPxSizeof(void)
{
    SPX_STATIC_ASSERT(!SPxIsPointer<T>::value,
	SPxSizeof_arg_cannot_be_pointer);
    return static_cast<unsigned int>(sizeof(T));
}

template<typename T, unsigned int Len>
inline unsigned int SPxSizeof(T (&)[Len])
{
    return Len * SPxSizeof<T>();
}

template<typename T>
inline unsigned int SPxSizeof(const T& /*obj*/)
{
    return SPxSizeof<T>();
}

template<typename T>
inline typename T::iterator SPxBegin(T& c)
{
    return c.begin();
}

template<typename T>
inline typename T::const_iterator SPxCBegin(const T& c)
{
    return c.begin();
}

template<typename T>
inline typename T::reverse_iterator SPxRBegin(T& c)
{
    return c.rbegin();
}

template<typename T>
inline typename T::const_reverse_iterator SPxCRBegin(const T& c)
{
    return c.rbegin();
}

template<typename T>
inline typename T::iterator SPxEnd(T& c)
{
    return c.end();
}

template<typename T>
inline typename T::const_iterator SPxCEnd(const T& c)
{
    return c.end();
}

template<typename T>
inline typename T::reverse_iterator SPxREnd(T& c)
{
    return c.rend();
}

template<typename T>
inline typename T::const_reverse_iterator SPxCREnd(const T& c)
{
    return c.rend();
}

template<typename T, unsigned int Len>
inline T* SPxBegin(T (&arr)[Len])
{
    return arr;
}

template<typename T, unsigned int Len>
inline const T* SPxCBegin(const T (&arr)[Len])
{
    return arr;
}

template<typename T, unsigned int Len>
inline T* SPxEnd(T (&arr)[Len])
{
    return arr + Len;
}

template<typename T, unsigned int Len>
inline const T* SPxCEnd(const T (&arr)[Len])
{
    return arr + Len;
}

template<typename T, unsigned int Len>
#ifdef SPX_TYPES_HAS_SAL
_Post_equal_to_(Len)
#endif
inline unsigned int SPxArrayLen(
#ifdef SPX_TYPES_HAS_SAL
    _In_
#endif
				const T (&)[Len])
{
    return Len;
}

template<typename T>
inline unsigned int SPxLength(const T& c)
{
    return static_cast<unsigned int>(c.size());
}

template<typename T, unsigned int Len>
#ifdef SPX_TYPES_HAS_SAL
_Post_equal_to_(Len)
#endif
inline unsigned int SPxLength(
#ifdef SPX_TYPES_HAS_SAL
    _In_
#endif
			      const T (&)[Len])
{
    return Len;
}

template<typename T>
inline bool SPxEmpty(const T& c, SPxTrueType)
{
    return c.empty();
}

template<typename T>
inline bool SPxEmpty(const T& c)
{
    return SPxEmpty(c, SPxBoolConstant<SPxHasEmptyMethod<T>::value>());
}

template<typename T, unsigned int Len>
inline bool SPxEmpty(const T(&arr)[Len])
{
    return false;
}

template<typename T>
inline typename T::pointer SPxData(T& c)
{
    return SPxEmpty(c) ? 0 : &c[0];
}

template<typename T>
inline typename T::const_pointer SPxData(const T& c)
{
    return SPxEmpty(c) ? 0 : &c[0];
}

template<typename T, unsigned int Len>
inline T* SPxData(T (&arr)[Len])
{
    return arr;
}

template<typename T, unsigned int Len>
inline const T* SPxData(const T (&arr)[Len])
{
    return arr;
}

extern UINT32 SPxGetStatusARGB(SPxStatus_t status);
extern UINT32 SPxGetStatusARGB(UINT32 status);
extern const char *SPxGetStatusText(SPxStatus_t status);
extern const char *SPxGetStatusText(UINT32 status);

/* Units utility functions. */
extern const char *SPxUnitsGetTextForDist(SPxUnitsDist_t units, int isHeight=FALSE);
extern const char *SPxUnitsGetTextForSpeed(SPxUnitsSpeed_t units);
extern double SPxUnitsGetDistMultFromMetres(SPxUnitsDist_t units, int isHeight=FALSE);
extern double SPxUnitsGetSpeedMultFromMps(SPxUnitsSpeed_t units);

#endif /* _SPX_TYPES_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
