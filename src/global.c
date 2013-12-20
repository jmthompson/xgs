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

int main(int argc, char **argv) {
    char *homedir;
    size_t len;

    len = strlen(XGS_DATA_DIR) + strlen(DATA_DIR) + 2;

    if ((data_dir = malloc(len)) == NULL) {
        fprintf(stderr, "Unable to allocate %d bytes: %s\n", len, strerror(errno));

        return(-1);
    }

    snprintf(data_dir, "%s/%s", DATA_DIR, XGS_DATA_DIR);
    
    homedir = getenv("HOME");

    if (!homedir || !strlen(homedir)) {
        homedir = ".";
    }

    len = strlen(XGS_USER_DIR) + strlen(homedir) + 2;

    if ((user_dir = malloc(len)) == NULL) {
        fprintf(stderr, "Unable to allocate %d bytes: %s\n", len, strerror(errno));

        return(-1);
    }

    snprintf(user_dir, "%s/%s", homedir, XGS_USER_DIR);
    
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

void EMUL_trace(int parm)
{
    m65816_setTrace(parm);
}

void EMUL_nmi()
{
    int    i;

    for (i = 0x0000 ; i < 0x0200 ; i++) {
        if (!mem_pages[i].readPtr) continue;
        memcpy(mem_pages[0xE800+i].writePtr,mem_pages[i].readPtr,256);
    }
     m65816_nmi();
}

void EMUL_handleWDM(byte parm)
{
    switch(parm) {
        case 0xC7 :    SMPT_prodosEntry();
                break;
        case 0xC8 :    SMPT_smartportEntry();
                break;
        case 0xFD :    m65816_setTrace(0);
                break;
        case 0xFE :    m65816_setTrace(1);
                break;
        case 0xFF :    schedulerStop(0);
                break;
        default :    break;
    }
}

static void displayUsage(char *myname)
{
    fprintf(stderr,"\nUsage: %s [-s5d1 filename ] [-s5d2 filename ] [-s6d1 filename ] [-s6d2 filename ] [-smpt# filename ] [-ram # ] [-trace]\n",myname);
    exit(1);
}

/*
 *
 * Returns the full path to a resource file. Searches xgs_user_dir first, and then xgs_data_dir.
 */

char *globalGetResource(const char *filename) {
    int len;
    char *path;

    len = strlen(filename) + strlen(data_dir) + 2;
    path = malloc(len);

    return path;
}

/*
 * Returns the full path to a user file.
 */

char *globalGetData(const char *filename) {
}

/*
char *EMUL_expandPath(const char *path)
{

    snprintf(output, len, "%s/%s", emul_path, path);

    return output;
}
*/
