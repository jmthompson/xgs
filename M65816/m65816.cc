/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <iostream>
#include <functional>
#include <boost/format.hpp>

#include "m65816.h"
#include "platform.h"
#include "registers.h"
#include "cycle_counts.h"
#include "core.h"

namespace m65816 {

bool stopped;
bool waiting;
bool nmi_pending;
bool abort_pending;
bool irq_pending;

cycles_t total_cycles;
unsigned int num_cycles;

static const unsigned int *cycle_counts;

static void (*executeOpcode)(unsigned int);

namespace registers {
    status_reg SR;
    uint16_t D;
    uint16_t PC;
    uint8_t PBR;
    uint8_t DBR;
    multibyte16_t S;
    multibyte16_t A;
    multibyte16_t X, Y;
}

using namespace registers;

using core_emulation = core<uint8_t, uint8_t, uint8_t, 0x0100>;
using core_m1x1 = core<uint8_t, uint8_t, uint16_t, 0x0000>;
using core_m0x1 = core<uint16_t, uint8_t, uint16_t, 0x0000>;
using core_m1x0 = core<uint8_t, uint16_t, uint16_t, 0x0000>;
using core_m0x0 = core<uint16_t, uint16_t, uint16_t, 0x0000>;

template<>
core_emulation::mem_type& core_emulation::A = registers::A;
template<>
core_emulation::index_type& core_emulation::X = registers::X;
template<>
core_emulation::index_type& core_emulation::Y = registers::Y;
template<>
core_emulation::stack_type& core_emulation::S = registers::S;
template<>
core_emulation::operand_type core_emulation::operand = {0};

template<>
core_m1x1::mem_type& core_m1x1::A = registers::A;
template<>
core_m1x1::index_type& core_m1x1::X = registers::X;
template<>
core_m1x1::index_type& core_m1x1::Y = registers::Y;
template<>
core_m1x1::stack_type& core_m1x1::S = registers::S;
template<>
core_m1x1::operand_type core_m1x1::operand = {0};

template<>
core_m0x1::mem_type& core_m0x1::A = registers::A;
template<>
core_m0x1::index_type& core_m0x1::X = registers::X;
template<>
core_m0x1::index_type& core_m0x1::Y = registers::Y;
template<>
core_m0x1::stack_type& core_m0x1::S = registers::S;
template<>
core_m0x1::operand_type core_m0x1::operand = {0};

template<>
core_m1x0::mem_type& core_m1x0::A = registers::A;
template<>
core_m1x0::index_type& core_m1x0::X = registers::X;
template<>
core_m1x0::index_type& core_m1x0::Y = registers::Y;
template<>
core_m1x0::stack_type& core_m1x0::S = registers::S;
template<>
core_m1x0::operand_type core_m1x0::operand = {0};

template<>
core_m0x0::mem_type& core_m0x0::A = registers::A;
template<>
core_m0x0::index_type& core_m0x0::X = registers::X;
template<>
core_m0x0::index_type& core_m0x0::Y = registers::Y;
template<>
core_m0x0::stack_type& core_m0x0::S = registers::S;
template<>
core_m0x0::operand_type core_m0x0::operand = {0};

void modeSwitch()
{
    if (SR.E) {
        SR.M  = true;
        SR.X  = true;
        S.B.H = 0x01;
        executeOpcode = &core<uint8_t, uint8_t, uint8_t, 0x0100>::executeOpcode;
        cycle_counts = cycle_counts_e1m1x1;
    }
    else if (SR.X) { // x = 1
        X.B.H = Y.B.H = 0;

        if (SR.M) { // m=1, x=1
            executeOpcode = &core<uint8_t, uint8_t, uint16_t, 0x0000>::executeOpcode;
            cycle_counts = cycle_counts_e0m1x1;
        }
        else {      // m=0, x=1
            executeOpcode = &core<uint16_t, uint8_t, uint16_t, 0x0000>::executeOpcode;
            cycle_counts = cycle_counts_e0m0x1;
        }
    }
    else {  // x = 0
        if (SR.M) { // m=1, x=0
            executeOpcode = &core<uint8_t, uint16_t, uint16_t, 0x0000>::executeOpcode;
            cycle_counts = cycle_counts_e0m1x0;
        }
        else { // m=0, x=0
            executeOpcode = &core<uint16_t, uint16_t, uint16_t, 0x0000>::executeOpcode;
            cycle_counts = cycle_counts_e0m0x0;
        }
    }
}

void loadVector(const uint16_t va)
{
    PC  = MREAD(0, va, VECTOR) | (MREAD(0, va + 1, VECTOR) << 8);
    PBR = 0;
}

unsigned int runUntil(const unsigned int max_cycles)
{
    unsigned int opcode, cycles_done = 0;

    while (cycles_done < max_cycles) {
        if (abort_pending) {
            return 0;
        }
        else if (nmi_pending) {
            executeOpcode(0x101);
        }
        else if (irq_pending && !SR.I) {
            executeOpcode(0x100);
        }
        else if (waiting) {
            return max_cycles;
        }

        opcode = MREAD(PBR, PC++, INSTR);
        num_cycles = cycle_counts[opcode];

        executeOpcode(opcode);

        cycles_done += num_cycles;
        total_cycles += num_cycles;
    }

    return cycles_done;
}

void reset(void)
{
    stopped = waiting = false;
    abort_pending = irq_pending = nmi_pending = false;

    SR.E = true;
    SR.M = true;
    SR.X = true;
    SR.I = true;

    D = PBR = DBR = 0;

    S.W = 0x01FF;
    A.W = X.W = Y.W = 0;

    modeSwitch();
    loadVector(0xFFFC);

    total_cycles = 0;
}

void nmi() { nmi_pending = true; }

void abort() { abort_pending = true; }

void setIRQ(bool state) { irq_pending = state; }

} // namespace m65816
