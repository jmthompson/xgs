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
 * File: sound-pulseaudio.c
 *
 * The PulseAudio sound driver.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <string.h>
#include <pulse/simple.h>
#include "sound.h"

pa_simple *pa;
static pa_sample_spec sample_spec;
byte *sample_buffer;
/*
static pa_context *context = NULL;
static pa_stream *stream = NULL;
static pa_threaded_mainloop *mainloop;
static pa_mainloop_api *mainloop_api = NULL;
*/

char *server = NULL;
char *device = NULL;

size_t SND_outputWrite(snd_sample_struct *, size_t);

void SND_update()
{
    snd_sample_struct sample;

    //pa_mainloop_iterate(mainloop, 0, NULL);

    if (output_index == OUTPUT_BUFFER_SIZE) {
        if (!SND_outputWrite(output_buffer, output_index)) return;

        output_index = 0;
    }
    SND_scanOscillators(&sample);

    if (snd_enable) {
        output_buffer[output_index].left = sample.left + snd_click_sample;
        output_buffer[output_index].right = sample.right + snd_click_sample;

        output_index++;
    }
}

int SND_outputInit(int rate)
{
    int    parm;
    pa_buffer_attr ba;

    sample_buffer = malloc(output_buffer_size * 2);

    sample_spec.format   = PA_SAMPLE_U8;
    sample_spec.channels = 2;
    sample_spec.rate     = rate;

    ba.maxlength = output_buffer_size * 4; // double size buffer for two 8-bit channels
    ba.minreq    = output_buffer_size * 2;
    ba.tlength   = output_buffer_size * 2;
    ba.prebuf    = 0;

    pa = pa_simple_new(
        server,
        "XGS",
        PA_STREAM_PLAYBACK,
        device,
        "Audio",
        &sample_spec,
        NULL,   // default channel map
        &ba,
        NULL
    );

    if (!pa) {
        return 0;
    }

    return sample_spec.rate;
}

void SND_outputShutdown(void)
{
    pa_simple_free(pa);
}

size_t SND_outputWrite(snd_sample_struct *samples, size_t nsamples)
{
    SND_generateSamples(samples, sample_buffer, nsamples);

    pa_simple_write(pa, sample_buffer, nsamples * 2, NULL);

    return nsamples * 2;
}
