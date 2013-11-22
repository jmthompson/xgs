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

// -P-
soundinfo mybuffers[SOUNDBUFFERS];

int soundbuffer;

HWAVEOUT phwo;
int inited = 0;

int SND_outputInit(int rate)
{
	PCMWAVEFORMAT soundformat;
	extern HWND	hWnd;
	int i, j;

	soundformat.wf.wFormatTag = WAVE_FORMAT_PCM;
	soundformat.wf.nChannels = 2;
	soundformat.wf.nSamplesPerSec = rate;
	soundformat.wf.nAvgBytesPerSec = rate * 4;
	soundformat.wf.nBlockAlign = 4;
	soundformat.wBitsPerSample = 16;
	if (waveOutOpen(&phwo, (UINT) WAVE_MAPPER, (LPWAVEFORMATEX) &soundformat, 
		(DWORD) hWnd, 0, CALLBACK_WINDOW) != MMSYSERR_NOERROR) {
		return 0;
	}

	for (i = 0; i < SOUNDBUFFERS; i++) {
		mybuffers[i].hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, OUTPUT_BUFFER_SIZE * 4);
		if (mybuffers[i].hData == NULL) {
			for (j = 0; j < i; j++) GlobalFree(mybuffers[j].hData);
			return 0;
		}
	}

	for (i = 0; i < SOUNDBUFFERS; i++) {
		if ((mybuffers[i].lpData = GlobalLock(mybuffers[i].hData)) == NULL) {
			for (j = 0; j < i; j++) GlobalUnlock(mybuffers[j].hData);
			for (j = 0; j < SOUNDBUFFERS; j++) GlobalFree(mybuffers[j].hData);
			return 0;
		}
	}

	for (i = 0; i < SOUNDBUFFERS; i++) {
		mybuffers[i].hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
			(DWORD) sizeof(WAVEHDR));
		if (!mybuffers[i].hWaveHdr) {
			for (j = 0; j < i; j++) GlobalFree(mybuffers[j].hWaveHdr);
			for (j = 0; j < SOUNDBUFFERS; j++) GlobalUnlock(mybuffers[j].hData);
			for (j = 0; j < SOUNDBUFFERS; j++) GlobalFree(mybuffers[j].hData);
			return 0;
		}
	}

	for (i = 0; i < SOUNDBUFFERS; i++) {
		mybuffers[i].lpWaveHdr = (LPWAVEHDR) GlobalLock(mybuffers[i].hWaveHdr);
		if (!mybuffers[i].lpWaveHdr) {
			for (j = 0; j < i; j++) GlobalUnlock(mybuffers[j].hWaveHdr);
			for (j = 0; j < SOUNDBUFFERS; j++) GlobalFree(mybuffers[j].hWaveHdr);
			for (j = 0; j < SOUNDBUFFERS; j++) GlobalUnlock(mybuffers[j].hData);
			for (j = 0; j < SOUNDBUFFERS; j++) GlobalFree(mybuffers[j].hData);
		}
	}

	for (i = 0; i < SOUNDBUFFERS; i++) {
		mybuffers[i].lpWaveHdr->lpData = mybuffers[i].lpData;
		mybuffers[i].lpWaveHdr->dwBufferLength = OUTPUT_BUFFER_SIZE * 4;
		mybuffers[i].lpWaveHdr->dwFlags = 0L;
		mybuffers[i].lpWaveHdr->dwLoops = 0L;
	}

	for (i = 0; i < SOUNDBUFFERS; i++)
		mybuffers[i].busy = FALSE;

	for (i = 0; i < SOUNDBUFFERS; i++)
		waveOutPrepareHeader(phwo, mybuffers[i].lpWaveHdr, sizeof(WAVEHDR));

	soundbuffer = 0;

	inited = 1;
	return rate;
}

void SND_outputShutdown(void)
{
	int j;

	inited = 0;

	if (waveOutReset(phwo) != MMSYSERR_NOERROR)
		printf ("waveOutReset failed.\n");
	for (j = 0; j < SOUNDBUFFERS; j++)
		waveOutUnprepareHeader(phwo, mybuffers[j].lpWaveHdr, sizeof(WAVEHDR));
	for (j = 0; j < SOUNDBUFFERS; j++) GlobalUnlock(mybuffers[j].hWaveHdr);
	for (j = 0; j < SOUNDBUFFERS; j++) GlobalFree(mybuffers[j].hWaveHdr);
	for (j = 0; j < SOUNDBUFFERS; j++) GlobalUnlock(mybuffers[j].hData);
	for (j = 0; j < SOUNDBUFFERS; j++) GlobalFree(mybuffers[j].hData);
	if (waveOutClose(phwo) != MMSYSERR_NOERROR)
		printf ("waveOutClose failed.\n");
}

size_t SND_outputWrite(snd_sample_struct *buffer, size_t len)
{
	UINT wResult;
	int i;

	if (!inited) return 0;

	if (len != OUTPUT_BUFFER_SIZE) {
		return 0;
	}

	soundbuffer = -1;
	i = 0;
	while ((i < SOUNDBUFFERS) && (soundbuffer == -1))
		if (mybuffers[i++].busy == FALSE) soundbuffer = i-1;

	if (soundbuffer == -1) return 0;

	mybuffers[soundbuffer].lpWaveHdr->dwFlags == WHDR_PREPARED;

	mybuffers[soundbuffer].busy = TRUE;

//	memcpy(mybuffers[soundbuffer].lpData, buffer, OUTPUT_BUFFER_SIZE);
	for (i = 0; i < OUTPUT_BUFFER_SIZE; i++) {
		((WORD *) (mybuffers[soundbuffer].lpData))[i*2] = (WORD) (buffer[i].left >> 2);
		((WORD *) (mybuffers[soundbuffer].lpData))[(i*2)+1] = (WORD) (buffer[i].right >> 2);
	}
	wResult = waveOutWrite(phwo, mybuffers[soundbuffer].lpWaveHdr, sizeof(WAVEHDR));
	if (wResult != 0) {
		return 0;
	}

	return OUTPUT_BUFFER_SIZE;
}
// -P-
