/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * Video mode driver for double high-resolution graphics mode
 */

#include <cstdlib>

#include "common.h"
#include "DblHires.h"

#include "hires_bases.h"
#include "standard_colors.h"

/**
 * Map DHR pixel values to standard colors. Pixels are grouped into
 * spans four; the particular mapping depends on the pixel's offset
 * in the span.
 */
static const unsigned int dhr_color_map[4][16] = {
    { 0, 2, 4,  6, 8, 10, 12, 14, 1,  3,  5,  7,  9, 11, 13, 15 },
    { 0, 4, 8, 12, 1,  5,  9, 13, 2,  6, 10, 14,  3,  7, 11, 15 },
    { 0, 8, 1,  9, 2, 10,  3, 11, 4, 12,  5, 13,  6, 14,  7, 15 },
    { 0, 1, 2,  3, 4,  5,  6,  7, 8,  9, 10, 11, 12, 13, 14, 15 }
};

// FIXME: pass value from VGC
static bool vid_a2mono = false;

void DblHires::renderLine(const unsigned int line_number, pixel_t *line)
{
    unsigned int base = hires_bases[line_number];

    for (unsigned int col = 0 ; col < 20 ; ++col, base += 2) {
        uint8_t val = (display_buffer[1][base] & 0x7F)
                    | ((display_buffer[0][base] & 0x7F) << 7)
                    | ((display_buffer[1][base + 1] & 0x7F) << 14)
                    | ((display_buffer[0][base + 1] & 0x7F) << 21);

		const uint8_t val2 = (col == 19)? 0 : (display_buffer[1][base + 2] << 3);

		if (vid_a2mono) {
			for (unsigned int i = 0 ; i < 28 ; ++i) {
                *line++ = val & 1? standard_colors[15] : standard_colors[0];
				val >>= 1;
			}
		}
        else {
			for (unsigned int i = 0 ; i < 28 ; ++i) {
				*line++ = standard_colors[dhr_color_map[i & 0x03][val & 0x0F]];
				val >>= 1;

				if (i == 24) val |= val2;
			}
		}
	}
}
