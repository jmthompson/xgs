/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "soft_switches.h"
#include "video.h"

/**
 * Implementations for the legacy Apple II video mode renderers
 */

// Text mode line starting offsets
static const unsigned int text_bases[24] = {0x0000, 0x0080, 0x0100, 0x0180, 0x0200,
                                            0x0280, 0x0300, 0x0380, 0x0028, 0x00A8,
                                            0x0128, 0x01A8, 0x0228, 0x02A8, 0x0328,
                                            0x03A8, 0x0050, 0x00D0, 0x0150, 0x01D0,
                                            0x0250, 0x02D0, 0x0350, 0x03D0};

// Hires mode line starting offsets
static unsigned int hires_bases[192] = {
    0x0000, 0x0400, 0x0800, 0x0C00, 0x1000, 0x1400, 0x1800, 0x1C00, 0x0080, 0x0480,
    0x0880, 0x0C80, 0x1080, 0x1480, 0x1880, 0x1C80, 0x0100, 0x0500, 0x0900, 0x0D00,
    0x1100, 0x1500, 0x1900, 0x1D00, 0x0180, 0x0580, 0x0980, 0x0D80, 0x1180, 0x1580,
    0x1980, 0x1D80, 0x0200, 0x0600, 0x0A00, 0x0E00, 0x1200, 0x1600, 0x1A00, 0x1E00,
    0x0280, 0x0680, 0x0A80, 0x0E80, 0x1280, 0x1680, 0x1A80, 0x1E80, 0x0300, 0x0700,
    0x0B00, 0x0F00, 0x1300, 0x1700, 0x1B00, 0x1F00, 0x0380, 0x0780, 0x0B80, 0x0F80,
    0x1380, 0x1780, 0x1B80, 0x1F80, 0x0028, 0x0428, 0x0828, 0x0C28, 0x1028, 0x1428,
    0x1828, 0x1C28, 0x00A8, 0x04A8, 0x08A8, 0x0CA8, 0x10A8, 0x14A8, 0x18A8, 0x1CA8,
    0x0128, 0x0528, 0x0928, 0x0D28, 0x1128, 0x1528, 0x1928, 0x1D28, 0x01A8, 0x05A8,
    0x09A8, 0x0DA8, 0x11A8, 0x15A8, 0x19A8, 0x1DA8, 0x0228, 0x0628, 0x0A28, 0x0E28,
    0x1228, 0x1628, 0x1A28, 0x1E28, 0x02A8, 0x06A8, 0x0AA8, 0x0EA8, 0x12A8, 0x16A8,
    0x1AA8, 0x1EA8, 0x0328, 0x0728, 0x0B28, 0x0F28, 0x1328, 0x1728, 0x1B28, 0x1F28,
    0x03A8, 0x07A8, 0x0BA8, 0x0FA8, 0x13A8, 0x17A8, 0x1BA8, 0x1FA8, 0x0050, 0x0450,
    0x0850, 0x0C50, 0x1050, 0x1450, 0x1850, 0x1C50, 0x00D0, 0x04D0, 0x08D0, 0x0CD0,
    0x10D0, 0x14D0, 0x18D0, 0x1CD0, 0x0150, 0x0550, 0x0950, 0x0D50, 0x1150, 0x1550,
    0x1950, 0x1D50, 0x01D0, 0x05D0, 0x09D0, 0x0DD0, 0x11D0, 0x15D0, 0x19D0, 0x1DD0,
    0x0250, 0x0650, 0x0A50, 0x0E50, 0x1250, 0x1650, 0x1A50, 0x1E50, 0x02D0, 0x06D0,
    0x0AD0, 0x0ED0, 0x12D0, 0x16D0, 0x1AD0, 0x1ED0, 0x0350, 0x0750, 0x0B50, 0x0F50,
    0x1350, 0x1750, 0x1B50, 0x1F50, 0x03D0, 0x07D0, 0x0BD0, 0x0FD0, 0x13D0, 0x17D0,
    0x1BD0, 0x1FD0
};

constexpr unsigned int kFontWidth40 = 14;
constexpr unsigned int kFontHeight40 = 8;
constexpr unsigned int kFontSize40 = (kFontWidth40 * kFontHeight40);
constexpr unsigned int kFontWidth80 = 7;
constexpr unsigned int kFontHeight80 = 8;
constexpr unsigned int kFontSize80 = (kFontWidth80 * kFontHeight80);
constexpr unsigned int kBlockHeight = 8;
constexpr unsigned int kBlockWidth = 14;

void Video::renderLine_text40(const unsigned int line_number, pixel_t *line)
{
  const unsigned int screen_row = line_number / kFontHeight40;
  const unsigned int font_row = line_number % kFontHeight40;

  for (unsigned int col = 0; col < 40; ++col) {
    const uint8_t ch = display_buffer[1][text_bases[screen_row] + col];
    const uint8_t *f = text_font + (ch * kFontSize40) + (font_row * kFontWidth40);

    for (unsigned int j = 0; j < kFontWidth40; j++) {
      *line++ = *f++ ? fgcolor : bgcolor;
    }
  }
}

void Video::renderLine_text80(const unsigned int line_number, pixel_t *line)
{
  const unsigned int screen_row = line_number / kFontHeight80;
  const unsigned int font_row = line_number % kFontHeight80;

  for (unsigned int col = 0; col < 80; ++col) {
    const uint8_t ch = display_buffer[col & 1][text_bases[screen_row] + (col >> 1)];
    const uint8_t *f = text_font + (ch * kFontSize80) + (font_row * kFontWidth80);

    for (unsigned int j = 0; j < kFontWidth80; j++) {
      *line++ = *f++ ? fgcolor : bgcolor;
    }
  }
}

void Video::renderLine_lores(const unsigned int line_number, pixel_t *line)
{
  const unsigned int screen_row = line_number / (kBlockHeight * 2);
  const bool is_top = (line_number % (kBlockHeight * 2)) < kBlockHeight;

  for (unsigned int col = 0; col < 40; ++col) {
    const uint8_t val = display_buffer[1][text_bases[screen_row] + col];
    const pixel_t color = Video::standard_colors[is_top ? val & 0x0F : val >> 4];

    for (unsigned int j = 0; j < kBlockWidth; ++j)
      *line++ = color;
  }
}

void Video::renderLine_dlores(const unsigned int line_number, pixel_t *line)
{
  const unsigned int screen_row = line_number / (kBlockHeight * 2);
  const bool is_top = (line_number % (kBlockHeight * 2)) < kBlockHeight;

  for (unsigned int col = 0; col < 80; ++col) {
    const uint8_t val =
        display_buffer[col & 1][text_bases[screen_row] + (col >> 1)];
    const pixel_t color = Video::standard_colors[is_top ? val & 0x0F : val >> 4];

    for (unsigned int j = 0; j < kBlockWidth / 2; ++j)
      *line++ = color;
  }
}

static unsigned int line_buffer[280];

void Video::renderLine_hires(const unsigned int line_number, pixel_t *line)
{
  for (unsigned int col = 0; col < 40; ++col) {
    uint8_t val = display_buffer[1][hires_bases[line_number] + col];
    unsigned int color1, color2;

    if (sw_a2mono) {
      color1 = color2 = 15;
    } else {
      if (col & 0x01) {
        if (val & 0x80) {
          color1 = 9; // Orange
          color2 = 6; // Blue
        } else {
          color1 = 12; // Green
          color2 = 3;  // Purple
        }
      } else {
        if (val & 0x80) {
          color1 = 6; // Blue
          color2 = 9; // Orange
        } else {
          color1 = 3;  // Purple
          color2 = 12; // Green
        }
      }
    }

    unsigned int *b = line_buffer + (col * 7);

    b[0] = (val & 0x01) ? color1 : 0;
    b[1] = (val & 0x02) ? color2 : 0;
    b[2] = (val & 0x04) ? color1 : 0;
    b[3] = (val & 0x08) ? color2 : 0;
    b[4] = (val & 0x10) ? color1 : 0;
    b[5] = (val & 0x20) ? color2 : 0;
    b[6] = (val & 0x40) ? color1 : 0;
  }

  if (!sw_a2mono) {
    for (unsigned int i = 0; i < 279; ++i) {
      if (line_buffer[i] && line_buffer[i + 1]) {
        line_buffer[i] = line_buffer[i + 1] = 15;
      }
    }

    for (unsigned int i = 0; i < 278; ++i) {
      if (line_buffer[i] && (line_buffer[i] != 15) && !line_buffer[i + 1] &&
          (line_buffer[i + 2] == line_buffer[i])) {
        line_buffer[i + 1] = line_buffer[i];
      }
    }

    for (unsigned int i = 279; i > 1; --i) {
      if (line_buffer[i] && (line_buffer[i] != 15) && !line_buffer[i - 1] &&
          (line_buffer[i - 2] == line_buffer[i])) {
        line_buffer[i - 1] = line_buffer[i];
      }
    }
  }

  for (unsigned int i = 0; i < 280; ++i) {
    const pixel_t color = Video::standard_colors[line_buffer[i]];

    *line++ = color;
    *line++ = color;
  }
}

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

void Video::renderLine_dhires(const unsigned int line_number, pixel_t *line)
{
  unsigned int base = hires_bases[line_number];

  for (unsigned int col = 0; col < 20; ++col, base += 2) {
    uint8_t val = (display_buffer[1][base] & 0x7F) |
                  ((display_buffer[0][base] & 0x7F) << 7) |
                  ((display_buffer[1][base + 1] & 0x7F) << 14) |
                  ((display_buffer[0][base + 1] & 0x7F) << 21);

    const uint8_t val2 = (col == 19) ? 0 : (display_buffer[1][base + 2] << 3);

    if (sw_a2mono) {
      for (unsigned int i = 0; i < 28; ++i) {
        *line++ = Video::standard_colors[val & 1 ? 15 : 0];
        val >>= 1;
      }
    } else {
      for (unsigned int i = 0; i < 28; ++i) {
        *line++ = Video::standard_colors[dhr_color_map[i & 0x03][val & 0x0F]];
        val >>= 1;

        if (i == 24) val |= val2;
      }
    }
  }
}
