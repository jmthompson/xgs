/*
 * This file is included by m65816.h. It sets up the precompiler
 * macros that m65816 uses to access the emulated hardware.
 */

#include "memory.h"

/* These are the core memory access macros used in the 65816 emulator. */
/* Set these to point to routines which handle your emulated machine's */
/* memory access (generally these routines will check for access to    */
/* memory-mapped I/O and things of that nature.)                       */

#define M_READ(a)	MEM_readMem(a)
#define M_WRITE(a,v)	MEM_writeMem(a,v)

/* Set this macro to your emulator's routine for handling the WDM	*/
/* pseudo-opcode. Useful for trapping certain emulated machine		*/
/* functions and emulating them in fast C code.				*/

#define E_WDM(v)	hardwareHandleWDM(v)