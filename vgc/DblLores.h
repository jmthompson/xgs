#ifndef DBLLORES_H_
#define DBLLORES_H_

#include "emulator/common.h"
#include "VideoMode.h"

class DblLores : public VideoMode {
    private:
        // The dimenions of one lores block in pixels
        const unsigned int kBlockHeight = 8;
        const unsigned int kBlockWidth  = 7;

        // The frame buffers (0 = even cols, 1 = odd columns)
        uint8_t *display_buffer[2];

    public:
        DblLores() = default;
        ~DblLores() = default;

        void setDisplayBuffer(uint8_t *even, uint8_t *odd) {
            display_buffer[0] = even;
            display_buffer[1] = odd;
        }

        void renderLine(const unsigned int, pixel_t *);
};

#endif // DBLLORES_H_
