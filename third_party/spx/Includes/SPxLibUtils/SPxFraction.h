/*********************************************************************
*
* (c) Copyright 2013 - 2014, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxFraction.h,v $
* ID: $Id: SPxFraction.h,v 1.3 2014/07/04 14:47:16 rew Exp $
*
* Purpose:
*   Generic SPxFraction class.
*
* Revision Control:
*   04/07/14 v1.3    AGC	Move some code to source file.
*
* Previous Changes:
*   26/03/14 1.2    AGC	Add const char* operators.
*   22/11/13 1.1    AGC	Initial version - moved from AV library.
**********************************************************************/

#ifndef _SPX_FRACTION_H
#define _SPX_FRACTION_H

#include <iosfwd>

template <typename T>
class SPxFraction
{
public:
    typedef T value_type;
    typedef SPxFraction<T> self_type;

    explicit SPxFraction(T numerator=T(), 
	T denominator=T())
	: Numerator(numerator), Denominator(denominator)
    {
	m_cache[0] = '\0';
    }
    explicit SPxFraction(const char *text)
	: Numerator(T()), Denominator(T())
    {
	m_cache[0] = '\0';
	str(text);
    }
    SPxFraction<T>& operator=(const char *text)
    {
	str(text);
	return *this;
    }
    const char *str(void) const;
    void str(const char *text);
    operator const char *(void) const
    {
	return str();
    }
    bool operator==(const self_type& rhs) const;
    bool operator!=(const self_type& rhs) const
    {
	return !operator==(rhs);
    }

    T Numerator;
    T Denominator;

private:
    mutable char m_cache[32];
};
template <typename T>
std::ostream& operator<<(std::ostream& os, const SPxFraction<T>& obj);
template <typename T>
std::istream& operator>>(std::istream& is, SPxFraction<T>& obj);

/* Common implementations of SPxFraction. */
typedef SPxFraction<unsigned int> SPxFractionU;
typedef SPxFraction<int> SPxFractionI;
typedef SPxFraction<float> SPxFractionF;
typedef SPxFraction<double> SPxFractionD;

#endif /* _SPX_FRACTION_H */

/*********************************************************************
*
* End of file
*
**********************************************************************/
