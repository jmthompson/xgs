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
 * File: sound-oss.c
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

#if 0
#define OSS_DEVICE_NAME		"/usr/src/emulators/xgs/sound.out"
#define IS_FILE
#else
#define OSS_DEVICE_NAME		"/dev/dsp"
#undef IS_FILE
#endif

int	sound_fd;
byte *sample_buffer;

size_t SND_outputWrite(snd_sample_struct *, size_t);

void SND_update()
{
	snd_sample_struct	sample;
#if 0
	static int noisy = 0;
#endif
	if (!snd_enable) return;
	if (output_index == output_buffer_size) {
		if (!SND_outputWrite(output_buffer, output_index)) return;
		output_index = 0;
#if 0
		size_t nsamples = SND_outputWrite(output_buffer,output_index);
		if (nsamples != output_index) {
		  if (nsamples == 0) {
		    /* we're really late! */
		    /* it's better to throw out this sound chunk, then... */
		    fprintf(stderr,"sound chunk lost...\n");
		    output_index = 0;
		    return;
		  } else {
		    output_index -= nsamples;
		    memmove(output_buffer,output_buffer+nsamples,output_index*sizeof(snd_sample_struct));
		  }
		} else {
		  output_index = 0;
		}
		noisy=0;
		/* reset snd_click_sample to avoid sample overflows */
		snd_click_sample = 0;
#endif
	}
	SND_scanOscillators(&sample);
	output_buffer[output_index].left = sample.left + snd_click_sample;
	output_buffer[output_index].right = sample.right + snd_click_sample;
	output_index++;
#if 0
	if (noisy) {
	  output_index++;
	} else {
	  if (output_buffer[output_index].left != 0
	      || output_buffer[output_index].right != 0) {
	    /* some noise appeared */
	    noisy = 1;
	    output_index++;
	  }
	}
#endif
}

int SND_outputInit(int rate)
{
	int	parm;

	sound_fd = -1;

    if (!(sample_buffer = malloc(output_buffer_size * 2))) {
        sound_fd = -1;

        return 0;
    }

	if ((sound_fd = open(OSS_DEVICE_NAME, O_WRONLY, 0)) == -1) {
		close(sound_fd);
        free(sample_buffer);
		sound_fd = -1;
		return 0;
	}
#ifdef IS_FILE
	return rate;
#else
	parm = 0x00200009;
	if (ioctl(sound_fd,SNDCTL_DSP_SETFRAGMENT,&parm) == -1) {
		close(sound_fd);
        free(sample_buffer);
		sound_fd = -1;
		return 0;
	}
	parm = AFMT_U8;
	if ((ioctl(sound_fd,SNDCTL_DSP_SETFMT,&parm) == -1) || (parm != AFMT_U8)) {
		close(sound_fd);
        free(sample_buffer);
		sound_fd = -1;
		return 0;
	}
	parm = 2;
	if ((ioctl(sound_fd,SNDCTL_DSP_CHANNELS,&parm) == -1) || (parm != 2)) {
		close(sound_fd);
        free(sample_buffer);
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
    free(sample_buffer);
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

    SND_generateSamples(buffer, sample_buffer, len);

	write(sound_fd,(char *) sample_buffer, len*2);
	return len;
}
