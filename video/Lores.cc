/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * Video mode driver for low-resolution graphics mode
 */

#include <cstdlib>

#include "common.h"
#include "Lores.h"

#include "text_bases.h"
#include "standard_colors.h"

void Lores::renderLine(const unsigned int line_number, pixel_t *line)
{
    const unsigned int screen_row = line_number / (kBlockHeight * 2);
    const bool is_top = (line_number % (kBlockHeight * 2)) < kBlockHeight;

    for (unsigned int col = 0 ; col < 40 ; ++col) {
        const uint8_t val   = display_buffer[text_bases[screen_row] + col];
        const pixel_t color = standard_colors[is_top? val & 0x0F : val >> 4];

        for (unsigned int j = 0 ; j < kBlockWidth ; ++j) *line++ = color;
    }
}
