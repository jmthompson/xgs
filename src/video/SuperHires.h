#ifndef SUPERHIRES_H_
#define SUPERHIRES_H_

#include "common.h"
#include "VideoMode.h"

class SuperHires : public VideoMode {
    private:
        // Scaling factor for converting IIGS RGB to 24-bit RGB
        const unsigned int kColorScale = 17;

        // The frame buffer
        uint8_t *display_buffer;

        void getPalette(const unsigned int palette_number, pixel_t *out);

    public:
        SuperHires() = default;
        ~SuperHires() = default;

        virtual unsigned int getWidth() { return 640; }
        virtual unsigned int getHeight() { return 200; }

        void setDisplayBuffer(uint8_t *buffer) { display_buffer = buffer; }
        void renderLine(const unsigned int, pixel_t *);
};

#endif // SUPERHIRES_H_
