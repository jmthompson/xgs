#ifndef CONFIG_H_
#define CONFIG_H_

#include "emulator/common.h"

const unsigned int kRom01Bytes = 131072;
const unsigned int kRom03Bytes = 262144;

const unsigned int kFont40Bytes = 28672;
const unsigned int kFont80Bytes = 14336;

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
        bool pal;

        uint8_t *font_40col[2];
        uint8_t *font_80col[2];

        struct {
            string smartport[kSmartportUnits];
        } vdisks;

        struct {
            bool trace;
        } debugger;
};

#endif // CONFIG_H_
