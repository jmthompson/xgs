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

void System::installMemory(uint8_t *mem, const unsigned int start_page, const unsigned int num_pages, MemoryPageType type)
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
