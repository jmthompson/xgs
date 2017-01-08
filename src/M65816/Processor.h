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
#include "xgscore/System.h"

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

namespace M65816 {

template <typename MemSizeType, typename IndexSizeType, typename StackSizeType, const uint16_t StackOffset>
class LogicEngine;
class LogicEngineBase;

/**
 * The StatusRegister class implements the status register as a series
 * of direct-accessible boolean flags, plus overloads to allow constructing
 * from and casting to/from uint8_t.
 */
class StatusRegister {
    public:
        bool N = false;
        bool V = false;
        bool M = true;
        bool X = true;
        bool D = false;
        bool I = true;
        bool Z = false;
        bool C = false;

        bool E = true;

        StatusRegister() = default;
        StatusRegister(uint8_t v) { operator=(v); }
        ~StatusRegister() = default;

        StatusRegister& operator= (uint8_t v)
        {
            N = v & 0x80;
            V = v & 0x40;
            D = v & 0x08;
            I = v & 0x04;
            Z = v & 0x02;
            C = v & 0x01;

            if (!E) {
                M = v & 0x20;
                X = v & 0x10;
            }

            return *this;
        }

        StatusRegister& operator&= (uint8_t v)
        {
            if (!(v & 0x80)) N = false;
            if (!(v & 0x40)) V = false;
            if (!(v & 0x08)) D = false;
            if (!(v & 0x04)) I = false;
            if (!(v & 0x02)) Z = false;
            if (!(v & 0x01)) C = false;

            if (!E) {
                if (!(v & 0x20)) M = false;
                if (!(v & 0x10)) X = false;
            }
        }

        StatusRegister& operator|= (uint8_t v)
        {
            if (v & 0x80) N = true;
            if (v & 0x40) V = true;
            if (v & 0x08) D = true;
            if (v & 0x04) I = true;
            if (v & 0x02) Z = true;
            if (v & 0x01) C = true;

            if (!E) {
                if (v & 0x20) M = true;
                if (v & 0x10) X = true;
            }
        }

        operator uint8_t() const {
            return (N << 7)|(V << 6)|(M << 5)|(X << 4)|(D << 3)|(I << 2)|(Z << 1)|C;
        };
};

class Processor {
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
 
        LogicEngineBase *engine = nullptr;
        System *system = nullptr;

        const unsigned int *cycle_counts;

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

        // Load the contents of a vector into the PC and PBR
        inline void loadVector(uint16_t va)
        {
            PC  = system->cpuRead(0, va) | (system->cpuRead(0, va + 1) << 8);
            PBR = 0;
        }

        // Called whenever the E, M, or X bits change.
        void modeSwitch();
};

#include "LogicEngine.h"

} // namespace M65816

#endif // PROCESSOR_H
