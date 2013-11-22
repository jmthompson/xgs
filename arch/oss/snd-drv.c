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
 * File: arch/oss/snd-drv.c
 *
 * The Open Sound System (OSS) sound driver.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "sound.h"
#include "snd-drv.h"

/*
 * File: arch/oss/sound-output.c
 *
 * OSS-specific routines for sound output
 */

#if 0
#define OSS_DEVICE_NAME		"/usr/src/emulators/xgs/sound.out"
#define IS_FILE
#else
#define OSS_DEVICE_NAME		"/dev/dsp"
#undef IS_FILE
#endif

int	sound_fd;
byte	snd_out_buffer[OUTPUT_BUFFER_SIZE*2];

int SND_outputInit(int rate)
{
	int	parm;

	sound_fd = -1;

	if ((sound_fd = open(OSS_DEVICE_NAME, O_WRONLY, 0)) == -1) {
		close(sound_fd);
		sound_fd = -1;
		return 0;
	}
#ifdef IS_FILE
	return rate;
#else
	parm = 0x00200009;
	if (ioctl(sound_fd,SNDCTL_DSP_SETFRAGMENT,&parm) == -1) {
		close(sound_fd);
		sound_fd = -1;
		return 0;
	}
	parm = AFMT_U8;
	if ((ioctl(sound_fd,SNDCTL_DSP_SETFMT,&parm) == -1) || (parm != AFMT_U8)) {
		close(sound_fd);
		sound_fd = -1;
		return 0;
	}
	parm = 2;
	if ((ioctl(sound_fd,SNDCTL_DSP_CHANNELS,&parm) == -1) || (parm != 2)) {
		close(sound_fd);
		sound_fd = -1;
		return 0;
	}
	parm = rate;
	ioctl(sound_fd,SNDCTL_DSP_SPEED,&parm);
	return parm;
#endif
}

void SND_outputShutdown(void)
{
	close(sound_fd);
	sound_fd = -1;
}

size_t SND_outputWrite(snd_sample_struct *buffer, size_t len)
{
	audio_buf_info	sndbuff;
	int		i;

	if (sound_fd == -1) return 0;
	ioctl(sound_fd,SNDCTL_DSP_GETOSPACE,&sndbuff);
	if (sndbuff.bytes < (len*2)) return 0;
	for (i = 0 ; i < len ; i++) {
		snd_out_buffer[i*2] = (byte) ((buffer[i].left >> 10) + 128);
		snd_out_buffer[i*2+1] = (byte) ((buffer[i].right >> 10) + 128);
	}
	write(sound_fd,(char *) snd_out_buffer, len*2);
	return len;
}
