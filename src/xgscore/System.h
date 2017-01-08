#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <iostream>
#include <map>
#include <boost/format.hpp>

#include "gstypes.h"
#include "Device.h"

using std::map;
using std::uint8_t;
using std::uint16_t;

namespace M65816 {
    class Processor;
}

class Mega2;
class VGC;

enum MemoryPageType {
    UNMAPPED = 0,
    ROM,
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
        // Size of a page in bytes
        static constexpr unsigned int kPageSize = 256;

        // Size of a bank in bytes
        static constexpr unsigned int kBankSize = 65536;

        // How many memory pages there are
        static constexpr unsigned int kNumPages = 65536;

        // Highest directly addressable page number 
        static constexpr unsigned int kMaxPage  = kNumPages - 1;

        // The I/O page sits above addressable memory and can
        // only be accessed via a read or write mapping.
        static constexpr unsigned int kIOPage = kMaxPage + 1;

        bool is_rom03;

        unsigned int read_map[kNumPages];
        unsigned int write_map[kNumPages];

        MemoryPage memory[kNumPages];

        map<string, Device *> devices;

        Device *io_read[kPageSize];
        Device *io_write[kPageSize];

        Device *cop_handler[256];
        Device *wdm_handler[256];

    public:
        cycles_t cycle_count = 0;
        vbls_t   vbl_count   = 0;

        M65816::Processor *cpu;

        System(const bool);
        ~System();

        void installProcessor(M65816::Processor *);
        void installMemory(uint8_t *, const unsigned int, const unsigned int, MemoryPageType);
        void installDevice(const string&, Device *);

        Device *getDevice(const string& name) { return devices[name]; }

        void reset();

        void handleCop(uint8_t command)
        {
            if (Device *dev = cop_handler[command]) {
                return dev->cop(command);
            }
        }

        void handleWdm(uint8_t command)
        {
            if (Device *dev = wdm_handler[command]) {
                return dev->wdm(command);
            }
        }

        inline void mapRead(const unsigned int src_page, const unsigned int dst_page)
        {
            read_map[src_page] = dst_page;
        }

        inline void mapWrite(const unsigned int src_page, const unsigned int dst_page)
        {
            write_map[src_page] = dst_page;
        }

        inline void mapIO(const unsigned int src_page)
        {
            read_map[src_page] = write_map[src_page] = kIOPage;
        }

        inline void setShadowed(const unsigned int page, const bool isShadowed)
        {
            memory[page].shadowed = isShadowed;
        }

        inline void setIoRead(const unsigned int& offset, Device *device)
        {
            io_read[offset] = device;
        }

        inline void setIoWrite(const unsigned int& offset, Device *device)
        {
            io_write[offset] = device;
        }

        inline void setCopHandler(const unsigned int& command, Device *device)
        {
            cop_handler[command] = device;
        }

        inline void setWdmHandler(const unsigned int& command, Device *device)
        {
            wdm_handler[command] = device;
        }

        MemoryPage& getPage(const unsigned int page)
        {
            return memory[page];
        }

        uint8_t cpuRead(uint8_t bank, uint16_t address)
        {
            const unsigned int page_no = read_map[(bank << 8) | (address >> 8)];
            const unsigned int offset  = address & 0xFF;
            MemoryPage& page = memory[page_no];

            if (page_no == kIOPage) {
                if (Device *dev = io_read[offset]) {
                    return dev->read(offset);
                }
                else {
                    return 0; // FIXME: should be random
                }
            }
            else if (page.read) {
                return page.read[offset];
            }
            else {
                return 0;
            }
        }

        void cpuWrite(uint8_t bank, uint16_t address, uint8_t val)
        {
            const unsigned int page_no = write_map[(bank << 8) | (address >> 8)];
            const unsigned int offset  = address & 0xFF;
            MemoryPage& page = memory[page_no];

            if (page_no == kIOPage) {
                if (Device *dev = io_write[offset]) {
                    dev->write(offset, val);
                }
            }
            else if (page.write) {
                page.write[offset] = val;

                if (page.shadowed) {
                    MemoryPage& spage = memory[(page_no & 0x01FF) | 0xE000];

                    spage.write[offset] = val;
                }
            }
        }
};

#endif // SYSTEM_H_
