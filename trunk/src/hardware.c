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

#include "xgs.h"

#include "adb.h"
#include "clock.h"
#include "disks.h"
#include "events.h"
#include "hardware.h"
#include "iwm.h"
#include "smtport.h"
#include "scheduler.h"
#include "sound.h"
#include "video.h"

static int in_vbl;

int hardwareInit()
{
    int err;

    if ((err = memoryInit())) return err;
    if ((err = videoInit())) {
        memoryShutdown();
        return err;
    }
    if ((err = soundInit())) {
        videoShutdown();
        memoryShutdown();
        return err;
    }
    if ((err = eventsInit())) {
        videoShutdown();
        memoryShutdown();
        return err;
    }
    if ((err = ADB_init())) {
        eventsShutdown();
        soundShutdown();
        videoShutdown();
        memoryShutdown();
        return err;
    }
    if ((err = clockInit())) {
        ADB_shutdown();
        eventsShutdown();
        soundShutdown();
        videoShutdown();
        memoryShutdown();
        return err;
    }
    if ((err = IWM_init())) {
        clockShutdown();
        ADB_shutdown();
        eventsShutdown();
        soundShutdown();
        videoShutdown();
        memoryShutdown();
        return err;
    }

    if ((err = SMPT_init())) {
        IWM_shutdown();
        clockShutdown();
        ADB_shutdown();
        eventsShutdown();
        soundShutdown();
        videoShutdown();
        memoryShutdown();
        return err;
    }

    if ((err = schedulerInit())) {
        IWM_shutdown();
        clockShutdown();
        ADB_shutdown();
        eventsShutdown();
        soundShutdown();
        videoShutdown();
        memoryShutdown();
        return err;
    }

    in_vbl = 0;

    g_vblirq_enable = 0;
    g_qtrsecirq_enable = 0;
    g_onesecirq_enable = 0;
    g_fastmode = 1;

    m65816_init();

    schedulerSetTargetSpeed(2.5);
    hardwareReset();

    return 0;
}

void hardwareBigTick(const long bigtick)
{
    g_vbl_count++;

    if (in_vbl == 0x00) {
        if (g_vblirq_enable) {
            if (!(mem_diagtype & 0x08)) {
                mem_diagtype |= 0x08;
                m65816_addIRQ();
            }
        }
        videoUpdate();
        IWM_update();
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
    eventsUpdate();
    soundUpdate();

    if (tick == 400) {
        in_vbl = 0x80;
        vid_vert_cnt = tick >> 1;
    } else if (tick == (MAX_TICKS - 1)) {
        vid_vert_cnt = 0;
        in_vbl = 0x00;
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
    memoryReset();
    videoReset();
    soundReset();
    ADB_reset();
    clockReset();
    IWM_reset();
    SMPT_reset();
    m65816_reset();
}

void hardwareShutdown()
{
    SMPT_shutdown();
    IWM_shutdown();
    clockShutdown();
    ADB_shutdown();
    eventsShutdown();
    soundShutdown();
    videoShutdown();
    memoryShutdown();
}

void hardwareSetTrace(int parm)
{
    m65816_setTrace(parm);
}

void hardwareRaiseNMI()
{
    int    i;

    for (i = 0x0000 ; i < 0x0200 ; i++) {
        if (!mem_pages[i].readPtr) continue;
        memcpy(mem_pages[0xE800+i].writePtr,mem_pages[i].readPtr,256);
    }
     m65816_nmi();
}

void hardwareHandleWDM(byte parm)
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

byte hardwareInVBL()
{
    return in_vbl;
}
