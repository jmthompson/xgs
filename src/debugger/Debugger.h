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

using std::string;
using std::uint8_t;
using std::uint32_t;

namespace M65816 {

class Processor;

struct Instruction {
    uint8_t bank;
    uint16_t address;
    const Opcode *opcode;

    std::vector<unsigned int> bytes;
};

class Debugger {
    private:
        Processor *cpu;

        uint8_t instruction[4];
        int instruction_len;

        string renderArgument(const Instruction& ins);
        string renderBytes(const Instruction& ins);

    public:
        Debugger(Processor *theCpu) : cpu(theCpu) {}
        ~Debugger() = default;

        Instruction decodeInstruction(uint8_t bank, uint16_t address);
        virtual void postExecute(uint8_t bank, uint16_t address);
};

} // namespace M65816

#endif // DEBUGGER_H
