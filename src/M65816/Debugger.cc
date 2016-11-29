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

#include "Debugger.h"
#include "Opcode.h"
#include "Processor.h"

using std::int32_t;

namespace M65816 {

Instruction Debugger::decodeInstruction(uint8_t bank, uint16_t address)
{
    Instruction ins;

    ins.bank = bank;
    ins.address = address;
    ins.bytes.push_back(cpu->board->readMemory(bank, address));
    ins.opcode = &opcodes[ins.bytes[0]];

    int len = ins.opcode->getLength();

    if (len == kLenIsMemorySize) {
        len = 2 + !cpu->SR.M;
    }
    else if (len == kLenIsIndexSize) {
        len = 2 + !cpu->SR.X;
    }

    for (int i = 1 ; i < len ; i++) {
        ins.bytes.push_back(cpu->board->readMemory(bank, address + i));
    }

    return ins;
}

std::string Debugger::renderArgument(const Instruction& ins)
{
    std::stringstream buffer;
    int i;

    // output integers as 2-digit hex
    buffer << std::hex << std::uppercase << std::setw(2) << std::setfill('0');

    switch (ins.opcode->getMode()) {
        case kAddrMode_immediate:
            buffer << "#$";

            for (i = ins.bytes.size() - 1 ; i > 0 ; i++) {
                buffer << ins.bytes[i];
            }

            break;

        case kAddrMode_a:
            buffer << "$" << ins.bytes[2] << ins.bytes[1];
            break;

        case kAddrMode_al:
            buffer << "$" << ins.bytes[3] << ins.bytes[2] << ins.bytes[1];
            break;

        case kAddrMode_d:
            buffer << "$" << ins.bytes[1];
            break;

        case kAddrMode_accumulator:
            buffer << "A";
            break;

        case kAddrMode_implied:
            buffer << "i";
            break;

        case kAddrMode_dix:
            buffer << "($" << ins.bytes[1] << "),y";
            break;

        case kAddrMode_dixl:
            buffer << "[$" << ins.bytes[1] << "],y";
            break;

        case kAddrMode_dxi:
            buffer << "($" << ins.bytes[1] << ",X)";
            break;

        case kAddrMode_dxx:
            buffer << "$" << ins.bytes[1] << ",X";
            break;

        case kAddrMode_dxy:
            buffer << "$" << ins.bytes[1] << ",Y";
            break;

        case kAddrMode_axx:
            buffer << "$" << ins.bytes[2] << ins.bytes[1] << ",X";
            break;

        case kAddrMode_alxx:
            buffer << "$" << ins.bytes[3] << ins.bytes[2] << ins.bytes[1] << ",X";
            break;

        case kAddrMode_axy:
            buffer << "$" << ins.bytes[2] << ins.bytes[1] << ",Y";
            break;

        case kAddrMode_pcr:
            {
                int8_t offset = ins.bytes[1];
                uint16_t dst  = ins.address + offset;

                buffer << "$" << ins.bank << "/" << (dst >> 8) << (dst & 0xFF);
            }
            break;

        case kAddrMode_pcrl:
            {
                int16_t offset = (ins.bytes[2] << 8) | ins.bytes[1];
                uint16_t dst   = ins.address + offset;

                buffer << "$" << ins.bank << "/" << (dst >> 8) << (dst & 0xFF);
            }
            break;

        case kAddrMode_ai:
        case kAddrMode_ail:
            buffer << "($" << ins.bytes[2] << ins.bytes[1] << ")";
            break;

        case kAddrMode_di:
            buffer << "($" << ins.bytes[1] << ")";
            break;

        case kAddrMode_dil:
            buffer << "[$" << ins.bytes[1] << "]";
            break;

        case kAddrMode_axi:
            buffer << "($" << ins.bytes[2] << ins.bytes[1] << ",X)";
            break;

        case kAddrMode_stack:
            buffer << "s";
            break;

        case kAddrMode_sr:
            buffer << "$" << ins.bytes[1] << ",S";
            break;

        case kAddrMode_srix:
            buffer << "($" << ins.bytes[1] << ",S),Y";
            break;

        case kAddrMode_blockmove:
            buffer << "$" << ins.bytes[1] << ",$" << ins.bytes[2];
            break;
    }

    return buffer.str();
}

void Debugger::executeHook()
{
    Instruction ins = decodeInstruction(cpu->PBR, cpu->PC);

    std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0');
    std::cout << "$" << cpu->PBR << "/" << (cpu->PC >> 8) << (cpu->PC & 0xFF) << "    ";
    std::cout << ins.opcode->getName() << "  ";
    std::cout << std::setw(16) << renderArgument(ins) << "    ";

    if (cpu->SR.E) {
        std::cout << "[N]";
    }
    else {
        std::cout << "[E]";
    }

    std::cout << " P=" << static_cast<int> (cpu->SR);
    std::cout << " DBR=" << cpu->DBR;
    std::cout << std::setw(4);
    std::cout << " A=" << cpu->A.W;
    std::cout << " X=" << cpu->X.W;
    std::cout << " Y=" << cpu->Y.W;
    std::cout << " S=" << cpu->S.W;
    std::cout << " D=" << cpu->D;

    std::cout << std::endl;
}

} // namespace M65816
