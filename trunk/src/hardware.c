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
 * File: hardware.c
 *
 * Generic hardware functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int hardwareInit() {
    int err;

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

    if ((err = schedulerInit())) {
        IWM_shutdown();
        CLK_shutdown();
        ADB_shutdown();
        SND_shutdown();
        VID_shutdown();
        MEM_shutdown();
        return err;
    }

    g_vbl_count = 0;
    g_vblirq_enable = 0;
    g_qtrsecirq_enable = 0;
    g_onesecirq_enable = 0;
    g_fastmode = 1;

    m65816_init();

    schedulerSetCPUSpeed(2.5);
    hardwareReset();

    return 0;
}

void hardwareBigTick(const long bigtick)
{
    g_vbl_count++;

    if (g_vbl_count == 0x00) {
        if (g_vblirq_enable) {
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

    if ((bigtick == 0) || (bigtick == 15) || (bigtick == 30) || (bigtick == 45)) {
        if (g_qtrsecirq_enable) {
            if (!(mem_diagtype & 0x10)) {
                mem_diagtype |= 0x10;
                m65816_addIRQ();
            }
        }
    }

    if (g_onesecirq_enable && !bigtick) {
        if (!(vid_vgcint & 0x40)) {
            vid_vgcint |= 0xC0;
            m65816_addIRQ();
        }
    }
}

void hardwareTick(const long tick, const long bigtick)
{
    SND_update();

    if (tick == 400) {
        g_vbl_count = 0x80;
        vid_vert_cnt = tick >> 1;
    } else if (tick == MAX_TICKS) {
        vid_vert_cnt = 0;
        g_vbl_count = 0x00;
        hardwareBigTick(bigtick);
    } else {
        vid_vert_cnt = tick >> 1;
    }

    if (g_scanirq_enable && (vid_vert_cnt < 200) && (slow_memory[0x019D00 + vid_vert_cnt] & 0x40)) {
        if (!(vid_vgcint & 0x20)) {
            vid_vgcint |= 0xA0;
            m65816_addIRQ();
        }
    }
}

void hardwareReset()
{
    MEM_reset();
    VID_reset();
    SND_reset();
    ADB_reset();
    CLK_reset();
    IWM_reset();
    SMPT_reset();
    m65816_reset();
}

void hardwareShutdown()
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

byte EMUL_getVBL(byte val)
{
    return g_vbl_count;
}
