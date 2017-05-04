#ifndef LORES_H_
#define LORES_H_

#include "common.h"
#include "VideoMode.h"

class Lores : public VideoMode {
    private:
        // The dimenions of one lores block in pixels
        const unsigned int kBlockHeight = 8;
        const unsigned int kBlockWidth  = 14;

        // The frame buffer
        uint8_t *display_buffer;

    public:
        Lores() = default;
        ~Lores() = default;

        void setDisplayBuffer(uint8_t *buffer) {
            display_buffer = buffer;
        }

        void renderLine(const unsigned int, pixel_t *);
};

#endif // LORES_H_
