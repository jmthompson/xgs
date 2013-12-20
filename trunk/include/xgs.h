#ifndef _XGS_H_
#define _XGS_H_

#include "config.h"

#ifndef HAVE_RANDOM
#ifdef random
#define HAVE_RANDOM
#endif
#endif

/* Figure out how many bits the system's random number generator uses.
   `random' and `lrand48' are assumed to return 31 usable bits.
   BSD `rand' returns a 31 bit value but the low order bits are unusable;
   so we'll shift it and treat it like the 15-bit USG `rand'.  */

#ifndef RAND_BITS
# ifdef HAVE_RANDOM
#  define RAND_BITS 31
# else /* !HAVE_RANDOM */
#  ifdef HAVE_LRAND48
#   define RAND_BITS 31
#   define random lrand48
#  else /* !HAVE_LRAND48 */
#   define RAND_BITS 15
#   if RAND_MAX == 32767
#    define random rand
#   else /* RAND_MAX != 32767 */
#    if RAND_MAX == 2147483647
#     define random() (rand () >> 16)
#    else /* RAND_MAX != 2147483647 */
#     ifdef USG
#      define random rand
#     else
#      define random() (rand () >> 16)
#     endif /* !BSD */
#    endif /* RAND_MAX != 2147483647 */
#   endif /* RAND_MAX != 32767 */
#  endif /* !HAVE_LRAND48 */
# endif /* !HAVE_RANDOM */
#endif /* !RAND_BITS */

// Name of the XGS folder in DATA_DIR
#define XGS_DATA_DIR "xgs"

// Name of the XGS folder in the user's home directory
#define XGS_USER_DIR  ".xgs"

#define ROM_FILE	"xgs.rom"
#define BRAM_FILE	"xgs.ram"
#define FONT40_FILE	"xgs40.fnt"
#define FONT80_FILE	"xgs80.fnt"

/* Number of smartport devices available on the emulated smartport */

#define NUM_SMPT_DEVS	14

/* These two are needed everywhere to get vital definitions */

#include "m65816.h"
#include "memory.h"

/* Union to split a word into two bytes. Used for endianness swapping */

typedef union {
	word16	W;
	struct	{ byte B1,B2; } B;
} wswap;

/* Same as above, but splits a 32-bit word into four bytes */

typedef union {
	word32	L;
	struct	{ byte B1,B2,B3,B4; } B;
} lswap;

/*
 * Globals
 */

extern float g_fast_mhz;
extern int g_ram_size;
extern long g_cpu_cycles;
extern int g_vbl_count;
extern int g_vbl;
extern int g_vblirq_enable;
extern int g_qtrsecirq_enable;
extern int g_onesecirq_enable;
extern int g_scanirq_enable;
extern int g_fastmode;

#endif /* _XGS_H_ */
