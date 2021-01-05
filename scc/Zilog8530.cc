/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2021 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the Zilog 8530 Serial Communications Controller.
 */

#include <cstdlib>

#include "Zilog8530.h"

using std::uint8_t;

/**
 * Reset the SCC to its powerup state.
 */
void Zilog8530::reset()
{
}

uint8_t Zilog8530::read(const unsigned int& offset)
{
    uint8_t val = 0;

    switch (offset) {
        case 0x38:      // channel B command
        case 0x39:      // channel A command
        case 0x3A:      // channel B data
        case 0x3B:      // channel A data
            break;

        default:
            break;
    }

    return val;
}

void Zilog8530::write(const unsigned int& offset, const uint8_t& val)
{
    switch (offset) {
        case 0x38:      // channel B command
        case 0x39:      // channel A command
        case 0x3A:      // channel B data
        case 0x3B:      // channel A data
            break;

        default:
            break;
    }
}

void Zilog8530::tick(const unsigned int frame_number)
{
}

void Zilog8530::microtick(const unsigned int line_number)
{
}
