/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * Video mode driver for 80-column text mode
 */

#include "emulator/common.h"
#include "Text80Col.h"

#include "text_bases.h"

void Text80Col::renderLine(const unsigned int line_number, pixel_t *line)
{
    const unsigned int screen_row = line_number / kFontHeight;
    const unsigned int font_row   = line_number % kFontHeight;

    for (unsigned int col = 0 ; col < 80 ; ++col) {
        const uint8_t ch = display_buffer[col & 1][text_bases[screen_row] + (col >> 1)];
        const uint8_t *f = text_font + (ch * kFontSize) + (font_row * kFontWidth);

        for (unsigned int j = 0 ; j < kFontWidth ; j++) {
            *line++ = *f++? fgcolor : bgcolor;
        }
    }
}
