#ifndef M65816_H_
#define M65816_H_

#include "config.h"
#include "SystemBoard.h"

class M65816 {
    private:
        bool stopped;
        bool waiting;
        bool trace;

        bool nmi_pending;
        bool irq_pending;
        bool abort_pending;

        duala PC;
        dualw A;
        dualw D;
        dualw S;
        dualw X;
        dualw Y;

        byte  DB;

        struct {
            bool N;
            bool V;
            bool M;
            bool X;
            bool D;
            bool I;
            bool Z;
            bool C;
            bool E;
        } P;

        duala   address;
        dualw   operand;
        int     opcode;

        SystemBoard *board;

        int executeOpcode_e0m0x0();
        int executeOpcode_e0m0x1();
        int executeOpcode_e0m1x0();
        int executeOpcode_e0m1x1();
        int executeOpcode_e1m1x1();

        void modeSwitch();

        void bcdToNibbles(word16, int *, int);
        word32 decimalAdd(word16, word16);
        word32 decimalSubtract(word16, word16);

#include "cycle_counts.inc"

    public:
        M65816(SystemBoard *);
        ~M65816();

        // Run until a certain number of cycles have been executed. Returns the number
        // of cycles actually executed, which may be slightly higher than requested.
        int runUntil(int cycles);

        // Execute the next instruction and return the number of cycles it took.
        int nextInstruction();

        // Reset the CPU
        void reset(void);

        // Raise a non-maskable interrupt
        void nmi() { nmi_pending = 1; }

        // Raise an interrupt
        void irq() { irq_pending = 1; }

        // Raise the ABORT signal
        void abort() { abort_pending = 1; }

        // Disable trace mode
        void disableTrace() { trace = 0; }

        // Enable trace mode
        void enableTrace() { trace = 1; }
};

#endif // M65816_H_
