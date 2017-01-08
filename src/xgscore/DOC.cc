/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class handles sound generation, including emulation of the
 * Ensoniq DOC and the legacy 1-bit speaker audio.
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>

#include <SDL.h>

#include "gstypes.h"
#include "DOC.h"
#include "System.h"
#include "Mega2.h"
#include "M65816/Processor.h"

using std::cerr;
using std::endl;
using std::string;
using std::uint8_t;
using std::uint16_t;
using std::uint32_t;

static unsigned int wp_masks[8]  = { 0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80 };
static unsigned int acc_masks[8] = { 0x00FF, 0x01FF, 0x03FF, 0x07FF, 0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF };

static unsigned int kSampleRates[DOC::kNumOscillators] = {
    298295, 223722, 178977, 149148, 127841, 111861,  99432,  89489,
     81353,  74574,  68837,  63920,  59659,  55930,  52640,  49716,
     47099,  44744,  42614,  40677,  38908,  37287,  35795,  34419,
     33144,  31960,  30858,  29829,  28867,  27965,  27118,  26320
};

static void buffer_callback(void *userdata, Uint8 *stream, int len)
{
    static_cast<DOC *>(userdata)->bufferCallback(stream, len);

}

DOC::~DOC()
{
    if (sound_device_id) {
        SDL_CloseAudioDevice(sound_device_id);
    }
}

void DOC::reset()
{
    click_sample = 0.0;

    doc_registers[0xA0] = 0x00;
    osc_enable[0] = true;

    for (unsigned int i = 1 ; i < kNumOscillators ; ++i) {
        doc_registers[0xA0 + i] = 0x01;
        osc_acc[i]    = 0;
        last_addr[i]  = 0;
        osc_enable[i] = true;

        updateOscillator(i);
    }

    doc_registers[0xE0] = 0xC1;
    doc_registers[0xE1] = 0x02;

    irq_index    = 0;
    buffer_index = 0;

    enableOscillators();
}

uint8_t DOC::read(const unsigned int& offset)
{
    uint8_t val = 0;

    switch (offset) {
        case 0x30:
            click_sample = click_sample? 0 : 1.0;

            break;
        case 0x3C:
            val = glu_ctrl_reg;

            break;
        case 0x3D:
            {
                val = glu_next_val;

                if (glu_ctrl_reg & 0x40) {
                    glu_next_val = doc_ram[glu_addr_reg & 0xFFFF];
                }
                else {
                    unsigned int reg = glu_addr_reg & 0xFF;

                    if (reg == 0xE0) {
                        int osc_num = pullIRQ();

                        if (osc_num == -1) {
                            doc_registers[0xE0] |= 0x80;
                        }
                        else {
                            doc_registers[0xE0] = (osc_num << 1) | 0x01;
                        }
                    }

                    glu_next_val = doc_registers[reg];
                }

                if (glu_ctrl_reg & 0x20) glu_addr_reg++;
            }

            break;
        case 0x3E:
            val = glu_addr_reg & 0xFF;

            break;
        case 0x3F:
            val = glu_addr_reg >> 8;

            break;
        default:
            break;
    }

    return val;
}

void DOC::write(const unsigned int& offset, const uint8_t& val)
{
    switch (offset) {
        case 0x30:
            click_sample = click_sample? 0 : 1.0;

            break;
        case 0x3C:
            glu_ctrl_reg = val;

            system_volume = ((float) (val & 0x07)) / 7.0;

            break;
        case 0x3D:
            {
                if (glu_ctrl_reg & 0x40) {
                    doc_ram[glu_addr_reg & 0xFFFF] = val;
                }
                else {
                    unsigned int reg = glu_addr_reg & 0xFF;

                    if ((reg == 0xE0) || ((reg & 0xE0) == 0x60)) {
                        /* read-only registers $E0 and $60 - $7F */
                    }
                    else {
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
            }
            break;
        case 0x3E:
            glu_addr_reg = (glu_addr_reg & 0xFF00) | val;

            break;
        case 0x3F:
            glu_addr_reg = (glu_addr_reg & 0x00FF) | (val << 8);

            break;
        default:
            break;
    }
}

void DOC::microtick(unsigned int mt)
{
    if (buffer_index == buffer_max) {
        return;
    }

    SDL_LockAudioDevice(sound_device_id);

    last_sample.left = last_sample.right = click_sample;

    scanOscillators(&last_sample);

    // Scale by system volume

    last_sample.left  *= system_volume;
    last_sample.right *= system_volume;

    // Now, clip if needed.

    if (last_sample.left > 1.0) {
        last_sample.left = 1.0;
    }
    else if (last_sample.left < -1.0) {
        last_sample.left = -1.0;
    }

    if (last_sample.right > 1.0) {
        last_sample.right = 1.0;
    }
    else if (last_sample.right < -1.0) {
        last_sample.right = -1.0;
    }

    sample_buffer[buffer_index].left  = last_sample.left;
    sample_buffer[buffer_index].right = last_sample.right;

    buffer_index++;

    SDL_UnlockAudioDevice(sound_device_id);
}

void DOC::setOutputDevice(string& device)
{
    sound_device = device;

    SDL_AudioSpec wanted, actual;
    int i;

    SDL_zero(wanted);

    wanted.freq     = kSampleRate;
    wanted.format   = AUDIO_F32SYS;
    wanted.channels = 2;
    wanted.samples  = kAudioBufferSize;
    wanted.callback = &buffer_callback;
    wanted.userdata = this;

    sound_device_id = SDL_OpenAudioDevice(sound_device.c_str(), 0, &wanted, &actual, 0);

    if (sound_device_id == 0) {
        throw std::runtime_error("Error opening audio device");
        //printSDLError("Error opening audio device");
    }

    buffer_len = actual.size * 2;
    buffer_max = buffer_len / sizeof(AudioSample);

    try {
        sample_buffer = new AudioSample[buffer_len];
    }
    catch (std::runtime_error& e) {
        SDL_CloseAudioDevice(sound_device_id);

        throw e;
    }

    cerr << boost::format("Audio device buffer is %d bytes (%d samples)\n") % actual.size % (actual.size / sizeof(AudioSample));
    cerr << boost::format("Internal sample buffer is %d bytes (%d samples)\n") % buffer_len % buffer_max;

    SDL_PauseAudioDevice(sound_device_id, 0);
}

/**
 * Enable oscillators because register $E1 was changed. This is a bit
 * tricky because of sync and swap modes, which pair even/odd registers
 * together. In sync mode, odd oscillators only start if the adjacent
 * lower-numbered even oscillator is also in sync mode. For swap mode,
 * odd oscillators don't start until the adjacent lower-numbered even
 * oscillator finishes. For free-run and one-shot mode just start the
 * oscillator.
 */
void DOC::enableOscillators(void)
{
    int    i,mode,last_mode;

    num_osc = ((doc_registers[0xE1] >> 1) & 0x1F) + 1;

    last_mode = -1;
    for (i = 0 ; i < num_osc ; i++) {
        mode = (doc_registers[0xA0 + i] >> 1) & 0x03;
        if (i & 0x01) {        /* odd */
            if (!osc_enable[i]) {
                switch (mode) {
                    case 0 :    osc_enable[i] = true;
                            break;
                    case 1 :    osc_enable[i] = true;
                            break;
                    case 2 :    osc_enable[i] = (last_mode == 2);
                            break;
                    case 3 :    osc_enable[i] = false;
                            break;
                }
            }
        }
        else {        /* even */
            osc_enable[i] = true;
        }
        last_mode = mode;
        doc_registers[0xA0 + i] = (doc_registers[0xA0 + i ] & 0xFE) +
                       !osc_enable[i];
        updateOscillator(i);
    }
}

void DOC::bufferCallback(Uint8 *stream, int len)
{
    int nbytes = buffer_index * sizeof(AudioSample);
    int i;

    if (nbytes >= len) {
        memmove(stream, sample_buffer, len);
        memmove(sample_buffer, (Uint8 *) sample_buffer + len, nbytes - len);

        buffer_index -= (len / sizeof(AudioSample));
    }
    else {
        if (nbytes) {
            memmove(stream, sample_buffer, nbytes);
        }

        for (i = nbytes; i < len; i += sizeof(last_sample)) {
            memmove(stream + i, &last_sample, sizeof(last_sample));
        }

        buffer_index = 0;
    }
}

void DOC::scanOscillators(AudioSample *samples_out)
{
    int addr;
    int osc_num;
    float sample;

    for (osc_num = 0 ; osc_num < num_osc; osc_num++) {
        if (!osc_enable[osc_num]) continue;

        /* Get the address of the next sample for this oscillator. */
        addr = (osc_acc[osc_num] >> osc_shift[osc_num]) & osc_accmask[osc_num];

        /*
         * The new address may be past the end of the
         * wavetable. If it is, halt the oscillator.
         */

        if (addr < last_addr[osc_num]) {
            if (!osc_mode[osc_num]) {
                last_addr[osc_num] = addr;
                sample = (float) osc_addrbase[osc_num][addr];
                if (osc_int[osc_num]) pushIRQ(osc_num);
                osc_acc[osc_num] += osc_freq[osc_num];
            } else {
                sample = 0; /* will be halted below */
            }
        } else {
            last_addr[osc_num] = addr;
            sample = (float) osc_addrbase[osc_num][addr];
            osc_acc[osc_num] += osc_freq[osc_num];
        }

        /* If it's a zero sample, halt the oscillator */
        if (!sample) osc_enable[osc_num] = false;

        /*
         * At this point, if we are now halted, then
         * the oscillator was running before and
         * was halted by us. Now we need to do some
         * housekeeping: the halted oscillator may need
         * to generate an interrupt, and if it's an
         * oscillator in swap mode then we need to
         * start its partner
         */

        if (!osc_enable[osc_num]) {
            doc_registers[0xA0 + osc_num] |= 0x01;
            if (osc_int[osc_num]) pushIRQ(osc_num);
            osc_acc[osc_num] = 0;
            last_addr[osc_num] = 0;
            if (osc_mode[osc_num] == 3) {
                if (osc_num & 0x01) {
                    osc_enable[osc_num - 1] = true;
                    doc_registers[0xA0 + osc_num - 1] &= 0xFE;
                } else {
                    osc_enable[osc_num + 1] = true;
                    doc_registers[0xA0 + osc_num + 1] &= 0xFE;
                }
            }
            continue;
        }

        /* Normalize sample to [ -1, 1 ] and scale to oscillator volume */
        sample = ((sample - 128) / 127.0) * osc_vol[osc_num];

        if (osc_chan[osc_num] & 0x01) {
            samples_out->right += sample;
        } else {
            samples_out->left  += sample;
        }
    }
}

void DOC::updateOscillator(const unsigned int osc_num)
{
    unsigned int ctrl;

    ctrl = doc_registers[0xA0 + osc_num];
    osc_chan[osc_num] = (ctrl >> 4) & 0x0F;
    osc_freq[osc_num] = (doc_registers[0x20 + osc_num] << 8) |
                 doc_registers[0x00 + osc_num];
    osc_vol[osc_num] =  (float) doc_registers[0x40 + osc_num] / 255.0;
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
        osc_enable[osc_num + 1] = true;
    }
}

void DOC::pushIRQ(const unsigned int osc_num)
{
    if (irq_index == kInterruptStackSize) return;

    if (!irq_index) system->cpu->raiseInterrupt();

    irq_stack[irq_index++] = osc_num;
}

const int DOC::pullIRQ()
{
    if (!irq_index) return -1;

    int osc_num = irq_stack[--irq_index];
    if (!irq_index) system->cpu->lowerInterrupt();

    return osc_num;
}
