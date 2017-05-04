/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * Video mode driver for high-resolution graphics mode
 */

#include <cstdlib>

#include "common.h"
#include "Hires.h"

#include "hires_bases.h"
#include "standard_colors.h"

static unsigned int line_buffer[280];

// FIXME: pass value from VGC
static bool vid_a2mono = false;

void Hires::renderLine(const unsigned int line_number, pixel_t *line)
{
    for (unsigned int col = 0 ; col < 40 ; ++col) {
        const uint8_t val = display_buffer[hires_bases[line_number] + col];
        unsigned int color1, color2;

        if (vid_a2mono) {
            color1 = color2 = 15;
        }
        else {
            if (col & 0x01) {
                if (val & 0x80) {
                    color1 = 9;     // Orange
                    color2 = 6;     // Blue
                }
                else {
                    color1 = 12;    // Green
                    color2 = 3;     // Purple
                }
            }
            else {
                if (val & 0x80) {
                    color1 = 6;     // Blue
                    color2 = 9;     // Orange
                }
                else {
                    color1 = 3;     // Purple
                    color2 = 12;    // Green
                }
            }
        }

        unsigned int *b = line_buffer + (col * 7);

        b[0] = (val & 0x01)? color1 : 0;
        b[1] = (val & 0x02)? color2 : 0;
        b[2] = (val & 0x04)? color1 : 0;
        b[3] = (val & 0x08)? color2 : 0;
        b[4] = (val & 0x10)? color1 : 0;
        b[5] = (val & 0x20)? color2 : 0;
        b[6] = (val & 0x40)? color1 : 0;
    }

    if (!vid_a2mono) {
        for (unsigned int i = 0 ; i < 279 ; ++i) {
            if (line_buffer[i] && line_buffer[i + 1]) {
                line_buffer[i] = line_buffer[i + 1] = 15;
            }
        }

        for (unsigned int i = 0 ; i < 278 ; ++i) {
            if (line_buffer[i] && (line_buffer[i] != 15) && !line_buffer[i + 1] && (line_buffer[i + 2] == line_buffer[i])) {
                line_buffer[i + 1] = line_buffer[i];
            }
        }

        for (unsigned int i = 279 ; i > 1 ; --i) {
            if (line_buffer[i] && (line_buffer[i] != 15) && !line_buffer[i - 1] && (line_buffer[i - 2] == line_buffer[i])) {
                line_buffer[i - 1] = line_buffer[i];
            }
        }
    }

    for (unsigned int i = 0 ; i < 280 ; ++i) {
        const pixel_t color = standard_colors[line_buffer[i]];

        *line++ = color;
        *line++ = color;
    }
}
