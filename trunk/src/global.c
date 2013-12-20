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

int main(int argc, char **argv) {
    char *homedir;
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

    hardwareInit();
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

static void displayUsage(char *myname)
{
    fprintf(stderr,"\nUsage: %s [-s5d1 filename ] [-s5d2 filename ] [-s6d1 filename ] [-s6d2 filename ] [-smpt# filename ] [-ram # ] [-trace]\n",myname);
    exit(1);
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
