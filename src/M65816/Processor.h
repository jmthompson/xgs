/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#ifndef PROCESSOR_H
#define PROCESSOR_H

#include "gstypes.h"
#include "StatusRegister.h"
#include "xgscore/System.h"

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

namespace M65816 {

template <typename MemSizeType, typename IndexSizeType, typename StackSizeType, const uint16_t StackOffset>
class LogicEngine;

class Debugger;

class Processor {
    friend class Debugger;
    friend class LogicEngine<uint16_t, uint16_t, uint16_t, 0>;
    friend class LogicEngine<uint16_t, uint8_t, uint16_t, 0>;
    friend class LogicEngine<uint8_t, uint16_t, uint16_t, 0>;
    friend class LogicEngine<uint8_t, uint8_t, uint16_t, 0>;
    friend class LogicEngine<uint8_t, uint8_t, uint8_t, 0x0100>;

    private:
        LogicEngine<uint16_t, uint16_t, uint16_t, 0> *engine_e0m0x0;
        LogicEngine<uint16_t, uint8_t, uint16_t, 0> *engine_e0m0x1;
        LogicEngine<uint8_t, uint16_t, uint16_t, 0> *engine_e0m1x0;
        LogicEngine<uint8_t, uint8_t, uint16_t, 0> *engine_e0m1x1;
        LogicEngine<uint8_t, uint8_t, uint8_t, 0x0100> *engine_e1m1x1;
 
        System *system = nullptr;
        Debugger *debugger = nullptr;

        bool stopped;
        bool waiting;

        bool nmi_pending;
        bool abort_pending;

        unsigned int irq_pending;

    public:

        // Status Register
        M65816::StatusRegister SR;

        // Direct Page Register
        uint16_t D;

        // Program Counter
        uint16_t PC;

        // Program Bank Register
        uint8_t PBR;

        // Data bank register
        uint8_t DBR;

        // Stack Pointer
        multibyte16_t S;

        // Accumulator
        multibyte16_t A;

        // Index Registers
        multibyte16_t X, Y;

        // Number of cycles executed
        unsigned int num_cycles;

        Processor();
        ~Processor();

        // Attach this CPU to a system
        void attach(System *);

        // Run until a certain number of cycles have been executed. Returns the number
        // of cycles actually executed, which may be slightly higher than requested.
        unsigned int runUntil(const unsigned int);

        // Reset the CPU
        void reset(void);

        // Raise a non-maskable interrupt
        void nmi() { nmi_pending = true; }

        // Raise the ABORT signal
        void abort() { abort_pending = true; }

        // Raise an interrupt
        void raiseInterrupt() { ++irq_pending; }

        // Lower an interrupt
        void lowerInterrupt() { if (irq_pending) --irq_pending; }

        void attachDebugger(M65816::Debugger *dbug) { debugger = dbug; }
        void detachDebugger() { debugger = nullptr; }

        // Load the contents of a vector into the PC and PBR
        inline void loadVector(uint16_t va)
        {
            PC  = system->cpuRead(0, va) | (system->cpuRead(0, va + 1) << 8);
            PBR = 0;
        }

        // called whenever the E, M, or X bits in SR are changed
        void modeSwitch()
        {
            if (SR.E) {
                SR.M  = true;
                SR.X  = true;
                S.B.H = 0x01;
            }

            if (SR.X) {
                X.B.H = Y.B.H = 0;
            }
        }
};

#include "LogicEngine.h"

const unsigned int cycle_counts_e0m0x0[259] = {
    8, 7, 8, 5, 7, 4, 7, 7, 3, 3, 2, 4, 8, 5, 8, 6,
    2, 7, 6, 8, 7, 5, 8, 7, 2, 6, 2, 2, 8, 6, 9, 6,
    6, 7, 8, 5, 4, 4, 7, 7, 4, 3, 2, 5, 5, 5, 8, 6,
    2, 7, 6, 8, 5, 5, 8, 7, 2, 6, 2, 2, 6, 6, 9, 6,
    7, 7, 2, 5, 0, 4, 7, 7, 3, 3, 2, 3, 3, 5, 8, 6,
    2, 7, 6, 8, 0, 5, 8, 7, 2, 6, 4, 2, 4, 6, 9, 6,
    6, 7, 6, 5, 4, 4, 7, 7, 4, 3, 2, 6, 5, 5, 8, 6,
    2, 7, 6, 8, 5, 5, 8, 7, 2, 6, 5, 2, 6, 6, 9, 6,
    2, 7, 3, 5, 4, 4, 4, 7, 2, 3, 2, 3, 5, 5, 5, 6,
    2, 7, 6, 8, 5, 5, 5, 7, 2, 6, 2, 2, 5, 6, 6, 6,
    3, 7, 3, 5, 4, 4, 4, 7, 2, 3, 2, 4, 5, 5, 5, 6,
    2, 7, 6, 8, 5, 5, 5, 7, 2, 6, 2, 2, 5, 6, 5, 6,
    3, 7, 3, 5, 4, 4, 7, 7, 2, 3, 2, 3, 5, 5, 8, 6,
    2, 7, 6, 8, 6, 5, 8, 7, 2, 6, 4, 3, 6, 6, 9, 6,
    3, 7, 3, 5, 4, 4, 7, 7, 2, 3, 2, 3, 5, 5, 8, 6,
    2, 7, 6, 8, 5, 5, 8, 7, 2, 6, 5, 2, 6, 6, 9, 6,
    8, 8, 0
};

const unsigned int cycle_counts_e0m0x1[259] = {
    8, 7, 8, 5, 7, 4, 7, 7, 3, 3, 2, 4, 8, 5, 8, 6,
    2, 6, 6, 8, 7, 5, 8, 7, 2, 5, 2, 2, 8, 5, 9, 6,
    6, 7, 8, 5, 4, 4, 7, 7, 4, 3, 2, 5, 5, 5, 8, 6,
    2, 6, 6, 8, 5, 5, 8, 7, 2, 5, 2, 2, 5, 5, 9, 6,
    7, 7, 2, 5, 0, 4, 7, 7, 4, 3, 2, 3, 3, 5, 8, 6,
    2, 6, 6, 8, 0, 5, 8, 7, 2, 5, 3, 2, 4, 5, 9, 6,
    6, 7, 6, 5, 4, 4, 7, 7, 5, 3, 2, 6, 5, 5, 8, 6,
    2, 6, 6, 8, 5, 5, 8, 7, 2, 5, 4, 2, 6, 5, 9, 6,
    2, 7, 3, 5, 3, 4, 3, 7, 2, 3, 2, 3, 4, 5, 4, 6,
    2, 6, 6, 8, 4, 5, 4, 7, 2, 5, 2, 2, 5, 5, 5, 6,
    2, 7, 2, 5, 3, 4, 3, 7, 2, 3, 2, 4, 4, 5, 4, 6,
    2, 6, 6, 8, 4, 5, 4, 7, 2, 5, 2, 2, 4, 5, 4, 6,
    2, 7, 3, 5, 3, 4, 7, 7, 2, 3, 2, 3, 4, 5, 8, 6,
    2, 6, 6, 8, 6, 5, 8, 7, 2, 5, 3, 3, 6, 5, 9, 6,
    2, 7, 3, 5, 3, 4, 7, 7, 2, 3, 2, 3, 4, 5, 8, 6,
    2, 6, 6, 8, 5, 5, 8, 7, 2, 5, 4, 2, 6, 5, 9, 6,
    8, 8, 0
};

const unsigned  int cycle_counts_e0m1x0[259] = {
    8, 6, 8, 4, 5, 3, 5, 6, 3, 2, 2, 4, 6, 4, 6, 5,
    2, 6, 5, 7, 5, 4, 6, 6, 2, 5, 2, 2, 6, 5, 7, 5,
    6, 6, 8, 4, 3, 3, 5, 6, 4, 2, 2, 5, 4, 4, 6, 5,
    2, 6, 5, 7, 4, 4, 6, 6, 2, 5, 2, 2, 5, 5, 7, 5,
    7, 6, 2, 4, 0, 3, 5, 6, 4, 2, 2, 3, 3, 4, 6, 5,
    2, 6, 5, 7, 0, 4, 6, 6, 2, 5, 4, 2, 4, 5, 7, 5,
    6, 6, 6, 4, 3, 3, 5, 6, 5, 2, 2, 6, 5, 4, 6, 5,
    2, 6, 5, 7, 4, 4, 6, 6, 2, 5, 5, 2, 6, 5, 7, 5,
    2, 6, 3, 4, 4, 3, 4, 6, 2, 2, 2, 3, 5, 4, 5, 5,
    2, 6, 5, 7, 5, 4, 5, 6, 2, 5, 2, 2, 4, 5, 5, 5,
    3, 6, 3, 4, 4, 3, 4, 6, 2, 2, 2, 4, 5, 4, 5, 5,
    2, 6, 5, 7, 5, 4, 5, 6, 2, 5, 2, 2, 5, 5, 5, 5,
    3, 6, 3, 4, 4, 3, 6, 6, 2, 2, 2, 3, 5, 4, 6, 5,
    2, 6, 5, 7, 6, 4, 8, 6, 2, 5, 4, 3, 6, 5, 7, 5,
    3, 6, 3, 4, 4, 3, 6, 6, 2, 2, 2, 3, 5, 4, 6, 5,
    2, 6, 5, 7, 5, 4, 8, 6, 2, 5, 5, 2, 6, 5, 7, 5,
    8, 8, 0
};

const unsigned  int cycle_counts_e0m1x1[259] = {
    8, 6, 8, 4, 5, 3, 5, 6, 3, 2, 2, 4, 6, 4, 6, 5,
    2, 5, 5, 7, 5, 4, 6, 6, 2, 4, 2, 2, 6, 4, 7, 5,
    6, 6, 8, 4, 3, 3, 5, 6, 4, 2, 2, 5, 4, 4, 6, 5,
    2, 5, 5, 7, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 5,
    7, 6, 2, 4, 7, 3, 5, 6, 3, 2, 2, 3, 3, 4, 6, 5,
    2, 5, 5, 7, 7, 4, 6, 6, 2, 4, 3, 2, 4, 4, 7, 5,
    6, 6, 6, 4, 3, 3, 5, 6, 4, 2, 2, 6, 5, 4, 6, 5,
    2, 5, 5, 7, 4, 4, 6, 6, 2, 4, 4, 2, 6, 4, 7, 5,
    2, 6, 3, 4, 3, 3, 3, 6, 2, 2, 2, 3, 4, 4, 4, 5,
    2, 6, 5, 7, 4, 4, 4, 6, 2, 5, 2, 2, 4, 5, 5, 5,
    2, 6, 2, 4, 3, 3, 3, 6, 2, 2, 2, 4, 4, 4, 4, 5,
    2, 5, 5, 7, 4, 4, 4, 6, 2, 4, 2, 2, 4, 4, 4, 5,
    2, 6, 3, 4, 3, 3, 5, 6, 2, 2, 2, 3, 4, 4, 4, 5,
    2, 5, 5, 7, 6, 4, 6, 6, 2, 4, 3, 3, 6, 4, 7, 5,
    2, 6, 3, 4, 3, 3, 5, 6, 2, 2, 2, 3, 4, 4, 6, 5,
    2, 5, 5, 7, 5, 4, 6, 6, 2, 4, 4, 2, 6, 4, 7, 5,
    8, 8, 0
};

const unsigned int cycle_counts_e1m1x1[259] = {
    8, 6, 8, 4, 5, 3, 5, 6, 3, 2, 2, 4, 6, 4, 6, 5,
    2, 5, 5, 7, 5, 4, 6, 6, 2, 4, 2, 2, 6, 4, 7, 5,
    6, 6, 8, 4, 3, 3, 5, 6, 4, 2, 2, 5, 4, 4, 6, 5,
    2, 5, 5, 7, 4, 4, 6, 6, 2, 4, 2, 2, 4, 4, 7, 5,
    6, 6, 2, 4, 0, 3, 5, 6, 3, 2, 2, 3, 3, 4, 6, 5,
    2, 5, 5, 7, 0, 4, 6, 6, 2, 4, 3, 2, 4, 4, 7, 5,
    6, 6, 6, 4, 3, 3, 5, 6, 4, 2, 2, 6, 5, 4, 6, 5,
    2, 5, 5, 7, 4, 4, 6, 6, 2, 4, 4, 2, 6, 4, 7, 5,
    2, 6, 3, 4, 3, 3, 3, 6, 2, 2, 2, 3, 4, 4, 4, 5,
    2, 5, 5, 7, 4, 4, 4, 6, 2, 4, 2, 2, 4, 4, 4, 5,
    2, 6, 2, 4, 3, 3, 3, 6, 2, 2, 2, 4, 4, 4, 4, 5,
    2, 5, 5, 7, 4, 4, 4, 6, 2, 4, 2, 2, 4, 4, 4, 5,
    2, 6, 3, 4, 3, 3, 5, 6, 2, 2, 2, 3, 4, 4, 6, 5,
    2, 5, 5, 7, 6, 4, 6, 6, 2, 4, 3, 3, 6, 4, 7, 5,
    2, 6, 3, 4, 3, 3, 5, 6, 2, 2, 2, 3, 4, 4, 6, 5,
    2, 5, 5, 7, 5, 4, 6, 6, 2, 4, 4, 2, 6, 4, 7, 5,
    7, 7, 0
};

} // namespace M65816

#endif // PROCESSOR_H
