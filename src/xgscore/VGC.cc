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

#include <cstdlib>
#include <stdexcept>

#include "common.h"

#include "VGC.h"

#include "xgscore/System.h"
#include "xgscore/Mega2.h"
#include "M65816/Processor.h"

void VGC::reset()
{
    mega2 = (Mega2 *) system->getDevice("mega2");

    mode = SUPER_HIRES;

    sw_vgcint           = false;
    sw_onesecirq_enable = false;
    sw_qtrsecirq_enable = false;
    sw_vblirq_enable    = false;
    sw_scanirq_enable   = false;

    sw_80col      = false;
    sw_altcharset = false;
    sw_text       = true;
    sw_mixed      = false;
    sw_page2      = false;
    sw_hires      = false;
    sw_dblres     = false;

    sw_a2mono = false;
    sw_linear = false;
    sw_super  = false;

    sw_bordercolor = 0;
    sw_textbgcolor = 0;
    sw_textfgcolor = 15;

    sw_vert_cnt  = 0;
    sw_horiz_cnt = 0;

    updateTextColors();
    modeChanged();

    clk_data_reg = 0;
    clk_state    = 0;
    clk_ctl_reg &= 0x3F;
}

uint8_t VGC::read(const unsigned int& offset)
{
    uint8_t val = 0;

    switch (offset) {
        case 0x0C:
            sw_80col = false;
            modeChanged();

            break;
        case 0x0D:
            sw_80col = true;
            modeChanged();

            break;
        case 0x0E:
            sw_altcharset = false;
            modeChanged();

            break;
        case 0x0F:
            sw_altcharset = true;
            modeChanged();

            break;
        case 0x19:
            val = in_vbl? 0x80  : 0x00;
            break;
        case 0x1A:
            val = sw_text? 0x80 : 0x00;
            break;
        case 0x1B:
            val = sw_mixed? 0x80 : 0x00;
            break;
        case 0x1C:
            val = sw_page2? 0x80 : 0x00;
            break;
        case 0x1D:
            val = sw_hires? 0x80 : 0x00;
            break;
        case 0x1E:
            val = sw_altcharset? 0x80 : 0x00;
            break;
        case 0x1F:
            val = sw_80col? 0x80 : 0x00;
            break;
        case 0x22:
            val = (sw_textfgcolor << 4) | sw_textbgcolor;
            break;
        case 0x23:
            val = sw_vgcint & 0xF8;
            if (sw_onesecirq_enable) val |= 0x04;
            if (sw_scanirq_enable)   val |= 0x02;

            break;
        case 0x29:
            val = 0x01;
            if (sw_super)  val |= 0x80;
            if (sw_linear) val |= 0x40;
            if (sw_a2mono) val |= 0x20;

            break;
        case 0x2E:
            val = ((sw_vert_cnt + 0xFA) >> 1);
            break;
        case 0x2F:
            sw_horiz_cnt = random() & 0x7F;

            val = ((sw_vert_cnt & 0x01) << 7) | sw_horiz_cnt;

            break;
        case 0x33:
            val = clk_data_reg;
            break;
        case 0x34:
            val = clk_ctl_reg | sw_bordercolor;
            break;
        case 0x50:
            sw_text = false;
            modeChanged();

            break;
        case 0x51:
            sw_text = true;
            modeChanged();

            break;
        case 0x52:
            sw_mixed = false;
            modeChanged();

            break;
        case 0x53:
            sw_mixed = true;
            modeChanged();

            break;
        case 0x54:
            sw_page2 = false;

            if (mega2->sw_80store) {
                mega2->updateMemoryMaps();
            } else {
                modeChanged();
            }

            break;
        case 0x55:
            sw_page2 = true;

            if (mega2->sw_80store) {
                mega2->updateMemoryMaps();
            } else {
                modeChanged();
            }

            break;
        case 0x56:
            sw_hires = false;
            modeChanged();

            break;
        case 0x57:
            sw_hires = true;
            modeChanged();

            break;
        case 0x5E:
            sw_dblres = true;
            modeChanged();

            break;
        case 0x5F:
            sw_dblres = false;
            modeChanged();

            break;
        default:
            break;
    }

    return val;
}

void VGC::write(const unsigned int& offset, const uint8_t& val)
{
    switch (offset) {
        case 0x0C:
            sw_80col = false;
            modeChanged();

            break;
        case 0x0D:
            sw_80col = true;
            modeChanged();

            break;
        case 0x0E:
            sw_altcharset = false;
            modeChanged();

            break;
        case 0x0F:
            sw_altcharset = true;
            modeChanged();

            break;
        case 0x22:
            sw_textfgcolor = (val >> 4) & 0x0F;
            sw_textbgcolor = val & 0x0F;

            updateTextColors();

            break;
        case 0x23:
            sw_onesecirq_enable = val & 0x04;
            sw_scanirq_enable   = val & 0x02;

            break;
        case 0x29:
            sw_super  = val & 0x80;
            sw_linear = val & 0x40;
            sw_a2mono = val & 0x20;

            modeChanged();

            break;

        case 0x32:
            if (!(val & 0x40)) {
                if (sw_vgcint & 0x40) system->cpu->lowerInterrupt();

                sw_vgcint &= ~0x40;
            }

            if (!(val & 0x20)) {
                if (sw_vgcint & 0x20) system->cpu->lowerInterrupt();

                sw_vgcint &= ~0x20;
            }

            if (!(sw_vgcint & 0x60)) sw_vgcint &= ~0x80;

            break;

        case 0x33:
            clk_data_reg = val;

            break;
        case 0x34:
            sw_bordercolor = val & 0x0F;
            setRtcControlReg(val);

            break;
        case 0x50:
            sw_text = false;
            modeChanged();

            break;
        case 0x51:
            sw_text = true;
            modeChanged();

            break;
        case 0x52:
            sw_mixed = false;
            modeChanged();

            break;
        case 0x53:
            sw_mixed = true;
            modeChanged();

            break;
        case 0x54:
            sw_page2 = false;

            if (mega2->sw_80store) {
                mega2->updateMemoryMaps();
            } else {
                modeChanged();
            }

            break;
        case 0x55:
            sw_page2 = true;

            if (mega2->sw_80store) {
                mega2->updateMemoryMaps();
            } else {
                modeChanged();
            }

            break;
        case 0x56:
            sw_hires = false;
            modeChanged();

            break;
        case 0x57:
            sw_hires = true;
            modeChanged();

            break;
        case 0x5E:
            sw_dblres = true;
            modeChanged();

            break;
        case 0x5F:
            sw_dblres = false;
            modeChanged();

            break;
        default:
            break;
    }
}

/**
 * Update the current video mode to match what is selected by the softswitches.
 */
void VGC::modeChanged()
{
    VideoModeType old_mode = mode;

    if (sw_super) {
        mode = SUPER_HIRES;
    }
    else {
        if (sw_text) {
            if (sw_80col) {
                mode = (!mega2->sw_80store && sw_page2)? TEXT_80COL_2 : TEXT_80COL_1;
            }
            else {
                mode = (!mega2->sw_80store && sw_page2)? TEXT_40COL_2 : TEXT_40COL_1;
            }
        } else {
            if (sw_hires) {
                if (sw_dblres && sw_80col) {
                    mode = (!mega2->sw_80store && sw_page2)? DBL_HIRES_2 : DBL_HIRES_1;
                }
                else {
                    mode = (!mega2->sw_80store && sw_page2)? HIRES_2 : HIRES_1;
                }
            }
            else {
                if (sw_dblres && sw_80col) {
                    mode = (!mega2->sw_80store && sw_page2)? DBL_LORES_2 : DBL_LORES_1;
                }
                else {
                    mode = (!mega2->sw_80store && sw_page2)? LORES_2 : LORES_1;
                }
            }
        }
    }
    if (old_mode != mode) {
        if (mode == SUPER_HIRES) {
            setScreenSize(640, 400);
        }
        else {
            setScreenSize(560, 384);
        }
    }
}

void VGC::setScreenSize(unsigned int w, unsigned int h) {
    unsigned int full_height = h + (kBorderSize * 2);
    unsigned int full_width  = w + (kBorderSize * 2);

    if (surface != nullptr) {
        SDL_FreeSurface(surface);
    }

    surface = SDL_CreateRGBSurface(0, full_width, full_height, 32, 0, 0, 0, 0);
    if (surface == nullptr) {
        throw std::runtime_error("SDL_CreateRGBSurface() failed");
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, full_width, full_height);
    if (texture == nullptr) {
        throw std::runtime_error("SDL_CreateTexture() failed");
    }

    Pixel *last = scanlines[0] = (Pixel *) surface->pixels + (full_width * kBorderSize) + kBorderSize;

    for (unsigned int i = 1 ; i < full_height ; i++) {
        last = scanlines[i] = last + full_width;
    }
}

/**
 * Update the display by sending the current image.
 */
void VGC::tick(const unsigned int frame_number)
{
    //(*VID_updateRoutine)();
    refreshText40Page1();

    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

    if (sw_vblirq_enable) {
        if (!(mega2->sw_diagtype & 0x08)) {
            mega2->sw_diagtype |= 0x08;

            system->cpu->raiseInterrupt();
        }
    }

    if (!(frame_number % 15)) {
        if (sw_qtrsecirq_enable) {
            if (!(mega2->sw_diagtype & 0x10)) {
                mega2->sw_diagtype |= 0x10;

                system->cpu->raiseInterrupt();
            }
        }
    }

    if (sw_onesecirq_enable && !frame_number) {
        if (!(sw_vgcint & 0x40)) {
            sw_vgcint |= 0xC0;

            system->cpu->raiseInterrupt();
        }
    }
} 

void VGC::microtick(const unsigned int line_number)
{
    sw_vert_cnt = line_number >> 1;

    in_vbl = (line_number >= 400);

    if (sw_scanirq_enable && (sw_vert_cnt < 200) && (ram[0x019D00 + sw_vert_cnt] & 0x40)) {
        if (!(sw_vgcint & 0x20)) {
            sw_vgcint |= 0xA0;
            system->cpu->raiseInterrupt();
        }
    }
}

void VGC::setRtcControlReg(uint8_t val)
{
    updateTextColors();

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
                clk_addr  = (clk_data_reg & 0x07) << 5;
            }
            else if ((clk_data_reg & 0x73) == 0x01) {
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
                    //std::cerr << boost::format("rtc bram read : %04X = %02X\n") % clk_addr % (unsigned int) clk_data_reg;
                }
                else {
                    bram[clk_addr] = clk_data_reg;
                    //std::cerr << boost::format("rtc bram write : %04X = %02X\n") % clk_addr % (unsigned int) clk_data_reg;
                }
            }
            else {
                if (clk_state & 0x80) {
                    if (!clk_addr) {
                        clk_curr_time.L = time(NULL) + kClockOffset;
                    }
#ifdef BIGENDIAN
                    clk_data_reg = clk_curr_time.B[3 - clk_addr];
#else
                    clk_data_reg = clk_curr_time.B[clk_addr];
#endif
                    //std::cerr << boost::format("rtc clock read : %04X = %02X\n") % clk_addr % (unsigned int) clk_data_reg;
                }
                else {
                    // Setting system time not implemented
                }
            }

            clk_state = 0;

            break;
        default:
            clk_state = 0;

            break;
    }

    clk_ctl_reg &= 0x7F;    // Clear transaction bit since we're done
}
