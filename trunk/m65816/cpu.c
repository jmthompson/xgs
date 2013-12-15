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

#include "config.h"
#include "m65816.h"

int	cpu_reset;
int	cpu_abort;
int	cpu_nmi;
int	cpu_irq;
int	cpu_stop;
int	cpu_wait;
int	cpu_trace;

void m65816_setTrace(int mode)
{
	cpu_trace = mode;
}

void m65816_reset(void)
{
	cpu_reset = 1;
}

void m65816_abort(void)
{
	cpu_abort = 1;
}

void m65816_nmi(void)
{
	cpu_nmi = 1;
}

void m65816_addIRQ(void)
{
	cpu_irq++;
#if 0
	printf("CPU irq count now %d\n",cpu_irq);
#endif
}

void m65816_clearIRQ(void)
{
	if (cpu_irq) cpu_irq--;
#if 0
	printf("CPU irq count now %d\n",cpu_irq);
#endif
}
