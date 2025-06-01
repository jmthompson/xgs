/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements a Smartport device in slot 7, supporting up to 14
 * ProDOS block devices of up to 32 MB each.
 */

#include <cstdint>
#include <cstdlib>

#include "config.h"
#include "cpu/registers.h"
#include "disks/VirtualDisk.h"
#include "memory.h"
#include "smartport.h"

using std::uint16_t;
using std::uint8_t;

namespace Smartport {

static uint8_t disk_buffer[512];

static unsigned char id_string[17] = "XGS SmartPort   ";

static const uint8_t smartport_rom[256] = {
    0xA9, 0x20, 0xA9, 0x00, 0xA9, 0x03, 0xA9, 0x00, 0xA9, 0x01, 0x85, 0x42, 0x64,
    0x43, 0x64, 0x44, 0xA9, 0x08, 0x85, 0x45, 0x64, 0x46, 0x64, 0x47, 0x42, 0xC7,
    0xB0, 0x77, 0xA9, 0xC7, 0x8D, 0xF8, 0x07, 0xA9, 0x07, 0xA2, 0x70, 0x4C, 0x01,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4C, 0x90,
    0xC7, 0x42, 0xC8, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x42, 0xC7, 0x60, 0x4C, 0xBA, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x93, 0x80,
};

static VirtualDisk *units[kSmartportUnits];

static void statusCmd(const uint8_t pbank, const uint16_t paddr)
{
  unsigned int unit_num = sysRead(pbank, paddr + 1);

  if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
    m65816::registers::A.B.L = 0x28;

    return;
  }

  VirtualDisk *unit = unit_num ? units[unit_num - 1] : nullptr;

  uint16_t status_addr =
      sysRead(pbank, paddr + 2) | (sysRead(pbank, paddr + 3) << 8);
  uint8_t status_code = sysRead(pbank, paddr + 4);

  switch (status_code) {
  case 0x00:
    if (!unit_num) {
      sysWrite(0, status_addr, kSmartportUnits);

      for (unsigned int i = 1; i < 8; ++i) {
        sysWrite(0, status_addr + i, 0);
      }

      m65816::registers::X.B.L = 8;
      m65816::registers::Y.B.L = 0;
      m65816::registers::A.B.L = 0;
    } else {
      if (unit == nullptr) {
        sysWrite(0, status_addr, 0xEC);
        sysWrite(0, status_addr + 1, 0);
        sysWrite(0, status_addr + 2, 0);
        sysWrite(0, status_addr + 3, 0);
      } else {
        sysWrite(0, status_addr, unit->locked ? 0xFC : 0xF8);
        sysWrite(0, status_addr + 1, unit->num_chunks & 0xFF);
        sysWrite(0, status_addr + 2, unit->num_chunks >> 8);
        sysWrite(0, status_addr + 3, 0);
      }

      m65816::registers::X.B.L = 4;
      m65816::registers::Y.B.L = 0;
      m65816::registers::A.B.L = 0;
    }

    break;
  case 0x01:
    m65816::registers::A.B.L = 0x21;

    break;
  case 0x02:
    m65816::registers::A.B.L = 0x21;

    break;
  case 0x03:
    if (unit == nullptr) {
      sysWrite(0, status_addr, 0xEC);
      sysWrite(0, status_addr + 1, 0);
      sysWrite(0, status_addr + 2, 0);
      sysWrite(0, status_addr + 3, 0);
    } else {
      sysWrite(0, status_addr, unit->locked ? 0xFC : 0xF8);
      sysWrite(0, status_addr + 1, unit->num_chunks & 0xFF);
      sysWrite(0, status_addr + 2, unit->num_chunks >> 8);
      sysWrite(0, status_addr + 3, 0);
    }

    sysWrite(0, status_addr + 4, 0x10);

    for (unsigned int i = 0; i < 16; ++i) {
      sysWrite(0, status_addr + 5 + i, id_string[i]);
    }

    sysWrite(0, status_addr + 21, 0x02);
    sysWrite(0, status_addr + 22, 0xC0);
    sysWrite(0, status_addr + 23, 0x00);
    sysWrite(0, status_addr + 24, 0x00);

    m65816::registers::X.B.L = 25;
    m65816::registers::Y.B.L = 0;
    m65816::registers::A.B.L = 0;

    break;
  default:
    m65816::registers::A.B.L = 0x21;

    break;
  }
}

static void readBlockCmd(const uint8_t pbank, const uint16_t paddr)
{
  unsigned int unit_num = sysRead(pbank, paddr + 1);

  if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
    m65816::registers::A.B.L = 0x28;

    return;
  }

  VirtualDisk *unit = unit_num ? units[unit_num - 1] : nullptr;

  if (unit == nullptr) {
    m65816::registers::A.B.L = 0x2F;

    return;
  }

  uint16_t buffer =
      sysRead(pbank, paddr + 2) | (sysRead(pbank, paddr + 3) << 8);
  uint32_t block = sysRead(pbank, paddr + 4) |
                   (sysRead(pbank, paddr + 5) << 8) |
                   (sysRead(pbank, paddr + 6) << 16);

  if (block > unit->num_chunks) {
    m65816::registers::A.B.L = 0x2D;

    return;
  }

  try {
    unit->read(disk_buffer, block, 1);
  } catch (std::runtime_error &e) {
    m65816::registers::A.B.L = 0x27;

    return;
  }

  for (unsigned int i = 0; i < sizeof(disk_buffer); ++i) {
    sysWrite(0, buffer + i, disk_buffer[i]);
  }

  m65816::registers::X.B.L = 0x00;
  m65816::registers::Y.B.L = 0x02;
  m65816::registers::A.B.L = 0;
}

static void writeBlockCmd(const uint8_t pbank, const uint16_t paddr)
{
  unsigned int unit_num = sysRead(pbank, paddr + 1);

  if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
    m65816::registers::A.B.L = 0x28;

    return;
  }

  VirtualDisk *unit = unit_num ? units[unit_num - 1] : nullptr;

  if (unit == nullptr) {
    m65816::registers::A.B.L = 0x2F;

    return;
  }

  if (unit->locked) {
    m65816::registers::A.B.L = 0x2B;

    return;
  }

  uint16_t buffer =
      sysRead(pbank, paddr + 2) | (sysRead(pbank, paddr + 3) << 8);
  uint32_t block = sysRead(pbank, paddr + 4) |
                   (sysRead(pbank, paddr + 5) << 8) |
                   (sysRead(pbank, paddr + 6) << 16);

  if (block > unit->num_chunks) {
    m65816::registers::A.B.L = 0x2D;

    return;
  }

  for (unsigned int i = 0; i < sizeof(disk_buffer); ++i) {
    disk_buffer[i] = sysRead(0, buffer + i);
  }

  try {
    unit->write(disk_buffer, block, 1);
  } catch (std::runtime_error &e) {
    m65816::registers::A.B.L = 0x27;

    return;
  }

  m65816::registers::X.B.L = 0x00;
  m65816::registers::Y.B.L = 0x02;
  m65816::registers::A.B.L = 0;
}

static void formatCmd(const uint8_t pbank, const uint16_t paddr)
{
  m65816::registers::A.B.L = 0;
}

static void controlCmd(const uint8_t pbank, const uint16_t paddr)
{
  m65816::registers::X.B.L = 0;
  m65816::registers::Y.B.L = 0;
  m65816::registers::A.B.L = 0;
}

static void initCmd(const uint8_t pbank, const uint16_t paddr) {}

static void openCmd(const uint8_t pbank, const uint16_t paddr) {}

static void closeCmd(const uint8_t pbank, const uint16_t paddr) {}

static void readCmd(const uint8_t pbank, const uint16_t paddr) {}

static void writeCmd(const uint8_t pbank, const uint16_t paddr) {}

static void statusCmdExt(const uint8_t pbank, const uint16_t paddr)
{
  unsigned int unit_num = sysRead(pbank, paddr + 1);

  if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
    m65816::registers::A.B.L = 0x28;

    return;
  }

  VirtualDisk *unit = unit_num ? units[unit_num - 1] : nullptr;

  uint16_t status_addr =
      sysRead(pbank, paddr + 2) | (sysRead(pbank, paddr + 3) << 8);
  uint8_t status_bank = sysRead(pbank, paddr + 4);
  uint8_t status_code = sysRead(pbank, paddr + 6);

  switch (status_code) {
  case 0x00:
    if (!unit_num) {
      sysWrite(0, status_addr, kSmartportUnits);

      for (unsigned int i = 1; i < 8; ++i) {
        sysWrite(status_bank, status_addr + i, 0);
      }

      m65816::registers::X.B.L = 8;
      m65816::registers::Y.B.L = 0;
      m65816::registers::A.B.L = 0;
    } else {
      if (unit == nullptr) {
        sysWrite(status_bank, status_addr, 0xEC);
        sysWrite(status_bank, status_addr + 1, 0);
        sysWrite(status_bank, status_addr + 2, 0);
        sysWrite(status_bank, status_addr + 3, 0);
        sysWrite(status_bank, status_addr + 4, 0);
      } else {
        sysWrite(status_bank, status_addr, unit->locked ? 0xFC : 0xF8);
        sysWrite(status_bank, status_addr + 1, unit->num_chunks & 0xFF);
        sysWrite(status_bank, status_addr + 2, unit->num_chunks >> 8);
        sysWrite(status_bank, status_addr + 3, 0);
        sysWrite(status_bank, status_addr + 4, 0);
      }

      m65816::registers::X.B.L = 5;
      m65816::registers::Y.B.L = 0;
      m65816::registers::A.B.L = 0;
    }

    break;
  case 0x01:
    m65816::registers::A.B.L = 0x21;

    break;
  case 0x02:
    m65816::registers::A.B.L = 0x21;

    break;
  case 0x03:
    if (unit == nullptr) {
      sysWrite(status_bank, status_addr, 0xEC);
      sysWrite(status_bank, status_addr + 1, 0);
      sysWrite(status_bank, status_addr + 2, 0);
      sysWrite(status_bank, status_addr + 3, 0);
      sysWrite(status_bank, status_addr + 4, 0);
    } else {
      sysWrite(status_bank, status_addr, unit->locked ? 0xFC : 0xF8);
      sysWrite(status_bank, status_addr + 1, unit->num_chunks & 0xFF);
      sysWrite(status_bank, status_addr + 2, unit->num_chunks >> 8);
      sysWrite(status_bank, status_addr + 3, 0);
      sysWrite(status_bank, status_addr + 4, 0);
    }

    sysWrite(0, status_addr + 5, 0x10);

    for (unsigned int i = 0; i < 16; ++i) {
      sysWrite(0, status_addr + 6 + i, id_string[i]);
    }

    sysWrite(0, status_addr + 22, 0x02);
    sysWrite(0, status_addr + 23, 0xC0);
    sysWrite(0, status_addr + 24, 0x00);
    sysWrite(0, status_addr + 25, 0x00);

    m65816::registers::X.B.L = 26;
    m65816::registers::Y.B.L = 0;
    m65816::registers::A.B.L = 0;

    break;
  default:
    m65816::registers::A.B.L = 0x21;

    break;
  }
}

static void readBlockCmdExt(const uint8_t pbank, const uint16_t paddr)
{
  unsigned int unit_num = sysRead(pbank, paddr + 1);

  if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
    m65816::registers::A.B.L = 0x28;

    return;
  }

  VirtualDisk *unit = unit_num ? units[unit_num - 1] : nullptr;

  if (unit == nullptr) {
    m65816::registers::A.B.L = 0x2F;

    return;
  }

  uint16_t buffer_addr =
      sysRead(pbank, paddr + 2) | (sysRead(pbank, paddr + 3) << 8);
  uint8_t buffer_bank = sysRead(pbank, paddr + 4);

  uint32_t block = sysRead(pbank, paddr + 6) |
                   (sysRead(pbank, paddr + 7) << 8) |
                   (sysRead(pbank, paddr + 8) << 16) |
                   (sysRead(pbank, paddr + 9) << 24);

  if (block > unit->num_chunks) {
    m65816::registers::A.B.L = 0x2D;

    return;
  }

  try {
    unit->read(disk_buffer, block, 1);
  } catch (std::runtime_error &e) {
    m65816::registers::A.B.L = 0x27;

    return;
  }

  for (unsigned int i = 0; i < sizeof(disk_buffer); ++i) {
    sysWrite(buffer_bank, buffer_addr + i, disk_buffer[i]);
  }

  m65816::registers::X.B.L = 0x00;
  m65816::registers::Y.B.L = 0x02;
  m65816::registers::A.B.L = 0;
}

static void writeBlockCmdExt(const uint8_t pbank, const uint16_t paddr)
{
  unsigned int unit_num = sysRead(pbank, paddr + 1);

  if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
    m65816::registers::A.B.L = 0x28;

    return;
  }

  VirtualDisk *unit = unit_num ? units[unit_num - 1] : nullptr;

  if (unit == nullptr) {
    m65816::registers::A.B.L = 0x2F;

    return;
  }

  if (unit->locked) {
    m65816::registers::A.B.L = 0x2B;

    return;
  }

  uint16_t buffer_addr =
      sysRead(pbank, paddr + 2) | (sysRead(pbank, paddr + 3) << 8);
  uint8_t buffer_bank = sysRead(pbank, paddr + 4);

  uint32_t block = sysRead(pbank, paddr + 6) |
                   (sysRead(pbank, paddr + 7) << 8) |
                   (sysRead(pbank, paddr + 8) << 16) |
                   (sysRead(pbank, paddr + 9) << 24);

  if (block > unit->num_chunks) {
    m65816::registers::A.B.L = 0x2D;

    return;
  }

  for (unsigned int i = 0; i < sizeof(disk_buffer); ++i) {
    disk_buffer[i] = sysRead(buffer_bank, buffer_addr + i);
  }

  try {
    unit->write(disk_buffer, block, 1);
  } catch (std::runtime_error &e) {
    m65816::registers::A.B.L = 0x27;

    return;
  }

  m65816::registers::X.B.L = 0x00;
  m65816::registers::Y.B.L = 0x02;
  m65816::registers::A.B.L = 0;
}

static void formatCmdExt(const uint8_t pbank, const uint16_t paddr)
{
  m65816::registers::A.B.L = 0;
}

static void controlCmdExt(const uint8_t pbank, const uint16_t paddr) {}

static void initCmdExt(const uint8_t pbank, const uint16_t paddr) {}

static void openCmdExt(const uint8_t pbank, const uint16_t paddr) {}

static void closeCmdExt(const uint8_t pbank, const uint16_t paddr) {}

static void readCmdExt(const uint8_t pbank, const uint16_t paddr) {}

static void writeCmdExt(const uint8_t pbank, const uint16_t paddr) {}

// Handle calls to the ProDOS block device entry point for slot 7

static void prodosEntry()
{
  uint8_t cmd = sysRead(0, 0x42);
  VirtualDisk *unit = (sysRead(0, 0x43) & 0x80) ? units[1] : units[0];

  if (!unit) {
    m65816::registers::A.B.L = 0x028; // NO DEVICE CONNECTED
    m65816::registers::SR.C = true;

    return;
  }

  uint16_t buffer = sysRead(0, 0x44) | (sysRead(0, 0x45) << 8);
  uint16_t block = sysRead(0, 0x46) | (sysRead(0, 0x47) << 8);

  m65816::registers::A.B.L = 0;

  switch (cmd) {
  case 0:
    m65816::registers::X.B.L = unit->num_chunks & 0xFF;
    m65816::registers::Y.B.L = unit->num_chunks >> 8;

    break;
  case 1:
    try {
      unit->read(disk_buffer, block, 1);

      for (unsigned int i = 0; i < sizeof(disk_buffer); ++i) {
        sysWrite(0, buffer + i, disk_buffer[i]);
      }
    } catch (std::runtime_error &e) {
      m65816::registers::A.B.L = 0x27; // I/O ERROR
    }

    break;
  case 2:
    if (unit->locked) {
      m65816::registers::A.B.L = 0x2B; // WRITE PROTECTED
    } else {
      try {
        for (unsigned int i = 0; i < sizeof(disk_buffer); ++i) {
          disk_buffer[i] = sysRead(0, buffer + i);
        }

        unit->write(disk_buffer, block, 1);
      } catch (std::runtime_error &e) {
        m65816::registers::A.B.L = 0x27; // I/O ERROR
      }
    }

    break;
  case 3:
    if (unit->locked) {
      m65816::registers::A.B.L = 0x2B; // WRITE PROTECTED
    }

    break;
  default:
    m65816::registers::A.B.L = 0x01; // BAD CALL

    break;
  }

  m65816::registers::SR.C = m65816::registers::A.B.L;
}

static void smartportEntry()
{
  uint16_t addr = (sysRead(0, m65816::registers::S.W + 1) |
                   (sysRead(0, m65816::registers::S.W + 2) << 8)) +
                  1;

  uint8_t cmd = sysRead(0, addr);
  uint16_t paddr = sysRead(0, addr + 1) | (sysRead(0, addr + 2) << 8);
  uint8_t pbank;

  if (cmd & 0x40) {
    pbank = sysRead(0, addr + 3);
    addr += 4;
  } else {
    pbank = 0;
    addr += 2;
  }

  sysWrite(0, m65816::registers::S.W + 1, addr & 0xFF);
  sysWrite(0, m65816::registers::S.W + 2, addr >> 8);

  // num_params = sysRead(pbank, paddr);

  switch (cmd) {
  case 0x00:
    statusCmd(pbank, paddr);
    break;
  case 0x01:
    readBlockCmd(pbank, paddr);
    break;
  case 0x02:
    writeBlockCmd(pbank, paddr);
    break;
  case 0x03:
    formatCmd(pbank, paddr);
    break;
  case 0x04:
    controlCmd(pbank, paddr);
    break;
  case 0x05:
    initCmd(pbank, paddr);
    break;
  case 0x06:
    openCmd(pbank, paddr);
    break;
  case 0x07:
    closeCmd(pbank, paddr);
    break;
  case 0x08:
    readCmd(pbank, paddr);
    break;
  case 0x09:
    writeCmd(pbank, paddr);
    break;
  case 0x40:
    statusCmdExt(pbank, paddr);
    break;
  case 0x41:
    readBlockCmdExt(pbank, paddr);
    break;
  case 0x42:
    writeBlockCmdExt(pbank, paddr);
    break;
  case 0x43:
    formatCmdExt(pbank, paddr);
    break;
  case 0x44:
    controlCmdExt(pbank, paddr);
    break;
  case 0x45:
    initCmdExt(pbank, paddr);
    break;
  case 0x46:
    openCmdExt(pbank, paddr);
    break;
  case 0x47:
    closeCmdExt(pbank, paddr);
    break;
  case 0x48:
    readCmdExt(pbank, paddr);
    break;
  case 0x49:
    writeCmdExt(pbank, paddr);
    break;
  default:
    m65816::registers::A.B.L = 0x01; // Invalid command
    break;
  }

  m65816::registers::SR.C = m65816::registers::A.B.L;
}

void start(void)
{
  installRom((uint8_t *) smartport_rom, 0xFFC7, 0xFFC7);
}

void stop(void)
{
  for (unsigned int i = 0; i < kSmartportUnits; ++i) {
    units[i] = nullptr;
  }
  for (unsigned int i = 0; i < kSmartportUnits; ++i) {
    unmountImage(i);
  }
}

void reset(void)
{
  for (unsigned int i = 0; i < kSmartportUnits; ++i) {
    unmountImage(i);
  }
}

void unmountImage(const unsigned int drive)
{
  if (units[drive] != nullptr) {
    units[drive]->close();

    delete units[drive];

    units[drive] = nullptr;
  }
}

void mountImage(const unsigned int drive, VirtualDisk *image)
{
  unmountImage(drive);

  image->open();

  units[drive] = image;
}

void wdm(const uint8_t command)
{
  switch (command) {
  case 0xC7:
    prodosEntry();

    break;
  case 0xC8:
    smartportEntry();

    break;
  }
}

} // namespace Smartport
