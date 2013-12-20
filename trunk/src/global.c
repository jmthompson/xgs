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
 * File: global.c
 *
 * Global functions & startup code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "xgs.h"

#include "adb.h"
#include "clock.h"
#include "disks.h"
#include "hardware.h"
#include "iwm.h"
#include "smtport.h"
#include "scheduler.h"
#include "sound.h"
#include "video.h"
#include "video-output.h"

// Begin globals
float g_fast_mhz;

int g_ram_size;

long g_cpu_cycles;
int g_vbl_count;
int g_vblirq_enable;
int g_qtrsecirq_enable;
int g_onesecirq_enable;
int g_scanirq_enable;

int g_fastmode;

int g_vbl_count;
// End globals

static char *data_dir;
static char *user_dir;
static int max_path;    // MAX(strlen(data_dir), strlen(user_dir))

static char *s5d1_image;
static char *s5d2_image;
static char *s6d1_image;
static char *s6d2_image;
static char *smtport_images[NUM_SMPT_DEVS];

static void displayUsage(const char *myname)
{
    fprintf(stderr,"\nUsage: %s [-s5d1 filename ] [-s5d2 filename ] [-s6d1 filename ] [-s6d2 filename ] [-smpt# filename ] [-ram # ] [-trace]\n",myname);
    exit(1);
}

int main(int argc, char **argv) {
    char *homedir;
    int i, err, unit;
    size_t len;

    len = strlen(XGS_DATA_DIR) + strlen(DATA_DIR) + 2;

    if ((data_dir = malloc(len)) == NULL) {
        fprintf(stderr, "Unable to allocate %d bytes: %s\n", len, strerror(errno));

        return(-1);
    }

    snprintf(data_dir, len, "%s/%s", DATA_DIR, XGS_DATA_DIR);
    
    max_path = len;

    homedir = getenv("HOME");

    if (!homedir || !strlen(homedir)) {
        homedir = ".";
    }

    len = strlen(XGS_USER_DIR) + strlen(homedir) + 2;

    if ((user_dir = malloc(len)) == NULL) {
        fprintf(stderr, "Unable to allocate %d bytes: %s\n", len, strerror(errno));

        return(-1);
    }

    snprintf(user_dir, len, "%s/%s", homedir, XGS_USER_DIR);
    
    if (mkdir(user_dir, 0700) < 0) {
        if (errno != EEXIST) {
            fprintf(stderr, "Unable to create %s: %s", user_dir, strerror(errno));
        }
    }

    if (len > max_path) {
        max_path = len;
    }

    g_fast_mhz = 2.5;
    g_ram_size = 2;

    s5d1_image = NULL;
    s5d2_image = NULL;
    s6d1_image = NULL;
    s6d2_image = NULL;

    for (i = 0 ; i < NUM_SMPT_DEVS ; i++) smtport_images[i] = NULL;

    i = 0;
    while (++i < argc) {
        if (!strcmp(argv[i],"-trace")) {
            EMUL_trace(1);
        } else if (!strcmp(argv[i],"-ram")) {
            if (++i >= argc) displayUsage(argv[0]);
            g_ram_size = atoi(argv[i]);
        } else if (!strcmp(argv[i],"-mhz")) {
            if (++i >= argc) displayUsage(argv[0]);
            g_fast_mhz = (float) atof(argv[i]);
        } else if (!strcmp(argv[i],"-s5d1")) {
            if (++i >= argc) displayUsage(argv[0]);
            s5d1_image = argv[i];
        } else if (!strcmp(argv[i],"-s5d2")) {
            if (++i >= argc) displayUsage(argv[0]);
            s5d2_image = argv[i];
        } else if (!strcmp(argv[i],"-s6d1")) {
            if (++i >= argc) displayUsage(argv[0]);
            s6d1_image = argv[i];
        } else if (!strcmp(argv[i],"-s6d2")) {
            if (++i >= argc) displayUsage(argv[0]);
            s6d2_image = argv[i];
        } else if (!strncmp(argv[i],"-smpt",5)) {
            unit = atoi(argv[i] + 5);
            if ((unit < 0) || (unit >= NUM_SMPT_DEVS)) displayUsage(argv[0]);
            if (++i >= argc) displayUsage(argv[0]);
            smtport_images[unit] = argv[i];
        } else displayUsage(argv[0]);
    }

    printf("Starting XGS Version %s\n\n", VERSION);

    if (hardwareInit() != 0) {
        return -1;
    }

    printf("\nLoading startup drives\n");

    for (i = 0 ; i < NUM_SMPT_DEVS ; i++) {
        if (!smtport_images[i]) continue;
        printf("    - Loading SmartPort device %d from \"%s\": ", i, smtport_images[i]);
        err = SMPT_loadDrive(i, smtport_images[i]);
        if (err) {
            printf("Failed (err #%d)\n",err);
        } else {
            printf("Done\n");
        }
    }
    if (s5d1_image) {
        printf("    - Loading S5, D1 from \"%s\": ", s5d1_image);
        err = IWM_loadDrive(5, 1, s5d1_image);
        if (err) {
            printf("Failed (err #%d)\n",err);
        } else {
            printf("Done\n");
        }
    }
    if (s5d2_image) {
        printf("    - Loading S5, D2 from \"%s\": ", s5d2_image);
        err = IWM_loadDrive(5, 2, s5d2_image);
        if (err) {
            printf("Failed (err #%d)\n",err);
        } else {
            printf("Done\n");
        }
    }
    if (s6d1_image) {
        printf("    - Loading S6, D1 from \"%s\": ", s6d1_image);
        err = IWM_loadDrive(6, 1, s6d1_image);
        if (err) {
            printf("Failed (err #%d)\n",err);
        } else {
            printf("Done\n");
        }
    }
    if (s6d2_image) {
        printf("    - Loading S6, D2 from \"%s\": ", s6d2_image);
        err = IWM_loadDrive(6, 2, s6d2_image);
        if (err) {
            printf("Failed (err #%d)\n",err);
        } else {
            printf("Done\n");
        }
    }
    printf("\n*** EMULATOR IS RUNNING ***\n");

    schedulerStart();
}

void globalShutdown()
{
    SMPT_shutdown();
    IWM_shutdown();
    CLK_shutdown();
    ADB_shutdown();
    SND_shutdown();
    VID_shutdown();
    MEM_shutdown();
}

/*
 * Tries to find a file in the user_dir first, and then the data_dir.
 * If found, opens it read-only and returns the file descriptor.
 */

int openDataFile(const char *filename) {
    int fd,  len;
    char *path;

    len = strlen(filename) + max_path + 2;
    path = malloc(len);

    if (!path) {
        fprintf(stderr, "openDataFile() unable to allocate %d bytes: %s\n", len, strerror(errno));

        return -1;
    }

    snprintf(path, len, "%s/%s", user_dir, filename);
    fd = open(path, O_RDONLY);

    if ((fd < 0) && (errno != ENOENT)) {
        free(path);

        return fd;
    }

    snprintf(path, len, "%s/%s", data_dir, filename);
    fd = open(path, O_RDONLY);

    free(path);

    return fd;
}

/*
 * Opens a user file (in user_dir) with the specified flags,
 * and return the file descriptor.
 */

int openUserFile(const char *filename, const int flags) {
    int fd,  len;
    char *path;

    len = strlen(filename) + max_path + 2;
    path = malloc(len);

    if (!path) {
        fprintf(stderr, "openUserFile() unable to allocate %d bytes: %s\n", len, strerror(errno));

        return -1;
    }

    snprintf(path, len, "%s/%s", user_dir, filename);

    fd = open(path, flags);

    free(path);

    return fd;
}
