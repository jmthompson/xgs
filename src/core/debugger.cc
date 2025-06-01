/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <iostream>
#include <sstream>
#include <string>
#include <cstdio>
#include <vector>
#include <boost/format.hpp>

#include "m65816.h"
#include "config.h"
#include "cpu/registers.h"
#include "debugger.h"

using std::cout;
using std::int8_t;
using std::int16_t;
using std::string;
using boost::format;

using m65816::mem_access_t;

struct opcode {
    const unsigned char mnemonic[4];
    const unsigned int mode;
    const int len;
};

static const opcode opcodes[256] = {
    {"BRK", kAddrMode_implied,     1                },
    {"ORA", kAddrMode_dxi,         2                },
    {"COP", kAddrMode_implied,     1                },
    {"ORA", kAddrMode_sr,          2                },
    {"TSB", kAddrMode_d,           2                },
    {"ORA", kAddrMode_d,           2                },
    {"ASL", kAddrMode_d,           2                },
    {"ORA", kAddrMode_dil,         2                },
    {"PHP", kAddrMode_implied,     1                },
    {"ORA", kAddrMode_immediate,   kLenIsMemorySize },
    {"ASL", kAddrMode_accumulator, 1                },
    {"PHD", kAddrMode_implied,     1                },
    {"TSB", kAddrMode_a,           3                },
    {"ORA", kAddrMode_a,           3                },
    {"ASL", kAddrMode_a,           3                },
    {"ORA", kAddrMode_al,          4                },
    {"BPL", kAddrMode_pcr,         2                },
    {"ORA", kAddrMode_dix,         2                },
    {"ORA", kAddrMode_di,          2                },
    {"ORA", kAddrMode_srix,        2                },
    {"TRB", kAddrMode_d,           2                },
    {"ORA", kAddrMode_dxx,         2                },
    {"ASL", kAddrMode_dxx,         2                },
    {"ORA", kAddrMode_dixl,        2                },
    {"CLC", kAddrMode_implied,     1                },
    {"ORA", kAddrMode_axy,         3                },
    {"INC", kAddrMode_accumulator, 1                },
    {"TCS", kAddrMode_implied,     1                },
    {"TRB", kAddrMode_a,           3                },
    {"ORA", kAddrMode_axx,         3                },
    {"ASL", kAddrMode_axx,         3                },
    {"ORA", kAddrMode_alxx,        4                },
    {"JSR", kAddrMode_a,           3                },
    {"AND", kAddrMode_dxi,         2                },
    {"JSL", kAddrMode_al,          4                },
    {"AND", kAddrMode_sr,          2                },
    {"BIT", kAddrMode_d,           2                },
    {"AND", kAddrMode_d,           2                },
    {"ROL", kAddrMode_d,           2                },
    {"AND", kAddrMode_dil,         2                },
    {"PLP", kAddrMode_implied,     1                },
    {"AND", kAddrMode_immediate,   kLenIsMemorySize },
    {"ROL", kAddrMode_accumulator, 1                },
    {"PLD", kAddrMode_implied,     1                },
    {"BIT", kAddrMode_a,           3                },
    {"AND", kAddrMode_a,           3                },
    {"ROL", kAddrMode_a,           3                },
    {"AND", kAddrMode_al,          4                },
    {"BMI", kAddrMode_pcr,         2                },
    {"AND", kAddrMode_dix,         2                },
    {"AND", kAddrMode_di,          2                },
    {"AND", kAddrMode_srix,        2                },
    {"BIT", kAddrMode_dxx,         2                },
    {"AND", kAddrMode_dxx,         2                },
    {"ROL", kAddrMode_dxx,         2                },
    {"AND", kAddrMode_dixl,        2                },
    {"SEC", kAddrMode_implied,     1                },
    {"AND", kAddrMode_axy,         3                },
    {"DEC", kAddrMode_accumulator, 1                },
    {"TSC", kAddrMode_implied,     1                },
    {"BIT", kAddrMode_axx,         3                },
    {"AND", kAddrMode_axx,         3                },
    {"ROL", kAddrMode_axx,         3                },
    {"AND", kAddrMode_alxx,        4                },
    {"RTI", kAddrMode_implied,     1                },
    {"EOR", kAddrMode_dxi,         2                },
    {"WDM", kAddrMode_immediate,   2                },
    {"EOR", kAddrMode_sr,          2                },
    {"MVP", kAddrMode_blockmove,   3                },
    {"EOR", kAddrMode_d,           2                },
    {"LSR", kAddrMode_d,           2                },
    {"EOR", kAddrMode_dil,         2                },
    {"PHA", kAddrMode_implied,     1                },
    {"EOR", kAddrMode_immediate,   kLenIsMemorySize },
    {"LSR", kAddrMode_accumulator, 1                },
    {"PHK", kAddrMode_implied,     1                },
    {"JMP", kAddrMode_a,           3                },
    {"EOR", kAddrMode_a,           3                },
    {"LSR", kAddrMode_a,           3                },
    {"EOR", kAddrMode_al,          4                },
    {"BVC", kAddrMode_pcr,         2                },
    {"EOR", kAddrMode_dix,         2                },
    {"EOR", kAddrMode_di,          2                },
    {"EOR", kAddrMode_srix,        2                },
    {"MVN", kAddrMode_blockmove,   3                },
    {"EOR", kAddrMode_dxx,         2                },
    {"LSR", kAddrMode_dxx,         2                },
    {"EOR", kAddrMode_dixl,        2                },
    {"CLI", kAddrMode_implied,     1                },
    {"EOR", kAddrMode_axy,         3                },
    {"PHY", kAddrMode_implied,     1                },
    {"TCD", kAddrMode_implied,     1                },
    {"JMP", kAddrMode_al,          4                },
    {"EOR", kAddrMode_axx,         3                },
    {"LSR", kAddrMode_axx,         3                },
    {"EOR", kAddrMode_alxx,        4                },
    {"RTS", kAddrMode_implied,     1                },
    {"ADC", kAddrMode_dxi,         2                },
    {"PER", kAddrMode_pcrl,        3                },
    {"ADC", kAddrMode_sr,          2                },
    {"STZ", kAddrMode_d,           2                },
    {"ADC", kAddrMode_d,           2                },
    {"ROR", kAddrMode_d,           2                },
    {"ADC", kAddrMode_dil,         2                },
    {"PLA", kAddrMode_implied,     1                },
    {"ADC", kAddrMode_immediate,   kLenIsMemorySize },
    {"ROR", kAddrMode_accumulator, 1                },
    {"RTL", kAddrMode_implied,     1                },
    {"JMP", kAddrMode_ai,          3                },
    {"ADC", kAddrMode_a,           3                },
    {"ROR", kAddrMode_a,           3                },
    {"ADC", kAddrMode_al,          4                },
    {"BVS", kAddrMode_pcr,         2                },
    {"ADC", kAddrMode_dix,         2                },
    {"ADC", kAddrMode_di,          2                },
    {"ADC", kAddrMode_srix,        2                },
    {"STZ", kAddrMode_dxx,         2                },
    {"ADC", kAddrMode_dxx,         2                },
    {"ROR", kAddrMode_dxx,         2                },
    {"ADC", kAddrMode_dixl,        2                },
    {"SEI", kAddrMode_implied,     1                },
    {"ADC", kAddrMode_axy,         3                },
    {"PLY", kAddrMode_implied,     1                },
    {"TDC", kAddrMode_implied,     1                },
    {"JMP", kAddrMode_axi,         3                },
    {"ADC", kAddrMode_axx,         3                },
    {"ROR", kAddrMode_axx,         3                },
    {"ADC", kAddrMode_alxx,        4                },
    {"BRA", kAddrMode_pcr,         2                },
    {"STA", kAddrMode_dxi,         2                },
    {"BRL", kAddrMode_pcrl,        3                },
    {"STA", kAddrMode_sr,          2                },
    {"STY", kAddrMode_d,           2                },
    {"STA", kAddrMode_d,           2                },
    {"STX", kAddrMode_d,           2                },
    {"STA", kAddrMode_dil,         2                },
    {"DEY", kAddrMode_implied,     1                },
    {"BIT", kAddrMode_immediate,   kLenIsMemorySize },
    {"TXA", kAddrMode_implied,     1                },
    {"PHB", kAddrMode_implied,     1                },
    {"STY", kAddrMode_a,           3                },
    {"STA", kAddrMode_a,           3                },
    {"STX", kAddrMode_a,           3                },
    {"STA", kAddrMode_al,          4                },
    {"BCC", kAddrMode_pcr,         2                },
    {"STA", kAddrMode_dix,         2                },
    {"STA", kAddrMode_di,          2                },
    {"STA", kAddrMode_srix,        2                },
    {"STY", kAddrMode_dxx,         2                },
    {"STA", kAddrMode_dxx,         2                },
    {"STX", kAddrMode_dxy,         2                },
    {"STA", kAddrMode_dixl,        2                },
    {"TYA", kAddrMode_implied,     1                },
    {"STA", kAddrMode_axy,         3                },
    {"TXS", kAddrMode_implied,     1                },
    {"TXY", kAddrMode_implied,     1                },
    {"STZ", kAddrMode_a,           3                },
    {"STA", kAddrMode_axx,         3                },
    {"STZ", kAddrMode_axx,         3                },
    {"STA", kAddrMode_alxx,        4                },
    {"LDY", kAddrMode_immediate,   kLenIsIndexSize  },
    {"LDA", kAddrMode_dxi,         2                },
    {"LDX", kAddrMode_immediate,   kLenIsIndexSize  },
    {"LDA", kAddrMode_sr,          2                },
    {"LDY", kAddrMode_d,           2                },
    {"LDA", kAddrMode_d,           2                },
    {"LDX", kAddrMode_d,           2                },
    {"LDA", kAddrMode_dil,         2                },
    {"TAY", kAddrMode_implied,     1                },
    {"LDA", kAddrMode_immediate,   kLenIsMemorySize },
    {"TAX", kAddrMode_implied,     1                },
    {"PLB", kAddrMode_implied,     1                },
    {"LDY", kAddrMode_a,           3                },
    {"LDA", kAddrMode_a,           3                },
    {"LDX", kAddrMode_a,           3                },
    {"LDA", kAddrMode_al,          4                },
    {"BCS", kAddrMode_pcr,         2                },
    {"LDA", kAddrMode_dix,         2                },
    {"LDA", kAddrMode_di,          2                },
    {"LDA", kAddrMode_srix,        2                },
    {"LDY", kAddrMode_dxx,         2                },
    {"LDA", kAddrMode_dxx,         2                },
    {"LDX", kAddrMode_dxy,         2                },
    {"LDA", kAddrMode_dixl,        2                },
    {"CLV", kAddrMode_implied,     1                },
    {"LDA", kAddrMode_axy,         3                },
    {"TSX", kAddrMode_implied,     1                },
    {"TYX", kAddrMode_implied,     1                },
    {"LDY", kAddrMode_axx,         3                },
    {"LDA", kAddrMode_axx,         3                },
    {"LDX", kAddrMode_axy,         3                },
    {"LDA", kAddrMode_alxx,        4                },
    {"CPY", kAddrMode_immediate,   kLenIsIndexSize  },
    {"CMP", kAddrMode_dxi,         2                },
    {"REP", kAddrMode_immediate,   2                },
    {"CMP", kAddrMode_sr,          2                },
    {"CPY", kAddrMode_d,           2                },
    {"CMP", kAddrMode_d,           2                },
    {"DEC", kAddrMode_d,           2                },
    {"CMP", kAddrMode_dil,         2                },
    {"INY", kAddrMode_implied,     1                },
    {"CMP", kAddrMode_immediate,   kLenIsMemorySize },
    {"DEX", kAddrMode_implied,     1                },
    {"WAI", kAddrMode_implied,     1                },
    {"CPY", kAddrMode_a,           3                },
    {"CMP", kAddrMode_a,           3                },
    {"DEC", kAddrMode_a,           3                },
    {"CMP", kAddrMode_al,          4                },
    {"BNE", kAddrMode_pcr,         2                },
    {"CMP", kAddrMode_dix,         2                },
    {"CMP", kAddrMode_di,          2                },
    {"CMP", kAddrMode_srix,        2                },
    {"PEI", kAddrMode_d,           2                },
    {"CMP", kAddrMode_dxx,         2                },
    {"DEC", kAddrMode_dxx,         2                },
    {"CMP", kAddrMode_dixl,        2                },
    {"CLD", kAddrMode_implied,     1                },
    {"CMP", kAddrMode_axy,         3                },
    {"PHX", kAddrMode_implied,     1                },
    {"STP", kAddrMode_implied,     1                },
    {"JML", kAddrMode_ail,         3                },
    {"CMP", kAddrMode_axx,         3                },
    {"DEC", kAddrMode_axx,         3                },
    {"CMP", kAddrMode_alxx,        4                },
    {"CPX", kAddrMode_immediate,   kLenIsIndexSize  },
    {"SBC", kAddrMode_dxi,         2                },
    {"SEP", kAddrMode_immediate,   2                },
    {"SBC", kAddrMode_sr,          2                },
    {"CPX", kAddrMode_d,           2                },
    {"SBC", kAddrMode_d,           2                },
    {"INC", kAddrMode_d,           2                },
    {"SBC", kAddrMode_di,          2                },
    {"INX", kAddrMode_implied,     1                },
    {"SBC", kAddrMode_immediate,   kLenIsMemorySize },
    {"NOP", kAddrMode_implied,     1                },
    {"XBA", kAddrMode_implied,     1                },
    {"CPX", kAddrMode_a,           3                },
    {"SBC", kAddrMode_a,           3                },
    {"INC", kAddrMode_a,           3                },
    {"SBC", kAddrMode_al,          4                },
    {"BEQ", kAddrMode_pcr,         2                },
    {"SBC", kAddrMode_dix,         2                },
    {"SBC", kAddrMode_di,          2                },
    {"SBC", kAddrMode_srix,        2                },
    {"PEA", kAddrMode_immediate,   3                },
    {"SBC", kAddrMode_dxx,         2                },
    {"INC", kAddrMode_dxx,         2                },
    {"SBC", kAddrMode_dixl,        2                },
    {"SED", kAddrMode_implied,     1                },
    {"SBC", kAddrMode_axy,         3                },
    {"PLX", kAddrMode_implied,     1                },
    {"XCE", kAddrMode_implied,     1                },
    {"JSR", kAddrMode_axi,         3                },
    {"SBC", kAddrMode_axx,         3                },
    {"INC", kAddrMode_axx,         3                },
    {"SBC", kAddrMode_alxx,        4                },
};

/**
 * True when the CPU is fetching an instruction
 */
bool ins_fetch = false;

bool vector_fetch = false;
uint16_t vector_addr;
uint16_t vector_data;

/**
 * The instruction being fetched
 */
struct {
    std::uint8_t pbr;
    std::uint16_t pc;

    unsigned int len;

    std::vector<unsigned int> bytes;
} cpu_instr;

static std::string renderArgument()
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

static std::string renderBytes()
{
    std::stringstream buffer;

    for (const int &b : cpu_instr.bytes) {
        buffer << format(" %02X") % b;
    }

    return buffer.str();
}

static void handleInstructionRead(const uint8_t bank, const uint16_t address, const uint8_t val)
{
    if (ins_fetch) {
        cpu_instr.bytes.push_back(val);
    }
    else {
        if (cpu_instr.bytes.size()) {
            cout << format("%02X/%04X:%-12s  %3s  %-18s |%c%c%c%c%c%c%c%c| E=%1d DBR=%02X A=%04X X=%04X Y=%04X S=%04X D=%04X\n")
                            % (int) cpu_instr.pbr
                            % cpu_instr.pc
                            % renderBytes()
                            % opcodes[cpu_instr.bytes[0]].mnemonic
                            % renderArgument()
                            % (m65816::registers::SR.N? 'n' : '-')
                            % (m65816::registers::SR.V? 'v' : '-')
                            % (m65816::registers::SR.M? 'm' : '-')
                            % (m65816::registers::SR.X? 'x' : '-')
                            % (m65816::registers::SR.D? 'd' : '-')
                            % (m65816::registers::SR.I? 'i' : '-')
                            % (m65816::registers::SR.Z? 'z' : '-')
                            % (m65816::registers::SR.C? 'c' : '-')
                            % (int) m65816::registers::SR.E
                            % (int) m65816::registers::DBR
                            % m65816::registers::A.W
                            % m65816::registers::X.W
                            % m65816::registers::Y.W
                            % m65816::registers::S.W
                            % m65816::registers::D;
        }

        cpu_instr.bytes.clear();

        cpu_instr.pbr = bank;
        cpu_instr.pc  = address;

        cpu_instr.bytes.push_back(val);

        int len = opcodes[val].len;

        if (len == kLenIsMemorySize) {
            cpu_instr.len = 2 + !m65816::registers::SR.M;
        }
        else if (len == kLenIsIndexSize) {
            cpu_instr.len = 2 + !m65816::registers::SR.X;
        }
        else {
            cpu_instr.len = len;
        }

        ins_fetch = true;
    }

    if (cpu_instr.bytes.size() == cpu_instr.len) {
        ins_fetch = false;
    }
}

static void handleVectorRead(const uint8_t bank, const uint16_t address, const uint8_t val)
{
    if (vector_fetch) {
        vector_fetch = false;
        vector_data |= (val << 8);

        //std::cerr << format("Fetch new PC %04X from vector %04X\n") % vector_data % vector_addr;
    }
    else {
        vector_fetch = true;
        vector_addr  = address;
        vector_data  = val;
    }
}

uint8_t debugRead(const uint8_t bank, const uint16_t address, const uint8_t val, const mem_access_t type)
{
    switch (type) {
        case m65816::INSTR:
            if (enable_trace) handleInstructionRead(bank, address, val);
            break;
        case m65816::VECTOR:
            handleVectorRead(bank, address, val);
            break;
        default:
             break;
    }

    return val;
}

uint8_t debugWrite(const uint8_t bank, const uint16_t address, const uint8_t val, const mem_access_t type)
{
    switch (type) {
        case m65816::STACK:
            //std::cerr << format("stack write: %02X (%04X)\n") % (unsigned int) val % address;
            break;
        default:
            break;
    }

    return val;
}
