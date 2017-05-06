#ifndef HIRES_H_
#define HIRES_H_

#include "emulator/common.h"
#include "VideoMode.h"

class Hires : public VideoMode {
    private:
        // The frame buffer
        uint8_t *display_buffer;

    public:
        Hires() = default;
        ~Hires() = default;

        void setDisplayBuffer(uint8_t *buffer) {
            display_buffer = buffer;
        }

        void renderLine(const unsigned int, pixel_t *);
};

#endif // HIRES_H_
