/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#pragma once

#include <cstdint>

namespace m65816 {

/**
 * Union definition of a 16-bit value that can also be accessed as its
 * component 8-bit values. This type is used to hold registers that have
 * variable widths, such as the accumulator and index registers.
 */

union multibyte16_t {
#ifdef BIGENDIAN
    struct { std::uint8_t H,L; } B;
#else
    struct { std::uint8_t L,H; } B;
#endif
    std::uint16_t W;

    operator std::uint8_t&  () { return B.L; }
    operator std::uint16_t& () { return W; }
};

/**
 * This enum type is used to classify memory accesses, primarily to faciliate
 * the writing of debuggers
 */
enum mem_access_t {
    DATA = 0,
    STACK,
    VECTOR,
    INSTR,
    OPADDR,
    OPERAND
};

// Typedef for holding CPU cycle counts
typedef std::uint64_t cycles_t;

// Total cycle count
extern cycles_t total_cycles;

// Reset the CPU
extern void reset(void);

// Raise a non-maskable interrupt
extern void nmi();

// Raise the ABORT signal
extern void abort();

// Set IRQ line state
extern void setIRQ(bool);

// Run until at least the specified number of cycles have been executed.
// Returns the actual number of cycles executed, which may be slightly
// higher than the number requested.
extern unsigned int runUntil(const unsigned int);

} // namespace m65816
