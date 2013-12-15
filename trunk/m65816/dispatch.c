/*********************************************************************
 *                                                                   *
 *                M65816: Portable 65816 CPU Emulator                *
 *                                                                   *
 *        Written and Copyright (C)1996 by Joshua M. Thompson        *
 *                                                                   *
 *  You are free to distribute this code for non-commercial purposes *
 * I ask only that you notify me of any changes you make to the code *
 *     Commercial use is prohibited without my written permission    *
 *                                                                   *
 *********************************************************************/

#define M65816_DISPATCH

#include "config.h"
#include "m65816.h"
#include "cpumicro.h"

dualw A;    /* Accumulator               */
dualw D;    /* Direct Page Register      */
byte  P;    /* Processor Status Register */
int   E;    /* Emulation Mode Flag       */
dualw S;    /* Stack Pointer             */
dualw X;    /* X Index Register          */
dualw Y;    /* Y Index Register          */
byte  DB;   /* Data Bank Register        */

union {
#ifdef WORDS_BIGENDIAN
    struct { byte Z,PB,H,L; } B;
    struct { word16 Z,PC; } W;
#else
    struct { byte L,H,PB,Z; } B;
    struct { word16 PC,Z; } W;
#endif
    word32    A;
} PC;

duala        atmp,opaddr;
dualw        wtmp,otmp,operand;
int        a1,a2,a3,a4,o1,o2,o3,o4;

int (**cpu_curr_opcode_table)();

static int cycle_count;

extern int    cpu_reset,cpu_abort,cpu_nmi,cpu_irq,cpu_stop,cpu_wait,cpu_trace;

extern void    (*cpu_opcode_table[1300])();

void m65816_init(void) {
    E = 1;
    F_setM(1);
    F_setX(1);
    m65816_modeSwitch();
}

int m65816_run(int max_cycles)
{
    int num_cycles,opcode;

    num_cycles = 0;

dispatch:
#ifdef DEBUG
    if (cpu_trace) goto debug;
debug_resume:
#endif
    if (cpu_reset) goto reset;
    if (cpu_stop) goto dispatch;
    if (cpu_abort) goto abort;
    if (cpu_nmi) goto nmi;
    if (cpu_irq) goto irq;
irq_return:
    if (cpu_wait) goto dispatch;
    opcode = M_READ(PC.A);
    PC.W.PC++;

    num_cycles += (**cpu_curr_opcode_table[opcode])();

    if (num_cycles < max_cycles) goto dispatch;

/* Special cases. Since these don't happen a lot more often than they    */
/* do happen, accessing them this way means most of the time the    */
/* generated code is _not_ branching. Only during the special cases do    */
/* we take the branch penalty (if there is one).            */

exit:
    return num_cycles;
#ifdef DEBUG
debug:
    m65816_debug();
    goto debug_resume;
#endif
reset:
    (**cpu_curr_opcode_table[256])();
    goto dispatch;
abort:
    (**cpu_curr_opcode_table[257])();
    goto dispatch;
nmi:
    (**cpu_curr_opcode_table[258])();
    goto dispatch;
irq:
    if (P & 0x04) goto irq_return;
    (**cpu_curr_opcode_table[259])();
    goto dispatch;
}

/* Recalculate opcode_offset based on the new processor mode */

void m65816_modeSwitch(void) {

    int opcode_offset;

    if (E) {
        opcode_offset = 1040;
    } else {
        if (F_getX) {
            X.B.H = 0;
            Y.B.H = 0;
        }
        opcode_offset = ((~P >> 4) & 0x03) * 260;
    }

    cpu_curr_opcode_table = cpu_opcode_table + opcode_offset;
}
