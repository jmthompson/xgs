/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * Emulates the IWM chip and four drives (two 3.5" in slot 5 and
 * two 5.25" in slot 6).
 *
 * Parts of this code are written and Copyright 1996 by Kent Dickey.
 * Used with permission.
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>

#include <SDL.h>

#include "gstypes.h"
#include "IWM.h"
#include "System.h"

using std::cerr;
using std::endl;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

static const uint8_t iwm_35track_len[80] = {
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

IWM::IWM()
{
}

IWM::~IWM()
{
}

void IWM::reset()
{
    slot4_motor = false;
    slot5_motor = false;
    slot6_motor = false;
    slot7_motor = false;

    iwm_q6        = false;
    iwm_q7        = false;
    iwm_motor_on  = false;
    iwm_motor_off = false;

    motor_off_frame = 0;

    iwm_drive_select = 0;
    iwm_mode         = 0;
    iwm_enable2      = false;
    iwm_reset        = 0;

    iwm_phase[0] = 0;
    iwm_phase[1] = 0;
    iwm_phase[2] = 0;
    iwm_phase[3] = 0;
}

uint8_t IWM::read(const unsigned int& offset)
{
    if (offset == 0x31) {
        uint8_t val = 0;

        if (iwm_35ctl) val |= 0x80;
        if (iwm_35sel) val |= 0x40;

        return val;
    }
    else {
        touchSwitches(offset & 0x0F);

        if (offset & 1) {
            return 0; // odd address, return 0
        }
        else if (!iwm_q7 && !iwm_q6) {
            if (iwm_enable2) {
                return 0xFF;    // readEnable2()
            }
            else {
                if (iwm_motor_on) {
                    if (iwm_35sel) {
                        return disks_35[iwm_drive_select].read(system->cycle_count);
                    }
                    else {
                        return disks_525[iwm_drive_select].read(system->cycle_count);
                    }
                }
                else {
                    return 0xFF;
                }
            }
        }
        else if (!iwm_q7 && iwm_q6) {
            unsigned int status;

            if (iwm_enable2) {
                status = 1;
            }
            else {
                if (iwm_35sel) {
                    if (iwm_motor_on) {
                        unsigned int state = (iwm_phase[2] << 3) | (iwm_phase[1] << 2) | (iwm_phase[0] << 1) | iwm_35ctl;

                        status = disks_35[iwm_drive_select].status(state);
                    }
                    else {
                        status = 1;
                    }
                }
                else {
                    status = disks_525[iwm_drive_select].status();
                }
            }

            return (status << 7) | (iwm_motor_on << 5) | iwm_mode;
        }
        else if (iwm_q7 && !iwm_q6) {
            uint8_t val;

            if (iwm_enable2) {
                val = 0xC0;

                if (++iwm_enable2_handshake > 3) {
                    iwm_enable2_handshake = 0;

                    val = 0x80;
                }
            }
            else {
                val = 0x80;
            }

            return val;
        }
        else {
            return 0;
        }
    }
}

void IWM::write(const unsigned int& offset, const uint8_t& val)
{
    if (offset == 0x31) {
        iwm_35ctl = val & 0x80;
        iwm_35sel = val & 0x40;
    }
    else {
        touchSwitches(offset & 0x0F);

        if (offset & 1) {
            // odd address, write something

            if (iwm_q7 && iwm_q6) {
                if (iwm_motor_on) {
                    if (iwm_enable2) {
                        //writeEnable2(val);
                    }
                    else if (iwm_35sel) {
                        disks_35[iwm_drive_select].write(system->cycle_count, val);
                    }
                    else {
                        disks_525[iwm_drive_select].write(system->cycle_count, val);
                    }
                }
                else {
                    iwm_mode = val & 0x1F;
                }
            }
            else {
                if (iwm_enable2) {
                    //writeEnable2(val);
                }
                else {
                }
            }
        }
        else {
            if (iwm_enable2) {
                //writeEnable2(val);
            }
            else {
            }
        }
    }
}

void IWM::tick(const unsigned int frame_number)
{
    if (iwm_motor_on && iwm_motor_off) {
        if (motor_off_frame <= frame_number) {
            iwm_motor_on = iwm_motor_off = false;
        }
    }

    if (!iwm_motor_on || iwm_motor_off) {
        disks_35[0].flush();
        disks_35[1].flush();

        disks_525[0].flush();
        disks_525[1].flush();
    }
}

void IWM::loadDrive(const unsigned int slot, const unsigned int drive, VirtualDisk *vdisk)
{
    if (slot == 5) {
        disks_35[drive & 1].load(vdisk);
    }
    else if (slot == 6) {
        disks_525[drive & 1].load(vdisk);
    }
}

void IWM::unloadDrive(const unsigned int slot, const unsigned int drive)
{
    //IWM_update();

    if (slot == 5) {
        disks_35[drive & 1].unload();
    }
    else if (slot == 6) {
        disks_525[drive & 1].unload();
    }
}

void IWM::touchSwitches(const unsigned int loc)
{
    unsigned int phase = loc >> 1;
    bool on = loc & 1;

    if (loc < 8) {
        unsigned int phase_up   = (phase - 1) & 3;
        unsigned int phase_down = (phase + 1) & 3;

        iwm_phase[phase] = on;

        if (iwm_motor_on) {
            if (iwm_35sel) {
                //if ((phase == 3) && on) IWM_doAction35();
            }
            else if (on) {
                disks_525[iwm_drive_select].phaseChange(phase);
            }

        } else {
            /* See if enable or reset is asserted */
            if (iwm_phase[0] && iwm_phase[2]) {
                iwm_reset = 1;
            } else {
                iwm_reset = 0;
            }
            if (iwm_phase[1] && iwm_phase[3]) {
                iwm_enable2 = true;
            } else {
                iwm_enable2 = false;
            }
        }
    } else {
        /* loc >= 8 */
        switch(loc) {
        case 0x08:
            if (iwm_mode & 0x04) {
                /* Turn off immediately */
                iwm_motor_off = false;
                iwm_motor_on  = false;
            } else {
                /* 1 second delay */
                if (iwm_motor_on && !iwm_motor_off) {
                    iwm_motor_off = true;
                    motor_off_frame = system->vbl_count + 60;
                }
            }
            break;
        case 0x09:
            iwm_motor_on = true;
            iwm_motor_off = false;
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
