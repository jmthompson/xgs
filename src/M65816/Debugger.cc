/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <memory>
#include <string>
#include <cstdio>
#include <vector>

#include <boost/format.hpp>

#include "Debugger.h"
#include "Opcode.h"
#include "Processor.h"

using std::cout;
using std::endl;
using std::int8_t;
using std::int16_t;
using std::string;

namespace M65816 {

Instruction Debugger::decodeInstruction(uint8_t bank, uint16_t address)
{
    Instruction ins;

    ins.bank = bank;
    ins.address = address;
    ins.bytes.push_back(cpu->system->cpuRead(bank, address));
    ins.opcode = &opcodes[ins.bytes[0]];

    int len = ins.opcode->getLength();

    if (len == kLenIsMemorySize) {
        len = 2 + !cpu->SR.M;
    }
    else if (len == kLenIsIndexSize) {
        len = 2 + !cpu->SR.X;
    }

    for (int i = 1 ; i < len ; i++) {
        ins.bytes.push_back(cpu->system->cpuRead(bank, address + i));
    }

    return ins;
}

std::string Debugger::renderArgument(const Instruction& ins)
{
    std::stringstream buffer;
    int i;

    switch (ins.opcode->getMode()) {
        case kAddrMode_immediate:

            buffer << "#$";

            for (i = ins.bytes.size() - 1 ; i > 0 ; i--) {
                buffer << boost::format("%02X") % ins.bytes[i];
            }

            break;

        case kAddrMode_a:
            buffer << boost::format("$%02X%02X") % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_al:
            buffer << boost::format("$%02X/%02X%02X") % ins.bytes[3] % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_d:
            buffer << boost::format("$%02X") % ins.bytes[1];
            break;

        case kAddrMode_accumulator:
        case kAddrMode_implied:
        case kAddrMode_stack:
            break;

        case kAddrMode_dix:
            buffer << boost::format("($%02X),Y") % ins.bytes[1];
            break;

        case kAddrMode_dixl:
            buffer << boost::format("[$%02X],Y") % ins.bytes[1];
            break;

        case kAddrMode_dxi:
            buffer << boost::format("($%02X),X") % ins.bytes[1];
            break;

        case kAddrMode_dxx:
            buffer << boost::format("$%02X,X") % ins.bytes[1];
            break;

        case kAddrMode_dxy:
            buffer << boost::format("$%02X,Y") % ins.bytes[1];
            break;

        case kAddrMode_axx:
            buffer << boost::format("$%02X%02X,X") % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_alxx:
            buffer << boost::format("$%02X/%02X%02X,X") % ins.bytes[3] % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_axy:
            buffer << boost::format("$%02X%02X,Y") % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_pcr:
            {
                int8_t offset = ins.bytes[1];
                uint16_t dst  = ins.address + 2 + offset;

                buffer << boost::format("$%04X") % dst;
            }
            break;

        case kAddrMode_pcrl:
            {
                int16_t offset = (ins.bytes[2] << 8) | ins.bytes[1];
                uint16_t dst   = ins.address + 3 + offset;

                buffer << boost::format("$%04X") % dst;
            }
            break;

        case kAddrMode_ai:
        case kAddrMode_ail:
            buffer << boost::format("($%02X%02X)") % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_di:
            buffer << boost::format("($%02X)") % ins.bytes[1];
            break;

        case kAddrMode_dil:
            buffer << boost::format("[$%02X]") % ins.bytes[1];
            break;

        case kAddrMode_axi:
            buffer << boost::format("($%02X%02X,X)") % ins.bytes[2] % ins.bytes[1];
            break;

        case kAddrMode_sr:
            buffer << boost::format("$%02X,S") % ins.bytes[1];
            break;

        case kAddrMode_srix:
            buffer << boost::format("($%02X,S),Y") % ins.bytes[1];
            break;

        case kAddrMode_blockmove:
            buffer << boost::format("$%02X,%02X") % ins.bytes[1] % ins.bytes[2];
            break;
    }

    return buffer.str();
}

std::string Debugger::renderBytes(const Instruction &ins)
{
    std::stringstream buffer;

    for (const int &b : ins.bytes) {
        buffer << boost::format{" %02X"} % b;
    }

    return buffer.str();
}

void Debugger::postExecute(uint8_t bank, uint16_t address)
{
    Instruction ins = decodeInstruction(bank, address);

    cout << boost::format("%02X/%04X:%-12s  %3s %-18s |%c%c%c%c%c%c%c%c| E=%1d DBR=%02X A=%04X X=%04X Y=%04X S=%04X D=%04X\n")
                % (int) bank
                % address
                % renderBytes(ins)
                % ins.opcode->getName()
                % renderArgument(ins)
                % (cpu->SR.N? 'n' : '-')
                % (cpu->SR.V? 'v' : '-')
                % (cpu->SR.M? 'm' : '-')
                % (cpu->SR.X? 'x' : '-')
                % (cpu->SR.D? 'd' : '-')
                % (cpu->SR.I? 'i' : '-')
                % (cpu->SR.Z? 'z' : '-')
                % (cpu->SR.C? 'c' : '-')
                % (int) cpu->SR.E
                % (int) cpu->DBR
                % cpu->A.W
                % cpu->X.W
                % cpu->Y.W
                % cpu->S.W
                % cpu->D;
}

} // namespace M65816
