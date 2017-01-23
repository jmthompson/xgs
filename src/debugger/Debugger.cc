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

#include "M65816/Processor.h"
#include "Debugger.h"

#include "opcodes.h"

using std::cout;
using std::endl;
using std::int8_t;
using std::int16_t;
using std::string;
using boost::format;

using M65816::mem_access_t;

uint8_t Debugger::memoryRead(const uint8_t bank, const uint16_t address, const uint8_t val, const mem_access_t type)
{
    switch (type) {
        case M65816::INSTR:
            handleInstructionRead(bank, address, val);
            break;
        case M65816::VECTOR:
            handleVectorRead(bank, address, val);
            break;
        default:
             break;
    }

    return val;
}

uint8_t Debugger::memoryWrite(const uint8_t bank, const uint16_t address, const uint8_t val, const mem_access_t type)
{
    switch (type) {
        case M65816::STACK:
            //std::cerr << format("stack write: %02X (%04X)\n") % (unsigned int) val % address;
            break;
        default:
            break;
    }

    return val;
}

void Debugger::handleInstructionRead(const uint8_t bank, const uint16_t address, const uint8_t val)
{
    if (ins_fetch) {
        cpu_instr.bytes.push_back(val);
    }
    else {
        cpu_instr.bytes.clear();

        cpu_instr.pbr = bank;
        cpu_instr.pc  = address;

        cpu_instr.bytes.push_back(val);

        int len = opcodes[val].len;

        if (len == kLenIsMemorySize) {
            cpu_instr.len = 2 + !system->cpu->SR.M;
        }
        else if (len == kLenIsIndexSize) {
            cpu_instr.len = 2 + !system->cpu->SR.X;
        }
        else {
            cpu_instr.len = len;
        }

        ins_fetch = true;
    }

    if (cpu_instr.bytes.size() == cpu_instr.len) {
        ins_fetch = false;

        cout << format("%02X/%04X:%-12s  %3s  %-18s |%c%c%c%c%c%c%c%c| E=%1d DBR=%02X A=%04X X=%04X Y=%04X S=%04X D=%04X\n")
                        % (int) cpu_instr.pbr
                        % cpu_instr.pc
                        % renderBytes()
                        % opcodes[cpu_instr.bytes[0]].mnemonic
                        % renderArgument()
                        % (system->cpu->SR.N? 'n' : '-')
                        % (system->cpu->SR.V? 'v' : '-')
                        % (system->cpu->SR.M? 'm' : '-')
                        % (system->cpu->SR.X? 'x' : '-')
                        % (system->cpu->SR.D? 'd' : '-')
                        % (system->cpu->SR.I? 'i' : '-')
                        % (system->cpu->SR.Z? 'z' : '-')
                        % (system->cpu->SR.C? 'c' : '-')
                        % (int) system->cpu->SR.E
                        % (int) system->cpu->DBR
                        % system->cpu->A.W
                        % system->cpu->X.W
                        % system->cpu->Y.W
                        % system->cpu->S.W
                        % system->cpu->D;
    }
}

void Debugger::handleVectorRead(const uint8_t bank, const uint16_t address, const uint8_t val)
{
    if (vector_fetch) {
        vector_fetch = false;
        vector_data |= (val << 8);

        std::cerr << format("Fetch new PC %04X from vector %04X\n") % vector_data % vector_addr;
    }
    else {
        vector_fetch = true;
        vector_addr  = address;
        vector_data  = val;
    }
}

std::string Debugger::renderArgument()
{
    std::stringstream buffer;
    int i;

    switch (opcodes[cpu_instr.bytes[0]].mode) {
        case kAddrMode_immediate:

            buffer << "#$";

            for (i = cpu_instr.bytes.size() - 1 ; i > 0 ; i--) {
                buffer << format("%02X") % cpu_instr.bytes[i];
            }

            break;

        case kAddrMode_a:
            buffer << format("$%02X%02X") % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_al:
            buffer << format("$%02X%02X%02X") % cpu_instr.bytes[3] % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_d:
            buffer << format("$%02X") % cpu_instr.bytes[1];
            break;

        case kAddrMode_accumulator:
        case kAddrMode_implied:
        case kAddrMode_stack:
            break;

        case kAddrMode_dix:
            buffer << format("($%02X),Y") % cpu_instr.bytes[1];
            break;

        case kAddrMode_dixl:
            buffer << format("[$%02X],Y") % cpu_instr.bytes[1];
            break;

        case kAddrMode_dxi:
            buffer << format("($%02X),X") % cpu_instr.bytes[1];
            break;

        case kAddrMode_dxx:
            buffer << format("$%02X,X") % cpu_instr.bytes[1];
            break;

        case kAddrMode_dxy:
            buffer << format("$%02X,Y") % cpu_instr.bytes[1];
            break;

        case kAddrMode_axx:
            buffer << format("$%02X%02X,X") % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_alxx:
            buffer << format("$%02X%02X%02X,X") % cpu_instr.bytes[3] % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_axy:
            buffer << format("$%02X%02X,Y") % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_pcr:
            {
                int8_t offset = cpu_instr.bytes[1];
                uint16_t dst  = cpu_instr.pc + 2 + offset;

                buffer << format("$%04X") % dst;
            }
            break;

        case kAddrMode_pcrl:
            {
                int16_t offset = (cpu_instr.bytes[2] << 8) | cpu_instr.bytes[1];
                uint16_t dst   = cpu_instr.pc + 3 + offset;

                buffer << format("$%04X") % dst;
            }
            break;

        case kAddrMode_ai:
        case kAddrMode_ail:
            buffer << format("($%02X%02X)") % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_di:
            buffer << format("($%02X)") % cpu_instr.bytes[1];
            break;

        case kAddrMode_dil:
            buffer << format("[$%02X]") % cpu_instr.bytes[1];
            break;

        case kAddrMode_axi:
            buffer << format("($%02X%02X,X)") % cpu_instr.bytes[2] % cpu_instr.bytes[1];
            break;

        case kAddrMode_sr:
            buffer << format("$%02X,S") % cpu_instr.bytes[1];
            break;

        case kAddrMode_srix:
            buffer << format("($%02X,S),Y") % cpu_instr.bytes[1];
            break;

        case kAddrMode_blockmove:
            buffer << format("$%02X,$%02X") % cpu_instr.bytes[1] % cpu_instr.bytes[2];
            break;
    }

    return buffer.str();
}

std::string Debugger::renderBytes()
{
    std::stringstream buffer;

    for (const int &b : cpu_instr.bytes) {
        buffer << format(" %02X") % b;
    }

    return buffer.str();
}
