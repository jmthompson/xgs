#ifndef SYSTEM_H_
#define SYSTEM_H_

#include <iostream>
#include <map>
#include <boost/format.hpp>

#include "emulator/common.h"

#include "Device.h"
#include "debugger/Debugger.h"

using std::uint8_t;
using std::uint16_t;

namespace M65816 { class Processor; }

enum mem_page_t {
    UNMAPPED = 0,
    ROM,
    FAST,
    SLOW
};

enum irq_source_t {
    UNKNOWN = 0,
    MEGA2_IRQ,
    VGC_IRQ,
    DOC_IRQ,
    ADB_IRQ
};

struct MemoryPage {
    uint8_t *read    = nullptr;
    uint8_t *write   = nullptr;
    bool    shadowed = false;

    mem_page_t type = UNMAPPED;
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

        std::map<std::string, Device *> devices;

        Device *io_read[kPageSize];
        Device *io_write[kPageSize];

        Device *cop_handler[256];
        Device *wdm_handler[256];

        bool irq_states[16];

        void updateIRQ();

    public:
        vbls_t vbl_count = 0;

        M65816::Processor *cpu;

#ifdef ENABLE_DEBUGGER
        Debugger *debugger;
#endif

        System(const bool);
        ~System();

        void installProcessor(M65816::Processor *);
        void installMemory(uint8_t *, const unsigned int, const unsigned int, mem_page_t);
        void installDevice(const std::string&, Device *);

#ifdef ENABLE_DEBUGGER
        void installDebugger(Debugger *dbg)
        {
            this->debugger = dbg;

            dbg->attach(this);
        }
#endif

        Device *getDevice(const std::string& name) { return devices[name]; }

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

        uint8_t sysRead(const uint8_t, const uint16_t);
        void sysWrite(const uint8_t, const uint16_t, uint8_t);

        uint8_t cpuRead(const uint8_t, const uint16_t, const M65816::mem_access_t);
        void cpuWrite(const uint8_t, const uint16_t, uint8_t, const M65816::mem_access_t);

        void raiseInterrupt(irq_source_t);
        void lowerInterrupt(irq_source_t);
};

#endif // SYSTEM_H_
