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
 * File: iwm_c
 *
 * Emulates the IWM chip and four drives (two 3.5" in slot 5 and
 * two 5.25" in slot 6).
 *
 * Parts of this code are written and Copyright 1996 by Kent Dickey.
 * Used with permission.
 */

#include "xgs.h"

#include "disks.h"
#include "hardware.h"
#include "iwm.h"

const byte iwm_phys_to_prodos_sec[] = {
    0x00, 0x08, 0x01, 0x09, 0x02, 0x0A, 0x03, 0x0B,
    0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F
};

const byte iwm_phys_to_dos_sec[] = {
    0x00, 0x07, 0x0E, 0x06, 0x0D, 0x05, 0x0C, 0x04,
    0x0B, 0x03, 0x0A, 0x02, 0x09, 0x01, 0x08, 0x0F
};

const byte iwm_disk_bytes[] = {
    0x96, 0x97, 0x9A, 0x9B,  0x9D, 0x9E, 0x9F, 0xA6, /* 0x00 */
    0xA7, 0xAB, 0xAC, 0xAD,  0xAE, 0xAF, 0xB2, 0xB3,
    0xB4, 0xB5, 0xB6, 0xB7,  0xB9, 0xBA, 0xBB, 0xBC, /* 0x10 */
    0xBD, 0xBE, 0xBF, 0xCB,  0xCD, 0xCE, 0xCF, 0xD3,
    0xD6, 0xD7, 0xD9, 0xDA,  0xDB, 0xDC, 0xDD, 0xDE, /* 0x20 */
    0xDF, 0xE5, 0xE6, 0xE7,  0xE9, 0xEA, 0xEB, 0xEC,
    0xED, 0xEE, 0xEF, 0xF2,  0xF3, 0xF4, 0xF5, 0xF6, /* 0x30 */
    0xF7, 0xF9, 0xFA, 0xFB,  0xFC, 0xFD, 0xFE, 0xFF
};

const byte iwm_35track_len[80] = {
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8
};

const int iwm_35track_index[80] = {
    0,    12,  24,  36,  48,  60 , 72,  84,
    96,  108, 120, 132, 144, 156, 168, 180,

    192, 203, 214, 225, 236, 247, 258, 269,
    280, 291, 302, 313, 324, 335, 346, 357,

    368, 378, 388, 398, 408, 418, 428, 438,
    448, 458, 468, 478, 488, 498, 508, 518,

    528, 537, 546, 555, 564, 573, 582, 591,
    600, 609, 618, 627, 636, 645, 654, 663,

    672, 680, 688, 696, 704, 712, 720, 728,
    736, 744, 752, 760, 768, 776, 784, 792
};

int iwm_from_disk_byte[256];

int iwm_slot4_motor;
int iwm_slot5_motor;
int iwm_slot6_motor;
int iwm_slot7_motor;

int iwm_enable2_handshake = 0;

static int    g_nib_pos = 0;
static int    g_nib_cnt = 0;
static byte    *g_nibptr;
static byte    *g_startnibptr;
static int    g_track_len;

int    iwm_last_cycle_count = 0;

Disk525    iwm_drive525[2];
Disk35    iwm_drive35[2];
int    iwm_motor_on;
int    iwm_motor_off;
int    iwm_motor_off_vbl_count;
int    iwm_phase[4];
int    iwm_mode;
int    iwm_drive_select;
int    iwm_q6;
int    iwm_q7;
int    iwm_enable2;
int    iwm_reset;
int    iwm_35sel = 0;
int    iwm_35ctl = 0;

byte    iwm_track_buf[6656];
byte    iwm_nib_buf[699];

int IWM_init()
{
    int    val;
    int    i;

    for (i = 0; i < 2; i++) {
        iwm_drive525[i].disk = NULL;
        iwm_drive525[i].cur_qtr_track = 0;
        iwm_drive525[i].vol_num = 254;
        iwm_drive525[i].time_last_read = 0;
        iwm_drive525[i].last_phase = 0;
        iwm_drive35[i].disk = NULL;
        iwm_drive35[i].cur_track = 0;
    }

    for (i = 0 ; i < 256 ; i++) {
        iwm_from_disk_byte[i] = -1;
    }
    for (i = 0 ; i < 64 ; i++) {
        val = iwm_disk_bytes[i];
        iwm_from_disk_byte[val] = i;
    }

    return 0;
}

void IWM_shutdown()
{
}

void IWM_reset()
{
    iwm_slot4_motor = 0;
    iwm_slot5_motor = 0;
    iwm_slot6_motor = 0;
    iwm_slot7_motor = 0;

    iwm_q6 = 0;
    iwm_q7 = 0;
    iwm_motor_on = 0;
    iwm_motor_off = 0;
    iwm_motor_off_vbl_count = 0;
    iwm_drive_select = 0;
    iwm_mode = 0;
    iwm_enable2 = 0;
    iwm_reset = 0;
    iwm_phase[0] = 0;
    iwm_phase[1] = 0;
    iwm_phase[2] = 0;
    iwm_phase[3] = 0;

    iwm_drive35[0].step = 0;
    iwm_drive35[0].motor_on = 0;
    iwm_drive35[1].step = 0;
    iwm_drive35[1].motor_on = 0;
}

void IWM_update()
{
    byte    buffer[0x1800];
    Disk525    *dsk;
    Disk35    *dsk2;
    Track    *trk;
    int    ret,track_num,head,i,j,start,len;

    if (iwm_motor_on && iwm_motor_off) {
        if (iwm_motor_off_vbl_count <= g_vbl_count) {
            iwm_motor_on = 0;
            iwm_motor_off = 0;
        }
    }

    if (iwm_motor_on == 0 || iwm_motor_off) {
        for (i = 0; i < 2; i++) {
            dsk = &(iwm_drive525[i]);
            if (!dsk->disk) continue;
            for (j = 0; j < 4*35; j++) {
                if (j & 3) break;
                trk = &(dsk->track[j]);
                track_num = j >> 2;

                if (!trk->track_valid) continue;
                
                if (trk->track_dirty) {
                    trk->track_dirty = 0;
                    ret = IWM_trackToUnix525(dsk, trk, track_num, &(buffer[0]));
                    if (ret != 1) break;

                    ret = DSK_writeChunk(dsk->disk, buffer, track_num * 8, 8);
                }
                if (j != dsk->cur_qtr_track) {
                    trk->track_valid = 0;
                    free(trk->nib_area);
                    trk->nib_area = NULL;
                }
            }
        }
        for (i = 0; i < 2; i++) {
            dsk2 = &(iwm_drive35[i]);
            if (!dsk2->disk) continue;
            for (j = 0; j < 80*2 ; j++) {
                trk = &(dsk2->track[j]);
                track_num = j >> 1;
                head = j & 1;

                if (!trk->track_valid) continue;
                
                if (trk->track_dirty) {
                    trk->track_dirty = 0;
                    ret = IWM_trackToUnix35(dsk2, trk, track_num, head, &(buffer[0]));
                    if (ret != 1) break;

                    len = iwm_35track_len[track_num];
                    start = iwm_35track_index[track_num] * 2 + (head * len);
                    ret = DSK_writeChunk(dsk2->disk, buffer, start, len);
                }
                if (j != (dsk2->cur_track * 2 + dsk2->head)) {
                    trk->track_valid = 0;
                    free(trk->nib_area);
                    trk->nib_area = NULL;
                }
            }
        }
    }
}

int IWM_loadDrive(int slot, int drive, char *path)
{
    disk_struct    *image;
    int        i,err;

    if ((drive != 1) && (drive != 2)) return -1;
    if (slot == 5) {
        drive--;
        if ((err = DSK_openImage(path, &image))) {
            iwm_drive35[drive].disk = NULL;
            return err;
        }
        if (image->header.num_blocks != 1600) {
            DSK_closeImage(image);
            return -2;
        }
        if (image->header.image_format != IMAGE_IF_RAW_PO) {
            DSK_closeImage(image);
            return -2;
        }
        iwm_drive35[drive].disk = image;
        iwm_drive35[drive].disk_switched = 1;
        iwm_drive35[drive].nib_pos = 0;
        for (i = 0 ; i < 80*2 ; i++) {
            iwm_drive35[drive].track[i].track_valid = 0;
            iwm_drive35[drive].track[i].track_dirty = 0;
        }
        return 0;
    } else if (slot == 6) {
        drive--;
        if ((err = DSK_openImage(path, &image))) {
            iwm_drive525[drive].disk = NULL;
            return err;
        }
        if (image->header.num_blocks != 280) {
            DSK_closeImage(image);
            return -2;
        }
        iwm_drive525[drive].disk = image;
        iwm_drive525[drive].nib_pos = 0;
        for (i = 0 ; i < 35*4 ; i++) {
            iwm_drive525[drive].track[i].track_valid = 0;
            iwm_drive525[drive].track[i].track_dirty = 0;
        }
        return 0;
    } else {
        return -1;
    }
}

int IWM_unloadDrive(int slot, int drive)
{
    IWM_update();
    if ((drive != 1) && (drive != 2)) return -1;
    if (slot == 5) {
        drive--;
        if (iwm_drive35[drive].disk) {
            DSK_closeImage(iwm_drive35[drive].disk);
        }
        iwm_drive35[drive].disk = NULL;
        return 0;
    } else if (slot == 6) {
        drive--;
        if (iwm_drive525[drive].disk) {
            DSK_closeImage(iwm_drive525[drive].disk);
        }
        iwm_drive525[drive].disk = NULL;
        return 0;
    } else {
        return -1;
    }
}

byte IWM_getDiskReg(byte val)
{
    val = 0;
    if (iwm_35ctl) val |= 0x80;
    if (iwm_35sel) val |= 0x40;
    return val;
}

byte IWM_setDiskReg(byte val)
{
    iwm_35ctl = (val & 0x80)? 1 : 0;
    iwm_35sel = (val & 0x40)? 1 : 0;
    return 0;
}

void IWM_touchSwitches(int loc)
{
    int    phase_up;
    int    phase_down;
    int    phase;
    int    on;
    int    drive;

    on = loc & 1;
    drive = iwm_drive_select;
    phase = loc >> 1;
    if (loc < 8) {
        iwm_phase[phase] = on;
        phase_up = (phase - 1) & 3;
        phase_down = (phase + 1) & 3;

        if (iwm_motor_on) {
            if (iwm_35sel) {
                if (phase == 3 && on) {
                    IWM_doAction35();
                }
            } else if (on) {
                IWM_phaseChange525(drive, phase);
            }
        } else {
            /* See if enable or reset is asserted */
            if (iwm_phase[0] && iwm_phase[2]) {
                iwm_reset = 1;
            } else {
                iwm_reset = 0;
            }
            if (iwm_phase[1] && iwm_phase[3]) {
                iwm_enable2 = 1;
            } else {
                iwm_enable2 = 0;
            }
        }
    } else {
        /* loc >= 8 */
        switch(loc) {
        case 0x08:
            if (iwm_mode & 0x04) {
                /* Turn off immediately */
                iwm_motor_off = 0;
                iwm_motor_on = 0;
            } else {
                /* 1 second delay */
                if (iwm_motor_on && !iwm_motor_off) {
                    iwm_motor_off = 1;
                    iwm_motor_off_vbl_count = g_vbl_count
                                    + 60;
                }
            }
            break;
        case 0x09:
            iwm_motor_on = 1;
            iwm_motor_off = 0;
            break;
        case 0x0A:
        case 0x0B:
            iwm_drive_select = on;
            break;
        case 0x0C:
        case 0x0D:
            iwm_q6 = on;
            break;
        case 0x0E:
        case 0x0F:
            iwm_q7 = on;
            break;
        default:
            break;
        }
    }
}

void IWM_phaseChange525(int drive, int phase)
{
    int    qtr_track;
    int    last_phase;
    int    phase_up;
    int    phase_down;
    int    delta;

    phase_up = (phase - 1) & 3;
    phase_down = (phase + 1) & 3;
    last_phase = iwm_drive525[drive].last_phase;

    qtr_track = iwm_drive525[drive].cur_qtr_track;

    delta = 0;
    if (last_phase == phase_up) {
        delta = 2;
        last_phase = phase;
    } else if (last_phase == phase_down) {
        delta = -2;
        last_phase = phase;
    }

    qtr_track += delta;
    if (qtr_track < 1) {
        qtr_track = 0;
        last_phase = 0;
    }
    if (qtr_track > 4*34) {
        qtr_track = 4*34;
        last_phase = 2;
    }

    iwm_drive525[drive].cur_qtr_track = qtr_track;
    iwm_drive525[drive].last_phase = last_phase;
}

int IWM_readStatus35()
{
    int    drive;
    int    state;

    drive = iwm_drive_select;

    if (iwm_motor_on) {
        /* Read status */
        state = (iwm_phase[2] << 3) + (iwm_phase[1] << 2) +
            (iwm_phase[0] << 1) + iwm_35ctl;

        switch(state) {
            case 0x00:    /* step direction */
                    return iwm_drive35[drive].step;
                    break;
            case 0x01:    /* disk in place */
                    /* 1 = no disk, 0 = disk */
                    return (iwm_drive35[drive].disk == NULL);
                    break;
            case 0x02:    /* disk is stepping */
                    return 1;
                    break;
            case 0x03:    /* disk is locked */
                    if (iwm_drive35[drive].disk) {
                        return (((iwm_drive35[drive].disk)->header.flags | IMAGE_FL_LOCKED) != 0);
                    } else {
                        return 0;
                    }
                    break;
            case 0x04:    /* motor on */
                    return (iwm_drive35[drive].motor_on != 0);
                    break;
            case 0x05:    /* at track 0 */
                    /* 1 = not at track 0, 0 = there */
                    return (iwm_drive35[drive].cur_track != 0);
                    break;
            case 0x06:    /* disk switched */
                    return (iwm_drive35[drive].disk_switched != 0);
                    break;
            case 0x07:    /* tachometer */
                    return (g_cpu_cycles & 1);
                    break;
            case 0x08:    /* lower head activate */
                    iwm_drive35[drive].head = 0;
                    return 0;
                    break;
            case 0x09:    /* upper head activate */
                    iwm_drive35[drive].head = 1;
                    return 0;
                    break;
            case 0x0C:    /* number of sides */
                    return 1;
                    break;
            case 0x0D:    /* disk ready */
                    return 0;
                    break;
            case 0x0F:    /* drive installed */
                    return 0;
                    break;
            default:
                    return 1;
        }
    } else {
        return 1;
    }
}

void IWM_doAction35()
{
    int    drive;
    int    state;
    int    track;

    drive = iwm_drive_select;
    if (iwm_motor_on) {
        /* Perform action */
        state = (iwm_phase[1] << 3) + (iwm_phase[0] << 2) +
            (iwm_35ctl << 1) + iwm_phase[2];
        switch(state) {
            case 0x00:    /* Set step direction inward */
                    /* towards higher tracks */
                    iwm_drive35[drive].step = 0;
                    break;
            case 0x01:    /* Set step direction outward */
                    /* towards lower tracks */
                    iwm_drive35[drive].step = 1;
                    break;
            case 0x03:    /* reset disk-switched flag? */
                    iwm_drive35[drive].disk_switched = 0;
                    break;
            case 0x04:    /* step disk */
                    if (iwm_drive35[drive].step) {
                        track = iwm_drive35[drive].cur_track;
                        if (track) track--;
                        iwm_drive35[drive].cur_track = track;
                    } else {
                        track = iwm_drive35[drive].cur_track;
                        if (track < 79) track++;
                        iwm_drive35[drive].cur_track = track;

                    }
                    break;
            case 0x08:    /* turn motor on */
                    iwm_drive35[drive].motor_on = 1;
                    break;
            case 0x09:    /* turn motor off */
                    iwm_drive35[drive].motor_on = 0;
                    break;
            case 0x0D:    /* eject disk */
                    IWM_unloadDrive(5, drive + 1);
                    break;
            default:    return;
        }
    } else {
        return;
    }
}

byte IWM_readLoc(int loc)
{
    word32    status;
    int    on;
    int    state;
    int    drive;
    int    val;
    Disk35    *dsk;

    loc = loc & 0x0F;
    on = loc & 1;

    IWM_touchSwitches(loc);

    state = (iwm_q7 << 1) + iwm_q6;
    drive = iwm_drive_select;

    if (on) {
        /* odd address, return 0 */
        return 0;
    } else {
        switch(state) {
        case 0x00:    /* q7 = 0, q6 = 0 */
            if (iwm_enable2) {
                return IWM_readEnable2();
            } else {
                if (iwm_motor_on) {
                    if (iwm_35sel) {
                        return IWM_readData35();
                    } else {
                        return IWM_readData525();
                    }
                } else {
                    return 0xFF;
                }
            }
            break;
        case 0x01:    /* q7 = 0, q6 = 1 */
            /* read IWM status reg */
            if (iwm_enable2) {
                status = 1;
            } else {
                if (iwm_35sel) {
                    status = IWM_readStatus35();
                } else {
                    if (iwm_drive525[drive].disk) {
                        status = (iwm_drive525[drive].disk->header.flags & IMAGE_FL_LOCKED)? 1 : 0 ;
                    } else {
                        status = 1;
                    }
                }
            }

            val = (status << 7) | (iwm_motor_on << 5) |
                  iwm_mode;
            return val;
            break;
        case 0x02:    /* q7 = 1, q6 = 0 */
            /* read handshake register */
            dsk = &(iwm_drive35[drive]);

            if (iwm_enable2) {
                return IWM_readEnable2Handshake();
            } else {
                return 0x80;
            }
            break;
        case 0x03:    /* q7 = 1, q6 = 1 */
            return 0;
        break;
        }
    }
    return 0;
}

byte IWM_readC0E0(byte val)
{
    return IWM_readLoc(0x00);
}

byte IWM_readC0E1(byte val)
{
    return IWM_readLoc(0x01);
}

byte IWM_readC0E2(byte val)
{
    return IWM_readLoc(0x02);
}

byte IWM_readC0E3(byte val)
{
    return IWM_readLoc(0x03);
}

byte IWM_readC0E4(byte val)
{
    return IWM_readLoc(0x04);
}

byte IWM_readC0E5(byte val)
{
    return IWM_readLoc(0x05);
}

byte IWM_readC0E6(byte val)
{
    return IWM_readLoc(0x06);
}

byte IWM_readC0E7(byte val)
{
    return IWM_readLoc(0x07);
}

byte IWM_readC0E8(byte val)
{
    return IWM_readLoc(0x08);
}

byte IWM_readC0E9(byte val)
{
    return IWM_readLoc(0x09);
}

byte IWM_readC0EA(byte val)
{
    return IWM_readLoc(0x0A);
}

byte IWM_readC0EB(byte val)
{
    return IWM_readLoc(0x0B);
}

byte IWM_readC0EC(byte val)
{
    return IWM_readLoc(0x0C);
}

byte IWM_readC0ED(byte val)
{
    return IWM_readLoc(0x0D);
}

byte IWM_readC0EE(byte val)
{
    return IWM_readLoc(0x0E);
}

byte IWM_readC0EF(byte val)
{
    return IWM_readLoc(0x0F);
}

byte IWM_writeLoc(int loc, int val)
{
    int    on;
    int    state;

    loc = loc & 0x0F;
    on = loc & 1;

    IWM_touchSwitches(loc);

    state = (iwm_q7 << 1) + iwm_q6;

    if (on) {
        /* odd address, write something */
        if (state == 0x03) {
            /* q7, q6 = 1,1 */
            if (iwm_motor_on) {
                if (iwm_enable2) {
                    IWM_writeEnable2(val);
                } else {
                    if (iwm_35sel) {
                        IWM_writeData35(val);
                    } else {
                        IWM_writeData525(val);
                    }
                }
            } else {
                /* write mode register */
                val = val & 0x1F;
                iwm_mode = val;
            }
        } else {
            if (iwm_enable2) {
                IWM_writeEnable2(val);
            } else {
            }
        }
        return 0;
    } else {
        /* even address */
        if (iwm_enable2) {
            IWM_writeEnable2(val);
        } else {
        }
        return 0;
    }

    return 0;

}

byte IWM_writeC0E0(byte val)
{
    return IWM_writeLoc(0x00, val);
}

byte IWM_writeC0E1(byte val)
{
    return IWM_writeLoc(0x01, val);
}

byte IWM_writeC0E2(byte val)
{
    return IWM_writeLoc(0x02, val);
}

byte IWM_writeC0E3(byte val)
{
    return IWM_writeLoc(0x03, val);
}

byte IWM_writeC0E4(byte val)
{
    return IWM_writeLoc(0x04, val);
}

byte IWM_writeC0E5(byte val)
{
    return IWM_writeLoc(0x05, val);
}

byte IWM_writeC0E6(byte val)
{
    return IWM_writeLoc(0x06, val);
}

byte IWM_writeC0E7(byte val)
{
    return IWM_writeLoc(0x07, val);
}

byte IWM_writeC0E8(byte val)
{
    return IWM_writeLoc(0x08, val);
}

byte IWM_writeC0E9(byte val)
{
    return IWM_writeLoc(0x09, val);
}

byte IWM_writeC0EA(byte val)
{
    return IWM_writeLoc(0x0A, val);
}

byte IWM_writeC0EB(byte val)
{
    return IWM_writeLoc(0x0B, val);
}

byte IWM_writeC0EC(byte val)
{
    return IWM_writeLoc(0x0C, val);
}

byte IWM_writeC0ED(byte val)
{
    return IWM_writeLoc(0x0D, val);
}

byte IWM_writeC0EE(byte val)
{
    return IWM_writeLoc(0x0E, val);
}

byte IWM_writeC0EF(byte val)
{
    return IWM_writeLoc(0x0F, val);
}

int IWM_readEnable2()
{
    return 0xFF;
}

int IWM_readEnable2Handshake()
{
    int    val;

    val = 0xC0;
    iwm_enable2_handshake++;
    if (iwm_enable2_handshake > 3) {
        iwm_enable2_handshake = 0;
        val = 0x80;
    }
    return val;
}

void IWM_writeEnable2(int val)
{
    return;
}

int IWM_readData525()
{
    Disk525    *dsk;
    Track    *trk;
    word32    last_read;
    word32    bits_read;
    int    pos;
    int    size;
    int    drive;
    int    skip_nibs;
    int    track_len;
    byte    ret;

    drive = iwm_drive_select;
    dsk = &(iwm_drive525[drive]);

    if (!dsk->disk) return (g_cpu_cycles & 0xFF) | 0x80;

    IWM_unixToNib525(dsk, dsk->cur_qtr_track >> 2);

    trk = &(dsk->track[dsk->cur_qtr_track]);
    if (!trk->track_valid) return (g_cpu_cycles & 0xFF) | 0x80;

    last_read = dsk->time_last_read;
    track_len = trk->track_len;

    bits_read = (g_cpu_cycles - last_read) >> 2;
    pos = dsk->nib_pos;
    size = trk->nib_area[pos];

    while (!size) {
        pos += 2;
        if (pos >= track_len) pos = 0;
        size = trk->nib_area[pos];
    }

    if (bits_read > (size + 2)) {
        /* skip some bits? */
        bits_read -= (size + 2);
        skip_nibs = bits_read >> 3;
        pos += skip_nibs*2;
        if (pos >= track_len) {
            pos = pos - track_len;
            if (pos >= track_len) {
                pos = pos % track_len;
            }
        }

        last_read += (skip_nibs * 32);

        if (bits_read >= 10 || g_cpu_cycles - last_read > 60) {
            /* We're way off, adjust last_read */
            last_read = (g_cpu_cycles - 4*8) & ( ~ 0x1F);
            dsk->time_last_read = last_read;
        }

        bits_read = 8;

        size = trk->nib_area[pos];
    }

    while (!size) {
        pos += 2;
        if (pos >= track_len) pos = 0;
        size = trk->nib_area[pos];
    }

    if (bits_read < size) {
        ret = trk->nib_area[pos+1] >> (size - bits_read);
    } else {
        ret = trk->nib_area[pos+1];
        pos += 2;
        if (pos >= track_len) pos = 0;
        dsk->time_last_read = last_read + size*4;
    }

    dsk->nib_pos = pos;

    return ret;
}

void IWM_writeData525(int val)
{
    Disk525    *dsk;
    Track    *trk;
    word32    last_read;
    word32    bits_read;
    int    size;
    int    drive;

    drive = iwm_drive_select;
    dsk = &(iwm_drive525[drive]);
    if (!dsk->disk) return;

    IWM_unixToNib525(dsk, dsk->cur_qtr_track >> 2);

    trk = &(dsk->track[dsk->cur_qtr_track]);
    if (!trk->track_valid) return;

    trk->track_dirty = 1;

    last_read = dsk->time_last_read;

    bits_read = (g_cpu_cycles - last_read) >> 2;

    size = bits_read;

    if (!val) val = g_cpu_cycles & 0xFF;

    IWM_nibOut525(dsk, trk, val, bits_read);

    dsk->time_last_read = last_read + size*4;
}

void IWM_nibblize525(byte *in, byte *nib_ptr)
{
    byte    *aux_buf;
    byte    *nib_out;
    int    val;
    int    val2;
    int    x;
    int    i;

    /* Convert 256(+1) data bytes to 342+1 disk nibbles */

    aux_buf = nib_ptr;
    nib_out = nib_ptr + 0x56;

    for (i = 0; i < 0x56; i++) {
        aux_buf[i] = 0;
    }

    x = 0x55;
    for (i = 0x101; i >= 0; i--) {
        val = in[i];
        if (i >= 0x100) {
            val = 0;
        }
        val2 = (aux_buf[x] << 1) + (val & 1);
        val = val >> 1;
        val2 = (val2 << 1) + (val & 1);
        val = val >> 1;
        nib_out[i] = val;
        aux_buf[x] = val2;
        x--;
        if (x < 0) {
            x = 0x55;
        }
    }
}

int IWM_unnib525()
{
    int    val;
    int    size;

    size = 0;
    while (!size) {
        size = g_nibptr[0];
        val = g_nibptr[1];
        g_nib_pos += 2;
        g_nib_cnt += 2;
        g_nibptr += 2;
        if (g_nib_pos > g_track_len) {
            g_nib_pos = 0;
            g_nibptr = g_startnibptr;
        }
    }

    return val;
}

int IWM_unnib525_4x4(void)
{
    int    val1;
    int    val2;

    val1 = IWM_unnib525();
    val2 = IWM_unnib525();

    return ((val1 << 1) + 1) & val2;
}

int IWM_trackToUnix525(Disk525 *dsk, Track *trk, int track_num, byte *outbuf)
{
    byte    aux_buf[0x80];
    byte    *buf;
    int    sector_done[16];
    int    num_sectors_done;
    int    start_pos;
    int    track_len;
    int    vol, track, phys_sec, log_sec, cksum;
    int    val;
    int    val2;
    int    prev_val;
    int    x;
    int    i;

    track_len = trk->track_len;
    g_track_len = track_len;
    g_nibptr = &(trk->nib_area[0]);
    g_startnibptr = g_nibptr;
    g_nib_pos = 0;
    g_nib_cnt = 0;
    start_pos = -1;

    for (i = 0; i < 16; i++) {
        sector_done[i] = 0;
    }

    num_sectors_done = 0;

    val = 0;
    while (g_nib_cnt < 2*track_len) {
        /* look for start of a sector */
        if (val != 0xD5) {
            val = IWM_unnib525();
            continue;
        }

        val = IWM_unnib525();
        if (val != 0xAA) {
            continue;
        }

        val = IWM_unnib525();
        if (val != 0x96) {
            continue;
        }

        /* It's a sector start */
        vol = IWM_unnib525_4x4();
        track = IWM_unnib525_4x4();
        phys_sec = IWM_unnib525_4x4();
        if (phys_sec < 0 || phys_sec > 15) {
            return -1;
        }
        if (dsk->disk->header.image_format == IMAGE_IF_RAW_DO) {
            log_sec = iwm_phys_to_dos_sec[phys_sec];
        } else {
            log_sec = iwm_phys_to_prodos_sec[phys_sec];
        }
        cksum = IWM_unnib525_4x4();
        if ((vol ^ track ^ phys_sec ^ cksum) != 0) {
            /* not correct format */
            return -1;
        }

        /* see what sector it is */
        if (track != track_num || (phys_sec < 0) || (phys_sec > 15)) {
            return -1;
        }

        if (sector_done[phys_sec]) {
            return -1;
        }

        /* So far so good, let's do it! */
        for (i = 0; i < 38; i++) {
            val = IWM_unnib525();
            if (val == 0xD5) {
                break;
            }
        }
        if (val != 0xD5) {
            return -1;
        }

        val = IWM_unnib525();
        if (val != 0xAA) {
            return -1;
        }

        val = IWM_unnib525();
        if (val != 0xAD) {
            return -1;
        }

        buf = outbuf + 0x100*log_sec;

        /* Data start! */
        prev_val = 0;
        for (i = 0x55; i >= 0; i--) {
            val = IWM_unnib525();
            val2 = iwm_from_disk_byte[val];
            if (val2 < 0) {
                return -1;
            }
            prev_val = val2 ^ prev_val;
            aux_buf[i] = prev_val;
        }

        /* rest of data area */
        for (i = 0; i < 0x100; i++) {
            val = IWM_unnib525();
            val2 = iwm_from_disk_byte[val];
            if (val2 < 0) {
                return -1;
            }
            prev_val = val2 ^ prev_val;
            buf[i] = prev_val;
        }

        /* checksum */
        val = IWM_unnib525();
        val2 = iwm_from_disk_byte[val];
        if (val2 < 0) {
            return -1;
        }
        if (val2 != prev_val) {
            return -1;
        }

        /* Got this far, data is good, merge aux_buf into buf */
        x = 0x55;
        for (i = 0; i < 0x100; i++) {
            val = aux_buf[x];
            val2 = (buf[i] << 1) + (val & 1);
            val = val >> 1;
            val2 = (val2 << 1) + (val & 1);
            buf[i] = val2;
            val = val >> 1;
            aux_buf[x] = val;
            x--;
            if (x < 0) {
                x = 0x55;
            }
        }
        sector_done[phys_sec] = 1;
        num_sectors_done++;
        if (num_sectors_done >= 16) {
            return 1;
        }
    }

    return -1;
}

void IWM_unixToNib525(Disk525 *dsk, int track_num)
{
    word32    val;
    word32    last_val;
    Track    *trk;
    int    phys_sec;
    int    log_sec;
    int    num_sync;
    int    err;
    int    i;
    int    old_pos;
    
    /* Read track from dsk int iwm_track_buf */

    if (track_num > 34 || track_num < 0) return;

    trk = &(dsk->track[track_num * 4]);

    /* Return if it's already in memory */

    if (trk->nib_area) return;

    old_pos = dsk->nib_pos;
    dsk->nib_pos = 0;

    trk->track_valid = 0;
    trk->track_dirty = 0;
    trk->overflow_size = 0;

    trk->nib_area = malloc(LEN_NIB_AREA_525);

    if (dsk->disk->header.image_format == IMAGE_IF_NIBBLE) {
        err = DSK_readChunk(dsk->disk, iwm_track_buf, track_num, 1);
        if (err) {
            dsk->nib_pos = old_pos;
            return;
        }

        trk->track_len = LEN_NIB_IMG_TRK_525 * 2;
        for (i = 0 ; i < trk->track_len ; i += 2) {
            trk->nib_area[i] = 8;
            trk->nib_area[i+1] = iwm_track_buf[i >> 1];
        }
    } else {
        err = DSK_readChunk(dsk->disk, iwm_track_buf, track_num * 8, 8);
        if (err) {
            dsk->nib_pos = old_pos;
            return;
        }

        /* create nibblized image */

        trk->track_len = AVG_NIB_AREA_525;
        for (i = 0 ; i < trk->track_len ; i += 2) {
            trk->nib_area[i] = 8;
            trk->nib_area[i+1] = 0xFF;
        }

        for (phys_sec = 0; phys_sec < 16; phys_sec++) {
            if (dsk->disk->header.image_format == IMAGE_IF_RAW_DO) {
                log_sec = iwm_phys_to_dos_sec[phys_sec];
            } else {
                log_sec = iwm_phys_to_prodos_sec[phys_sec];
            }

            /* Create sync headers */
            if (!phys_sec) {
                num_sync = 70;
            } else {
                num_sync = 14;
            }

            for (i = 0; i < num_sync; i++) {
                IWM_nibOut525(dsk, trk, 0xFF, 10);
            }
            IWM_nibOut525(dsk, trk, 0xD5, 8);    /* prolog */
            IWM_nibOut525(dsk, trk, 0xAA, 8);    /* prolog */
            IWM_nibOut525(dsk, trk, 0x96, 8);    /* prolog */
            IWM_4x4nibOut525(dsk, trk, dsk->vol_num);
            IWM_4x4nibOut525(dsk, trk, track_num);
            IWM_4x4nibOut525(dsk, trk, phys_sec);
            IWM_4x4nibOut525(dsk, trk, dsk->vol_num ^ track_num ^ phys_sec);
            IWM_nibOut525(dsk, trk, 0xDE, 8);    /* epi */
            IWM_nibOut525(dsk, trk, 0xAA, 8);    /* epi */
            IWM_nibOut525(dsk, trk, 0xEB, 8);    /* epi */

            /* Inter sync */
            IWM_nibOut525(dsk, trk, 0xFF, 8);
            for (i = 0; i < 5; i++) {
                IWM_nibOut525(dsk, trk, 0xFF, 10);
            }
            IWM_nibOut525(dsk, trk, 0xD5, 8); /* data prolog */
            IWM_nibOut525(dsk, trk, 0xAA, 8); /* data prolog */
            IWM_nibOut525(dsk, trk, 0xAD, 8); /* data prolog */

            IWM_nibblize525( &(iwm_track_buf[log_sec*256]), iwm_nib_buf);

            last_val = 0;
            for (i = 0; i < 0x156; i++) {
                val = iwm_nib_buf[i];
                IWM_nibOut525(dsk, trk, iwm_disk_bytes[last_val ^ val], 8);
                last_val = val;
            }
            IWM_nibOut525(dsk, trk, iwm_disk_bytes[last_val], 8);

            IWM_nibOut525(dsk, trk, 0xDE, 8); /* data epilog */
            IWM_nibOut525(dsk, trk, 0xAA, 8); /* data epilog */
            IWM_nibOut525(dsk, trk, 0xEB, 8); /* data epilog */
            IWM_nibOut525(dsk, trk, 0xFF, 8); /* data epilog */
            for (i = 0; i < 6; i++) {
                IWM_nibOut525(dsk, trk, 0xFF, 10);
            }
        }
    }

    trk->track_valid = 1;
    trk->track_dirty = 0;

    dsk->nib_pos = old_pos;
}

void IWM_4x4nibOut525(Disk525 *dsk, Track *trk, word32 val)
{
    IWM_nibOut525(dsk, trk, 0xAA | (val >> 1), 8);
    IWM_nibOut525(dsk, trk, 0xAA | val, 8);
}

void IWM_nibOut525(Disk525 *dsk, Track *trk, byte val, int size)
{
    int    pos;
    int    old_size;
    int    track_len;
    int    overflow_size;

    pos = dsk->nib_pos;
    track_len = trk->track_len;
    overflow_size = trk->overflow_size;

    old_size = trk->nib_area[pos];

    while (size >= (10 + old_size)) {
        size = size - old_size;
        pos += 2;
        if (pos >= track_len) {
            pos = 0;
        }
        old_size = trk->nib_area[pos];
    }

    if (size > 10)  size = 10;

    if (!(val & 0x80)) val |= 0x80;

    trk->nib_area[pos++] = size;
    trk->nib_area[pos++] = val;
    if (pos >= track_len) pos = 0;

    overflow_size += (size - old_size);
    if ((overflow_size > 8) && (size > 8)) {
        overflow_size -= trk->nib_area[pos];
        trk->nib_area[pos++] = 0;
        trk->nib_area[pos++] = 0;
        if (pos >= track_len) pos = 0;
    }

    dsk->nib_pos = pos;
    trk->overflow_size = overflow_size;
}

int IWM_readData35()
{
    Disk35    *dsk;
    Track    *trk;
    int    pos,drive;
    byte    ret;

    drive = iwm_drive_select;
    dsk = &(iwm_drive35[drive]);
    if (!dsk->disk) return (g_cpu_cycles & 0xFF) | 0x80;

    IWM_unixToNib35(dsk, dsk->cur_track);

    trk = &(dsk->track[dsk->cur_track * 2 + dsk->head]);
    if (!trk->track_valid) return (g_cpu_cycles & 0xFF) | 0x80;

    /* Assume everyone is reading 3.5" disks with latch mode enabled */

    pos = dsk->nib_pos;
    ret = trk->nib_area[pos + 1];
    pos += 2;
    if (pos >= trk->track_len) pos = 0;
    dsk->nib_pos = pos;

    return ret;
}

void IWM_writeData35(int val)
{
    Disk35    *dsk;
    Track    *trk;
    int    drive;

    drive = iwm_drive_select;
    dsk = &(iwm_drive35[drive]);
    if (!dsk->disk) return;

    IWM_unixToNib35(dsk, dsk->cur_track);

    trk = &(dsk->track[dsk->cur_track * 2 + dsk->head]);
    if (!trk->track_valid) return;
    trk->track_dirty = 1;

    if (!val) val = g_cpu_cycles & 0xFF;
    IWM_nibOut35(dsk, trk, val, 8);
}

void IWM_nibblize35(byte *in, byte *nib_ptr, byte *csum)
{
    int    i,j;
    word32    c1,c2,c3,c4;
    byte    val;
    byte    w1,w2,w3,w4;
    byte    b1[175],b2[175],b3[175];
    
    /* We always return 00s for the twelve 'tag' bytes in each of    */
    /* the 524-byte disk sectors, since the IIgs never uses them.    */

    b1[171] = 0;
    b1[172] = 0;
    b1[173] = 0;
    b1[174] = 0;
    b2[171] = 0;
    b2[172] = 0;
    b2[173] = 0;
    b2[174] = 0;
    b3[171] = 0;
    b3[172] = 0;
    b3[173] = 0;
    b3[174] = 0;

    /* Copy from the user's buffer to our buffer, while computing    */
    /* the three-byte data checksum.                */

    i = 0;
    j = 170;
    c1 = 0;
    c2 = 0;
    c3 = 0;
    while(1) {
        c1 = (c1 & 0xFF) << 1;
        if (c1 & 0x0100) c1++;

        val = in[i++];
        c3 += val;
        if (c1 & 0x0100) {
            c3++;
            c1 &= 0xFF;
        }
        b1[j] = (val ^ c1) & 0xFF;

        val = in[i++];
        c2 += val;
        if (c3 > 0xFF) {
            c2++;
            c3 &= 0xFF;
        }
        b2[j] = (val ^ c3) & 0xFF;

        if (--j < 0) break;

        val = in[i++];
        c1 += val;
        if (c2 > 0xFF) {
            c1++;
            c2 &= 0xFF;
        }
        b3[j+1] = (val ^ c2) & 0xFF;
    }
    c4 =  ((c1 & 0xC0) >> 6);
    c4 |= ((c2 & 0xC0) >> 4);
    c4 |= ((c3 & 0xC0) >> 2);

    i = 174;
    j = 0;
    while(i >= 0) {
        w1 = b1[i] & 0x3F;
        w2 = b2[i] & 0x3F;
        w3 = b3[i] & 0x3F;
        w4 =  ((b1[i] & 0xC0) >> 2);
        w4 |= ((b2[i] & 0xC0) >> 4);
        w4 |= ((b3[i] & 0xC0) >> 6);

        nib_ptr[j++] = w4;
        nib_ptr[j++] = w1;
        nib_ptr[j++] = w2;

        if (i) nib_ptr[j++] = w3;
        i--;
    }

    csum[0] = c1 & 0x3F;
    csum[1] = c2 & 0x3F;
    csum[2] = c3 & 0x3F;
    csum[3] = c4 & 0x3F;
}

int IWM_unnib35()
{
    int    val,size;

    size = 0;
    while (!size) {
        size = g_nibptr[0];
        val = g_nibptr[1];
        g_nib_pos += 2;
        g_nib_cnt += 2;
        g_nibptr += 2;
        if (g_nib_pos > g_track_len) {
            g_nib_pos = 0;
            g_nibptr = g_startnibptr;
        }
    }
    return val;
}

int IWM_trackToUnix35(Disk35 *dsk, Track *trk, int track_num, int head, byte *outbuf)
{
    int    sector_done[12],num_sectors_done,sector_count;
    int    start_pos,track_len;
    int    track, sector, side, format, cksum;
    int    i,j;
    word32    c1,c2,c3,c4;
    byte    b1[175],b2[175],b3[175];
    byte    denib[524];
    byte    val,val2;

    track_len = trk->track_len;
    g_track_len = track_len;
    g_nibptr = &(trk->nib_area[0]);
    g_startnibptr = g_nibptr;
    g_nib_pos = 0;
    g_nib_cnt = 0;
    start_pos = -1;
    sector_count = iwm_35track_len[track_num];

    num_sectors_done = 0;
    for (i = 0 ; i < sector_count ; i++) sector_done[i] = 0;

    val = 0;
    while (g_nib_cnt < 2*track_len) {
        if (val != 0xD5) {
            val = IWM_unnib35();
            continue;
        }

        val = IWM_unnib35();
        if (val != 0xAA) {
            continue;
        }

        val = IWM_unnib35();
        if (val != 0x96) {
            continue;
        }

        val = IWM_unnib35();
        track = iwm_from_disk_byte[val];
        val = IWM_unnib35();
        sector = iwm_from_disk_byte[val];
        val = IWM_unnib35();
        side = iwm_from_disk_byte[val];
        val = IWM_unnib35();
        format = iwm_from_disk_byte[val];
        val = IWM_unnib35();
        cksum = iwm_from_disk_byte[val];
        
        if ((track ^ sector ^ side ^ format ^ cksum)) return -1;

        if (side & 0x01) track |= 0x40;
        if (track != track_num) return -1;
        if (sector < 0 || sector >= sector_count) return -1;
        if ((side >> 5) != head) return -1;
        if (sector_done[sector]) return -1;

        for (i = 0; i < 38; i++) {
            val = IWM_unnib35();
            if (val == 0xD5) break;
        }
        if (val != 0xD5) return -1;

        val = IWM_unnib35();
        if (val != 0xAA) return -1;

        val = IWM_unnib35();
        if (val != 0xAD) return -1;

        val = IWM_unnib35();
        if (iwm_from_disk_byte[val] != sector) return -1;

        i = 174;
        while (i >= 0) {
            val = IWM_unnib35();
            val2 = iwm_from_disk_byte[val];

            val = IWM_unnib35();
            b1[i] = iwm_from_disk_byte[val]
                | ((val2 << 2) & 0xC0);
            val = IWM_unnib35();
            b2[i] = iwm_from_disk_byte[val]
                | ((val2 << 4) & 0xC0);
            if (i) {
                val = IWM_unnib35();
                b3[i] = iwm_from_disk_byte[val]
                    | ((val2 << 6) & 0xC0);
            }
            i--;
        }

        c1 = 0;
        c2 = 0;
        c3 = 0;
        i = 0;
        j = 174;
        while(1) {
            c1 = (c1 & 0xFF) << 1;
            if (c1 & 0x0100) c1++;

            val = b1[j];
            val = (val ^ c1) & 0xFF;
            c3 += val;
            if (c1 & 0x0100) {
                c3++;
                c1 &= 0xFF;
            }
            denib[i++] = val;

            val = b2[j];
            val = (val ^ c3) & 0xFF;
            c2 += val;
            if (c3 > 0xFF) {
                c2++;
                c3 &= 0xFF;
            }
            denib[i++] = val;

            if (--j < 0) break;

            val = b3[j+1];
            val = (val ^ c2) & 0xFF;
            c1 += val;
            if (c2 > 0xFF) {
                c1++;
                c2 &= 0xFF;
            }
            denib[i++] = val;
        }
        c4 =  ((c1 & 0xC0) >> 6);
        c4 |= ((c2 & 0xC0) >> 4);
        c4 |= ((c3 & 0xC0) >> 2);

        val = IWM_unnib35();
        val = iwm_from_disk_byte[val];
        if ((c4 & 0x3F) != val) return -1;
        val = IWM_unnib35();
        val = iwm_from_disk_byte[val];
        if ((c3 & 0x3F) != val) return -1;
        val = IWM_unnib35();
        val = iwm_from_disk_byte[val];
        if ((c2 & 0x3F) != val) return -1;
        val = IWM_unnib35();
        val = iwm_from_disk_byte[val];
        if ((c1 & 0x3F) != val) return -1;

        memcpy(outbuf + (sector * 512), denib + 12, 512);

        sector_done[sector] = 1;

        num_sectors_done++;
        if (num_sectors_done >= sector_count) return 1;
    }
    return -1;
}

void IWM_unixToNib35(Disk35 *dsk, int track_num)
{
    byte    csum[4];
    byte    val;
    Track    *trk;
    int    sec,side,sum,start,len,old_pos,i,err;
    
    if (track_num > 79 || track_num < 0) {
        return;
    }

    trk = &(dsk->track[track_num * 2 + dsk->head]);
    if (trk->track_valid) return;

    old_pos = dsk->nib_pos;
    dsk->nib_pos = 0;

    len = iwm_35track_len[track_num];
    start = iwm_35track_index[track_num] * 2 + (dsk->head * len);
    err = DSK_readChunk(dsk->disk, iwm_track_buf, start, len);
    if (err) {
        dsk->nib_pos = old_pos;
        return;
    }

    trk->track_dirty = 0;
    trk->track_len = LEN_NIB_SECTOR_35 * len;
    trk->nib_area = malloc(trk->track_len);

    for (i = 0; i < trk->track_len; i += 2) {
        trk->nib_area[i] = 8;
        trk->nib_area[i+1] = 0xFF;
    }

    /* create nibblized image */

    for (sec = 0 ; sec < len ; sec++) {
        side = dsk->head? 0x20 : 0x00;
        if (track_num & 0x40) side |= 0x01;
        sum = ((track_num & 0x3F) ^ sec ^ side ^ 0x22) & 0x3F;

        /* Thirteen 5-nibble selfsync fields */

        for (i = 0 ; i < 13 ; i++) {
            IWM_nibOut35(dsk, trk, 0xFF, 8);
            IWM_nibOut35(dsk, trk, 0xFF, 8);
            IWM_nibOut35(dsk, trk, 0xFF, 8);
            IWM_nibOut35(dsk, trk, 0xFF, 8);
            IWM_nibOut35(dsk, trk, 0xFF, 8);
        }

        IWM_nibOut35(dsk, trk, 0xFF, 8);    /* addr prolog */
        IWM_nibOut35(dsk, trk, 0xD5, 8);    /* addr prolog */
        IWM_nibOut35(dsk, trk, 0xAA, 8);    /* addr prolog */
        IWM_nibOut35(dsk, trk, 0x96, 8);    /* addr prolog */
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[track_num & 0x3F], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[sec], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[side], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[0x22], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[sum], 8);
        IWM_nibOut35(dsk, trk, 0xDE, 8);    /* addr epilog */
        IWM_nibOut35(dsk, trk, 0xAA, 8);    /* addr epilog */
        IWM_nibOut35(dsk, trk, 0xFF, 8);    /* addr epilog */

        /* One 5-nibble selfsync field */

        IWM_nibOut35(dsk, trk, 0xFF, 8);
        IWM_nibOut35(dsk, trk, 0xFF, 8);
        IWM_nibOut35(dsk, trk, 0xFF, 8);
        IWM_nibOut35(dsk, trk, 0xFF, 8);
        IWM_nibOut35(dsk, trk, 0xFF, 8);

        IWM_nibOut35(dsk, trk, 0xFF, 8);    /* data prolog */
        IWM_nibOut35(dsk, trk, 0xD5, 8);    /* data prolog */
        IWM_nibOut35(dsk, trk, 0xAA, 8);    /* data prolog */
        IWM_nibOut35(dsk, trk, 0xAD, 8);    /* data prolog */
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[sec], 8);
        IWM_nibblize35( &(iwm_track_buf[sec*512]), iwm_nib_buf, csum);
        for (i = 0; i < 699; i++) {
            val = iwm_nib_buf[i];
            IWM_nibOut35(dsk, trk, iwm_disk_bytes[val], 8);
        }
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[csum[3]], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[csum[2]], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[csum[1]], 8);
        IWM_nibOut35(dsk, trk, iwm_disk_bytes[csum[0]], 8);
        IWM_nibOut35(dsk, trk, 0xDE, 8);    /* data epilog */
        IWM_nibOut35(dsk, trk, 0xAA, 8);    /* data epilog */
        IWM_nibOut35(dsk, trk, 0xFF, 8);    /* data epilog */
    }

    trk->track_valid = 1;
    trk->track_dirty = 0;

    dsk->nib_pos = old_pos;
}

void IWM_nibOut35(Disk35 *dsk, Track *trk, byte val, int size)
{
    int    pos;

    pos = dsk->nib_pos;
    if (pos >= trk->track_len) pos = 0;
    trk->nib_area[pos++] = size;
    trk->nib_area[pos++] = val;
    if (pos >= trk->track_len) pos = 0;
    dsk->nib_pos = pos;
}
