#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <iostream>
#include <string>

#include "config.h"

#include <boost/format.hpp>

using std::cerr;
using std::string;
using std::uint8_t;
using std::uint16_t;

// How many memory pages there are
const unsigned int NUM_PAGES = 65536;

// Highest directly addressable page number 
const unsigned int MAX_PAGE  = NUM_PAGES - 1;

// The I/O page sits above addressable memory and can
// only be accessed via a read or write mapping.
const unsigned int IO_PAGE = MAX_PAGE + 1;

// Size of a page in bytes
const unsigned int PAGE_SIZE = 256;

// Size of a bank in bytes
const unsigned int BANK_SIZE = 65536;

enum MemoryPageType {
    UNMAPPED = 0,
    FAST,
    SLOW
};

struct MemoryPage {
    uint8_t *read    = nullptr;
    uint8_t *write   = nullptr;
    bool    shadowed = false;

    MemoryPageType type = UNMAPPED;
};

class System {
    private:
        uint8_t *fast_ram;
        uint8_t *slow_ram;
        uint8_t *rom;

        unsigned int rom_version;

        unsigned int read_map[NUM_PAGES];
        unsigned int write_map[NUM_PAGES];

        MemoryPage memory[NUM_PAGES];

        bool sw_80store;
        bool sw_auxrd;
        bool sw_auxwr;
        bool sw_altzp;

        bool sw_lcbank2;
        bool sw_lcread;
        bool sw_lcwrite;
        bool sw_lcsecond;

        bool sw_shadow_text;
        bool sw_shadow_text2;
        bool sw_shadow_hires1;
        bool sw_shadow_hires2;
        bool sw_shadow_super;
        bool sw_shadow_aux;
        bool sw_shadow_lc;

        bool sw_slot_reg[8];

        bool sw_intcxrom;
        bool sw_slotc3rom;
        bool sw_rombank;

        bool sw_diagtype;

        // temporary
        bool vid_hires = false;
        bool vid_page2 = false;

        void updateMemoryMaps();
        void buildLanguageCard(unsigned int bank, unsigned int src_bank);

        uint8_t ioRead(uint8_t io_offset);
        void    ioWrite(uint8_t io_offset, uint8_t value);

    public:
        System(unsigned int ramSize, const string& romPath);
        ~System();

        void reset();

        void handleCop(uint8_t operand);
        void handleWdm(uint8_t operand);

        inline uint8_t cpuRead(uint8_t bank, uint16_t address)
        {
            unsigned int page_no = read_map[(bank << 8) | (address >> 8)];
            unsigned int offset  = address & 0xFF;
            MemoryPage& page     = memory[page_no];

            if (page_no == IO_PAGE) {
                return ioRead(offset);
            }
            else if (page.read) {
                return page.read[offset];
            }
            else {
                return 0;
            }
        }

        void inline cpuWrite(uint8_t bank, uint16_t address, uint8_t val)
        {
            unsigned int page_no = write_map[(bank << 8) | (address >> 8)];
            unsigned int offset  = address & 0xFF;
            MemoryPage& page     = memory[page_no];

            //cerr << boost::format("write: %02X => %02X/%04X\n") % (int) val % (int) bank % address;

            if (page_no == IO_PAGE) {
                return ioWrite(offset, val);
            }
            else if (page.write == nullptr) {
                return;
            }
            else {
#if 0
                if ((page.type == VIDEO) && (page.write[offset] != val) {
                    m_slowram_changed[page_no & 0x1FF] |= m_change_masks[offset];
                }
#endif
                page.write[offset] = val;

                if (page.shadowed) {
                    MemoryPage& spage = memory[(page_no & 0x01FF) + 0xE000];

                    spage.write[offset] = val;
                }
            }
        }
};

#endif // SYSTEM_H_
