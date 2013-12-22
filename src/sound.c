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
 * File: sound.c
 *
 * Sound initialization and core routines.
 */

#include "xgs.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include "hardware.h"
#include "sound.h"

char *sound_device = NULL;
static SDL_AudioDeviceID sound_device_id;

static int    glu_ctrl_reg,glu_next_val;
static word16 glu_addr_reg;
static int    sr,num_osc;

const static int wp_masks[8]  = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80 };
const static int acc_masks[8] = { 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF };

const static int sample_rates[32] = {
    298295, 223722, 178977, 149148, 127841, 111861,  99432,  89489,
     81353,  74574,  68837,  63920,  59659,  55930,  52640,  49716,
     47099,  44744,  42614,  40677,  38908,  37287,  35795,  34419,
     33144,  31960,  30858,  29829,  28867,  27965,  27118,  26320
};

static int    doc_registers[256];
static byte   doc_ram[65536];

static int    last_addr[32];
static word32 osc_acc[32];
static int    osc_enable[32];
static int    osc_chan[32];
static int    osc_freq[32];
static int    osc_vol[32];
static int    osc_wp[32];
static int    osc_int[32];
static int    osc_ws[32];
static int    osc_res[32];
static int    osc_mode[32];
static int    osc_shift[32];
static int    osc_accmask[32];
static byte  *osc_addrbase[32];

static int    irq_stack[IRQ_STACK_SIZE];
static int    irq_index;

static word32 snd_click_sample;

static snd_sample_struct *sample_buffer;
static int buffer_index, buffer_size;
static SDL_mutex *buffer_mutex;

static void bufferCallback(void *, Uint8 *, int len);
static void enableOscillators(void);
static void scanOscillators(snd_sample_struct *);
static void updateOscillator(int);

static void pushIRQ(int);
static int  pullIRQ(void);

int soundInit()
{
    SDL_AudioSpec wanted, actual;
    int i;

    buffer_mutex = SDL_CreateMutex();
    if (buffer_mutex == NULL) {
        printSDLError("Error creating mutex");

        return 1;
    }

    SDL_zero(wanted);

    wanted.freq     = SAMPLE_RATE;
    wanted.format   = AUDIO_U8;
    wanted.channels = 2;
    wanted.samples  = OUTPUT_BUFFER_SIZE * 2;
    wanted.callback = &bufferCallback;

    sound_device_id = SDL_OpenAudioDevice(sound_device, 0, &wanted, &actual, 0);

    if (sound_device_id == 0) {
        printSDLError("Error opening audio device");

        return 1;
    }

    printf("Got audio buffer size of %d samples\n", actual.samples);

    buffer_size = actual.samples;

    sample_buffer = malloc(buffer_size * sizeof(snd_sample_struct));
    if (sample_buffer == NULL) {
        perror("Unable to allocate sound buffer");

        SDL_CloseAudioDevice(sound_device_id);

        return 1;
    }

    for (i = 0xEA00 ; i < 0xEB00 ; i++) {
        mem_pages[i].readPtr   = mem_pages[i].writePtr = doc_ram+((i-0xEA00) * 256);
        mem_pages[i].readFlags = mem_pages[i].writeFlags = 0;
    }

    return 0;
}

void soundReset()
{
    int    i;

    snd_click_sample = 0;

    for (i = 0 ; i < 32 ; i++) {
        doc_registers[0xA0 + i] = 0x01;
        osc_acc[i] = 0;
        last_addr[i] = 0;
        osc_enable[i] = 0;
        updateOscillator(i);
    }

    doc_registers[0xA0] = 0x00;
    osc_enable[0] = 1;

    doc_registers[0xE0] = 0xC1;
    doc_registers[0xE1] = 0x02;

    irq_index = 0;

    buffer_index = 0;
    memset(sample_buffer, 0, sizeof(snd_sample_struct) * buffer_size); 

    enableOscillators();

    SDL_PauseAudioDevice(sound_device_id, 0);
}

void soundShutdown()
{
    SDL_CloseAudioDevice(sound_device_id);
}

void soundUpdate()
{
    snd_sample_struct sample;

    if (SDL_LockMutex(buffer_mutex) == 0) {
        if (buffer_index < buffer_size) {
            scanOscillators(&sample);

            sample_buffer[buffer_index].left  = sample.left  + snd_click_sample;
            sample_buffer[buffer_index].right = sample.right + snd_click_sample;
            buffer_index++;
        }

        SDL_UnlockMutex(buffer_mutex);
    }
    else {
        printSDLError("Failed to lock sound mutex");
    }
}

byte soundClickSpeaker(byte val)
{
    snd_click_sample ^= SHRT_MAX;

    return 0;
}

byte SND_readSoundCtl(byte val)
{
    return glu_ctrl_reg;
}

byte SND_readSoundData(byte val)
{
    int    reg,osc_num;

    val = glu_next_val;
    if (glu_ctrl_reg & 0x40) {
        glu_next_val = doc_ram[glu_addr_reg & 0xFFFF];
    } else {
        reg = glu_addr_reg & 0xFF;
        if (reg == 0xE0) {
            osc_num = pullIRQ();
            if (osc_num != -1) {
                doc_registers[0xE0] = (osc_num << 1) | 0x01;
            } else {
                doc_registers[0xE0] |= 0x80;
            }
        }
        glu_next_val = doc_registers[reg];
    }
    if (glu_ctrl_reg & 0x20) glu_addr_reg++;
    return val;
}

byte SND_readSoundAddrL(byte val)
{
    return (glu_addr_reg & 0xFF);
}

byte SND_readSoundAddrH(byte val)
{
    return (glu_addr_reg >> 8);
}

byte SND_writeSoundCtl(byte val)
{
    glu_ctrl_reg = val;
    return 0;
}

byte SND_writeSoundData(byte val)
{
    int    reg;

    if (glu_ctrl_reg & 0x40) {
        doc_ram[glu_addr_reg & 0xFFFF] = val;
    } else {
        reg = glu_addr_reg & 0xFF;
        if ((reg == 0xE0) || ((reg & 0xE0) == 0x60)) {
            /* read-only registers $E0 and $60 - $7F */
        } else {
            doc_registers[reg] = val;
        }
        if (reg == 0xE1) enableOscillators();
        if ((reg & 0xE0) == 0xA0) {
            osc_acc[reg & 0x1F] = 0;
            last_addr[reg & 0x1F] = 0;
        }
        if (reg < 0xE0) updateOscillator(reg & 0x1F);
    }
    if (glu_ctrl_reg & 0x20) glu_addr_reg++;
    return 0;
}

byte SND_writeSoundAddrL(byte val)
{
    glu_addr_reg = (glu_addr_reg & 0xFF00) | val;
    return 0;
}

byte SND_writeSoundAddrH(byte val)
{
    glu_addr_reg = (val << 8) | (glu_addr_reg & 0x00FF);
    return 0;
}

/* Enable oscillators because register $E1 was changed. This is a bit    */
/* tricky because of sync and swap modes, which pair even/odd registers    */
/* together. In sync mode, odd oscillators only start if the adjacent    */
/* lower-numbered even oscillator is also in sync mode. For swap mode,    */
/* odd oscillators don't start until the adjacent lower-numbered even    */
/* oscillator finishes. For free-run and one-shot mode just start the    */
/* oscillator.                                */

void enableOscillators(void)
{
    int    i,mode,last_mode;

    num_osc = ((doc_registers[0xE1] >> 1) & 0x1F) + 1;

    last_mode = -1;
    for (i = 0 ; i < num_osc ; i++) {
        mode = (doc_registers[0xA0 + i] >> 1) & 0x03;
        if (i & 0x01) {        /* odd */
            if (!osc_enable[i]) {
                switch(mode) {
                    case 0 :    osc_enable[i] = 1;
                            break;
                    case 1 :    osc_enable[i] = 1;
                            break;
                    case 2 :    osc_enable[i] = (last_mode == 2);
                            break;
                    case 3 :    osc_enable[i] = 0;
                            break;
                }
            }
        } else {        /* even */
            osc_enable[i] = 1;
        }
        last_mode = mode;
        doc_registers[0xA0 + i] = (doc_registers[0xA0 + i ] & 0xFE) +
                       !osc_enable[i];
        updateOscillator(i);
    }
    sr = sample_rates[num_osc];
}

static void bufferCallback(void *userdata, Uint8 *stream, int len)
{
    int i = 0;
    int j = 0;

    if (SDL_LockMutex(buffer_mutex) == 0) {
        while ((i < len) && (j < buffer_size)) {
            stream[i++] = (Uint8) ((sample_buffer[j].left  >> 10) + 128);
            stream[i++] = (Uint8) ((sample_buffer[j].right >> 10) + 128);
            j++;
        }

        buffer_index = 0;
        memset(sample_buffer, 0, sizeof(snd_sample_struct) * buffer_size); 

        SDL_UnlockMutex(buffer_mutex);
    }
}

static void scanOscillators(snd_sample_struct *out_sample)
{
    int addr,this_sample;
    int osc_num;
    snd_sample_struct sample;

    sample.left = 0;
    sample.right = 0;
    for (osc_num = 0 ; osc_num < num_osc; osc_num++) {
        if (!osc_enable[osc_num]) continue;

        /* Get the address of the next sample for this    */
        /* oscillator.                    */

        addr = (osc_acc[osc_num] >> osc_shift[osc_num]) &
            osc_accmask[osc_num];

        /* The new address may be past the end of the    */
        /* wavetable.If it is, halt the oscillator.    */

        if (addr < last_addr[osc_num]) {
            if (!osc_mode[osc_num]) {
                last_addr[osc_num] = addr;
                this_sample = osc_addrbase[osc_num][addr];
                if (osc_int[osc_num]) pushIRQ(osc_num);
                osc_acc[osc_num] += osc_freq[osc_num];
            } else {
                osc_enable[osc_num] = 0;
            }
        } else {
            last_addr[osc_num] = addr;
            this_sample = osc_addrbase[osc_num][addr];
            osc_acc[osc_num] += osc_freq[osc_num];
        }

        /* If it's a zero sample, halt the oscillator */

        if (!this_sample) osc_enable[osc_num] = 0;

        /* At this point, if we are now halted, then    */
        /* the oscillator was running before and        */
        /* was halted by us. Now we need to do some     */
        /* housekeeping: the halted oscillator may need */
        /* to generate an interrupt, and if it's an     */
        /* oscillator in swap mode then we need to      */
        /* start its partner                            */

        if (!osc_enable[osc_num]) {
            doc_registers[0xA0 + osc_num] |= 0x01;
            if (osc_int[osc_num]) pushIRQ(osc_num);
            osc_acc[osc_num] = 0;
            last_addr[osc_num] = 0;
            if (osc_mode[osc_num] == 3) {
                if (osc_num & 0x01) {
                    osc_enable[osc_num - 1] = 1;
                    doc_registers[0xA0 + osc_num - 1] &= 0xFE;
                } else {
                    osc_enable[osc_num + 1] = 1;
                    doc_registers[0xA0 + osc_num + 1] &= 0xFE;
                }
            }
            continue;
        }

        /* Add the sample into the output "supersample" */

        if (osc_chan[osc_num] & 0x01) {
            sample.right += ((int) this_sample - 128) * osc_vol[osc_num];
        } else {
            sample.left += ((int) this_sample - 128) * osc_vol[osc_num];
        }
    }
    out_sample->left = sample.left;
    out_sample->right = sample.right;
}

static void updateOscillator(int osc_num)
{
    int    ctrl;

    ctrl = doc_registers[0xA0 + osc_num];
    osc_chan[osc_num] = (ctrl >> 4) & 0x0F;
    osc_freq[osc_num] = (doc_registers[0x20 + osc_num] << 8) |
                 doc_registers[0x00 + osc_num];
    osc_vol[osc_num] =  doc_registers[0x40 + osc_num];
    osc_wp[osc_num] =  doc_registers[0x80 + osc_num];
    osc_int[osc_num] = ctrl & 0x08;
    osc_mode[osc_num] = (ctrl >> 1) & 0x03;
    osc_res[osc_num] = doc_registers[0xC0 + osc_num] & 0x07;
    osc_ws[osc_num] =  (doc_registers[0xC0 + osc_num] >> 3) & 0x07;
    osc_shift[osc_num] = 9 + osc_res[osc_num] - osc_ws[osc_num];
    osc_accmask[osc_num] = acc_masks[osc_ws[osc_num]];
    osc_addrbase[osc_num] = doc_ram +
                ((osc_wp[osc_num] & wp_masks[osc_ws[osc_num]]) << 8);
    osc_enable[osc_num] = !(ctrl & 0x01);
    if (osc_enable[osc_num] &&
       !(osc_num & 0x01) && (osc_mode[osc_num] == 2)) {
        doc_registers[0xA0 + osc_num + 1] &= 0xFE;
        osc_enable[osc_num+1] = 1;
    }
}

static void pushIRQ(int osc_num)
{
    if (irq_index == IRQ_STACK_SIZE) return;
    if (!irq_index) m65816_addIRQ();
    irq_stack[irq_index++] = osc_num;
}

static int pullIRQ(void)
{
    int    osc_num;

    if (!irq_index) return -1;
    osc_num = irq_stack[--irq_index];
    if (!irq_index) m65816_clearIRQ();
    return osc_num;
}

