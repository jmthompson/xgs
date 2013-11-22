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

#include "cpu.h"
#include "memory.h"
#include "emul.h"

int	cpu_reset;
int	cpu_abort;
int	cpu_nmi;
int	cpu_irq;
int	cpu_stop;
int	cpu_wait;
int	cpu_trace;

int	cpu_update_period;
#if defined( __sparc__ ) && defined( __GNUC__ )
register word32	cpu_cycle_count asm ("g5");
#else
word32	cpu_cycle_count;
#endif

void CPU_setUpdatePeriod(int period)
{
	cpu_update_period = period;
}

void CPU_setTrace(int mode)
{
	cpu_trace = mode;
}

void CPU_reset(void)
{
	cpu_reset = 1;
}

void CPU_abort(void)
{
	cpu_abort = 1;
}

void CPU_nmi(void)
{
	cpu_nmi = 1;
}

void CPU_addIRQ(void)
{
	cpu_irq++;
#if 0
	printf("CPU irq count now %d\n",cpu_irq);
#endif
}

void CPU_clearIRQ(void)
{
	if (cpu_irq) cpu_irq--;
#if 0
	printf("CPU irq count now %d\n",cpu_irq);
#endif
}
