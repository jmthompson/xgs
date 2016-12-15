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

#include "System.h"

using std::cout;
using std::endl;
using std::ifstream;
using std::uint8_t;
using std::string;

System::System(const unsigned int ramSize, const string &romPath)
{
    unsigned int ram_pages = ramSize << 12;

    ifstream romfile(romPath, std::ifstream::binary);
    // TODO: check romfile.fail()
    romfile.seekg(0, romfile.end);

    unsigned int romPages = romfile.tellg() >> 8;

    romfile.seekg(0, romfile.beg);
    cout << "about to allocate " << romPages << " pages of rom" << endl;
    rom = new uint8_t[romPages * 256];

    romfile.read((char *) rom, romPages * 256);
    romfile.close();

    slow_ram = new uint8_t[2 * BANK_SIZE];
    fast_ram = new uint8_t[ramSize * 1048576];
    ram_pages = ramSize * 4096;

    if (romPages == 1024) {
        rom_version = 3;
    }
    else {
        rom_version = 1;
    }

    cout << "Read ROM v" << rom_version << endl;

    // Initialize the static parts of the memory map

    unsigned int page;
    uint8_t *p;

    for (page = 0 ; page < NUM_PAGES;  page++) {
        read_map[page]  = write_map[page] = page;
    }

    for (page = 0, p = fast_ram ; page < ram_pages ; page++, p += PAGE_SIZE) {
        memory[page].read = memory[page].write = p;
        memory[page].type = FAST;
    }

    for (page = 0xE000, p = slow_ram ; page < 0xE200 ; page++, p += PAGE_SIZE) {
        memory[page].read = memory[page].write = p;
        memory[page].type = SLOW;
    }

    for (page = MAX_PAGE - romPages + 1, p = rom ; page <= MAX_PAGE ; page++, p += PAGE_SIZE) {
        memory[page].read  = p;
        memory[page].write = nullptr;
        memory[page].type  = FAST;
    }

    reset();
}

System::~System()
{
    delete[] fast_ram;
    delete[] slow_ram;
    delete[] rom;
}

/**
 * Reset the system to its powerup state.
 */
void System::reset()
{
    sw_80store = false;
    sw_auxrd   = false;
    sw_auxwr   = false;
    sw_altzp   = false;

    sw_lcbank2  = false;
    sw_lcread   = false;
    sw_lcwrite  = false;
    sw_lcsecond = false;

    sw_shadow_text   = true;
    sw_shadow_text2  = false;
    sw_shadow_hires1 = true;
    sw_shadow_hires2 = true;
    sw_shadow_super  = false;
    sw_shadow_aux    = true;
    sw_shadow_lc     = true;

    for (int i = 0 ; i < 5 ; i++) sw_slot_reg[i] = false;

    sw_slot_reg[7] = true;

    updateMemoryMaps();
}

void System::handleCop(uint8_t operand)
{
}

void System::handleWdm(uint8_t operand)
{
}

void System::updateMemoryMaps()
{
    unsigned int page;

    /*
     * Legacy main/aux memory Bank switching
     */

    for (page = 0x0000 ; page < 0x0002 ; page++) {
        read_map[page] = write_map[page] = sw_altzp? page + 0x0100 : page;
    }
    for (page = 0x0002 ; page < 0x0003 ; page++) {
        read_map[page]  = sw_auxrd? page + 0x0100 : page;
        write_map[page] = sw_auxwr? page + 0x0100 : page;
    }
    for (page = 0x0004 ; page < 0x0008 ; page++) {
        if (sw_80store) {
            read_map[page] = write_map[page] = vid_page2? page + 0x0100 : page;
        }
        else {
            read_map[page]  = sw_auxrd? page + 0x0100 : page;
            write_map[page] = sw_auxwr? page + 0x0100 : page;
        }
    }
    for (page = 0x0008 ; page < 0x0020 ; page++) {
        read_map[page]  = sw_auxrd? page + 0x0100 : page;
        write_map[page] = sw_auxwr? page + 0x0100 : page;
    }
    for (page = 0x0020 ; page < 0x0040 ; page++) {
        if (sw_80store && vid_hires) {
            read_map[page] = write_map[page] = vid_page2? page + 0x0100 : page;
        }
        else {
            read_map[page]  = sw_auxrd? page + 0x0100 : page;
            write_map[page] = sw_auxwr? page + 0x0100 : page;
        }
    }
    for (page = 0x0040 ; page < 0x00C0 ; page++) {
        read_map[page]  = sw_auxrd? page + 0x0100 : page;
        write_map[page] = sw_auxwr? page + 0x0100 : page;
    }

    /*
     * Language cards
     */

    buildLanguageCard(0xE0, 0xE0);
    buildLanguageCard(0xE1, 0xE1);

    if (sw_shadow_lc) {
        buildLanguageCard(0x00, sw_altzp? 0x01 : 0x00);
        buildLanguageCard(0x01, 0x01);
    }
    else {
        for (page = 0x00C0 ; page <= 0x00FF ; page++) {
            read_map[page]  = sw_auxrd? page + 0x0100 : page;
            write_map[page] = sw_auxwr? page + 0x0100 : page;
        }
        for (page = 0x01C0 ; page <= 0x01FF ; page++) {
            read_map[page] = write_map[page] = page;
        }
    }

    /*
     * Shadowing
     * FIXME: support shadowing in all banks
     */

    for (page = 0x0004 ; page < 0x0008 ; page++) {
        memory[page].shadowed = sw_shadow_text;
    }
    for (page = 0x0008 ; page < 0x000C ; page++) {
        memory[page].shadowed = sw_shadow_text2;
    }
    for (page = 0x0020 ; page < 0x0040 ; page++) {
        memory[page].shadowed = sw_shadow_hires1;
    }
    for (page = 0x0060 ; page < 0x0080 ; page++) {
        memory[page].shadowed = sw_shadow_hires2;
    }
    for (page = 0x0104 ; page < 0x0108 ; page++) {
        memory[page].shadowed = sw_shadow_text;
    }
    for (page = 0x0108 ; page < 0x010C ; page++) {
        memory[page].shadowed = sw_shadow_text2;
    }
    for (page = 0x0120 ; page < 0x0140 ; page++) {
        memory[page].shadowed = (sw_shadow_hires1 && sw_shadow_aux) || sw_shadow_super;
    }
    for (page = 0x0160 ; page < 0x0180 ; page++) {
        memory[page].shadowed = (sw_shadow_hires2 && sw_shadow_aux) || sw_shadow_super;
    }
}

/**
 * Build a 16k language card in dst_bank, using RAM pages from src_bank. The
 * separate src_bank is used to account for the ALTZP softswitch when building
 * the bank 0 language card.
 */
void System::buildLanguageCard(unsigned int dst_bank, unsigned int src_bank)
{
    unsigned int page = 0xC0;

    cout << std::hex;

    dst_bank <<= 8;
    src_bank <<= 8;

    read_map[dst_bank|0xC0] = write_map[dst_bank|0xC0] = IO_PAGE;

    for (page = 0xC0 ; page <= 0xCF ; page++) {
        read_map[dst_bank|page] = write_map[dst_bank|page] = 0xFF00|page;
    }

    unsigned int lcpage = sw_lcbank2? 0xD0 : 0xC0;

    for (page = 0xD0 ; page <= 0xDF ; page++) {
        read_map[dst_bank|page]  = sw_lcread?  src_bank|lcpage : 0xFF00|page;
        write_map[dst_bank|page] = sw_lcwrite? src_bank|lcpage : 0xFF00|page;
    }

    for (page = 0xE0 ; page <= 0xFF ; page++) {
        read_map[dst_bank|page]  = sw_lcread?  src_bank|page : 0xFF00|page;
        write_map[dst_bank|page] = sw_lcwrite? src_bank|page : 0xFF00|page;
    }
}

#if 0

byte MEM_getSlotReg(byte val)
{
    register int    i;

    val = 0;
    for (i = 7 ; i >= 0 ; i--) {
        val = val << 1;
        val |= m_slot_reg[i];
    }
    return val;
}

byte MEM_setSlotReg(byte val)
{
    register int    i;

    for (i = 0 ; i < 8 ; i++) {
        m_slot_reg[i] = val & 0x01;
        val = val >> 1;
    }
    updateMemoryMaps();
    return 0;
}

byte MEM_getCYAReg(byte val)
{
    val = 0;
    if (g_fastmode) val |= 0x80;
    if (iwm_slot7_motor) val |= 0x08;
    if (iwm_slot6_motor) val |= 0x04;
    if (iwm_slot5_motor) val |= 0x02;
    if (iwm_slot4_motor) val |= 0x01;
    return val;
}

byte MEM_setCYAReg(byte val)
{
    g_fastmode = (val & 0x80)? 1 : 0;
    iwm_slot7_motor = (val & 0x08)? 1 : 0;
    iwm_slot6_motor = (val & 0x04)? 1 : 0;
    iwm_slot5_motor = (val & 0x02)? 1 : 0;
    iwm_slot4_motor = (val & 0x01)? 1 : 0;
    if (g_fastmode) {
        schedulerSetTargetSpeed(g_fast_mhz);
    } else {
        schedulerSetTargetSpeed(1.0);
    }
    return 0;
}

byte MEM_getIntEnReg(byte val)
{
    val = 0;
    if (g_qtrsecirq_enable) val |= 0x10;
    if (g_vblirq_enable) val |= 0x08;
    if (adb_m2mouseswirq) val |= 0x04;
    if (adb_m2mousemvirq) val |= 0x02;
    if (adb_m2mouseenable) val |= 0x01;
    return val;
}

byte MEM_setIntEnReg(byte val)
{
    adb_m2mouseenable = (val & 0x01)? 1 : 0;
    adb_m2mousemvirq = (val & 0x02)? 1 : 0;
    adb_m2mouseswirq = (val & 0x04)? 1 : 0;
    g_vblirq_enable = (val & 0x08)? 1 : 0;
    g_qtrsecirq_enable = (val & 0x10)? 1 : 0;
    return 0;
}

byte MEM_getDiagType(byte val)
{
    return m_diagtype;
}

byte MEM_setVBLClear(byte val)
{
    if (m_diagtype & 0x10) m65816_clearIRQ();
    if (m_diagtype & 0x08) m65816_clearIRQ();
    m_diagtype &= ~0x18;
    return 0;
}

byte MEM_setLCx80(byte val)
{
    sw_lcbank2 = 1;
    sw_lcread = 1;
    sw_lcwrite = 0;
    sw_lcsecond = 0x80;

    updateMemoryMaps();

    return 0;
}

byte MEM_setLCx81(byte val)
{
    sw_lcbank2 = 1;
    sw_lcread = 0;
    if (sw_lcsecond == 0x81) sw_lcwrite = 1;
    sw_lcsecond = 0x81;
    updateMemoryMaps();
    return 0;
}

byte MEM_setLCx82(byte val)
{
    sw_lcbank2 = 1;
    sw_lcread = 0;
    sw_lcwrite = 0;
    sw_lcsecond = 0x82;
    updateMemoryMaps();
    return 0;
}

byte MEM_setLCx83(byte val)
{
    sw_lcbank2 = 1;
    sw_lcread = 1;
    if (sw_lcsecond == 0x83) sw_lcwrite = 1;
    sw_lcsecond = 0x83;
    updateMemoryMaps();
    return 0;
}

byte MEM_setLCx88(byte val)
{
    sw_lcbank2 = 0;
    sw_lcread = 1;
    sw_lcwrite = 0;
    sw_lcsecond = 0x88;
    updateMemoryMaps();
    return 0;
}

byte MEM_setLCx89(byte val)
{
    sw_lcbank2 = 0;
    sw_lcread = 0;
    if (sw_lcsecond == 0x89) sw_lcwrite = 1;
    sw_lcsecond = 0x89;
    updateMemoryMaps();
    return 0;
}

byte MEM_setLCx8A(byte val)
{
    sw_lcbank2 = 0;
    sw_lcread = 0;
    sw_lcwrite = 0;
    sw_lcsecond = 0x8A;
    updateMemoryMaps();
    return 0;
}

byte MEM_setLCx8B(byte val)
{
    sw_lcbank2 = 0;
    sw_lcread = 1;
    if (sw_lcsecond == 0x8B) sw_lcwrite = 1;
    sw_lcsecond = 0x8B;
    updateMemoryMaps();
    return 0;
}

byte MEM_getC071(byte val)
{
    return c07x_rom[0x01];
}

byte MEM_getC072(byte val)
{
    return c07x_rom[0x02];
}

byte MEM_getC073(byte val)
{
    return c07x_rom[0x03];
}

byte MEM_getC074(byte val)
{
    return c07x_rom[0x04];
}

byte MEM_getC075(byte val)
{
    return c07x_rom[0x05];
}

byte MEM_getC076(byte val)
{
    return c07x_rom[0x06];
}

byte MEM_getC077(byte val)
{
    return c07x_rom[0x07];
}

byte MEM_getC078(byte val)
{
    return c07x_rom[0x08];
}

byte MEM_getC079(byte val)
{
    return c07x_rom[0x09];
}

byte MEM_getC07A(byte val)
{
    return c07x_rom[0x0A];
}

byte MEM_getC07B(byte val)
{
    return c07x_rom[0x0B];
}

byte MEM_getC07C(byte val)
{
    return c07x_rom[0x0C];
}

byte MEM_getC07D(byte val)
{
    return c07x_rom[0x0D];
}

byte MEM_getC07E(byte val)
{
    return c07x_rom[0x0E];
}

byte MEM_getC07F(byte val)
{
    return c07x_rom[0x0F];
}
#endif
