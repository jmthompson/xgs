#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstdlib>

using std::uint8_t;

const unsigned int kRom01Bytes = 131072;
const unsigned int kRom03Bytes = 262144;

const unsigned int kFont40Bytes = 57344;
const unsigned int kFont80Bytes = 28672;

struct Config {
    public:
        uint8_t *rom;
        unsigned int rom_start_page;
        unsigned int rom_pages;

        uint8_t *fast_ram;
        unsigned int fast_ram_pages;

        uint8_t *slow_ram;

        bool rom03;
        bool use_debugger;
        unsigned int framerate;

        uint8_t *font_40col[2];
        uint8_t *font_80col[2];
};

#endif // CONFIG_H_
