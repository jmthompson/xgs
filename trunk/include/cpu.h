#ifndef _GSCPU_H_
#define _GSCPU_H_

typedef unsigned char	byte;
typedef signed char	sbyte;

#if SIZEOF_LONG == 4
typedef unsigned long	word32;
typedef signed long	sword32;
#elif SIZEOF_INT == 4
typedef unsigned int	word32;
typedef signed int	sword32;
#endif

#if SIZEOF_SHORT == 2
typedef unsigned short	word16;
typedef signed short	sword16;
#elif SIZEOF_INT == 2
typedef unsigned int	word16;
typedef signed int	sword16;
#endif

typedef signed char	offset_s;	/* short offset           */
typedef signed short	offset_l;	/* long offset            */

/* Union definition of a 16-bit value that can also be */
/* accessed as its component 8-bit values. Useful for  */
/* registers, which change sized based on teh settings */
/* the M and X program status register bits.           */

typedef union {
#ifdef WORDS_BIGENDIAN
	struct { byte	H,L; } B;
#else
	struct { byte	L,H; } B;
#endif
	word16	W;
} dualw;

/* Same as above but for addresses. */

typedef union {
#ifdef WORDS_BIGENDIAN
	struct { byte	Z,B,H,L; } B;
	struct { word16	H,L; } W;
#else
	struct { byte	L,H,B,Z; } B;
	struct { word16	L,H; } W;
#endif
	word32	A;
} duala;

/* Definitions of the 65816 registers, in case you want	*/
/* to access these from your own routines (such as from	*/
/* a WDM opcode handler routine.			*/

extern dualw	A;	/* Accumulator		     */
extern dualw	D;	/* Direct Page Register      */
extern byte	P;	/* Processor Status Register */
extern int	E;	/* Emulation Mode Flag       */
extern dualw	S;	/* Stack Pointer             */
extern dualw	X;	/* X Index Register          */
extern dualw	Y;	/* Y Index Register          */
extern byte	DB;	/* Data Bank Register        */

/* Current cycle count */

#if defined ( __sparc__ ) && defined ( __GNUC__ )
register word32	cpu_cycle_count asm ("g5");
#else
extern word32	cpu_cycle_count;
#endif

#ifndef CPU_DISPATCH

extern union {		/* Program Counter	     */
#ifdef WORDS_BIGENDIAN
	struct { byte Z,PB,H,L; } B;
	struct { word16 Z,PC; } W;
#else
	struct { byte L,H,PB,Z; } B;
	struct { word16 PC,Z; } W;
#endif
	word32	A;
} PC;

#endif

/* These are the core memory access macros used in the 65816 emulator. */
/* Set these to point to routines which handle your emulated machine's */
/* memory access (generally these routines will check for access to    */
/* memory-mapped I/O and things of that nature.)                       */

#define M_READ(a)	MEM_readMem(a)
#define M_WRITE(a,v)	MEM_writeMem(a,v)

/* Set this macro to your emulator's "update" routine. Your update */
/* routine would probably do things like update hardware sprites,  */
/* and check for user keypresses. CPU_run() calls this routine     */
/* periodically to make sure the rest of your emulator gets time   */
/* to run.                                                         */

#define E_UPDATE(v)	EMUL_hardwareUpdate(v)

/* Set this macro to your emulator's routine for handling the WDM	*/
/* pseudo-opcode. Useful for trapping certain emulated machine		*/
/* functions and emulating them in fast C code.				*/

#define E_WDM(v)	EMUL_handleWDM(v)

/* Set the 65816 emulator's update period (the number of instructions	*/
/* executed between calls to the E_UPDATE routine.)			*/

void CPU_setUpdatePeriod(int period);

/* Set the 65816 emulator's trace mode to off (0) or on (nonzero) */

void CPU_setTrace(int mode);

/* Send a reset to the 65816 emulator. You should do this at	*/
/* startup before you call CPU_run().				*/

void CPU_reset(void);

/* Send an abort to the 65816 emulator. */

void CPU_abort(void);

/* Send aa non-maskable interrupt to the 65816 emulator */

void CPU_nmi(void);

/* Send a standard (maskable) interrupt to the 65816 emulator */

void CPU_addIRQ(void);

/* Clear a previously sent interrupt (if one is still pending) */

void CPU_clearIRQ(void);

/* This routine never returns; it sits in a loop processing opcodes */
/* until the emulator exits. Periodic calls to the emulator update  */
/* macro are made to allow the rest of the emulator to function.    */

void CPU_run(void);

/* Internal routine called when the mode bits (e/m/x) change */

void CPU_modeSwitch(void);

/* Internal routine to print the next instruction in trace mode */

void CPU_debug(void);

#endif /* _GSCPU_H_ */
