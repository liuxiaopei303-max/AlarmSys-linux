/*********************************************************************
*
* (c) Copyright 2015 - 2017, Cambridge Pixel Ltd.
*
* File: $RCSfile: SPxRandom.h,v $
* ID: $Id: SPxRandom.h,v 1.8 2017/04/12 09:21:12 rew Exp $
*
* Purpose:
*   Header for random number utilities.
*
* Revision Control:
*   12/04/17 v1.8    AGC	Support set/get of seeds.
*
* Previous Changes:
*   17/03/17 1.7    AGC	Remove unused functions/members.
*   16/03/17 1.6    AGC	Fix last change.
*   16/03/17 1.5    AGC	Avoid bias in large generated values.
*   04/01/17 1.4    AGC	Add SPxRandomNormal.
*			Add SPxRandomTriangle.
*			Avoid infinite loop with zero range in SPxRandomUniformReal.
*   29/03/16 1.3    AGC	Fix SPxRandomUniformReal.
*   23/12/15 1.2    AGC	Support multi value seeds.
*   22/10/15 1.1    AGC	Initial Version.
*
**********************************************************************/
#ifndef SPX_RANDOM_H_
#define SPX_RANDOM_H_

#include <cstddef>
#include "SPxLibUtils/SPxTypes.h"

#define SPX_RANDOM_ENGINE_ELEMENTS 624

class SPxRandomDevice
{
public:
    SPxRandomDevice();
    ~SPxRandomDevice();

    UINT32 operator()();

    static UINT32 minVal() { return 0; }
    static UINT32 maxVal() { return 0xFFFFFFFF; }
};

class SPxRandomEngine
{
public:
    explicit SPxRandomEngine(UINT32 seed = 0);
    explicit SPxRandomEngine(SPxRandomDevice& rd);
    ~SPxRandomEngine();

    void seed(UINT32 seedVal = 0);
    void seed(const UINT32 *seedStart, unsigned int numSeeds);
    void seed(SPxRandomDevice& rd);
    UINT32 operator()();
    void discard(UINT32 num);

    unsigned int getNumSeeds() const;
    void getSeeds(UINT32 *seeds, unsigned int seedsLen) const;

    static UINT32 minVal() { return 0; }
    static UINT32 maxVal() { return 0xFFFFFFFF; }

private:
    static const unsigned int NUM_ELEMENTS = SPX_RANDOM_ENGINE_ELEMENTS;
    UINT32 index;
    UINT32 mt[NUM_ELEMENTS];
    UINT32 start[NUM_ELEMENTS];
    UINT32 numStart;

    void twist();
};

template<typename T=int>
class SPxRandomUniformInt
{
public:
    explicit SPxRandomUniformInt(T a, T b)
	: m_a(a), m_b(b) {}
    ~SPxRandomUniformInt() {}

    template<typename G>
    T operator()(G& gen)
    {
	return getRand(gen, m_a, m_b);
    }

private:
    T m_a;
    T m_b;

    template<typename G>
    static T getRand(G& gen, T a, T b)
    {
	T val = T();
	UINT64 lval = UINT64();
	UINT64 range = static_cast<UINT64>(b - a);
	UINT64 genRange = gen.maxVal() - gen.minVal();
	if( range < genRange )
	{
	    UINT64 scale = (genRange + range) / range;
	    do
	    {
		lval = gen() - gen.minVal();
	    } while(lval >= (scale * range));
	    val = static_cast<T>(lval / scale);
	}
	else
	{
	    T scale = static_cast<T>((range + genRange) / genRange);
	    do
	    {
		lval = (gen() - gen.minVal()) + getRand(gen, 0, scale) * genRange;
	    }
	    while( lval >= range );
	    val = static_cast<T>(lval);
	}
	val = val + a;
	return val;
    }
};

template<typename T=double>
class SPxRandomUniformReal
{
public:
    explicit SPxRandomUniformReal(T a, T b)
	: m_uniInt(0ULL, 0x1FFFFFFFFFFFFFULL), m_a(a), m_b(b) {}
    ~SPxRandomUniformReal() {}

    template<typename G>
    T operator()(G& gen)
    {
	T val = T();
	T range = m_b - m_a;
	do
	{
	    val = m_a + m_uniInt(gen) * range / 0x1FFFFFFFFFFFFFULL;
	}
	while( (range > T()) && ((val < m_a) || (val >= m_b)) );
	return val;
    }

private:
    SPxRandomUniformInt<UINT64> m_uniInt;
    T m_a;
    T m_b;

};

template<typename T=double>
class SPxRandomNormal
{
public:
    explicit SPxRandomNormal(T mean, T stdDev)
	: m_uniReal1(0.0, 1.0), m_uniReal2(0.0, 1.0),
	m_mean(mean), m_stdDev(stdDev) {}

    template<typename G>
    T operator()(G& gen)
    {
	T u = m_uniReal1(gen);
	T v = m_uniReal2(gen);
	T val = genNormalVal(u, v);
	return m_mean + m_stdDev * val;
    }

private:
    SPxRandomUniformReal<T> m_uniReal1;
    SPxRandomUniformReal<T> m_uniReal2;
    T m_mean;
    T m_stdDev;

    static T genNormalVal(T u, T v);
};

template<typename T=double>
class SPxRandomTriangle
{
public:
    explicit SPxRandomTriangle(T min, T max)
	: m_uniReal(min, max) {}

    template<typename G>
    T operator()(G& gen)
    {
	return (m_uniReal(gen) + m_uniReal(gen)) / 2.0;
    }

private:
    SPxRandomUniformReal<T> m_uniReal;
};

#endif /* SPX_RANDOM_H_ */

/*********************************************************************
*
* End of file
*
**********************************************************************/
