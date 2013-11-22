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
 * File: arch/sparc/snd-drv.c
 *
 * The Sun /dev/audio sound output driver.
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

#if defined ( __svr4__ ) || defined ( __SVR4 )
#define SVR4
#endif

/* Sun 4.1 -I/usr/demo/SOUND/multimedia ??? */
#include <errno.h>
#include <fcntl.h>
#include <stropts.h>
#ifdef SVR4
#include <sys/audioio.h>
#else
#include <sun/audioio.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>

#include "sound.h"
#include "snd-drv.h"

#ifndef SVR4
/******** START OF FIX SUN **********************************/
/* SINCE CC, GCC and SUN can't get their act together, let's do it
 * for them. This is to fix some SERIOUS Bugs with Sun's include files.
 *
 * CC works fine with /usr/include/sys/ioccom.h, but not gnu's
 * GCC works fine with it's version of Sun's ioccom.h, but not sun's
 * You mix them up and your code doesn't run. ARGH!!!
 */
/* from sys/ioccom.h */
#undef _IO
#define _IO(x,y)        (_IOC_VOID|((x)<<8)|y)
#undef _IOR
#define _IOR(x,y,t)     (_IOC_OUT|((sizeof(t)&_IOCPARM_MASK)<<16)|((x)<<8)|y)
#undef _IOWR
#define _IOWR(x,y,t)    (_IOC_INOUT|((sizeof(t)&_IOCPARM_MASK)<<16)|((x)<<8)|y)
/* from audioio.h  A = 0x41 */
#undef AUDIO_GETINFO
#define AUDIO_GETINFO   _IOR(0x41, 1, audio_info_t)
#undef AUDIO_SETINFO
#define AUDIO_SETINFO   _IOWR(0x41, 2, audio_info_t)
#undef AUDIO_DRAIN
#define AUDIO_DRAIN     _IO(0x41, 3)
#undef AUDIO_GETDEV
#define AUDIO_GETDEV    _IOR(0x41, 4, int)
/* from stropts.h S = 0x53*/
#undef I_FLUSH
#define I_FLUSH         _IO(0x53,05)
/******** END OF FIX SUN **********************************/
#endif /* SVR4 */

#ifndef AUDIO_ENCODING_LINEAR
#define AUDIO_ENCODING_LINEAR (3)
#endif
#ifndef AUDIO_ENCODING_ULAW
#define AUDIO_ENCODING_ULAW (3)
#endif
#ifndef AUDIO_DEV_UNKNOWN
#define AUDIO_DEV_UNKNOWN (0)
#endif
#ifndef AUDIO_DEV_AMD
#define AUDIO_DEV_AMD (1)
#endif
#ifndef AUDIO_SPEAKER
#define AUDIO_SPEAKER 0x01
#endif
#ifndef AUDIO_HEADPHONE
#define AUDIO_HEADPHONE 0x02
#endif
#ifndef AUDIO_LINE_OUT
#define AUDIO_LINE_OUT 0x04
#endif
#ifndef AUDIO_MIN_GAIN
#define AUDIO_MIN_GAIN (0)
#endif
#ifndef AUDIO_MAX_GAIN
#define AUDIO_MAX_GAIN (255)
#endif

static int devAudio;
static audio_info_t audio_info;
sword16 snd_out_buffer[OUTPUT_BUFFER_SIZE*2];

int SND_outputInit(int rate)
{
  int ret;
#ifdef SVR4	/* was SOLARIS */
  audio_device_t type;
#else
  int type;
#endif

  devAudio = open("/dev/audio", O_WRONLY | O_NDELAY);
  if (devAudio == -1)
  {
    if (errno == EBUSY) fprintf(stderr,"Audio_Init: Audio device is busy. - ");
    else fprintf(stderr,"Audio_Init: Error %x opening audio device. - ",errno);
    return 0;
  }
  ret = ioctl(devAudio, AUDIO_GETDEV, &type);
#ifdef SVR4
  if ( (   (strcmp(type.name, "SUNW,dbri"))   /* Not DBRI (SS10's) */
        && (strcmp(type.name, "SUNW,CS4231")) /* and not CS4231 (SS5's) */
        && (strcmp(type.name, "SUNW,sb16")))  /* and SoundBlaster 16 */
      || ret) /* or ioctrl failed */
#else
  if (ret || (type==AUDIO_DEV_UNKNOWN) || (type==AUDIO_DEV_AMD) )
#endif
  { /* SPARC AU AUDIO */
    fprintf(stderr,"AMD telephone quality audio not supported.\n");
    return 0;
  }
  else /* DBRI or CS4231 or SB16 */
  {
    static int valid[] = { 8000, 9600, 11025, 16000, 18900, 22050, 32000,
			     37800, 44100, 48000, 0};
    int i = 0;
    int best = 8000;

    AUDIO_INITINFO(&audio_info);

    while(valid[i])
      { 
	if (abs(valid[i] - rate) < abs(best - rate)) best = valid[i];
	i++;
      }
    rate = best;
    audio_info.play.sample_rate = rate;
    audio_info.play.encoding = AUDIO_ENCODING_LINEAR;
    audio_info.play.precision = 16;
    audio_info.play.channels = 2;

    ret = ioctl(devAudio, AUDIO_SETINFO, &audio_info);
    if (ret)
    {
      fprintf(stderr,"AUDIO BRI FATAL ERROR %d\n",errno);
      return 0;
    }
    /* configure devaudio for Asynchronous I/O */
    ret = fcntl(devAudio, F_SETFL, O_NONBLOCK);
    if (ret==-1)
    {
      fprintf(stderr,"Couldn't configure audio for asynchronous I/O, errno=%d\n",errno);
    }
  }
  return rate;
}

void SND_outputShutdown(void)
{
  long ret;

  /* FLUSH AUDIO DEVICE */
  ret = ioctl(devAudio, I_FLUSH, FLUSHW);
  if (ret == -1) fprintf(stderr,"Sparc Audio: off flush err %d\n",errno);

  /* FLUSH AUDIO DEVICE AGAIN */
  ret = ioctl(devAudio, I_FLUSH, FLUSHW);
  if (ret == -1) fprintf(stderr,"Sparc Audio: off flush err %d\n",errno);

  /* SHUT THINGS DOWN  */
  close(devAudio);
}

size_t SND_outputWrite(snd_sample_struct *buffer, size_t len)
{
  int i;
  ssize_t wbyte;
  const int *intbuffer=(int*)buffer;

  for (i = 0 ; i < len*2 ; i++)
    snd_out_buffer[i] = intbuffer[i];
	
  wbyte = write(devAudio,snd_out_buffer,len*4);	/* 4 bytes per sample */
  if (wbyte < 0) {
#ifdef DEBUG
    printf("SND_outputWrite %d errno=%d\n",wbyte,errno);
#endif
    return 0;
  }
  return wbyte/4;
}
