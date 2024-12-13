#ifndef TEXT40COL_H_
#define TEXT40COL_H_

#include "emulator/common.h"
#include "emulator/Video.h"
#include "VideoMode.h"

class Text40Col : public VideoMode {
    private:
        // Dimensions of the character matrix in the text font
        const unsigned int kFontWidth  = 14;
        const unsigned int kFontHeight = 8;
        const unsigned int kFontSize   = (kFontWidth * kFontHeight);

        // Pointer to text-mode font data
        const uint8_t *text_font;

        // The frame buffer
        uint8_t *display_buffer;

        pixel_t fgcolor;
        pixel_t bgcolor;

    public:
        Text40Col() = default;
        ~Text40Col() = default;

        void setTextFont(const uint8_t *new_font) {
            text_font = new_font;
        }

        void setDisplayBuffer(uint8_t *buffer) {
            display_buffer = buffer;
        }

        void setForeground(pixel_t new_color) { fgcolor = new_color; }
        void setBackground(pixel_t new_color) { bgcolor = new_color; }
        void renderLine(const unsigned int, pixel_t *);
};

#endif // TEXT40COL_H_
