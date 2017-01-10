/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <iostream>
#include <boost/format.hpp>

#include "Processor.h"
#include "cycle_counts.h"

namespace M65816 {

Processor::Processor()
{
}

Processor::~Processor()
{
}

void Processor::attach(System *theSystem)
{
    system = theSystem;

    engine_e0m0x0 = new LogicEngine<uint16_t, uint16_t, uint16_t, 0>(this);
    engine_e0m0x1 = new LogicEngine<uint16_t, uint8_t, uint16_t, 0>(this);
    engine_e0m1x0 = new LogicEngine<uint8_t, uint16_t, uint16_t, 0>(this);
    engine_e0m1x1 = new LogicEngine<uint8_t, uint8_t, uint16_t, 0>(this);
    engine_e1m1x1 = new LogicEngine<uint8_t, uint8_t, uint8_t, 0x0100>(this);
}

void Processor::modeSwitch()
{
    if (SR.E) {
        SR.M  = true;
        SR.X  = true;
        S.B.H = 0x01;

        engine = engine_e1m1x1;
        cycle_counts = cycle_counts_e1m1x1;
    }
    else {
        if (SR.X) { // x = 1
            X.B.H = Y.B.H = 0;

            if (SR.M) { // m=1, x=1
                engine = engine_e0m1x1;
                cycle_counts = cycle_counts_e0m1x1;
            }
            else {      // m=0, x=1
                engine = engine_e0m0x1;
                cycle_counts = cycle_counts_e0m0x1;
            }
        }
        else {  // x = 0
            if (SR.M) { // m=1, x=0
                engine = engine_e0m1x0;
                cycle_counts = cycle_counts_e0m1x0;
            }
            else { // m=0, x=0
                engine = engine_e0m0x0;
                cycle_counts = cycle_counts_e0m0x0;
            }
        }
    }
}

unsigned int Processor::runUntil(const unsigned int max_cycles)
{
    unsigned int opcode;

    num_cycles = 0;

    while (num_cycles < max_cycles) {
        if (abort_pending) {
            engine->executeOpcode(0x102);

            return 0;
        }
        else if (nmi_pending) {
            engine->executeOpcode(0x101);

            return 0;
        }
        else if (irq_pending) {
            engine->executeOpcode(0x100);

            return 0;
        }
        else if (waiting) {
            return max_cycles;
        }

        unsigned int opcode     = system->cpuRead(PBR, PC, INSTR);
        unsigned int new_cycles = cycle_counts[opcode];

        // We must never go over our max cycle count
        if ((num_cycles + new_cycles) > max_cycles) break;

        ++PC;

        num_cycles += new_cycles;

        engine->executeOpcode(opcode);
    }

    return num_cycles;
}

void Processor::reset(void)
{
    stopped = false;
    waiting = false;

    nmi_pending   = false;
    abort_pending = false;
    irq_pending   = 0;

    SR.E = true;
    SR.M = true;
    SR.X = true;
    SR.I = true;

    D = PBR = DBR = 0;

    S.W = 0x01FF;
    A.W = X.W = Y.W = 0;

    modeSwitch();
    loadVector(0xFFFC);
}

} // namespace M65816
