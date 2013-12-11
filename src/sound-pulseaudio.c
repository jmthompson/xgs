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
#include <pulse/pulseaudio.h>
#include "sound.h"

static pa_context *context = NULL;
static pa_stream *stream = NULL;
static pa_threaded_mainloop *mainloop;
static pa_mainloop_api *mainloop_api = NULL;
static pa_sample_spec sample_spec;

char *server = NULL;
char *device = NULL;

byte *sample_buffer;

size_t SND_outputWrite(snd_sample_struct *, size_t);

void SND_update()
{
	snd_sample_struct sample;

    //pa_mainloop_iterate(mainloop, 0, NULL);

	if (output_index == OUTPUT_BUFFER_SIZE) {
		if (!SND_outputWrite(output_buffer, output_index)) return;

		output_index = 0;
/*
        output_index = 0;

		snd_click_sample = 0;

        fprintf(stderr,"sound chunk lost...\n");
*/
	}
	SND_scanOscillators(&sample);

	if (snd_enable) {
	    output_buffer[output_index].left = sample.left + snd_click_sample;
	    output_buffer[output_index].right = sample.right + snd_click_sample;

	    output_index++;
    }
}

static void SND_outputStateCallback(pa_context *c, void *userdata) {

    switch (pa_context_get_state(c)) {
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
            break;

        case PA_CONTEXT_READY: {
            pa_cvolume cv;

            pa_threaded_mainloop_lock(mainloop);

            stream = pa_stream_new(c, "Audio", &sample_spec, NULL);
            assert(stream);

            //pa_stream_set_write_callback(stream, SND_outputWrite, NULL);
            pa_stream_connect_playback(stream, device, NULL, 0, pa_cvolume_set(&cv, sample_spec.channels, PA_VOLUME_NORM), NULL);

            pa_threaded_mainloop_unlock(mainloop);

            break;
        }

        case PA_CONTEXT_TERMINATED:
            //quit(0);
            break;

        case PA_CONTEXT_FAILED:
        default:
            //fprintf(stderr, _("Connection failure: %s\n"), pa_strerror(pa_context_errno(c)));
            //quit(1);
            break;
    }
}

int SND_outputInit(int rate)
{
	int	parm;

    sample_spec.format = PA_SAMPLE_U8;
    sample_spec.channels = 2;
    sample_spec.rate = rate;

    if (!(sample_buffer = malloc(output_buffer_size * 2))) {
        return 0;
    }

    if (!(mainloop = pa_threaded_mainloop_new())) {
        free(sample_buffer);

        return 0;
    }

    mainloop_api = pa_threaded_mainloop_get_api(mainloop);
    context      = pa_context_new(mainloop_api, "xgs");

    if (!(context = pa_context_new(mainloop_api, "xgs"))) {
        pa_threaded_mainloop_free(mainloop);
        free(sample_buffer);

        return 0;
    }

    pa_context_set_state_callback(context, SND_outputStateCallback, NULL);

    if (pa_context_connect(context, server, 0, NULL) < 0) {
        pa_threaded_mainloop_free(mainloop);
        free(sample_buffer);

        return 0;
    }

    pa_threaded_mainloop_start(mainloop);

    return sample_spec.rate;
}

void SND_outputShutdown(void)
{
    pa_threaded_mainloop_stop(mainloop);

    free(sample_buffer);
    // TODO: disconnect PA
}

size_t SND_outputWrite(snd_sample_struct *buffer, size_t len)
{
    int space, nsamples;

    pa_threaded_mainloop_lock(mainloop);

    space    = pa_stream_writable_size(stream);
    nsamples = len * 2;

    size_t SND_outputWrite(snd_sample_struct *, size_t);

    //printf("pa_strem_writeable_size returned %d (need at least %d)\n", space, nsamples);

    if (nsamples > space) nsamples = space;

    if (nsamples > 0) {
        SND_generateSamples(buffer, sample_buffer, len);

        //printf("writing %d samples\n", nsamples);

        pa_stream_write(stream, sample_buffer, nsamples, NULL, 0, PA_SEEK_RELATIVE);
    }

    pa_threaded_mainloop_unlock(mainloop);

	return len;
}
