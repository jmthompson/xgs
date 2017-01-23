#ifndef DBLHIRES_H_
#define DBLHIRES_H_

#include "common.h"
#include "VideoMode.h"

class DblHires : public VideoMode {
    private:
        // The frame buffers (0 = even cols, 1 = odd columns)
        uint8_t *display_buffer[2];

    public:
        DblHires() = default;
        ~DblHires() = default;

        void setDisplayBuffer(uint8_t *even, uint8_t *odd) {
            display_buffer[0] = even;
            display_buffer[1] = odd;
        }

        void renderLine(const unsigned int, pixel_t *);
};

#endif // DBLHIRES_H_
