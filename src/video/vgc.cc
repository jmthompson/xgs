/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the video output features of the Mega II and
 * the VGC.
 */

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory.h>

#include "common.h"
#include "config.h"
#include "interrupts.h"
#include "memory.h"
#include "soft_switches.h"
#include "util.h"
#include "vgc.h"
#include "video.h"

using std::cerr;
using std::endl;

#define DEFINE_TOGGLE(name, post)                                                  \
  static uint8_t clear_##name(const uint8_t offset, const uint8_t _v)              \
  {                                                                                \
    sw_##name = false;                                                             \
    post return 0;                                                                 \
  }                                                                                \
  static uint8_t get_##name(const uint8_t offset, const uint8_t _v)                \
  {                                                                                \
    return sw_##name ? 0x80 : 0x00;                                                \
  }                                                                                \
  static uint8_t set_##name(const uint8_t offset, const uint8_t val)               \
  {                                                                                \
    sw_##name = true;                                                              \
    post return 0;                                                                 \
  }

namespace Vgc
{

static uint8_t font_40col[kFont40Bytes * 2];
static uint8_t font_80col[kFont80Bytes * 2];
static lineRenderer *modes[Video::kLinesPerFrame];

// Dimensions of the border
static unsigned int border_width;
static unsigned int border_height;

// Dimensions of the current video mode
static unsigned int content_width;
static unsigned int content_height;

// Where the actual video content area begins and ends in the frame
static unsigned int content_left;
static unsigned int content_right;
static unsigned int content_top;
static unsigned int content_bottom;

static uint8_t *scbs;

static pixel_t *scanline[Video::kLinesPerFrame];
static pixel_t border;

static uint8_t clk_data_reg;
static uint8_t clk_ctl_reg;

unsigned int video_width;
unsigned int video_height;

/**
 * Clock state. 0 = waiting for command, 1 = waiting for second
 * byte of two-part command, 2 = waiting for data real/write
 * Bit 7 is set for read and clear for write. Bit 6 is set if
 * BRAM is being accessed, or clear if clock register.
 */
static unsigned int clk_state;

// Clock register/BRAM Location being accessed.
static unsigned int clk_addr;

static uint8_t bram[256];

static union {
  uint8_t B[4];
  time_t L;
} clk_curr_time;

static void updateBorderColor() { border = Video::standard_colors[sw_bordercolor]; }

static void updateTextColors()
{
  Video::fgcolor = Video::standard_colors[sw_textfgcolor];
  Video::bgcolor = Video::standard_colors[sw_textbgcolor];
}

void updateTextFont()
{
  if (sw_80col)
    Video::text_font = &font_80col[sw_altcharset];
  else
    Video::text_font = &font_40col[sw_altcharset];
}

static void drawBorder(pixel_t *line, const unsigned int len)
{
  unsigned int i = 0;

  for (unsigned int i = 0; i < len; ++i) {
    line[i] = border;
  }
}

static void setScreenSize(unsigned int w, unsigned int h)
{
  content_width = w;
  content_height = h;

  border_width = 40;
  border_height = (Video::kLinesPerFrame - content_height) / 2;

  content_top = border_height;
  content_left = border_width;
  content_bottom = content_top + content_height - 1;
  content_right = content_left + content_width - 1;

  video_width = content_width + (border_width * 2);
  video_height = Video::kLinesPerFrame;

  scanline[0] = (pixel_t *)Video::frame_buffer;

  for (unsigned int i = 1; i < video_height; ++i) {
    scanline[i] = scanline[i - 1] + video_width;
  }
}

/**
 * Update the current video mode to match what is selected by the softswitches.
 */
static void modeChanged()
{
  unsigned int height, width;

  if (sw_super) {
    width = 640;
    height = 200;
    Video::display_buffer[0] = getPageReadPointer(0xe120);

    for (unsigned int i = 0; i < 200; ++i)
      modes[i] = Video::renderLine_shr;
  } else {
    width = 560;
    height = 192;
    lineRenderer *new_mode, *mixed_mode;

    if (sw_text) {
      if (sw_80col) {
        new_mode = mixed_mode = Video::renderLine_text80;
      } else {
        new_mode = mixed_mode = Video::renderLine_text40;
      }

      if (sw_page2) {
        Video::display_buffer[0] = getPageReadPointer(0xe108);
        Video::display_buffer[1] = getPageReadPointer(0xe008);
      } else {
        Video::display_buffer[0] = getPageReadPointer(0xe104);
        Video::display_buffer[1] = getPageReadPointer(0xe004);
      }
    } else {
      if (sw_hires) {
        if (sw_80col && sw_dblres) {
          new_mode = Video::renderLine_dhires;
        } else {
          new_mode = Video::renderLine_hires;
        }
        if (sw_page2) {
          Video::display_buffer[0] = getPageReadPointer(0xe140);
          Video::display_buffer[1] = getPageReadPointer(0xe040);
        } else {
          Video::display_buffer[0] = getPageReadPointer(0xe120);
          Video::display_buffer[1] = getPageReadPointer(0xe020);
        }
      } else {
        if (sw_80col && sw_dblres) {
          new_mode = Video::renderLine_dlores;
        } else {
          new_mode = Video::renderLine_dlores;
        }
        if (sw_page2) {
          Video::display_buffer[0] = getPageReadPointer(0xe108);
          Video::display_buffer[1] = getPageReadPointer(0xe008);
        } else {
          Video::display_buffer[0] = getPageReadPointer(0xe104);
          Video::display_buffer[1] = getPageReadPointer(0xe004);
        }
      }

      if (sw_mixed) {
        if (sw_80col) {
          mixed_mode = Video::renderLine_text80;
        } else {
          mixed_mode = Video::renderLine_text40;
        }
      } else {
        mixed_mode = new_mode;
      }
    }

    for (unsigned int i = 0; i < 160; ++i)
      modes[i] = new_mode;
    for (unsigned int i = 160; i < 192; ++i)
      modes[i] = mixed_mode;
  }

  setScreenSize(width, height);
}

DEFINE_TOGGLE(80col, modeChanged();)
DEFINE_TOGGLE(altcharset, modeChanged();)
DEFINE_TOGGLE(text, modeChanged();)
DEFINE_TOGGLE(mixed, modeChanged();)
DEFINE_TOGGLE(
    page2, if (sw_80store) { updateMemoryMaps(); } else { modeChanged(); }
)
DEFINE_TOGGLE(hires, modeChanged();)
DEFINE_TOGGLE(dblres, modeChanged();)

static uint8_t get_text_color_reg(const uint8_t _o, const uint8_t _v)
{
  return (sw_textfgcolor << 4) | sw_textbgcolor;
}

static uint8_t set_text_color_reg(const uint8_t _o, const uint8_t val)
{
  sw_textfgcolor = (val >> 4) & 0x0F;
  sw_textbgcolor = val & 0x0F;

  updateTextColors();

  return 0;
}

static uint8_t get_vgcint(const uint8_t _o, const uint8_t _v)
{
  uint8_t val = sw_vgcint & 0xF8;

  if (sw_onesecirq_enable) val |= 0x04;
  if (sw_scanirq_enable) val |= 0x02;

  return val;
}

static uint8_t set_vgcint(const uint8_t _o, const uint8_t val)
{
  sw_onesecirq_enable = val & 0x04;
  sw_scanirq_enable = val & 0x02;

  return 0;
}

static uint8_t get_newvideo(const uint8_t _o, const uint8_t _v)
{
  uint8_t val = 0x01;

  if (sw_super) val |= 0x80;
  if (sw_linear) val |= 0x40;
  if (sw_a2mono) val |= 0x20;

  return val;
}

static uint8_t set_newvideo(const uint8_t _o, const uint8_t val)
{
  sw_super = val & 0x80;
  sw_linear = val & 0x40;
  sw_a2mono = val & 0x20;

  if (!(val & 0x01)) {
    cerr << "WARNING: attempt to enable the A17 bank latch in NEWVIDEO" << endl;
  }

  modeChanged();

  return 0;
}

static uint8_t get_vert_cnt(const uint8_t _o, const uint8_t _v)
{
  return sw_vert_cnt >> 1;
}

static uint8_t get_horiz_cnt(const uint8_t _o, const uint8_t _v)
{
  sw_horiz_cnt = random<int>(0, 255) & 0x7F;
  return ((sw_vert_cnt & 0x01) << 7) | sw_horiz_cnt;
}

static uint8_t get_clk_data(const uint8_t _o, const uint8_t _v)
{
  return clk_data_reg;
}

static uint8_t set_clk_data(const uint8_t _o, const uint8_t val)
{
  clk_data_reg = val;
  return 0;
}

static void setRtcControlReg(uint8_t val)
{
  updateBorderColor();

  clk_ctl_reg = val & 0xF0;

  // need to make sure there is a data transfer going on.
  // if not then just zero the data register.
  if (!(clk_ctl_reg & 0x20)) {
    clk_data_reg = 0;
  }

  switch (clk_state & 0x0F) {
  case 0:
    if ((clk_data_reg & 0x78) == 0x38) {
      clk_state = (clk_data_reg & 0x80) | 0x41;
      clk_addr = (clk_data_reg & 0x07) << 5;
    } else if ((clk_data_reg & 0x73) == 0x01) {
      clk_state = (clk_data_reg & 0x80) | 0x02;
      clk_addr = (clk_data_reg >> 2) & 0x03;
    }

    break;
  case 1:
    clk_addr |= ((clk_data_reg >> 2) & 0x1F);
    clk_state++;

    break;
  case 2:
    if (clk_state & 0x40) {
      if (clk_state & 0x80) {
        clk_data_reg = bram[clk_addr];
      } else {
        bram[clk_addr] = clk_data_reg;
      }
    } else {
      if (clk_state & 0x80) {
        if (!clk_addr) {
          // clk_curr_time.L = time(NULL) + kClockOffset;
          clk_curr_time.L = kClockOffset;
        }
#ifdef BIGENDIAN
        clk_data_reg = clk_curr_time.B[3 - clk_addr];
#else
        clk_data_reg = clk_curr_time.B[clk_addr];
#endif
      } else {
        // Setting system time not implemented
      }
    }

    clk_state = 0;

    break;
  default:
    clk_state = 0;

    break;
  }

  clk_ctl_reg &= 0x7F; // Clear transaction bit since we're done
}

static uint8_t get_clk_ctl(const uint8_t _o, const uint8_t _v)
{
  return clk_ctl_reg | sw_bordercolor;
}

static uint8_t set_clk_ctl(const uint8_t _o, const uint8_t val)
{
  sw_bordercolor = val & 0x0F;
  setRtcControlReg(val);
  return 0;
}
static uint8_t clear_scanint(const uint8_t _o, const uint8_t val)
{
  if (!(val & 0x40)) sw_vgcint &= ~0x40;
  if (!(val & 0x20)) sw_vgcint &= ~0x20;

  if (!(sw_vgcint & 0x60) && (sw_vgcint & 0x80)) {
    lowerInterrupt(VGC_IRQ);

    sw_vgcint &= ~0x80;
  }

  return 0;
}

void start(void)
{
  loadFile(font40_file, sizeof(font_40col), font_40col);
  loadFile(font80_file, sizeof(font_80col), font_80col);

  setIoReadHandler(0x0C, clear_80col);
  setIoReadHandler(0x0D, set_80col);
  setIoReadHandler(0x0E, clear_altcharset);
  setIoReadHandler(0x0F, set_altcharset);
  setIoReadHandler(0x1A, get_text);
  setIoReadHandler(0x1B, get_mixed);
  setIoReadHandler(0x1C, get_page2);
  setIoReadHandler(0x1D, get_hires);
  setIoReadHandler(0x1E, get_altcharset);
  setIoReadHandler(0x1F, get_80col);
  setIoReadHandler(0x22, get_text_color_reg);
  setIoReadHandler(0x23, get_vgcint);
  setIoReadHandler(0x29, get_newvideo);
  setIoReadHandler(0x2E, get_vert_cnt);
  setIoReadHandler(0x2F, get_horiz_cnt);
  setIoReadHandler(0x33, get_clk_data);
  setIoReadHandler(0x34, get_clk_ctl);
  setIoReadHandler(0x50, clear_text);
  setIoReadHandler(0x51, set_text);
  setIoReadHandler(0x52, clear_mixed);
  setIoReadHandler(0x53, set_mixed);
  setIoReadHandler(0x54, clear_page2);
  setIoReadHandler(0x55, set_page2);
  setIoReadHandler(0x56, clear_hires);
  setIoReadHandler(0x57, set_hires);
  setIoReadHandler(0x5E, clear_dblres);
  setIoReadHandler(0x5F, set_dblres);

  setIoWriteHandler(0x0C, clear_80col);
  setIoWriteHandler(0x0D, set_80col);
  setIoWriteHandler(0x0E, clear_altcharset);
  setIoWriteHandler(0x0F, set_altcharset);
  setIoWriteHandler(0x22, set_text_color_reg);
  setIoWriteHandler(0x23, set_vgcint);
  setIoWriteHandler(0x29, set_newvideo);
  setIoWriteHandler(0x32, clear_scanint);
  setIoWriteHandler(0x33, set_clk_data);
  setIoWriteHandler(0x34, set_clk_ctl);
  setIoWriteHandler(0x50, clear_text);
  setIoWriteHandler(0x51, set_text);
  setIoWriteHandler(0x52, clear_mixed);
  setIoWriteHandler(0x53, set_mixed);
  setIoWriteHandler(0x54, clear_page2);
  setIoWriteHandler(0x55, set_page2);
  setIoWriteHandler(0x56, clear_hires);
  setIoWriteHandler(0x57, set_hires);
  setIoWriteHandler(0x5E, clear_dblres);
  setIoWriteHandler(0x5F, set_dblres);

  scbs = getPageReadPointer(0xe19d);
}

void stop(void) {}

void reset(void)
{
  uint8_t *buffer;

  sw_vgcint = false;
  sw_onesecirq_enable = false;
  sw_scanirq_enable = false;

  sw_80col = false;
  sw_altcharset = false;
  sw_text = true;
  sw_mixed = false;
  sw_page2 = false;
  sw_hires = false;
  sw_dblres = false;

  sw_a2mono = false;
  sw_linear = false;
  sw_super = false;

  sw_bordercolor = 0;
  sw_textbgcolor = 0;
  sw_textfgcolor = 15;

  sw_vert_cnt = 0;
  sw_horiz_cnt = 0;

  updateBorderColor();
  updateTextColors();
  updateTextFont();
  modeChanged();

  clk_data_reg = 0;
  clk_state = 0;
  clk_ctl_reg &= 0x3f;
}

/**
 * Update the display by sending the current image.
 */
void tick(const unsigned int frame_number)
{
  if (sw_onesecirq_enable && !frame_number) {
    if (!(sw_vgcint & 0x40)) {
      sw_vgcint |= 0xC0;

      raiseInterrupt(VGC_IRQ);
    }
  }
}

void microtick(const unsigned int line_number)
{
  pixel_t *line = scanline[line_number];

  if ((line_number < content_top) || (line_number > content_bottom)) {
    drawBorder(line, video_width);
  } else {
    drawBorder(line, content_left);

    modes[line_number - content_top](
        line_number - content_top, line + content_left
    );

    drawBorder(line + content_right + 1, video_width - content_right);
  }

  sw_vert_cnt = line_number + 256;

  // Wrap vertical count so that 512-517 maps to 250-255.
  if (sw_vert_cnt > 511) sw_vert_cnt -= Video::kLinesPerFrame;

  if (sw_scanirq_enable && (line_number < 200) && (scbs[line_number] & 0x40)) {
    if (!(sw_vgcint & 0x20)) {
      sw_vgcint |= 0xA0;
      raiseInterrupt(VGC_IRQ);
    }
  }
}

} // namespace Vgc
