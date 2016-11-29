/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "Opcode.h"

using std::uint8_t;
using std::uint32_t;

namespace M65816 {

class Processor;

struct Instruction {
    uint8_t bank;
    uint16_t address;
    const Opcode *opcode;

    std::vector<uint8_t> bytes;
};

class Debugger {
    private:
        Processor *cpu;

        uint8_t instruction[4];
        int instruction_len;

    public:
        Debugger(Processor *theCpu) : cpu(theCpu) {}
        ~Debugger() = default;

        Instruction decodeInstruction(uint8_t bank, uint16_t address);
        std::string renderArgument(const Instruction &ins);
        virtual void executeHook();
};

} // namespace M65816

#endif // DEBUGGER_H
