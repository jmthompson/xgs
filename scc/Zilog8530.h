#ifndef ZILOG8530_H_
#define ZILOG8530_H_

#include <vector>

#include "emulator/Device.h"

class Zilog8530 : public Device {
    friend class VGC;

    private:
        std::vector<unsigned int>& ioReadList()
        {
            static std::vector<unsigned int> locs = {
                0x38, 0x39, 0x3A, 0x3B
            };

            return locs;
        }

        std::vector<unsigned int>& ioWriteList()
        {
            static std::vector<unsigned int> locs = {
                0x38, 0x39, 0x3A, 0x3B
            };

            return locs;
        }

    public:
        Zilog8530() = default;
        ~Zilog8530() = default;

        void reset();
        uint8_t read(const unsigned int& offset);
        void write(const unsigned int& offset, const uint8_t& value);

        void tick(const unsigned int);
        void microtick(const unsigned int);
};

#endif // ZILOG8530_H_
