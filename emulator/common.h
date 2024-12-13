#pragma once

/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2017 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <cstdlib>
#include <iostream>
#include <random>
#include <string>
#include <boost/format.hpp>
#include "M65816/m65816.h"

using boost::format;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::string;

using m65816::cycles_t;

namespace xgs {

enum mem_page_t {
    UNMAPPED = 0,
    ROM,
    FAST,
    SLOW
};

enum irq_source_t {
    UNKNOWN = 0,
    MEGA2_IRQ,
    VGC_IRQ,
    DOC_IRQ,
    ADB_IRQ
};

struct MemoryPage {
    uint8_t *read    = nullptr;
    uint8_t *write   = nullptr;
    uint8_t *swrite  = nullptr;

    mem_page_t type = UNMAPPED;
};

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
    typedef typename std::conditional<std::is_integral<T>::value,
							std::uniform_int_distribution<T>,
							std::uniform_real_distribution<T>>::type Dist;
    Dist dist(a,b);
    return dist(mt);
}


// Typedef for holding VBL counts
typedef std::uint32_t vbls_t;

// Maximum number of Smartport units
const unsigned int kSmartportUnits = 8;

const unsigned int kVersionMajor = VERSION_MAJOR;
const unsigned int kVersionMinor = VERSION_MINOR;

}; // namespace xgs
