/*********************************************************************
 *                                                                   *
 *                     XGS : Apple IIGS Emulator                     *
 *                                                                   *
 *        Written and Copyright (C)1996 by Joshua M. Thompson        *
 *                                                                   *
 *  You are free to distribute this code for non-commercial purposes *
 * I ask only that you notify me of any changes you make to the code *
 *     Commercial use is prohibited without my written permission    *
 *                                                                   *
 *********************************************************************/

#define CLOCK_DEBUG(x)

/*
 * File: clock.c
 *
 * Emulates the clock registers at $C033 and $C034 (which also has the
 * screen border color as well).
 *
 * This is a partial implementation of the clock chip. We don't
 * support setting the time registers, and we don't support accessing 
 * the BRAM through the old single-byte commands (since the IIGS
 * doesn't seem to use those commands anyway).
 */

#include "xgs.h"

#include <stdio.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include "clock.h"
#include "emul.h"
#include "video.h"

/* The clock chip data and control registers */

byte	clk_data_reg;
byte	clk_ctl_reg;

byte	bram[256];

union {
	byte		B[4];
	time_t		L;
} clk_curr_time;

/* Clock state. 0 = waiting for command, 1 = waiting for second	*/
/* byte of two-part command, 2 = waiting for data real/write	*/
/* Bit 7 is set for read and clear for write. Bit 6 is set if	*/
/* BRAM is being accessed, or clear if clock register.		*/

int	clk_state;

/* Clock register/BRAM Location  being accessed. */

int	clk_addr;

int CLK_init()
{
    int fd;

	printf("\nInitialzing clock and battery RAM\n");
	printf("    - Loading battery RAM: ");

    fd = openUserFile(BRAM_FILE, O_RDONLY);

    if (fd < 0) {
		printf("Failed\n");
	}
    else {
		if (read(fd, bram, 256) != 256) {
			printf("Failed\n");
		} else {
			printf("Done\n");
		}
		close(fd);
	}

	return 0;
}

void CLK_update()
{
}

void CLK_reset()
{
	clk_data_reg = 0;
	clk_ctl_reg &= 0x3F;
	clk_state = 0;
}

void CLK_shutdown()
{
    int fd;

	printf("\nShutting down the clock and battery RAM\n");
	printf("    - Saving battery RAM: ");

    fd = openUserFile(BRAM_FILE, O_CREAT|O_WRONLY);

    if (fd < 0) {
		printf("Failed\n");

		return;
	}

	if (write(fd, bram, 256) != 256) {
		printf("Failed\n");
	}
    else {
        printf("Done\n");
    }

    close(fd);
}

byte CLK_getData(byte val)
{
	CLOCK_DEBUG(printf("gD:             clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	return clk_data_reg;
}

byte CLK_setData(byte val)
{
	CLOCK_DEBUG(printf("sD:    val %X    clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	clk_data_reg = val;
	CLOCK_DEBUG(printf("sD:              clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	return 0;
}

byte CLK_getControl(byte val)
{
	byte ret_val;
	CLOCK_DEBUG(printf("gC    ret_val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)ret_val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	ret_val=(clk_ctl_reg | VID_getBorder(0));
	CLOCK_DEBUG(printf("gC    ret_val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)ret_val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	return(ret_val);
}

byte CLK_setControl(byte val)
{
	VID_setBorder((byte) (val & 0x0F));
	clk_ctl_reg = val & 0xF0;

	/* need to make sure there is a data transfer going on if not then just
	   zero the data register. */
	if (!(clk_ctl_reg & 0x20))
		clk_data_reg = 0;

	switch (clk_state & 0x0F) {
		case 0 : if ((clk_data_reg & 0x78) == 0x38) {
					CLOCK_DEBUG(printf("sC 0: 1st:val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
						(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
					clk_state = (clk_data_reg & 0x80) | 0x41;
					clk_addr = (clk_data_reg & 0x07) << 5;
				} else if ((clk_data_reg & 0x73) == 0x01) {
					CLOCK_DEBUG(printf("sC 0: 2cd:val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
						(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
					clk_state = (clk_data_reg & 0x80) | 0x02;
					clk_addr = (clk_data_reg >> 2) & 0x03;
				}
					CLOCK_DEBUG(printf("sC 0: 3rd:val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
						(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
				break;
		case 1 :
				CLOCK_DEBUG(printf("sC 1:     val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
					(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
				clk_addr |= ((clk_data_reg >> 2) & 0x1F);
				clk_state++;
				break;
		case 2 : if (clk_state & 0x40) {
					if (clk_state & 0x80) {
					CLOCK_DEBUG(printf("sC 2: Ar  val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
						(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
						clk_data_reg = bram[clk_addr];
					} else {
					CLOCK_DEBUG(printf("sC 2: Aw  val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
						(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
						bram[clk_addr] = clk_data_reg;
					}
				} else {
					CLOCK_DEBUG(printf("sC 2: B   val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
						(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
					if (clk_state & 0x80) {
						if (!clk_addr) clk_curr_time.L = time(NULL) + CLK_OFFSET;
#ifdef WORDS_BIGENDIAN
						clk_data_reg = clk_curr_time.B[3-clk_addr];
#else
						clk_data_reg = clk_curr_time.B[clk_addr];
#endif
					} else {
						/* Can't set system time */
					}
				}
				clk_state = 0;
				break;
		default :	clk_state = 0;
				CLOCK_DEBUG(printf("sC 9:     val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
					(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
				break;
	}
	CLOCK_DEBUG(printf("sC  : *1  val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	clk_ctl_reg &= 0x7F;	/* Clear transaction bit since we're done */
	CLOCK_DEBUG(printf("sC  : *2  val %X clk_state %X clk_addr of %X clk_data_reg %X clk_ctl_reg %X.\n",
		(int)val,(int)clk_state,(int)clk_addr,(int)clk_data_reg,(int)clk_ctl_reg));
	return 0;
}
