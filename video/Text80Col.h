#ifndef TEXT80COL_H_
#define TEXT80COL_H_

#include "common.h"
#include "VideoMode.h"

class Text80Col : public VideoMode {
    private:
        // Dimensions of the character matrix in the text font
        const unsigned int kFontWidth  = 7;
        const unsigned int kFontHeight = 8;
        const unsigned int kFontSize   = (kFontWidth * kFontHeight);

        // Pointer to text-mode font data
        const uint8_t *text_font;

        // The frame buffers (0 = even cols, 1 = odd columns)
        uint8_t *display_buffer[2];

        pixel_t fgcolor;
        pixel_t bgcolor;

    public:
        Text80Col() = default;
        ~Text80Col() = default;

        void setTextFont(const uint8_t *new_font) {
            text_font = new_font;
        }

        void setDisplayBuffer(uint8_t *even, uint8_t *odd) {
            display_buffer[0] = even;
            display_buffer[1] = odd;
        }

        void setForeground(pixel_t new_color) { fgcolor = new_color; }
        void setBackground(pixel_t new_color) { bgcolor = new_color; }
        void renderLine(const unsigned int, pixel_t *);
};

#endif // TEXT80COL_H_
