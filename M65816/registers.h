#pragma once

/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/**
 * The status_reg class implements the status register as a series
 * of direct-accessible boolean flags, plus overloads to allow constructing
 * from and casting to/from uint8_t.
 */

#include <cstdint>
#include "m65816.h"

using std::uint8_t;

namespace m65816 {

struct status_reg {
    bool N = false;
    bool V = false;
    bool M = true;
    bool X = true;
    bool D = false;
    bool I = true;
    bool Z = false;
    bool C = false;
    bool E = true;

    status_reg() = default;
    status_reg(uint8_t v) { operator=(v); }
    ~status_reg() = default;

    status_reg& operator= (uint8_t v)
    {
        N = v & 0x80;
        V = v & 0x40;
        D = v & 0x08;
        I = v & 0x04;
        Z = v & 0x02;
        C = v & 0x01;

        if (!E) {
            M = v & 0x20;
            X = v & 0x10;
        }

        return *this;
    }

    status_reg& operator&= (uint8_t v)
    {
        if (!(v & 0x80)) N = false;
        if (!(v & 0x40)) V = false;
        if (!(v & 0x08)) D = false;
        if (!(v & 0x04)) I = false;
        if (!(v & 0x02)) Z = false;
        if (!(v & 0x01)) C = false;

        if (!E) {
            if (!(v & 0x20)) M = false;
            if (!(v & 0x10)) X = false;
        }
        return *this;
    }

    status_reg& operator|= (uint8_t v)
    {
        if (v & 0x80) N = true;
        if (v & 0x40) V = true;
        if (v & 0x08) D = true;
        if (v & 0x04) I = true;
        if (v & 0x02) Z = true;
        if (v & 0x01) C = true;

        if (!E) {
            if (v & 0x20) M = true;
            if (v & 0x10) X = true;
        }
        return *this;
    }

    operator uint8_t() const {
        return (N << 7)|(V << 6)|(M << 5)|(X << 4)|(D << 3)|(I << 2)|(Z << 1)|C;
    };
};

namespace registers {
    extern status_reg SR;
    extern uint16_t D;
    extern uint16_t PC;
    extern uint8_t PBR;
    extern uint8_t DBR;
    extern multibyte16_t S;
    extern multibyte16_t A;
    extern multibyte16_t X, Y;
}

} // namespace m65816
