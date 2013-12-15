#ifndef _M65816_H_
#define _M65816_H_

typedef unsigned char byte;
typedef signed char   sbyte;

#if SIZEOF_LONG == 4
typedef unsigned long word32;
typedef signed long   sword32;
#elif SIZEOF_INT == 4
typedef unsigned int word32;
typedef signed int   sword32;
#endif

#if SIZEOF_SHORT == 2
typedef unsigned short word16;
typedef signed short   sword16;
#elif SIZEOF_INT == 2
typedef unsigned int word16;
typedef signed int   sword16;
#endif

typedef signed char  offset_s;    /* short offset */
typedef signed short offset_l;    /* long offset  */

/* Union definition of a 16-bit value that can also be */
/* accessed as its component 8-bit values. Useful for  */
/* registers, which change sized based on teh settings */
/* the M and X program status register bits.           */

typedef union {
#ifdef WORDS_BIGENDIAN
    struct { byte H,L; } B;
#else
    struct { byte L,H; } B;
#endif
    word16 W;
} dualw;

/* Same as above but for addresses. */

typedef union {
#ifdef WORDS_BIGENDIAN
    struct { byte   Z,B,H,L; } B;
    struct { word16 H,L; } W;
#else
    struct { byte   L,H,B,Z; } B;
    struct { word16 L,H; } W;
#endif
    word32 A;
} duala;

/* Definitions of the 65816 registers, in case you want    */
/* to access these from your own routines (such as from    */
/* a WDM opcode handler routine.                           */

extern dualw A;    /* Accumulator               */
extern dualw D;    /* Direct Page Register      */
extern byte  P;    /* Processor Status Register */
extern int   E;    /* Emulation Mode Flag       */
extern dualw S;    /* Stack Pointer             */
extern dualw X;    /* X Index Register          */
extern dualw Y;    /* Y Index Register          */
extern byte  DB;   /* Data Bank Register        */

#ifndef M65816_DISPATCH

extern union {        /* Program Counter         */
#ifdef WORDS_BIGENDIAN
    struct { byte Z,PB,H,L; } B;
    struct { word16 Z,PC; } W;
#else
    struct { byte L,H,PB,Z; } B;
    struct { word16 PC,Z; } W;
#endif
    word32    A;
} PC;

#endif

/* This include file defines the M_READ, M_WRITE, E_UPDATE and E_WDM macros. */

#include "m65816_hw.h"

/* Initialize CPU emulation */

void m65816_init(void);

/* Set the 65816 emulator's trace mode to off (0) or on (nonzero) */

void m65816_setTrace(int mode);

/* Send a reset to the 65816 emulator. You should do this at    */
/* startup before you call m65816_run().                */

void m65816_reset(void);

/* Send an abort to the 65816 emulator. */

void m65816_abort(void);

/* Send aa non-maskable interrupt to the 65816 emulator */

void m65816_nmi(void);

/* Send a standard (maskable) interrupt to the 65816 emulator */

void m65816_addIRQ(void);

/* Clear a previously sent interrupt (if one is still pending) */

void m65816_clearIRQ(void);

/* This routine never returns; it sits in a loop processing opcodes */
/* until the emulator exits. Periodic calls to the emulator update  */
/* macro are made to allow the rest of the emulator to function.    */

int m65816_run(int);

/* Internal routine called when the mode bits (e/m/x) change */

void m65816_modeSwitch(void);

/* Internal routine to print the next instruction in trace mode */

void m65816_debug(void);

#endif /* _M65816_H_ */
