/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "Processor.h"
#include "Debugger.h"
#include "SystemBoard/SystemBoard.h"

namespace M65816 {

Processor::Processor(SystemBoard *sb)
{
    board = sb;

    engine_e0m0x0 = new LogicEngine<uint16_t, uint16_t, uint16_t, 0>(this);
    engine_e0m0x1 = new LogicEngine<uint16_t, uint8_t, uint16_t, 0>(this);
    engine_e0m1x0 = new LogicEngine<uint8_t, uint16_t, uint16_t, 0>(this);
    engine_e0m1x1 = new LogicEngine<uint8_t, uint8_t, uint16_t, 0>(this);
    engine_e1m1x1 = new LogicEngine<uint8_t, uint8_t, uint8_t, 0x0100>(this);
}

unsigned int Processor::runUntil(const unsigned int max_cycles)
{
    unsigned int opcode;

    num_cycles = 0;

    while (num_cycles < max_cycles) {
        if (abort_pending) {
            opcode = 0x102;
        }
        else if (nmi_pending) {
            opcode = 0x101;
        }
        else if (irq_pending) {
            opcode = 0x100;
        }
        else if (waiting) {
            return max_cycles;
        }
        else {
            opcode = board->readMemory(PBR, PC);

            if (debugger) debugger->executeHook();

            ++PC;
        }

        if (SR.E) {
            num_cycles += cycle_counts_e1m1x1[opcode];

            engine_e1m1x1->executeOpcode(opcode);
        }
        else if (SR.M) {
            if (SR.X) {
                num_cycles += cycle_counts_e0m1x1[opcode];

                engine_e0m1x1->executeOpcode(opcode);
            }
            else {
                num_cycles += cycle_counts_e0m1x0[opcode];

                engine_e0m1x0->executeOpcode(opcode);
            }
        }
        else {
            if (SR.X) {
                num_cycles += cycle_counts_e0m0x1[opcode];

                engine_e0m0x1->executeOpcode(opcode);
            }
            else {
                num_cycles += cycle_counts_e0m0x0[opcode];

                engine_e0m0x0->executeOpcode(opcode);
            }
        }
    }
}

void Processor::reset(void)
{
    stopped = false;
    waiting = false;

    nmi_pending   = false;
    irq_pending   = false;
    abort_pending = false;

    SR.E = true;
    SR.M = true;
    SR.X = true;
    SR.I = true;

    D = PBR = DBR = 0;

    S.W = 0x01FF;
    A.W = X.W = Y.W = 0;

    loadVector(0xFFFC);
}

} // namespace M65816
