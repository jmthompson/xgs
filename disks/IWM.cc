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

#include "emulator/common.h"

#include "disks/IWM.h"

#include "emulator/xgs.h"
#include "M65816/m65816.h"

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
    iwm_motor_spindown = false;

    motor_off_frame = 0;

    iwm_drive_select = 0;
    iwm_mode         = 0;
    iwm_enable2      = false;
    iwm_reset        = 0;

    iwm_phase[0] = false;
    iwm_phase[1] = false;
    iwm_phase[2] = false;
    iwm_phase[3] = false;
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
                        return disks_35[iwm_drive_select].read(m65816::total_cycles);
                    }
                    else {
                        return disks_525[iwm_drive_select].read(m65816::total_cycles);
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
                        disks_35[iwm_drive_select].write(m65816::total_cycles, val);
                    }
                    else {
                        disks_525[iwm_drive_select].write(m65816::total_cycles, val);
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
    if (iwm_motor_on && iwm_motor_spindown) {
        if (motor_off_frame <= xgs::vbl_count) {
            iwm_motor_on = iwm_motor_spindown = false;
        }
    }

    if (!iwm_motor_on || iwm_motor_spindown) {
        disks_35[0].flush();
        disks_35[1].flush();

        disks_525[0].flush();
        disks_525[1].flush();
    }
}

void IWM::loadDrive(const unsigned int slot, const unsigned int drive, VirtualDisk *vdisk)
{
    if (slot == 5) {
        disks_35[drive].load(vdisk);
    }
    else if (slot == 6) {
        disks_525[drive].load(vdisk);
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
    if (loc < 8) {
        unsigned int phase = loc >> 1;
        unsigned int phase_up   = (phase - 1) & 3;
        unsigned int phase_down = (phase + 1) & 3;
        bool on = loc & 1;

        iwm_phase[phase] = on;

        if (iwm_motor_on) {
            if (iwm_35sel) {
                if ((phase == 3) && on) {
                    unsigned int state = (iwm_phase[1] << 3) + (iwm_phase[0] << 2) + (iwm_35ctl << 1) + iwm_phase[2];

                    disks_35[iwm_drive_select].action(state);
                }
                else {
                    return;
                }
            }
            else if (on) {
                disks_525[iwm_drive_select].phaseChange(phase);
            }

        }
        else {
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
    }
    else {      // loc >= 8
        switch(loc) {
            case 0x08:
                if (iwm_mode & 0x04) {
                    /* Turn off immediately */
                    iwm_motor_spindown = false;
                    iwm_motor_on  = false;
                }
                else {
                    if (iwm_motor_on && !iwm_motor_spindown) {
                        iwm_motor_spindown = true;

                        /* 1 second delay */
                        motor_off_frame = xgs::vbl_count + 60;
                    }
                }

                break;
            case 0x09:
                iwm_motor_on = true;
                iwm_motor_spindown = false;

                break;
            case 0x0A:
                iwm_drive_select = 0;

                break;
            case 0x0B:
                iwm_drive_select = 1;

                break;
            case 0x0C:
                iwm_q6 = false;

                break;
            case 0x0D:
                iwm_q6 = true;

                break;
            case 0x0E:
                iwm_q7 = false;

                break;
            case 0x0F:
                iwm_q7 = true;

                break;
            default:
                break;
        }
    }
}
