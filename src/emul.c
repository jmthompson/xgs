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
 * File: emul.c
 *
 * Startup and shutdown code for the emulator, as well as the update
 * routine called 50 times per second.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "adb.h"
#include "app.h"
#include "clock.h"
#include "disks.h"
#include "emul.h"
#include "iwm.h"
#include "smtport.h"
#include "sound.h"
#include "video.h"
#include "video-output.h"

int	emul_period;

int	emul_vbl;
int	emul_vblirq;
int	emul_qtrsecirq;
int	emul_onesecirq;
int	emul_scanirq;

int cpu_cycle_count;

int	emul_speed;
int	emul_speed2;
int	emul_delay;

long emul_tick;
long emul_microtick;

int	emul_vbl_count;

char	emul_buffer[256];

word32	emul_last_cycles;

long    emul_target_cycles; 
double	emul_target_speed;

double	emul_times[60];
double	emul_total_time;

long emul_cycles[60];
long emul_total_cycles;

long	emul_last_time;
long	emul_this_time;

char	*emul_path;
char	*emul_s5d1,*emul_s5d2;
char	*emul_s6d1,*emul_s6d2;
char	*emul_smtport[NUM_SMPT_DEVS];

void EMUL_doVBL()
{
	long cycles;
	double	this_time,last_time,diff,speed;
	long new_period;

    emul_delay = 0;
#ifdef HAVE_GETTIMEOFDAY
	if (emul_delay > 0) {
#ifdef EMX_HAVE_SLEEP2        
		_sleep2(emul_delay);
#else
		usleep(emul_delay);
#endif  // end of #ifdef EMX_HAVE_SLEEP2
	}
#endif  // end of #ifdef HAVE_GETTIMEOFDAY

	cycles = cpu_cycle_count - emul_last_cycles;
	emul_last_cycles = cpu_cycle_count;

	emul_vbl_count++;

	if (emul_vbl == 0x00) {
		if (emul_vblirq) {
			if (!(mem_diagtype & 0x08)) {
				mem_diagtype |= 0x08;
				m65816_addIRQ();
			}
		}
		VID_update();
		MEM_update();
		ADB_update();
		CLK_update();
		IWM_update();
		SMPT_update();
	}

	if ((emul_tick == 0) || (emul_tick == 15) || (emul_tick == 30) || (emul_tick == 45)) {
		if (emul_qtrsecirq) {
			if (!(mem_diagtype & 0x10)) {
				mem_diagtype |= 0x10;
				m65816_addIRQ();
			}
		}
	}

	if (emul_onesecirq && !emul_tick) {
		if (!(vid_vgcint & 0x40)) {
			vid_vgcint |= 0xC0;
			m65816_addIRQ();
		}
	}

	emul_this_time = EMUL_getCurrentTime();

	this_time = (double) emul_this_time;
	last_time = (double) emul_last_time;

	diff = abs(this_time - last_time);

	emul_last_time = emul_this_time;

	emul_total_time -= emul_times[emul_tick];
	emul_times[emul_tick] = diff;
	emul_total_time += diff;

	emul_total_cycles -= emul_cycles[emul_tick];
	emul_cycles[emul_tick] = cycles;
	emul_total_cycles += cycles;

	speed = (emul_total_cycles / emul_total_time)  / 1000;

	new_period = speed * 32.0;

	if (new_period < 15) new_period = 15;

	emul_period = new_period;
	//m65816_setUpdatePeriod(new_period);

	if ((speed > emul_target_speed) && emul_target_cycles) {
		emul_delay = ((emul_total_cycles - emul_target_cycles) * 1000000) / emul_total_cycles;
	} else {
		emul_delay = 0;
	}

	if (!emul_tick) {
		sprintf(emul_buffer,"Target Speed: %2.1f MHz  Actual Speed: %2.1f MHz  Period = %3d cycles", emul_target_speed, speed,new_period);
		VID_outputStatus2(emul_buffer);
	}

	if (++emul_tick == 60) emul_tick = 0;
}

void EMUL_hardwareUpdate(int cycles)
{
	SND_update();
	emul_microtick++;
	if (emul_microtick == 400) {
		emul_vbl = 0x80;
		vid_vert_cnt = emul_microtick >> 1;
	} else if (emul_microtick == 524) {
		emul_microtick = 0;
		vid_vert_cnt = 0;
		emul_vbl = 0x00;
		EMUL_doVBL();
	} else {
		vid_vert_cnt = emul_microtick >> 1;
	}

	if (emul_scanirq && (vid_vert_cnt < 200) && (slow_memory[0x019D00 + vid_vert_cnt] & 0x40)) {
		if (!(vid_vgcint & 0x20)) {
			vid_vgcint |= 0xA0;
			m65816_addIRQ();
		}
	}
}

int EMUL_init(int argc, char *argv[])
{
	int	err,i,unit;

	mem_ramsize = 2;
	snd_enable = 1;

	emul_s5d1 = NULL;
	emul_s5d2 = NULL;
	emul_s6d1 = NULL;
	emul_s6d2 = NULL;
	for (i = 0 ; i < NUM_SMPT_DEVS ; i++) emul_smtport[i] = NULL;

	i = 0;
	while (++i < argc) {
		if (!strcmp(argv[i],"-trace")) {
			EMUL_trace(1);
		} else if (!strcmp(argv[i],"-nosound")) {
			snd_enable = 0;
		} else if (!strcmp(argv[i],"-ram")) {
			if (++i >= argc) EMUL_displayUsage(argv[0]);
			mem_ramsize = atoi(argv[i]);
		} else if (!strcmp(argv[i],"-s5d1")) {
			if (++i >= argc) EMUL_displayUsage(argv[0]);
			emul_s5d1 = argv[i];
		} else if (!strcmp(argv[i],"-s5d2")) {
			if (++i >= argc) EMUL_displayUsage(argv[0]);
			emul_s5d2 = argv[i];
		} else if (!strcmp(argv[i],"-s6d1")) {
			if (++i >= argc) EMUL_displayUsage(argv[0]);
			emul_s6d1 = argv[i];
		} else if (!strcmp(argv[i],"-s6d2")) {
			if (++i >= argc) EMUL_displayUsage(argv[0]);
			emul_s6d2 = argv[i];
		} else if (!strncmp(argv[i],"-smpt",5)) {
			unit = atoi(argv[i] + 5);
			if ((unit < 0) || (unit >= NUM_SMPT_DEVS)) EMUL_displayUsage(argv[0]);
			if (++i >= argc) EMUL_displayUsage(argv[0]);
			emul_smtport[unit] = argv[i];
		} else EMUL_displayUsage(argv[0]);
	}

	printf("XGS Version %s\n", VERSION);
	printf("(c) 2013 by Joshua M. Thompson\n\n");
    printf("Data directory = %s\n\n", emul_path);

	if ((err = MEM_init())) return err;
	if ((err = VID_init())) {
		MEM_shutdown();
		return err;
	}
	if ((err = SND_init())) {
		VID_shutdown();
		MEM_shutdown();
		return err;
	}
	if ((err = ADB_init())) {
		SND_shutdown();
		VID_shutdown();
		MEM_shutdown();
		return err;
	}
	if ((err = CLK_init())) {
		ADB_shutdown();
		SND_shutdown();
		VID_shutdown();
		MEM_shutdown();
		return err;
	}
	if ((err = IWM_init())) {
		CLK_shutdown();
		ADB_shutdown();
		SND_shutdown();
		VID_shutdown();
		MEM_shutdown();
		return err;
	}

	if ((err = SMPT_init())) {
		IWM_shutdown();
		CLK_shutdown();
		ADB_shutdown();
		SND_shutdown();
		VID_shutdown();
		MEM_shutdown();
		return err;
	}

	printf("\nInitializing emulator core\n");

	emul_last_time = EMUL_getCurrentTime();

	for (i = 0 ; i < 60 ; i++) {
		emul_times[i] = 0.0;
		emul_cycles[i] = 0;
	}
	emul_total_time = 0.0;
	emul_total_cycles = 0;
	emul_tick = 0;
	emul_microtick = 0;

	emul_last_cycles = 0;

	emul_vbl = 0;
	emul_vblirq = 0;
	emul_qtrsecirq = 0;
	emul_onesecirq = 0;
	emul_speed = 1;
	emul_speed2 = 1;

	emul_target_cycles = 2500000;
	emul_target_speed = 2.5;

    m65816_init();
	EMUL_reset();

	return 0;
}

void EMUL_run()
{
	char	*path;
	int	i,err;

	printf("\nLoading startup drives\n");
	for (i = 0 ; i < NUM_SMPT_DEVS ; i++) {
		if (!emul_smtport[i]) continue;
		path = EMUL_expandPath(emul_smtport[i]);
		printf("    - Loading SmartPort device %d from \"%s\": ", i, path);
		err = SMPT_loadDrive(i, path);
		if (err) {
			printf("Failed (err #%d)\n",err);
		} else {
			printf("Done\n");
		}
	}
	if (emul_s5d1) {
		path = EMUL_expandPath(emul_s5d1);
		printf("    - Loading S5, D1 from \"%s\": ", path);
		err = IWM_loadDrive(5, 1, path);
		if (err) {
			printf("Failed (err #%d)\n",err);
		} else {
			printf("Done\n");
		}
	}
	if (emul_s5d2) {
		path = EMUL_expandPath(emul_s5d2);
		printf("    - Loading S5, D2 from \"%s\": ", path);
		err = IWM_loadDrive(5, 2, path);
		if (err) {
			printf("Failed (err #%d)\n",err);
		} else {
			printf("Done\n");
		}
	}
	if (emul_s6d1) {
		path = EMUL_expandPath(emul_s6d1);
		printf("    - Loading S6, D1 from \"%s\": ", path);
		err = IWM_loadDrive(6, 1, path);
		if (err) {
			printf("Failed (err #%d)\n",err);
		} else {
			printf("Done\n");
		}
	}
	if (emul_s6d2) {
		path = EMUL_expandPath(emul_s6d2);
		printf("    - Loading S6, D2 from \"%s\": ", path);
		err = IWM_loadDrive(6, 2, path);
		if (err) {
			printf("Failed (err #%d)\n",err);
		} else {
			printf("Done\n");
		}
	}
	printf("\n*** EMULATOR IS RUNNING ***\n");

    emul_period = 64;

    while(1) {
	    int num_cycles = m65816_run(emul_period);

        cpu_cycle_count += num_cycles;

        EMUL_hardwareUpdate(num_cycles);
    }
}

void EMUL_reset()
{
	emul_delay = 0;

	MEM_reset();
	VID_reset();
	SND_reset();
	ADB_reset();
	CLK_reset();
	IWM_reset();
	SMPT_reset();
	m65816_reset();
}

void EMUL_shutdown()
{
	SMPT_shutdown();
	IWM_shutdown();
	CLK_shutdown();
	ADB_shutdown();
	SND_shutdown();
	VID_shutdown();
	MEM_shutdown();
	APP_shutdown();
}

void EMUL_trace(int parm)
{
	m65816_setTrace(parm);
}

void EMUL_nmi()
{
	int	i;

	for (i = 0x0000 ; i < 0x0200 ; i++) {
		if (!mem_pages[i].readPtr) continue;
		memcpy(mem_pages[0xE800+i].writePtr,mem_pages[i].readPtr,256);
	}
 	m65816_nmi();
}

void EMUL_handleWDM(byte parm)
{
	switch(parm) {
		case 0xC7 :	SMPT_prodosEntry();
				break;
		case 0xC8 :	SMPT_smartportEntry();
				break;
		case 0xFD :	m65816_setTrace(0);
				break;
		case 0xFE :	m65816_setTrace(1);
				break;
		case 0xFF :	EMUL_shutdown();
				break;
		default :	break;
	}
}

byte EMUL_getVBL(byte val)
{
	return emul_vbl;
}

void EMUL_displayUsage(char *myname)
{
	fprintf(stderr,"\nUsage: %s [-s5d1 filename ] [-s5d2 filename ] [-s6d1 filename ] [-s6d2 filename ] [-smpt# filename ] [-ram # ] [-trace]\n",myname);
	exit(1);
}

#ifdef hpux
#include <time.h>
#include <sys/time.h>

int     usleep( unsigned int microSeconds )
{
	unsigned int	    Seconds, uSec;
	int		     nfds, readfds, writefds, exceptfds;
	struct  timeval	 Timer;

	nfds = readfds = writefds = exceptfds = 0;

	if( (microSeconds == (unsigned long) 0) 
		|| microSeconds > (unsigned long) 4000000 )
	{
		errno = ERANGE;	 /* value out of range */
		perror( "usleep time out of range ( 0 -> 4000000 ) " );
		return -1;
	}

	Seconds = microSeconds / (unsigned long) 1000000;
	uSec    = microSeconds % (unsigned long) 1000000;

	Timer.tv_sec	    = Seconds;
	Timer.tv_usec	   = uSec;

	if( select( nfds, &readfds, &writefds, &exceptfds, &Timer ) < 0 )
	{
		perror( "usleep (select) failed" );
		return -1;
	}

	return 0;
}
#endif
