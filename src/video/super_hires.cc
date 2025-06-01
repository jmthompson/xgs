/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "video.h"

/*
 * This is the Super Hires video mode implementation
 */

// Scaling factor for converting IIGS RGB to 24-bit RGB
constexpr unsigned int kColorScale = 17;

static void getPalette(const unsigned int palette_number, pixel_t *out)
{
    uint8_t *color = Video::display_buffer[0] + 0x7E00 + (palette_number << 5);
    unsigned int r,g,b;

    for (unsigned int i = 0 ; i < 16 ; ++i, color += 2) {

        b = (color[1] & 0x0F) * kColorScale;
        g = ((color[0] >> 4) & 0x0F) * kColorScale;
        r = (color[0] & 0x0F) * kColorScale;

        out[i] = (r << 16) | (g << 8) | b;
    }
}

void Video::renderLine_shr(const unsigned int line_number, pixel_t *line)
{
    unsigned int scb = display_buffer[0][0x7D00 + line_number];
    uint8_t *buffer  = display_buffer[0] + (line_number * 160);
    pixel_t palette[16];

    getPalette(scb & 0x0F, palette);

    if (scb & 0x80) {
        for (unsigned int col = 0 ; col < 160 ; ++col) {
            uint8_t v = buffer[col];

            line[3] = palette[(v & 0x03) + 12];
            line[2] = palette[((v >> 2) & 0x03) + 8];
            line[1] = palette[((v >> 4) & 0x03) + 4];
            line[0] = palette[v >> 6];

            line += 4;
        }
    }
    else {
        bool fill_mode = scb & 0x20;
        unsigned int last_pixel = 0;

        for (unsigned int col = 0 ; col < 160 ; ++col) {
            uint8_t v;

            v = buffer[col] >> 4;

            if (fill_mode && !v) {
                v = last_pixel;
            }
            else {
                last_pixel = v;
            }

            line[0] = line[1] = palette[v];

            v = buffer[col] & 0x0F;

            if (fill_mode && !v) {
                v = last_pixel;
            }
            else {
                last_pixel = v;
            }

            line[2] = line[3] = palette[v];

            line += 4;
        }
    }
}
