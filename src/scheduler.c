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
 * File: scheduler-posix.c
 *
 * This is the scheduler for systems with high-resolution POSIX timers.
 * It sets up a timer with a period of one microtick, and every time it
 * fires it runs a small chunk of CPU emulation as well as the various
 * hardware updates.
 */

#include <signal.h>
#include <time.h>

#include "xgs.h"
#include "m65816.h"
#include "hardware.h"
#include "scheduler.h"
#include "sound.h"
#include "video.h"

static timer_t master_timer;

static float target_speed;
static float actual_speed;

static long frames;

static long emul_last_time;
static long emul_this_time;

static long emul_last_cycles;

static long emul_times[60];
static long emul_total_time;

static long emul_cycles[60];
static long emul_total_cycles;

int schedulerInit() {
    struct timespec ts;
    struct sigevent sev;
    int i;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIGUSR1;

    clock_getres(CLOCK_MONOTONIC, &ts);
    printf("Native timer resolution is %d ns\n", ts.tv_nsec);

    if (timer_create(CLOCK_MONOTONIC, &sev, &master_timer) < 0) {
        perror("Error creating master timer");

        return errno;
    }

	emul_last_time = schedulerGetTime();

    for (i = 0 ; i < g_framerate; i++) {
        emul_times[i] = 0.0;
        emul_cycles[i] = 0;
    }

    emul_total_time = 0.0;
    emul_total_cycles = 0;
    emul_last_cycles = 0;

    return 0;
}

void schedulerStart() {
    struct itimerspec ts;
    sigset_t ss;
    int signum;

    sigemptyset(&ss);
    sigaddset(&ss, SIGUSR1);
    sigaddset(&ss, SIGUSR2);

    sigprocmask(SIG_BLOCK,  &ss, NULL);

    memset(&ts, 0, sizeof(ts));

    ts.it_interval.tv_nsec = (1000000000 / g_framerate);
    ts.it_value.tv_sec     = 0;
    ts.it_value.tv_nsec    = 1000000;

    if (timer_settime(master_timer, 0, &ts, NULL) < 0) {
        printf("failed to set timer: %s\n", strerror(errno));

        return;
    }

    printf("Timer period is %0.1f ms (%d Hz)\n", (float) ts.it_interval.tv_nsec / 1000000.0, g_framerate);

    while(1) {
        if (sigwait(&ss, &signum) != 0) {
            perror("sigwait failed");

            continue;
        }

        if (signum == SIGUSR1) {
            schedulerTick();
        }
        else if (signum == SIGUSR2) {
            schedulerStop();
        }
    }
}

void schedulerStop() {
    timer_delete(master_timer);

    hardwareShutdown();
    exit(0);
}

long schedulerGetTime() {
	struct timeval tv;

	gettimeofday(&tv, NULL);
    
    return (long) (tv.tv_sec * 1000) + (tv.tv_usec / 1000); 
}

void schedulerHousekeeping() {
    long   cycles;
    float  diff;

    cycles = g_cpu_cycles - emul_last_cycles;
    emul_last_cycles = g_cpu_cycles;

	emul_this_time = schedulerGetTime();

	diff = emul_this_time - emul_last_time;

	emul_last_time = emul_this_time;

	emul_total_time -= emul_times[frames];
	emul_times[frames] = diff;
	emul_total_time += diff;

	emul_total_cycles -= emul_cycles[frames];
	emul_cycles[frames] = cycles;
	emul_total_cycles += cycles;

	actual_speed = ((float) emul_total_cycles / (float) emul_total_time) / 1000.0;

    if (frames == 0) {
        printf("Current speed = %0.1f MHz (%ld cycles, %ld ms)\n", actual_speed, emul_total_cycles, emul_total_time);
    }
}

float schedulerGetActualSpeed() {
    return actual_speed;
}

float schedulerGetTargetSpeed() {
    return target_speed;
}

void schedulerSetTargetSpeed(float mhz) {
    target_speed = mhz;
}

void schedulerTick()
{
    int line;

    for (line = 0 ; line < LINES_PER_FRAME ; line++) {
        int num_cycles = m65816_run(target_speed * 32);

        g_cpu_cycles += num_cycles;

        soundUpdate();
        hardwareTick(line);
    }

    hardwareBigTick(frames++);

    if (frames == g_framerate) {
        frames = 0;
    }

    schedulerHousekeeping();
}
