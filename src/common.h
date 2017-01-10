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

#include <cstdint>

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

// Typedef for holding VBL counts
typedef std::uint32_t vbls_t;

// Typedef for holding CPU cycle counts
typedef std::uint64_t cycles_t;

#endif // COMMON_H_
