/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <boost/format.hpp>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "config.h"
#include "debugger.h"
#include "m65816.h"
#include "memory.h"
#include "soft_switches.h"
#include "util.h"

#define PAGE_NO(bank, address) ((bank << 8) | (address >> 8))
#define OFFSET(address) (address & 0xFF)

static uint8_t *memory;
static uint8_t *read_map[kNumPages];
static uint8_t *write_map[kNumPages];
static uint8_t *shadow_map[kNumPages];

static ioHandler *io_read[kPageSize];
static ioHandler *io_write[kPageSize];

// We point pages to this to indicate that they are IO pages
static uint8_t io[kPageSize];

// Storage for unreadable pages
static uint8_t unreadable[kPageSize];

// Storage for unwriteable pages
static uint8_t unwriteable[kPageSize];

static void mapRead(const uint32_t src_page, const uint32_t dst_page)
{
//  std::cout << (boost::format("mapping %04X to %04X\n") % src_page % dst_page).str();
  read_map[src_page] = memory + (dst_page << 8);
}

static void mapWrite(const uint32_t src_page, const uint32_t dst_page)
{
  write_map[src_page] = memory + (dst_page << 8);
}

static void mapIO(const uint32_t src_page)
{
  read_map[src_page] = write_map[src_page] = io;
}

static void setShadowed(const uint32_t page, const bool isShadowed)
{
  const uint32_t dst_page = isShadowed ? page | 0xE000 : page;

  shadow_map[page] = memory + (dst_page << 8);
}

static uint8_t unmappedIo(const uint8_t offset, const uint8_t val) { return 0; }

void setupMemory(void)
{
  unsigned int rom_size = rom03 ? kRom03Bytes : kRom01Bytes;
  unsigned int rom_start = (kNumBanks * kBankSize) - rom_size;

  if ((ram_size < 256) || (ram_size > 14336)) {
    throw std::runtime_error("RAM size must be between 256 and 14336");
  }

  memory = (uint8_t *)malloc(kNumBanks * kBankSize);
  if (!memory) throw std::runtime_error("Unable to allocate memory");

  loadFile(rom_file, rom_size, memory + rom_start);

  uint8_t *page = memory;
  
  // Map RAM as r/w
  for (uint32_t page_no = 0; page_no < 0xFC00 ; page_no++) {
    read_map[page_no] = write_map[page_no] = shadow_map[page_no] = page;
    page += kPageSize;
  }
  
  // Map ROM as ro
  for (uint32_t page_no = 0xFC00; page_no < kNumPages; page_no++) {
    read_map[page_no] = page;
    write_map[page_no] = shadow_map[page_no] = unwriteable;
    page += kPageSize;
  }

  // TODO: unmap writes for ram above ram_size

  for (unsigned int offset = 0; offset < kPageSize; offset++) {
    io_read[offset] = io_write[offset] = unmappedIo;
  }
}

void freeMemory(void)
{
  free(memory);
}

void installRom(uint8_t *mem, const uint16_t start_page, const uint16_t end_page)
{
  uint8_t *p = mem;

  for (uint16_t page = start_page; page <= end_page; page++, p += kPageSize) {
    read_map[page] = p;
  }
}

/**
 * Read the value from a memory location, honoring page remapping, but
 * ignoring I/O areas. Used internally by the emulator to access memory.
 */
uint8_t sysRead(const uint8_t bank, const uint16_t address)
{
  uint8_t *base = read_map[PAGE_NO(bank, address)];

  return base[OFFSET(address)];
}

/**
 * Write a value from a memory location, honoring page remapping, but
 * ignoring I/O areas. Used internally by the emulator to access memory.
 */
void sysWrite(const uint8_t bank, const uint16_t address, uint8_t val)
{
  uint32_t page = PAGE_NO(bank, address);
  uint8_t offset = OFFSET(address);
  uint8_t *base = write_map[page];
  uint8_t *sbase = shadow_map[page];

  base[offset] = val;
  if (sbase != base) sbase[offset] = val;
}

uint8_t
cpuRead(const uint8_t bank, const uint16_t address, const m65816::mem_access_t type)
{
  uint8_t *base, val;
  uint32_t offset = OFFSET(address);

  if (type == m65816::VECTOR) {
    base = read_map[PAGE_NO(0xFF, address)];
  } else {
    base = read_map[PAGE_NO(bank, address)];
  }

  if (base == io) {
    val = io_read[offset](offset, 0);
  } else {
    val = base[offset];
  }

  if (enable_trace) {
    return debugRead(bank, address, val, type);
  } else {
    return val;
  }
}

void cpuWrite(
    const uint8_t bank, const uint16_t address, uint8_t val,
    const m65816::mem_access_t type
)
{
  uint32_t page = PAGE_NO(bank, address);
  uint8_t offset = OFFSET(address);
  uint8_t *base = write_map[page];
  uint8_t *sbase = shadow_map[page];

  if (enable_trace) {
    val = debugWrite(bank, address, val, type);
  }

  if (base == io) {
    io_write[offset](offset, val);
  } else {
    base[offset] = val;
    if (sbase != base) sbase[offset] = val;
  }
}

void setIoReadHandler(const uint8_t offset, ioHandler *device)
{
  io_read[offset] = device;
}

void setIoWriteHandler(const uint8_t offset, ioHandler *device)
{
  io_write[offset] = device;
}

/**
 * Build a 16k language card in dst_bank, using RAM pages from src_bank. The
 * separate src_bank is used to account for the ALTZP softswitch when building
 * the bank 0 language card.
 */
void buildLanguageCard(unsigned int dst_bank, unsigned int src_bank)
{
  unsigned int page;

  dst_bank <<= 8;
  src_bank <<= 8;

  mapIO(dst_bank | 0xC0);

  for (page = 0xC1; page <= 0xCF; page++) {
    mapRead(dst_bank | page, 0xFF00 | page);
    mapWrite(dst_bank | page, 0xFF00 | page);
  }

  unsigned int offset = sw_lcbank2 ? 0 : 0x10;

  for (page = 0xD0; page <= 0xDF; page++) {
    mapRead(
        dst_bank | page, sw_lcread ? src_bank | (page - offset) : 0xFF00 | page
    );
    mapWrite(
        dst_bank | page, sw_lcwrite ? src_bank | (page - offset) : 0xFF00 | page
    );
  }

  for (page = 0xE0; page <= 0xFF; page++) {
    mapRead(dst_bank | page, sw_lcread ? src_bank | page : 0xFF00 | page);
    mapWrite(dst_bank | page, sw_lcwrite ? src_bank | page : 0xFF00 | page);
  }
}

void updateMemoryMaps()
{
  unsigned int page;

  /*
   * Legacy main/aux memory Bank switching
   */

  for (page = 0x0000; page < 0x0002; page++) {
    mapRead(page, sw_altzp ? page | 0x0100 : page);
    mapWrite(page, sw_altzp ? page | 0x0100 : page);
  }
  for (page = 0x0002; page < 0x0004; page++) {
    mapRead(page, sw_auxrd ? page | 0x0100 : page);
    mapWrite(page, sw_auxwr ? page | 0x0100 : page);
  }
  for (page = 0x0004; page < 0x0008; page++) {
    if (sw_80store) {
      mapRead(page, sw_page2 ? page | 0x0100 : page);
      mapWrite(page, sw_page2 ? page | 0x0100 : page);
    } else {
      mapRead(page, sw_auxrd ? page | 0x0100 : page);
      mapWrite(page, sw_auxwr ? page | 0x0100 : page);
    }
  }
  for (page = 0x0008; page < 0x0020; page++) {
    mapRead(page, sw_auxrd ? page | 0x0100 : page);
    mapWrite(page, sw_auxwr ? page | 0x0100 : page);
  }
  for (page = 0x0020; page < 0x0040; page++) {
    if (sw_80store && sw_hires) {
      mapRead(page, sw_page2 ? page | 0x0100 : page);
      mapWrite(page, sw_page2 ? page | 0x0100 : page);
    } else {
      mapRead(page, sw_auxrd ? page | 0x0100 : page);
      mapWrite(page, sw_auxwr ? page | 0x0100 : page);
    }
  }
  for (page = 0x0040; page < 0x00C0; page++) {
    mapRead(page, sw_auxrd ? page | 0x0100 : page);
    mapWrite(page, sw_auxwr ? page | 0x0100 : page);
  }

  // Language cards

  buildLanguageCard(0xE0, 0xE0);
  buildLanguageCard(0xE1, 0xE1);

  if (sw_shadow_lc) {
    buildLanguageCard(0x00, sw_altzp ? 0x01 : 0x00);
    buildLanguageCard(0x01, 0x01);
  } else {
    for (page = 0x00C0; page <= 0x00FF; page++) {
      mapRead(page, sw_auxrd ? page + 0x0100 : page);
      mapWrite(page, sw_auxwr ? page + 0x0100 : page);
    }
    for (page = 0x01C0; page <= 0x01FF; page++) {
      mapRead(page, page);
      mapWrite(page, page);
    }
  }

  /*
   * Shadowing
   * FIXME: support shadowing in all banks
   */

  for (page = 0x0004; page < 0x0008; page++) {
    setShadowed(page, sw_shadow_text);
  }
  for (page = 0x0008; page < 0x000C; page++) {
    setShadowed(page, sw_shadow_text2);
  }
  for (page = 0x0020; page < 0x0040; page++) {
    setShadowed(page, sw_shadow_hires1);
  }
  for (page = 0x0060; page < 0x0080; page++) {
    setShadowed(page, sw_shadow_hires2);
  }
  for (page = 0x0104; page < 0x0108; page++) {
    setShadowed(page, sw_shadow_text);
  }
  for (page = 0x0108; page < 0x010C; page++) {
    setShadowed(page, sw_shadow_text2);
  }
  for (page = 0x0120; page < 0x0140; page++) {
    setShadowed(page, (sw_shadow_hires1 && sw_shadow_aux) || sw_shadow_super);
  }
  for (page = 0x0140; page < 0x0160; page++) {
    setShadowed(page, (sw_shadow_hires2 && sw_shadow_aux) || sw_shadow_super);
  }
  for (page = 0x0160; page < 0x01A0; page++) {
    setShadowed(page, sw_shadow_aux || sw_shadow_super);
  }
}

/**
 * Returns a pointer to the read area for a given page number.
 */
uint8_t *getPageReadPointer(uint16_t page_no) { return read_map[page_no]; }
