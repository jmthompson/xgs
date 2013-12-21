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
 * File: adb.c
 *
 * Emulates the ADB subsystem, including the Keyboard GLU registers
 * and the standard Apple II keybaord registers ($C000, etc.)
 */

#include "xgs.h"

#include "adb.h"
#include "hardware.h"
#include "video.h"

byte    adb_m2mouseenable;
byte    adb_m2mousemvirq;
byte    adb_m2mouseswirq;

int    adb_pdl0, adb_pdl1;
word32    adb_pdl0_time,adb_pdl1_time;

byte    ski_kbd_reg;
byte    ski_modifier_reg;
byte    ski_data_reg;
byte    ski_status_reg;
byte    ski_mode_byte;
byte    ski_conf[3];
byte    ski_error;

byte    ski_ram[96];

byte    ski_data[16];

byte    ski_button0;
byte    ski_button1;
int    ski_xdelta;
int    ski_ydelta;

int    ski_input_index;
int    ski_output_index;
byte    ski_input_buffer[ADB_INPUT_BUFFER];

int    ski_read,ski_written;

byte    ski_status_irq;

ski_command    ski_curr;

const ski_command    ski_table[] = {
    { 0x00, 0, 1 },        /* Abort current command */
    { 0x01, 0, 0 },        /* Abort current command */
    { 0x02, 0, 0 },        /* Reset SKI */
    { 0x03, 0, 0 },        /* Flush type-ahead buffer */
    { 0x04, 1, 0 },        /* Set mode bits */
    { 0x05, 1, 0 },        /* Clear mode bits */
    { 0x06, 3, 0 },        /* Set configuration */
    { 0x07, 4, 0 },        /* Sync (clear modes + set configuration) */
    { 0x08, 2, 0 },        /* Write byte to address */
    { 0x09, 2, 1 },        /* Read byte from address */
    { 0x0A, 0, 1 },        /* Read mode byte */
    { 0x0B, 0, 3 },        /* Read configuration */
    { 0x0C, 0, 1 },        /* Read+clear error byte */
    { 0x0D, 0, 1 },        /* Return version number */
    { 0x0E, 0, 2 },        /* Read character sets available */
    { 0x0F, 0, 2 },        /* Read layouts available */
    { 0x10, 0, 0 },        /* Reset system */
    { 0x11, 0, 1 },        /* Send ADB keycode */
    { 0x40, 0, 0 },        /* Reset ADB */
    { 0x48, 1, 0 },        /* Send command */
    { 0x73, 0, 0 },        /* Disable SRQ on device 3 (mouse) */
    { 0xB3, 2, 0 },        /* Listne on device 3 (mouse), reg 3 */
    { -1, 0, 0 }        /* END OF TABLE */
};
    
int ADB_init()
{
    ski_modifier_reg = 0;
    ski_data_reg = 0;
    ski_status_reg = 0x10;
    ski_mode_byte = 0x10;
    ski_status_irq = 0;

    return 0;
}

void ADB_reset()
{
    ski_input_index = 0;
    ski_output_index = 0;
    ski_error = 0;
    ski_curr.command = -1;
    ski_curr.bytes_to_read = -1;
    ski_curr.bytes_to_write = -1;
    ski_status_reg &= ~0xC1;
    adb_m2mouseenable = 0;
    adb_m2mousemvirq = 0;
    adb_m2mouseswirq = 0;
    ski_button0 = 0;
    ski_button1 = 0;
}

void ADB_shutdown()
{
}

byte ADB_readKey(byte val)
{
    byte    key;

    if ((ski_input_index != ski_output_index)) {
        key = ski_input_buffer[ski_output_index++];
        if (ski_output_index == ADB_INPUT_BUFFER) ski_output_index = 0;
        if (key & 0x80) {
            ski_kbd_reg = key;
            ski_modifier_reg |= 0x10;
        } else {
            ski_kbd_reg = key | 0x80;
            ski_modifier_reg &= ~0x10;
        }
    }
    return ski_kbd_reg;
}

byte ADB_clearKey(byte val)
{
    ski_kbd_reg &= 0x7F;
    return 0;
}

byte ADB_readMouse(byte val)
{
    int    button,delta;

    if (!(ski_status_reg & 0x80)) return 0;
    if (ski_status_reg & 0x02) {
        delta = ski_ydelta;
        button = ski_button0;
        ski_status_reg &= ~0x82;
        if (ski_status_reg & 0x40) m65816_clearIRQ();
    } else {
        delta = ski_xdelta;
        button = ski_button1;
        ski_status_reg |= 0x02;
    }
    val = 0;
    if (!button) val |= 0x80;
    if (delta > 0x3F)
        delta = 0x3F;
    else if (delta < 0) {
        val |= 0x40;
        if (delta < -0x40)
            delta = 0x0;
        else
            delta &= 0x3F;
    }
    val |= delta;
    return val;
}

byte ADB_readModifiers(byte val)
{
    return ski_modifier_reg;
}

byte ADB_readCommand(byte val)
{
    byte    v;

    if (ski_status_irq) {
        v = ski_status_irq;
        ski_status_irq = 0;
        m65816_clearIRQ();
        return v;
    }
    v = ski_data_reg;
    ski_status_reg &= ~0x20;
    if ((ski_curr.bytes_to_write > 0) && (ski_written < ski_curr.bytes_to_write)) {
        ski_data_reg = ski_data[ski_written++];
        ski_status_reg |= 0x20;
        if (ski_curr.bytes_to_write == ski_written) {
            ski_curr.command = -1;
            ski_curr.bytes_to_read = -1;
            ski_curr.bytes_to_write = -1;
        }
    }
    return v;
}


byte ADB_readCommandKey(byte val)
{
    return ski_modifier_reg & 0x80;
}

byte ADB_readOptionKey(byte val)
{
    return (ski_modifier_reg & 0x40)? 0x80 : 0x00;
}

byte ADB_readStatus(byte val)
{
    if (ski_status_irq) {
        return ski_status_reg | 0x20;
    } else {
        return ski_status_reg;
    }
}

byte ADB_readM2MouseX(byte val)
{
    printf("Access to Mega II Mouse Delta X register!\n");
    return 0;
}

byte ADB_readM2MouseY(byte val)
{
    printf("Access to Mega II Mouse Delta Y register!\n");
    return 0;
}

byte ADB_setCommand(byte val)
{
    int    i;

    if (ski_curr.bytes_to_read < 0) {
        i = 0;
        while (ski_table[i].command != -1) {
            if (ski_table[i].command == val) break;
            i++;
        }
        if (ski_table[i].command == -1) {
            /* printf("Unknown ADB command : %d\n",(int) val); */
            return 0;
        }
        ski_curr = ski_table[i];
        ski_read = 0;
        ski_written = 0;
#if 0
        printf("New ADB command: %d %d %d\n",val,ski_curr.bytes_to_read,ski_curr.bytes_to_write);
#endif
    } else {
        ski_data[ski_read++] = val;
    }
    if (ski_curr.bytes_to_read == ski_read) {
        switch(ski_curr.command) {
            case 0x00 :    ski_data[0] = 0xA5;        /* ??? (behavior hacked from ROM) */
                    break;
            case 0x01 :    ski_curr.command = -1;        /* Abort current command */
                    ski_curr.bytes_to_read = -1;
                    ski_curr.bytes_to_write = -1;
                    ski_status_reg &= ~0x20;
                    break;
            case 0x02 :    ADB_init();            /* Reset SKI */
                    break;
            case 0x03 :                    /* Flush type-ahead buffer */
                    break;
            case 0x04 :    ski_mode_byte |= ski_data[0];    /* Set mode bits */
                    break;
            case 0x05 :    ski_mode_byte &= ~ski_data[0];    /* Clear mode bits */
                    break;
            case 0x06 :    ski_conf[0] = ski_data[0];    /* Set configuration */
                    ski_conf[1] = ski_data[1];
                    ski_conf[2] = ski_data[2];
                    break;
            case 0x07 :    ski_mode_byte &= ~ski_data[0];    /* Sync */
                    ski_conf[0] = ski_data[1];
                    ski_conf[1] = ski_data[2];
                    ski_conf[2] = ski_data[3];
                    break;
            case 0x08 :    i = ski_data[0];        /* Write SKI RAM */
                    if (i > 0x5F) i = 0x5F;
                    ski_ram[i] = ski_data[1];
                    break;
            case 0x09 :    i = ski_data[0] | (ski_data[1] << 8);    /* Read SKI RAM/ROM */
                    if (i > 0x5F) i = 0;        /* no ROM to read... */
                    ski_data[0] = ski_ram[i];
                    break;
            case 0x0A :    ski_data[0] = ski_mode_byte;    /* Read mode byte */
                    break;
            case 0x0B :    ski_data[0] = ski_conf[0];    /* Read configuration */
                    ski_data[1] = ski_conf[1];
                    ski_data[2] = ski_conf[2];
                    break;
            case 0x0C :    ski_data[0] = ski_error;    /* Reset+Reset error codes */
                    ski_error = 0;
                    break;
            case 0x0D :    ski_data[0] = ADB_VERSION;    /* Return version nibble */
                    break;
            case 0x0E :    ski_data[0] = 1;        /* Read available char sets */
                    ski_data[1] = 0;
                    break;
            case 0x0F :    ski_data[0] = 1;        /* Read available layouts */
                    ski_data[1] = 0;
                    break;
            case 0x10 :    hardwareReset();            /* Reset system */
                    return 0;            /* because we were just reset */
            case 0x11 :                    /* Send ADB keycode */
                    break;
            case 0x40 :    ADB_init();            /* Reset ADB */
                    break;
            case 0x48 :                    /* Send command */
                    break;
            default :    /* printf("Unimplemented ADB command : %d\n",(int) ski_curr.command); */
                    break;
        }
        if (ski_curr.bytes_to_write > 0) {
            ski_status_reg |= 0x20;
            ski_data_reg = ski_data[0];
            ski_written = 1;
            if (ski_curr.bytes_to_write == ski_written) {
                ski_curr.command = -1;
                ski_curr.bytes_to_read = -1;
                ski_curr.bytes_to_write = -1;
            }
        } else {
            ski_curr.command = -1;
            ski_curr.bytes_to_read = -1;
            ski_curr.bytes_to_write = -1;
        }
    }
    return 0;
}

byte ADB_setStatus(byte val)
{
    if (val & 0x40) {
        ski_status_reg |= 0x40;
    } else {
        ski_status_reg &= ~0x40;
    }
    return 0;
}

byte ADB_triggerPaddles(byte val)
{
    adb_pdl0_time = g_cpu_cycles + (adb_pdl0 * 11);
    adb_pdl1_time = g_cpu_cycles + (adb_pdl1 * 11);
    return 0;
}

byte ADB_readPaddle0(byte val)
{
    if (g_cpu_cycles < adb_pdl0_time) {
        return 0x80;
    } else {
        return 0x00;
    }
}

byte ADB_readPaddle1(byte val)
{
    if (g_cpu_cycles < adb_pdl1_time) {
        return 0x80;
    } else {
        return 0x00;
    }
}

byte ADB_readPaddle2(byte val)
{
    return 0x80;
}

byte ADB_readPaddle3(byte val)
{
    return 0x80;
}
