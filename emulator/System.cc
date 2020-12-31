/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the functionality of both the FPI/CYA and
 * the Mega II, since they are pretty tightly entwined. As such it
 * emulates the bus interface used the CPU to access the system bus.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/format.hpp>
#include "System.h"
#include "M65816/Processor.h"

using std::cerr;
using std::endl;
using std::uint8_t;
using std::string;

System::System(const bool rom03)
{
    is_rom03 = rom03;

    for (unsigned int page = 0 ; page < System::kNumPages;  page++) {
        read_map[page] = write_map[page] = page;
    }
}

System::~System()
{
    for (auto const& iter : devices) {
        iter.second->detach();
    }
}

void System::installProcessor(M65816::Processor *theCpu)
{
    cpu = theCpu;

    cpu->attach(this);
}

void System::installMemory(uint8_t *mem, const unsigned int start_page, const unsigned int num_pages, mem_page_t type)
{
    unsigned int i, page;
    uint8_t *p;

    for (i = 0, page = start_page, p = mem; i < num_pages ; ++i, ++page, p += System::kPageSize) {
        memory[page].type  = type;
        memory[page].read  = p;
        memory[page].write = (type == ROM? nullptr : p);
    }
}

void System::installDevice(const string& name, Device *d)
{
    devices[name] = d;

    d->attach(this);
}

/**
 * Reset the system to its powerup state.
 */
void System::reset()
{
    for (auto const& iter : devices) {
        iter.second->reset();
    }

    // Must come last so that the Mega2 device has a change to build the
    // language card, otherwise the CPU will fetch an invalid reset vector
    cpu->reset();
}

/**
 * Read the value from a memory location, honoring page remapping, but
 * ignoring I/O areas. Used internally by the emulator to access memory.
 */
uint8_t System::sysRead(const uint8_t bank, const uint16_t address)
{
    const unsigned int page_no = read_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = memory[page_no];

    if (page.read) {
        return page.read[offset];
    }
    else {
        return 0;
    }
}

/**
 * Write a value from a memory location, honoring page remapping, but
 * ignoring I/O areas. Used internally by the emulator to access memory.
 */
void System::sysWrite(const uint8_t bank, const uint16_t address, uint8_t val)
{
    const unsigned int page_no = write_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = memory[page_no];

    if (page.write) {
        page.write[offset] = val;

        if (page.swrite) {
            page.swrite[offset] = val;
        }
    }
}

uint8_t System::cpuRead(const uint8_t bank, const uint16_t address, const M65816::mem_access_t type)
{
    const unsigned int page_no = read_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = (type == M65816::VECTOR)? memory[page_no|0xFF00] : memory[page_no];
    uint8_t val;

    if (page_no == kIOPage) {
        if (Device *dev = io_read[offset]) {
            val = dev->read(offset);
        }
        else {
            val = 0; // FIXME: should be random
        }
    }
    else if (page.read) {
        val = page.read[offset];
    }
    else {
        val = 0;
    }

#ifdef ENABLE_DEBUGGER
    if (debugger) {
        return debugger->memoryRead(bank, address, val, type);
    }
    else {
        return val;
    }
#else
    return val;
#endif
}

void System::cpuWrite(const uint8_t bank, const uint16_t address, uint8_t val, const M65816::mem_access_t type)
{
    const unsigned int page_no = write_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = memory[page_no];

#ifdef ENABLE_DEBUGGER
    if (debugger) { val = debugger->memoryWrite(bank, address, val, type); }
#endif

    if (page_no == kIOPage) {
        if (Device *dev = io_write[offset]) {
            dev->write(offset, val);
        }
    }
    else if (page.write) {
        page.write[offset] = val;

        if (page.swrite) {
            page.swrite[offset] = val;
        }
    }
}

void System::raiseInterrupt(irq_source_t source)
{
    irq_states[source] = true;

//    cerr << boost::format("raiseInterrupt(%d)\n") % source;

    updateIRQ();
}

void System::lowerInterrupt(irq_source_t source)
{
    irq_states[source] = false;

//    cerr << boost::format("lowerInterrupt(%d)\n") % source;

    updateIRQ();
}

void System::updateIRQ()
{
    bool raised = false;

    for (unsigned int i = 0 ; i < 16 ; i++) {
        raised |= irq_states[i];
    }

    cpu->setIRQ(raised);
}
