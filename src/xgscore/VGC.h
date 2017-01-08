#ifndef VGC_H_
#define VGC_H_

#include <cstdlib>
#include <iostream>
#include <SDL.h>

#include "xgscore/Device.h"

enum VideoModeType {
    TEXT_40COL_1 = 0,
    TEXT_40COL_2,
    TEXT_80COL_1,
    TEXT_80COL_2,
    LORES_1,
    LORES_2,
    HIRES_1,
    HIRES_2,
    DBL_LORES_1,
    DBL_LORES_2,
    DBL_HIRES_1,
    DBL_HIRES_2,
    SUPER_HIRES
};

typedef std::uint32_t Pixel;

static const Pixel text_colors[16] = {
    0x000000,
    0xDD0033,
    0x000099,
    0xDD22DD,
    0x007722,
    0x555555,
    0x2222FF,
    0x66AAFF,
    0x885500,
    0xFF6600,
    0xAAAAAA,
    0xFF9988,
    0x00DD00,
    0xFFFF00,
    0x55FF99,
    0xFFFFFF
};

class Mega2;

class VGC : public Device {
    friend class Mega2;

    private:
        /**
         * This is the difference (in seconds) between the IIGS's time
         * (secs since 1/1/04 00:00:00) and Unix time (secs since
         * 1/1/70 00:00:00).
         */
        constexpr static std::uint32_t kClockOffset = 2082826800;

        Mega2 *mega2;

        uint8_t *ram;

        SDL_Renderer *renderer = nullptr;
        SDL_Surface  *surface  = nullptr;
        SDL_Texture  *texture  = nullptr;

        VideoModeType mode = TEXT_40COL_1;

        const uint8_t *font_40col[2];
        const uint8_t *font_80col[2];

        bool sw_super;
        bool sw_linear;
        bool sw_a2mono;

        unsigned int sw_bordercolor;
        unsigned int sw_textfgcolor;
        unsigned int sw_textbgcolor;

        bool sw_80col;
        bool sw_altcharset;

        bool sw_text;
        bool sw_mixed;
        bool sw_page2;
        bool sw_hires;
        bool sw_dblres;

        bool sw_vgcint;

        bool sw_vert_cnt;
        bool sw_horiz_cnt;

        bool sw_onesecirq_enable;
        bool sw_qtrsecirq_enable;
        bool sw_vblirq_enable;
        bool sw_scanirq_enable;

        bool in_vbl;

        Pixel *scanlines[480];
        Pixel border,textfg,textbg;

        uint8_t clk_data_reg;
        uint8_t clk_ctl_reg;

        /**
         * Clock state. 0 = waiting for command, 1 = waiting for second
         * byte of two-part command, 2 = waiting for data real/write
         * Bit 7 is set for read and clear for write. Bit 6 is set if
         * BRAM is being accessed, or clear if clock register.
         */
        unsigned int clk_state;

        // Clock register/BRAM Location being accessed.
        unsigned int clk_addr;

        uint8_t bram[256] = {
0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x0D, 0x06, 0x02, 0x01, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
0x00, 0x00, 0x07, 0x06, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x0F, 0x06, 0x06, 0x00, 0x05, 0x06,
0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05, 0x02, 0x02, 0x00,
0x00, 0x00, 0x2D, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02, 0x02, 0x06, 0x08, 0x00,
0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x0A, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x36, 0x2D, 0x9C, 0x87
        };

        union {
            uint8_t B[4];
            time_t  L;
        } clk_curr_time;

        void setScreenSize(unsigned int w, unsigned int h);
        void modeChanged();

        // Convert a byte from the text font files to a text foreground or background pixel
        inline Pixel fontToPixel(const uint8_t font_byte) { return font_byte == 253? textbg : textfg; }

        // Update the text & border pixels to match the current register values
        void updateTextColors()
        {
            textfg = text_colors[sw_textfgcolor];
            textbg = text_colors[sw_textbgcolor];
            border = text_colors[sw_bordercolor];
        }

        void setRtcControlReg(uint8_t val);

        void refreshText40Row(const unsigned int, uint16_t);
        void refreshText40Page1();
        void refreshText40Page2();
        void refreshText80Row(const unsigned int, uint16_t);
        void refreshText80Page1();
        void refreshText80Page2();

    protected:
        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs = {
                0x0C, 0x0D, 0x0E, 0x0F, 0x19, 0x1A, 0x1B, 0x1C,
                0x1D, 0x1E, 0x1F, 0x22, 0x23, 0x29, 0x2E, 0x2F,
                0x33, 0x34, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
                0x56, 0x57, 0x5E, 0x5F
            };

            return locs;
        }

        std::vector<unsigned int>& ioWriteList()
        {
            static std::vector<unsigned int> locs = {
                0x0C, 0x0D, 0x0E, 0x0F, 0x22, 0x23, 0x29, 0x32,
                0x33, 0x34, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55,
                0x56, 0x57, 0x5E, 0x5F
            }; 

            return locs;
        }

    public:
        // Size of the border.
        static const unsigned int kBorderSize = 40;

        // Dimensions of the usable video area. The height is doubled
        // to make the aspect ratio square (an actual IIgs is 640x200)
        static const unsigned int kVideoWidth  = 640;
        static const unsigned int kVideoHeight = 400;

        // Number of video lines in a single frame
        static const unsigned int kLinesPerFrame = 524;

        VGC() = default;
        ~VGC() = default;

        void setMemory(uint8_t *r) { ram = r; }
        void setRenderer(SDL_Renderer *r) { renderer = r; }

        void setFont40(const uint8_t *main, const uint8_t *alt)
        {
            font_40col[0] = main;
            font_40col[1] = alt;
        }

        void setFont80(const uint8_t *main, const uint8_t *alt)
        {
            font_80col[0] = main;
            font_80col[1] = alt;
        }

        void reset();
        uint8_t read(const unsigned int& offset);
        void write(const unsigned int& offset, const uint8_t& value);

        void tick(const unsigned int);
        void microtick(const unsigned int);
};

#endif // VGC_H_
