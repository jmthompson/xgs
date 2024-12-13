/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements a Smartport device in slot 7, supporting up to 14
 * ProDOS block devices of up to 32 MB each.
 */

#include <cstdlib>
#include <stdexcept>

#include "emulator/common.h"

#include "disks/Smartport.h"
#include "disks/VirtualDisk.h"
#include "emulator/xgs.h"
#include "M65816/registers.h"

using std::uint8_t;
using std::uint16_t;

static uint8_t disk_buffer[512];

static unsigned char id_string[17] = "XGS SmartPort   ";

static const uint8_t smartport_rom[256] = {
    0xA9, 0x20, 0xA9, 0x00, 0xA9, 0x03, 0xA9, 0x00, 0xA9, 0x01, 0x85, 0x42, 0x64, 0x43, 0x64, 0x44,
    0xA9, 0x08, 0x85, 0x45, 0x64, 0x46, 0x64, 0x47, 0x42, 0xC7, 0xB0, 0x77, 0xA9, 0xC7, 0x8D, 0xF8,
    0x07, 0xA9, 0x07, 0xA2, 0x70, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x4C, 0x90, 0xC7, 0x42, 0xC8, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x42, 0xC7, 0x60, 0x4C, 0xBA, 0xFA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x93, 0x80,
};

Smartport::Smartport()
{
    memset(units, 0, sizeof(units));
    start();
}

Smartport::~Smartport()
{
    stop();
}

void Smartport::start()
{
    Device::start();

    xgs::installMemory((uint8_t *) smartport_rom, 0xFFC7, 1, xgs::ROM);
    xgs::setWdmHandler(0xC7, this);
    xgs::setWdmHandler(0xC8, this);
}

void Smartport::stop()
{
    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        units[i] = nullptr;
    }
    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        unmountImage(i);
    }
}

void Smartport::reset()
{
    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        unmountImage(i);
    }
}

void Smartport::wdm(const uint8_t command)
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

void Smartport::mountImage(const unsigned int drive, VirtualDisk *image)
{
    unmountImage(drive);

    image->open();

    units[drive] = image;
}

void Smartport::unmountImage(const unsigned int drive)
{
    if (units[drive] != nullptr) {
        units[drive]->close();

        delete units[drive];

        units[drive] = nullptr;
    }
}

// Handle calls to the ProDOS block device entry point for slot 7

void Smartport::prodosEntry()
{
    uint8_t cmd = xgs::sysRead(0, 0x42);
    VirtualDisk *unit = (xgs::sysRead(0, 0x43) & 0x80)? units[1] : units[0];

    if (!unit) {
            m65816::registers::A.B.L = 0x028; // NO DEVICE CONNECTED
            m65816::registers::SR.C  = true;

        return;
    }

    uint16_t buffer = xgs::sysRead(0, 0x44) | (xgs::sysRead(0, 0x45) << 8);
    uint16_t block  = xgs::sysRead(0, 0x46) | (xgs::sysRead(0, 0x47) << 8);

    m65816::registers::A.B.L = 0;

    switch (cmd) {
        case 0:
            m65816::registers::X.B.L = unit->num_chunks & 0xFF;
            m65816::registers::Y.B.L = unit->num_chunks >> 8;

            break;
        case 1:
            try {
                unit->read(disk_buffer, block, 1);

                for (unsigned int i = 0 ; i < sizeof(disk_buffer); ++i) {
                    xgs::sysWrite(0, buffer + i, disk_buffer[i]);
                }
            }
            catch (std::runtime_error& e) {
                m65816::registers::A.B.L = 0x27;  // I/O ERROR
            }

            break;
        case 2:
            if (unit->locked) {
                m65816::registers::A.B.L = 0x2B;   // WRITE PROTECTED
            }
            else {
                try {
                    for (unsigned int i = 0 ; i < sizeof(disk_buffer); ++i) {
                        disk_buffer[i] = xgs::sysRead(0, buffer + i);
                    }

                    unit->write(disk_buffer, block, 1);
                }
                catch (std::runtime_error& e) {
                    m65816::registers::A.B.L = 0x27;  // I/O ERROR
                }
            }

            break;
        case 3:
            if (unit->locked) {
                m65816::registers::A.B.L = 0x2B;   // WRITE PROTECTED
            }

            break;
        default :
            m65816::registers::A.B.L = 0x01; // BAD CALL

            break;
    }

    m65816::registers::SR.C = m65816::registers::A.B.L;
}

void Smartport::smartportEntry()
{
    uint16_t addr  = (xgs::sysRead(0, m65816::registers::S.W + 1) | (xgs::sysRead(0, m65816::registers::S.W + 2) << 8)) + 1;

    uint8_t  cmd   = xgs::sysRead(0, addr);
    uint16_t paddr = xgs::sysRead(0, addr + 1) | (xgs::sysRead(0, addr + 2) << 8);
    uint8_t  pbank;

    if (cmd & 0x40) {
        pbank = xgs::sysRead(0, addr + 3);
        addr += 4;
    }
    else {
        pbank = 0;
        addr += 2;
    }
    
    xgs::sysWrite(0, m65816::registers::S.W + 1, addr & 0xFF);
    xgs::sysWrite(0, m65816::registers::S.W + 2, addr >> 8);

    //num_params = xgs::sysRead(pbank, paddr);

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

void Smartport::statusCmd(const uint8_t pbank, const uint16_t paddr)
{
    unsigned int unit_num = xgs::sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        m65816::registers::A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    uint16_t status_addr = xgs::sysRead(pbank, paddr + 2) | (xgs::sysRead(pbank, paddr + 3) << 8); 
    uint8_t  status_code = xgs::sysRead(pbank, paddr + 4);

    switch(status_code) {
        case 0x00:
            if (!unit_num) {
                xgs::sysWrite(0, status_addr, kSmartportUnits);

                for (unsigned int i = 1 ; i < 8 ; ++i) {
                    xgs::sysWrite(0, status_addr + i, 0);
                }

                m65816::registers::X.B.L = 8;
                m65816::registers::Y.B.L = 0;
                m65816::registers::A.B.L = 0;
            }
            else {
                if (unit == nullptr) {
                    xgs::sysWrite(0, status_addr, 0xEC);
                    xgs::sysWrite(0, status_addr + 1, 0);
                    xgs::sysWrite(0, status_addr + 2, 0);
                    xgs::sysWrite(0, status_addr + 3, 0);
                }
                else {
                    xgs::sysWrite(0, status_addr, unit->locked? 0xFC: 0xF8);
                    xgs::sysWrite(0, status_addr + 1, unit->num_chunks & 0xFF);
                    xgs::sysWrite(0, status_addr + 2, unit->num_chunks >> 8);
                    xgs::sysWrite(0, status_addr + 3, 0);
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
                xgs::sysWrite(0, status_addr, 0xEC);
                xgs::sysWrite(0, status_addr + 1, 0);
                xgs::sysWrite(0, status_addr + 2, 0);
                xgs::sysWrite(0, status_addr + 3, 0);
            }
            else {
                xgs::sysWrite(0, status_addr, unit->locked? 0xFC: 0xF8);
                xgs::sysWrite(0, status_addr + 1, unit->num_chunks & 0xFF);
                xgs::sysWrite(0, status_addr + 2, unit->num_chunks >> 8);
                xgs::sysWrite(0, status_addr + 3, 0);
            }

            xgs::sysWrite(0, status_addr + 4, 0x10);

            for (unsigned int i = 0 ; i < 16 ; ++i) {
                xgs::sysWrite(0, status_addr + 5 + i, id_string[i]);
            }

            xgs::sysWrite(0, status_addr + 21, 0x02);
            xgs::sysWrite(0, status_addr + 22, 0xC0);
            xgs::sysWrite(0, status_addr + 23, 0x00);
            xgs::sysWrite(0, status_addr + 24, 0x00);

            m65816::registers::X.B.L = 25;
            m65816::registers::Y.B.L = 0;
            m65816::registers::A.B.L = 0;

            break;
        default:
            m65816::registers::A.B.L = 0x21;

            break;
    }
}

void Smartport::readBlockCmd(const uint8_t pbank, const uint16_t paddr)
{
    unsigned int unit_num = xgs::sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        m65816::registers::A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        m65816::registers::A.B.L = 0x2F;

        return;
    }

    uint16_t buffer = xgs::sysRead(pbank, paddr + 2) | (xgs::sysRead(pbank, paddr + 3) << 8);
    uint32_t block  = xgs::sysRead(pbank, paddr + 4) | (xgs::sysRead(pbank, paddr + 5) << 8)
                        | (xgs::sysRead(pbank, paddr + 6) << 16);

    if (block > unit->num_chunks) {
        m65816::registers::A.B.L = 0x2D;

        return;
    }

    try {
        unit->read(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        m65816::registers::A.B.L = 0x27;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        xgs::sysWrite(0, buffer + i, disk_buffer[i]);
    }

    m65816::registers::X.B.L = 0x00;
    m65816::registers::Y.B.L = 0x02;
    m65816::registers::A.B.L = 0;
}

void Smartport::writeBlockCmd(const uint8_t pbank, const uint16_t paddr)
{
    unsigned int unit_num = xgs::sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        m65816::registers::A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        m65816::registers::A.B.L = 0x2F;

        return;
    }

    if (unit->locked) {
        m65816::registers::A.B.L = 0x2B;

        return;
    }

    uint16_t buffer = xgs::sysRead(pbank, paddr + 2) | (xgs::sysRead(pbank, paddr + 3) << 8);
    uint32_t block  = xgs::sysRead(pbank, paddr + 4) | (xgs::sysRead(pbank, paddr + 5) << 8)
                        | (xgs::sysRead(pbank, paddr + 6) << 16);

    if (block > unit->num_chunks) {
        m65816::registers::A.B.L = 0x2D;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        disk_buffer[i] = xgs::sysRead(0, buffer + i);
    }

    try {
        unit->write(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        m65816::registers::A.B.L = 0x27;

        return;
    }

    m65816::registers::X.B.L = 0x00;
    m65816::registers::Y.B.L = 0x02;
    m65816::registers::A.B.L = 0;
}

void Smartport::formatCmd(const uint8_t pbank, const uint16_t paddr)
{
    m65816::registers::A.B.L = 0;
}

void Smartport::controlCmd(const uint8_t pbank, const uint16_t paddr)
{
    m65816::registers::X.B.L = 0;
    m65816::registers::Y.B.L = 0;
    m65816::registers::A.B.L = 0;
}

void Smartport::initCmd(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::openCmd(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::closeCmd(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::readCmd(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::writeCmd(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::statusCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    unsigned int unit_num = xgs::sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        m65816::registers::A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    uint16_t status_addr = xgs::sysRead(pbank, paddr + 2) | (xgs::sysRead(pbank, paddr + 3) << 8);
    uint8_t  status_bank = xgs::sysRead(pbank, paddr + 4);
    uint8_t  status_code = xgs::sysRead(pbank, paddr + 6);

    switch (status_code) {
        case 0x00:
            if (!unit_num) {
                xgs::sysWrite(0, status_addr, kSmartportUnits);

                for (unsigned int i = 1 ; i < 8 ; ++i) {
                    xgs::sysWrite(status_bank, status_addr + i, 0);
                }

                m65816::registers::X.B.L = 8;
                m65816::registers::Y.B.L = 0;
                m65816::registers::A.B.L = 0;
            }
            else {
                if (unit == nullptr) {
                    xgs::sysWrite(status_bank, status_addr, 0xEC);
                    xgs::sysWrite(status_bank, status_addr + 1, 0);
                    xgs::sysWrite(status_bank, status_addr + 2, 0);
                    xgs::sysWrite(status_bank, status_addr + 3, 0);
                    xgs::sysWrite(status_bank, status_addr + 4, 0);
                }
                else {
                    xgs::sysWrite(status_bank, status_addr, unit->locked? 0xFC: 0xF8);
                    xgs::sysWrite(status_bank, status_addr + 1, unit->num_chunks & 0xFF);
                    xgs::sysWrite(status_bank, status_addr + 2, unit->num_chunks >> 8);
                    xgs::sysWrite(status_bank, status_addr + 3, 0);
                    xgs::sysWrite(status_bank, status_addr + 4, 0);
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
                xgs::sysWrite(status_bank, status_addr, 0xEC);
                xgs::sysWrite(status_bank, status_addr + 1, 0);
                xgs::sysWrite(status_bank, status_addr + 2, 0);
                xgs::sysWrite(status_bank, status_addr + 3, 0);
                xgs::sysWrite(status_bank, status_addr + 4, 0);
            }
            else {
                xgs::sysWrite(status_bank, status_addr, unit->locked? 0xFC: 0xF8);
                xgs::sysWrite(status_bank, status_addr + 1, unit->num_chunks & 0xFF);
                xgs::sysWrite(status_bank, status_addr + 2, unit->num_chunks >> 8);
                xgs::sysWrite(status_bank, status_addr + 3, 0);
                xgs::sysWrite(status_bank, status_addr + 4, 0);
            }

            xgs::sysWrite(0, status_addr + 5, 0x10);

            for (unsigned int i = 0 ; i < 16 ; ++i) {
                xgs::sysWrite(0, status_addr + 6 + i, id_string[i]);
            }

            xgs::sysWrite(0, status_addr + 22, 0x02);
            xgs::sysWrite(0, status_addr + 23, 0xC0);
            xgs::sysWrite(0, status_addr + 24, 0x00);
            xgs::sysWrite(0, status_addr + 25, 0x00);

            m65816::registers::X.B.L = 26;
            m65816::registers::Y.B.L = 0;
            m65816::registers::A.B.L = 0;

            break;
        default :
            m65816::registers::A.B.L = 0x21;

            break;
    }
}

void Smartport::readBlockCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    unsigned int unit_num = xgs::sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        m65816::registers::A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        m65816::registers::A.B.L = 0x2F;

        return;
    }

    uint16_t buffer_addr = xgs::sysRead(pbank, paddr + 2) | (xgs::sysRead(pbank, paddr + 3) << 8);
    uint8_t  buffer_bank = xgs::sysRead(pbank, paddr + 4);

    uint32_t block = xgs::sysRead(pbank, paddr + 6)
                        | (xgs::sysRead(pbank, paddr + 7) << 8)
                        | (xgs::sysRead(pbank, paddr + 8) << 16)
                        | (xgs::sysRead(pbank, paddr + 9) << 24);

    if (block > unit->num_chunks) {
        m65816::registers::A.B.L = 0x2D;

        return;
    }

    try {
        unit->read(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        m65816::registers::A.B.L = 0x27;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        xgs::sysWrite(buffer_bank, buffer_addr + i, disk_buffer[i]);
    }

    m65816::registers::X.B.L = 0x00;
    m65816::registers::Y.B.L = 0x02;
    m65816::registers::A.B.L = 0;
}

void Smartport::writeBlockCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    unsigned int unit_num = xgs::sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        m65816::registers::A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        m65816::registers::A.B.L = 0x2F;

        return;
    }

    if (unit->locked) {
        m65816::registers::A.B.L = 0x2B;

        return;
    }

    uint16_t buffer_addr = xgs::sysRead(pbank, paddr + 2) | (xgs::sysRead(pbank, paddr + 3) << 8); 
    uint8_t  buffer_bank = xgs::sysRead(pbank, paddr + 4);

    uint32_t block = xgs::sysRead(pbank, paddr + 6)
                        | (xgs::sysRead(pbank, paddr + 7) << 8)
                        | (xgs::sysRead(pbank, paddr + 8) << 16)
                        | (xgs::sysRead(pbank, paddr + 9) << 24);

    if (block > unit->num_chunks) {
        m65816::registers::A.B.L = 0x2D;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        disk_buffer[i] = xgs::sysRead(buffer_bank, buffer_addr + i);
    }

    try {
        unit->write(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        m65816::registers::A.B.L = 0x27;

        return;
    }

    m65816::registers::X.B.L = 0x00;
    m65816::registers::Y.B.L = 0x02;
    m65816::registers::A.B.L = 0;
}

void Smartport::formatCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    m65816::registers::A.B.L = 0;
}

void Smartport::controlCmdExt(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::initCmdExt(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::openCmdExt(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::closeCmdExt(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::readCmdExt(const uint8_t pbank, const uint16_t paddr)
{
}

void Smartport::writeCmdExt(const uint8_t pbank, const uint16_t paddr)
{
}
