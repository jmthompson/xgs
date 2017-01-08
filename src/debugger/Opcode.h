/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#ifndef OPCODE_H
#define OPCODE_H

#include <string>

using std::uint8_t;

namespace M65816 {

class Opcode {
    private:
        const uint8_t code;
        const std::string name;
        const unsigned int mode;
        const int len;

    public:
        Opcode(const uint8_t theCode, const char *theName, const unsigned int theMode, const int theLen)
            : code(theCode), len(theLen), name(theName), mode(theMode) {}

        uint8_t getCode () const { return code; }
        const std::string & getName () const { return name; }
        unsigned int getMode () const { return mode; }
        int getLength () const { return len; }
};

const int kLenIsMemorySize = -1;
const int kLenIsIndexSize  = -2;

// Addressing modes

const unsigned int kAddrMode_immediate   = 1;
const unsigned int kAddrMode_a           = 2;
const unsigned int kAddrMode_al          = 3;
const unsigned int kAddrMode_d           = 4;
const unsigned int kAddrMode_accumulator = 5;
const unsigned int kAddrMode_implied     = 6;
const unsigned int kAddrMode_dix         = 7;
const unsigned int kAddrMode_dixl        = 8;
const unsigned int kAddrMode_dxi         = 9;
const unsigned int kAddrMode_dxx         = 10;
const unsigned int kAddrMode_dxy         = 11;
const unsigned int kAddrMode_axx         = 12;
const unsigned int kAddrMode_alxx        = 13;
const unsigned int kAddrMode_axy         = 14;
const unsigned int kAddrMode_pcr         = 15;
const unsigned int kAddrMode_pcrl        = 16;
const unsigned int kAddrMode_ai          = 17;
const unsigned int kAddrMode_ail         = 171; // same as _ai in the data sheet but acts differently
const unsigned int kAddrMode_di          = 18;
const unsigned int kAddrMode_dil         = 19;
const unsigned int kAddrMode_axi         = 20;
const unsigned int kAddrMode_stack       = 21;
const unsigned int kAddrMode_sr          = 22;
const unsigned int kAddrMode_srix        = 23;
const unsigned int kAddrMode_blockmove   = 24;

extern const Opcode opcodes[256];

} // namespace M65816

#endif // OPCODE_H
