/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2017 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#ifndef COMMON_H_
#define COMMON_H_

#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/format.hpp>

using boost::format;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::string;

/**
 * This file contains typesdefs and functions shared by all the separate XGS
 * subprojects.
 */

// http://stackoverflow.com/questions/105252/how-do-i-convert-between-big-endian-and-little-endian-values-in-c

template <typename T> T swap_endian(T u)
{
    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for (std::size_t k = 0; k < sizeof(T); k++) 
        dest.u8[k] = source.u8[sizeof(T) - k - 1];
    
    return dest.u;
}

template<typename T = int>
T random(T a , T b)
{
    static std::random_device rd;
    static std::mt19937 mt(rd());
    typedef std::conditional<std::is_integral<T>::value,
							std::uniform_int_distribution<T>,
							std::uniform_real_distribution<T>>::type Dist;
    Dist dist(a,b);
    return dist(mt);
}


// Typedef for holding VBL counts
typedef std::uint32_t vbls_t;

// Typedef for holding CPU cycle counts
typedef std::uint64_t cycles_t;

// Typedef representing pixel data in the video driver (32-bit color)
typedef std::uint32_t pixel_t;

// Maximum number of Smartport units
const unsigned int kSmartportUnits = 8;

const unsigned int kVersionMajor = VERSION_MAJOR;
const unsigned int kVersionMinor = VERSION_MINOR;

#endif // COMMON_H_
