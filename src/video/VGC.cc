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

#include "standard_colors.h"

using std::cerr;

VGC::VGC()
{
    const unsigned int w = 800;
    const unsigned int h = 600;

    window = SDL_CreateWindow("XGS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        //printSDLError("Failed to create window");

        throw std::runtime_error("SDL_CreateWindow() failed");
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer == nullptr) {
        //printSDLError("Failed to create renderer");

        throw std::runtime_error("SDL_CreateRenderer() failed");
    }

    dest_rect.w = kVideoWidth;
    dest_rect.h = kVideoHeight * 2; // otherwise it will look stretched
    dest_rect.x = (w - dest_rect.w) / 2;
    dest_rect.y = (h - dest_rect.h) / 2;

    SDL_RenderSetLogicalSize(renderer, w, h);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
}

void VGC::reset()
{
    uint8_t *buffer;

    mega2 = (Mega2 *) system->getDevice("mega2");

    sw_vgcint           = false;
    sw_onesecirq_enable = false;
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

    display_buffers.text1_main = system->getPage(0xE004).read;
    display_buffers.text1_aux  = system->getPage(0xE104).read;
    display_buffers.text2_main = system->getPage(0xE008).read;
    display_buffers.text2_aux  = system->getPage(0xE108).read;
    display_buffers.hires1_main = system->getPage(0xE020).read;
    display_buffers.hires1_aux  = system->getPage(0xE120).read;
    display_buffers.hires2_main = system->getPage(0xE040).read;
    display_buffers.hires2_aux  = system->getPage(0xE140).read;
    display_buffers.super_hires = system->getPage(0xE120).read;

    updateBorderColor();
    updateTextColors();
    updateTextFont();

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
            val = sw_vert_cnt >> 1;
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
            updateTextFont();

            break;
        case 0x0F:
            sw_altcharset = true;
            updateTextFont();

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

            if (!(val & 0x01)) {
                cerr << format("WARNING: attempt to enable the A17 bank latch in NEWVIDEO\n");
            }

            modeChanged();

            break;

        case 0x32:
            if (!(val & 0x40)) sw_vgcint &= ~0x40;
            if (!(val & 0x20)) sw_vgcint &= ~0x20;

            if (!(sw_vgcint & 0x60) && (sw_vgcint & 0x80)) {
                system->lowerInterrupt(VGC_IRQ);

                sw_vgcint &= ~0x80;
            }

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
    VideoMode *new_mode;

    if (sw_super) {
        mode_super_hires.setDisplayBuffer(display_buffers.super_hires);

        for (unsigned int i = 0 ; i < 200 ; ++i) modes[i] = &mode_super_hires;
    }
    else {
        VideoMode *new_mode, *mixed_mode;

        if (!mega2->sw_80store && sw_page2) {
            mode_text40.setDisplayBuffer(display_buffers.text2_main);
            mode_text80.setDisplayBuffer(display_buffers.text2_aux, display_buffers.text2_main);

            mode_lores.setDisplayBuffer(display_buffers.text2_main);
            mode_dbl_lores.setDisplayBuffer(display_buffers.text2_aux, display_buffers.text2_main);

            mode_hires.setDisplayBuffer(display_buffers.hires2_main);
            mode_dbl_hires.setDisplayBuffer(display_buffers.hires2_aux, display_buffers.hires2_main);
        }
        else {
            mode_text40.setDisplayBuffer(display_buffers.text1_main);
            mode_text80.setDisplayBuffer(display_buffers.text1_aux, display_buffers.text1_main);

            mode_lores.setDisplayBuffer(display_buffers.text1_main);
            mode_dbl_lores.setDisplayBuffer(display_buffers.text1_aux, display_buffers.text1_main);

            mode_hires.setDisplayBuffer(display_buffers.hires1_main);
            mode_dbl_hires.setDisplayBuffer(display_buffers.hires1_aux, display_buffers.hires1_main);
        }

        if (sw_text) {
            if (sw_80col) {
                new_mode = mixed_mode = &mode_text80;
            }
            else {
                new_mode = mixed_mode = &mode_text40;
            }
        }
        else {
            if (sw_hires) {
                if (sw_80col && sw_dblres) {
                    new_mode = &mode_dbl_hires;
                }
                else {
                    new_mode = &mode_hires;
                }
            }
            else {
                if (sw_80col && sw_dblres) {
                    new_mode = &mode_dbl_lores;
                }
                else {
                    new_mode = &mode_lores;
                }
            }

            if (sw_mixed) {
                if (sw_80col) {
                    mixed_mode = &mode_text80;
                }
                else {
                    mixed_mode = &mode_text40;
                }
            }
            else {
                mixed_mode = new_mode;
            }
        }

        for (unsigned int i = 0 ; i < 160 ; ++i) modes[i] = new_mode;
        for (unsigned int i = 160 ; i < 192 ; ++i) modes[i] = mixed_mode;
    }

    setScreenSize(modes[0]->getWidth(), modes[0]->getHeight());
}

void VGC::setScreenSize(unsigned int w, unsigned int h) {
    content_width  = w;
    content_height = h;

    border_width  = 40;
    border_height = (kLinesPerFrame - content_height) / 2;

    content_top    = border_height;
    content_left   = border_width;
    content_bottom = content_top + content_height - 1;
    content_right  = content_left + content_width - 1;

    video_width  = content_width + (border_width * 2);
    video_height = kLinesPerFrame;

    if (surface != nullptr) {
        SDL_FreeSurface(surface);
    }

    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }

    surface = SDL_CreateRGBSurface(0, video_width, kLinesPerFrame, 32, 0, 0, 0, 0);

    if (surface == nullptr) {
        throw std::runtime_error("SDL_CreateRGBSurface() failed");
    }

    frame_buffer[0] = (pixel_t *) surface->pixels;

    for (unsigned int i = 1 ; i < kLinesPerFrame ; i++) {
        frame_buffer[i] = frame_buffer[i - 1] + video_width;
    }

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, video_width, video_height);
    if (texture == nullptr) {
        throw std::runtime_error("SDL_CreateTexture() failed");
    }

/*
    std::cerr << format("New video mode = %dx%d, border = %dx%d, content area = %dx%d / %d,%d,%d,%d\n")
                    % video_width % video_height
                    % border_width % border_height
                    % content_width % content_height
                    % content_top % content_left % content_bottom % content_right;
*/
}

/**
 * Update the display by sending the current image.
 */
void VGC::tick(const unsigned int frame_number)
{
    SDL_UpdateTexture(texture, NULL, surface->pixels, surface->pitch);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, &dest_rect);
    SDL_RenderPresent(renderer);

    if (sw_onesecirq_enable && !frame_number) {
        if (!(sw_vgcint & 0x40)) {
            sw_vgcint |= 0xC0;

            system->raiseInterrupt(VGC_IRQ);
        }
    }
} 

void VGC::microtick(const unsigned int line_number)
{
    pixel_t *line = frame_buffer[line_number];

    if ((line_number < content_top) || (line_number > content_bottom)) {
        drawBorder(line, video_width);
    }
    else {
        drawBorder(line, content_left);

        modes[line_number - content_top]->renderLine(line_number - content_top, line + content_left);

        drawBorder(line + content_right + 1, video_width - content_right);
    }

    sw_vert_cnt = line_number + 256;

    // Wrap vertical count so that 512-517 maps to 250-255.
    if (sw_vert_cnt > 511) sw_vert_cnt -= kLinesPerFrame;

    if (sw_scanirq_enable && (line_number < 200) && (ram[0x019D00 + line_number] & 0x40)) {
        if (!(sw_vgcint & 0x20)) {
            sw_vgcint |= 0xA0;
            system->raiseInterrupt(VGC_IRQ);
        }
    }
}

void VGC::setRtcControlReg(uint8_t val)
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
                }
                else {
                    bram[clk_addr] = clk_data_reg;
                }
            }
            else {
                if (clk_state & 0x80) {
                    if (!clk_addr) {
                        //clk_curr_time.L = time(NULL) + kClockOffset;
                        clk_curr_time.L = kClockOffset;
                    }
#ifdef BIGENDIAN
                    clk_data_reg = clk_curr_time.B[3 - clk_addr];
#else
                    clk_data_reg = clk_curr_time.B[clk_addr];
#endif
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

void VGC::updateBorderColor()
{
    border = standard_colors[sw_bordercolor];
}

void VGC::updateTextColors()
{
    pixel_t textfg = standard_colors[sw_textfgcolor];
    pixel_t textbg = standard_colors[sw_textbgcolor];

    mode_text40.setForeground(textfg);
    mode_text80.setForeground(textfg);

    mode_text40.setBackground(textbg);
    mode_text80.setBackground(textbg);
}

void VGC::updateTextFont()
{
    mode_text40.setTextFont(font_40col[sw_altcharset]);
    mode_text80.setTextFont(font_80col[sw_altcharset]);
}

void VGC::toggleFullscreen()
{
    fullscreen = !fullscreen;

    SDL_SetWindowFullscreen(window, fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}
