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

/*
 * File: arch/generic/snd-drv.c
 *
 * The Do-Nothing sound driver.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "sound.h"
#include "snd-drv.h"

int SND_outputInit(int rate)
{
	return 0;
}

void SND_outputShutdown(void)
{
}

size_t SND_outputWrite(snd_sample_struct *buffer, size_t len)
{
	return len;
}
