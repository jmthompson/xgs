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

#define ROM_FILE	"xgs.rom"
#define BRAM_FILE	"xgs.ram"
#define FONT40_FILE	"xgs40.fnt"
#define FONT80_FILE	"xgs80.fnt"

/* These two are needed everywhere to get vital definitions */

#include "cpu.h"
#include "memory.h"

#define VERSION		"XGS Version 0.60"

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

#endif /* _XGS_H_ */
