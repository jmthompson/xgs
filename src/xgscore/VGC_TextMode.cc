/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This file contains the text mode rendering functions.
 */

#include <cstdlib>
#include <stdexcept>

#include "gstypes.h"
#include "System.h"
#include "VGC.h"

/* Base address table for text/lores pages 1. */

static uint16_t textbases1[24] ={
    0x0400, 0x0480, 0x0500, 0x0580, 0x0600, 0x0680, 0x0700, 0x0780,
    0x0428, 0x04A8, 0x0528, 0x05A8, 0x0628, 0x06A8, 0x0728, 0x07A8,
    0x0450, 0x04D0, 0x0550, 0x05D0, 0x0650, 0x06D0, 0x0750, 0x07D0 
};

/* Base address table for text/lores pages 2. */

static uint16_t textbases2[24] ={
    0x0800, 0x0880, 0x0900, 0x0980, 0x0A00, 0x0A80, 0x0B00, 0x0B80,
    0x0828, 0x08A8, 0x0928, 0x09A8, 0x0A28, 0x0AA8, 0x0B28, 0x0BA8,
    0x0850, 0x08D0, 0x0950, 0x09D0, 0x0A50, 0x0AD0, 0x0B50, 0x0BD0 
};

void VGC::refreshText40Row(const unsigned int row, uint16_t addr)
{
    for (unsigned int col = 0 ; col < 40 ; col++, addr++) {
        const uint8_t *font = font_40col[sw_altcharset] + (ram[addr] * 224);

        for (unsigned int i = 0 ; i < 16 ; i++) {
            Pixel *scrn = scanlines[row * 16 + i] + (col * 14);

            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
        }
    }
}

void VGC::refreshText40Page1()
{
    unsigned int first = sw_text? 0 : 20;

    for (unsigned int row = first ; row < 24 ; ++row) {
        refreshText40Row(row, textbases1[row]);
    }
}

void VGC::refreshText40Page2()
{
    unsigned first = sw_text? 0 : 20;

    for (unsigned row = first ; row < 24 ; row++) {
        refreshText40Row(row, textbases2[row]);
    }
}

void VGC::refreshText80Row(const unsigned int row, uint16_t addr)
{
    for (unsigned int col = 0 ; col < 40 ; col++, addr++) {
        const uint8_t *font = font_80col[sw_altcharset] + (ram[addr] * 112);

        for (unsigned int i = 0 ; i < 16 ; i++) {
            Pixel *scrn = scanlines[row * 16 + i] + (col * 14 + 7);

            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
        }

        font = font_80col[sw_altcharset] + (ram[addr + 0x10000] * 112);

        for (unsigned int i = 0 ; i < 16 ; i++) {
            Pixel *scrn = scanlines[row * 16 + i] + (col * 14);

            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
            *scrn++ = fontToPixel(*font++);
        }
    }
}

void VGC::refreshText80Page1()
{
    unsigned int first = sw_text? 0 : 20;

    for (unsigned int row = first ; row < 24 ; row++) {
        refreshText80Row(row, textbases1[row]);
    }
}

void VGC::refreshText80Page2()
{
    unsigned int first = sw_text? 0 : 20;

    for (unsigned int row = first ; row < 24 ; row++) {
        refreshText80Row(row, textbases2[row]);
    }
}
