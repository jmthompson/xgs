#ifndef VGC_H_
#define VGC_H_

#include <cstdlib>
#include <SDL.h>

#include "common.h"
#include "xgscore/Device.h"
#include "Text40Col.h"
#include "Text80Col.h"
#include "Lores.h"
#include "DblLores.h"
#include "Hires.h"
#include "DblHires.h"
#include "SuperHires.h"

class Mega2;

class VGC : public Device {
    friend class Mega2;

    public:
        // Number of video lines in a single frame
        static const unsigned int kLinesPerFrame = 262;

        // Border width. The border height is not fixed
        static const unsigned int kBorderWidth = 40;

        // This is how big the final output image will be.
        static const unsigned int kVideoWidth  = 720;
        static const unsigned int kVideoHeight = kLinesPerFrame;

    private:
        /**
         * This is the difference (in seconds) between the IIGS's time
         * (secs since 1/1/04 00:00:00) and Unix time (secs since
         * 1/1/70 00:00:00).
         */
        constexpr static std::uint32_t kClockOffset = 2082826800;

        Mega2 *mega2;

        uint8_t *ram;

        SDL_Renderer *renderer  = nullptr;
        SDL_Rect     *dest_rect = nullptr;

        SDL_Surface  *surface = nullptr;
        SDL_Texture  *texture = nullptr;

        const uint8_t *font_40col[2];
        const uint8_t *font_80col[2];

        VideoMode *modes[kLinesPerFrame];

        Text40Col mode_text40;
        Text80Col mode_text80;
        Lores     mode_lores;
        DblLores  mode_dbl_lores;
        Hires     mode_hires;
        DblHires  mode_dbl_hires;
        SuperHires mode_super_hires;

        // Pointers to the display buffers for each video page
        struct {
            uint8_t *text1_main;
            uint8_t *text1_aux;
            uint8_t *text2_main;
            uint8_t *text2_aux;
            uint8_t *hires1_main;
            uint8_t *hires1_aux;
            uint8_t *hires2_main;
            uint8_t *hires2_aux;
            uint8_t *super_hires;
        } display_buffers;

        // Actual size of the frames being generated (content + border)
        unsigned int video_width;
        unsigned int video_height;

        // Dimensions of the border
        unsigned int border_width;
        unsigned int border_height;

        // Dimensions of the current video mode
        unsigned int content_width;
        unsigned int content_height;

        // Where the actual video content area begins and ends in the frame
        unsigned int content_left;
        unsigned int content_right;
        unsigned int content_top;
        unsigned int content_bottom;

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

        unsigned int sw_vgcint;

        bool sw_vert_cnt;
        bool sw_horiz_cnt;

        bool sw_onesecirq_enable;
        bool sw_scanirq_enable;

        pixel_t *frame_buffer[kVideoHeight];
        pixel_t border;

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

        uint8_t bram[256];

        union {
            uint8_t B[4];
            time_t  L;
        } clk_curr_time;

        void setRtcControlReg(uint8_t val);
        void setScreenSize(unsigned int w, unsigned int h);
        void modeChanged();

        void updateBorderColor();
        void updateTextColors();
        void updateTextFont();

        inline void drawBorder(pixel_t *line, const unsigned int len)
        {
            unsigned int i = 0;

            for (unsigned int i = 0 ; i < len ; ++i) {
                line[i] = border;
            }
        }

    protected:
        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs = {
                0x0C, 0x0D, 0x0E, 0x0F, 0x1A, 0x1B, 0x1C, 0x1D,
                0x1E, 0x1F, 0x22, 0x23, 0x29, 0x2E, 0x2F, 0x33,
                0x34, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56,
                0x57, 0x5E, 0x5F
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
        VGC() = default;
        ~VGC() = default;

        void setRenderer(SDL_Renderer *, SDL_Rect *);

        void setMemory(uint8_t *r) { ram = r; }

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
