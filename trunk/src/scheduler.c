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

#ifdef HAVE_TIMERFD
static int master_timer;
#else
static timer_t master_timer;
#endif

static float target_speed;
static float actual_speed;

static long bigticks;
static long ticks;

long emul_last_time;
long emul_this_time;

long emul_last_cycles;

long emul_times[FRAMES_PER_SECOND];
long emul_total_time;

long emul_cycles[FRAMES_PER_SECOND];
long emul_total_cycles;

int schedulerInit() {
    int i;
    struct timespec ts;
#ifndef HAVE_TIMERFD
    struct sigevent   sev;

    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIGUSR1;
#endif

    clock_getres(CLOCK_MONOTONIC, &ts);
    printf("Native timer resolution is %d ns\n", ts.tv_nsec);

#ifdef HAVE_TIMERFD
    if ((master_timer = timerfd_create(CLOCK_MONOTONIC, 0)) < 0) {
#else
    if (timer_create(CLOCK_MONOTONIC, &sev, &master_timer) < 0) {
#endif
        perror("Error creating master timer");

        return errno;
    }

	emul_last_time = schedulerGetTime();

    for (i = 0 ; i < FRAMES_PER_SECOND; i++) {
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
    struct sigaction  sa;

    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);

    sa.sa_handler = &schedulerStop;
    sigaction(SIGTERM, &sa, NULL);

#ifndef HAVE_TIMERFD
    sa.sa_handler = &schedulerTick;
    sigaction(SIGUSR1, &sa, NULL);
#endif

    memset(&ts, 0, sizeof(ts));

    float delay = ((float) SAMPLE_RATE / (float) OUTPUT_BUFFER_SIZE) * 1000000000;

    ts.it_interval.tv_nsec = (1000000000 / FRAMES_PER_SECOND);
    ts.it_value.tv_sec     = (long) delay / 1000000000;
    ts.it_value.tv_nsec    = (long) delay % 1000000000;

#ifdef HAVE_TIMERFD
    printf("Using timerfd_settime\n");

    if (timerfd_settime(master_timer, 0, &ts, NULL) < 0) {
#else
    printf("Using timer_settime\n");

    if (timer_settime(master_timer, 0, &ts, NULL) < 0) {
#endif
        printf("failed to set timer: %s\n", strerror(errno));

        return;
    }

    printf("Timer period = %.01f ms, starting in %.1f seconcds\n", (float) ts.it_interval.tv_nsec / 1000000.0, delay / 1000000000);

    while(1) {
#ifdef HAVE_TIMERFD
        long long num_overflows;

        read(master_timer, &num_overflows, sizeof(num_overflows));

        schedulerTick(0);
#else
        sleep(1);
#endif
    }
}

void schedulerStop(int val) {
    struct sigaction  sa;

    sa.sa_handler = SIG_DFL;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGTERM, &sa, NULL);

#ifdef HAVE_TIMERFD
    close(master_timer);
#else
    timer_delete(master_timer);

    sigaction(SIGUSR1, &sa, NULL);
#endif

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

	emul_total_time -= emul_times[bigticks];
	emul_times[bigticks] = diff;
	emul_total_time += diff;

	emul_total_cycles -= emul_cycles[bigticks];
	emul_cycles[bigticks] = cycles;
	emul_total_cycles += cycles;

	actual_speed = ((float) emul_total_cycles / (float) emul_total_time) / 1000.0;

    if (bigticks == 0) {
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

void schedulerTick(int val)
{
    for (ticks = 0 ; ticks < LINES_PER_FRAME ; ticks++) {
        int num_cycles = m65816_run(target_speed * 32);

        g_cpu_cycles += num_cycles;

        hardwareTick(ticks, bigticks);
    }

    ticks = 0;

    hardwareBigTick(bigticks++);

    if (bigticks == FRAMES_PER_SECOND) {
        bigticks = 0;
    }

    schedulerHousekeeping();
}
