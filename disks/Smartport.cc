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
#include "M65816/Processor.h"
#include "emulator/System.h"

using std::uint8_t;
using std::uint16_t;

uint8_t disk_buffer[512];

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
    0x42, 0xC7, 0x60, 0x4C, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x93, 0x80,
};

Smartport::Smartport()
{
    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        units[i] = nullptr;
    }
}

Smartport::~Smartport()
{
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

void Smartport::attach(System *theSystem)
{
    Device::attach(theSystem);

    system->installMemory((uint8_t *) smartport_rom, 0xFFC7, 1, ROM);
    system->setWdmHandler(0xC7, this);
    system->setWdmHandler(0xC8, this);
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
    M65816::Processor *cpu = system->cpu;

    uint8_t cmd = system->sysRead(0, 0x42);
    VirtualDisk *unit = (system->sysRead(0, 0x43) & 0x80)? units[1] : units[0];

    if (!unit) {
        cpu->A.B.L = 0x028; // NO DEVICE CONNECTED
        cpu->SR.C  = true;

        return;
    }

    uint16_t buffer = system->sysRead(0, 0x44) | (system->sysRead(0, 0x45) << 8);
    uint16_t block  = system->sysRead(0, 0x46) | (system->sysRead(0, 0x47) << 8);

    cpu->A.B.L = 0;

    switch (cmd) {
        case 0:
            cpu->X.B.L = unit->num_chunks & 0xFF;
            cpu->Y.B.L = unit->num_chunks >> 8;

            break;
        case 1:
            try {
                unit->read(disk_buffer, block, 1);

                for (unsigned int i = 0 ; i < sizeof(disk_buffer); ++i) {
                    system->sysWrite(0, buffer + i, disk_buffer[i]);
                }
            }
            catch (std::runtime_error& e) {
                cpu->A.B.L = 0x27;  // I/O ERROR
            }

            break;
        case 2:
            if (unit->locked) {
                cpu->A.B.L = 0x2B;   // WRITE PROTECTED
            }
            else {
                try {
                    for (unsigned int i = 0 ; i < sizeof(disk_buffer); ++i) {
                        disk_buffer[i] = system->sysRead(0, buffer + i);
                    }

                    unit->write(disk_buffer, block, 1);
                }
                catch (std::runtime_error& e) {
                    cpu->A.B.L = 0x27;  // I/O ERROR
                }
            }

            break;
        case 3:
            if (unit->locked) {
                cpu->A.B.L = 0x2B;   // WRITE PROTECTED
            }

            break;
        default :
            cpu->A.B.L = 0x01; // BAD CALL

            break;
    }

    cpu->SR.C = cpu->A.B.L;
}

void Smartport::smartportEntry()
{
    M65816::Processor *cpu = system->cpu;

    uint16_t addr  = (system->sysRead(0, cpu->S.W + 1) | (system->sysRead(0, cpu->S.W + 2) << 8)) + 1;

    uint8_t  cmd   = system->sysRead(0, addr);
    uint16_t paddr = system->sysRead(0, addr + 1) | (system->sysRead(0, addr + 2) << 8);
    uint8_t  pbank;

    if (cmd & 0x40) {
        pbank = system->sysRead(0, addr + 3);
        addr += 4;
    }
    else {
        pbank = 0;
        addr += 2;
    }
    
    system->sysWrite(0, cpu->S.W + 1, addr & 0xFF);
    system->sysWrite(0, cpu->S.W + 2, addr >> 8);

    //num_params = system->sysRead(pbank, paddr);

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
            cpu->A.B.L = 0x01; // Invalid command
            break;
    }

    cpu->SR.C = cpu->A.B.L;
}

void Smartport::statusCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    unsigned int unit_num = system->sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        cpu->A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    uint16_t status_addr = system->sysRead(pbank, paddr + 2) | (system->sysRead(pbank, paddr + 3) << 8); 
    uint8_t  status_code = system->sysRead(pbank, paddr + 4);

    switch(status_code) {
        case 0x00:
            if (!unit_num) {
                system->sysWrite(0, status_addr, kSmartportUnits);

                for (unsigned int i = 1 ; i < 8 ; ++i) {
                    system->sysWrite(0, status_addr + i, 0);
                }

                cpu->X.B.L = 8;
                cpu->Y.B.L = 0;
                cpu->A.B.L = 0;
            }
            else {
                if (unit == nullptr) {
                    system->sysWrite(0, status_addr, 0xEC);
                    system->sysWrite(0, status_addr + 1, 0);
                    system->sysWrite(0, status_addr + 2, 0);
                    system->sysWrite(0, status_addr + 3, 0);
                }
                else {
                    system->sysWrite(0, status_addr, unit->locked? 0xFC: 0xF8);
                    system->sysWrite(0, status_addr + 1, unit->num_chunks & 0xFF);
                    system->sysWrite(0, status_addr + 2, unit->num_chunks >> 8);
                    system->sysWrite(0, status_addr + 3, 0);
                }

                cpu->X.B.L = 4;
                cpu->Y.B.L = 0;
                cpu->A.B.L = 0;
            }

            break;
        case 0x01:
            cpu->A.B.L = 0x21;

            break;
        case 0x02:
            cpu->A.B.L = 0x21;

            break;
        case 0x03:
            if (unit == nullptr) {
                system->sysWrite(0, status_addr, 0xEC);
                system->sysWrite(0, status_addr + 1, 0);
                system->sysWrite(0, status_addr + 2, 0);
                system->sysWrite(0, status_addr + 3, 0);
            }
            else {
                system->sysWrite(0, status_addr, unit->locked? 0xFC: 0xF8);
                system->sysWrite(0, status_addr + 1, unit->num_chunks & 0xFF);
                system->sysWrite(0, status_addr + 2, unit->num_chunks >> 8);
                system->sysWrite(0, status_addr + 3, 0);
            }

            system->sysWrite(0, status_addr + 4, 0x10);

            for (unsigned int i = 0 ; i < 16 ; ++i) {
                system->sysWrite(0, status_addr + 5 + i, id_string[i]);
            }

            system->sysWrite(0, status_addr + 21, 0x02);
            system->sysWrite(0, status_addr + 22, 0xC0);
            system->sysWrite(0, status_addr + 23, 0x00);
            system->sysWrite(0, status_addr + 24, 0x00);

            cpu->X.B.L = 25;
            cpu->Y.B.L = 0;
            cpu->A.B.L = 0;

            break;
        default:
            cpu->A.B.L = 0x21;

            break;
    }
}

void Smartport::readBlockCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    unsigned int unit_num = system->sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        cpu->A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        cpu->A.B.L = 0x2F;

        return;
    }

    uint16_t buffer = system->sysRead(pbank, paddr + 2) | (system->sysRead(pbank, paddr + 3) << 8);
    uint32_t block  = system->sysRead(pbank, paddr + 4) | (system->sysRead(pbank, paddr + 5) << 8)
                        | (system->sysRead(pbank, paddr + 6) << 16);

    if (block > unit->num_chunks) {
        cpu->A.B.L = 0x2D;

        return;
    }

    try {
        unit->read(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        cpu->A.B.L = 0x27;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        system->sysWrite(0, buffer + i, disk_buffer[i]);
    }

    cpu->X.B.L = 0x00;
    cpu->Y.B.L = 0x02;
    cpu->A.B.L = 0;
}

void Smartport::writeBlockCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    unsigned int unit_num = system->sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        cpu->A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        cpu->A.B.L = 0x2F;

        return;
    }

    if (unit->locked) {
        cpu->A.B.L = 0x2B;

        return;
    }

    uint16_t buffer = system->sysRead(pbank, paddr + 2) | (system->sysRead(pbank, paddr + 3) << 8);
    uint32_t block  = system->sysRead(pbank, paddr + 4) | (system->sysRead(pbank, paddr + 5) << 8)
                        | (system->sysRead(pbank, paddr + 6) << 16);

    if (block > unit->num_chunks) {
        cpu->A.B.L = 0x2D;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        disk_buffer[i] = system->sysRead(0, buffer + i);
    }

    try {
        unit->write(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        cpu->A.B.L = 0x27;

        return;
    }

    cpu->X.B.L = 0x00;
    cpu->Y.B.L = 0x02;
    cpu->A.B.L = 0;
}

void Smartport::formatCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    cpu->A.B.L = 0;
}

void Smartport::controlCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    cpu->X.B.L = 0;
    cpu->Y.B.L = 0;
    cpu->A.B.L = 0;
}

void Smartport::initCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::openCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::closeCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::readCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::writeCmd(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::statusCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    unsigned int unit_num = system->sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        cpu->A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    uint16_t status_addr = system->sysRead(pbank, paddr + 2) | (system->sysRead(pbank, paddr + 3) << 8);
    uint8_t  status_bank = system->sysRead(pbank, paddr + 4);
    uint8_t  status_code = system->sysRead(pbank, paddr + 6);

    switch (status_code) {
        case 0x00:
            if (!unit_num) {
                system->sysWrite(0, status_addr, kSmartportUnits);

                for (unsigned int i = 1 ; i < 8 ; ++i) {
                    system->sysWrite(status_bank, status_addr + i, 0);
                }

                cpu->X.B.L = 8;
                cpu->Y.B.L = 0;
                cpu->A.B.L = 0;
            }
            else {
                if (unit == nullptr) {
                    system->sysWrite(status_bank, status_addr, 0xEC);
                    system->sysWrite(status_bank, status_addr + 1, 0);
                    system->sysWrite(status_bank, status_addr + 2, 0);
                    system->sysWrite(status_bank, status_addr + 3, 0);
                    system->sysWrite(status_bank, status_addr + 4, 0);
                }
                else {
                    system->sysWrite(status_bank, status_addr, unit->locked? 0xFC: 0xF8);
                    system->sysWrite(status_bank, status_addr + 1, unit->num_chunks & 0xFF);
                    system->sysWrite(status_bank, status_addr + 2, unit->num_chunks >> 8);
                    system->sysWrite(status_bank, status_addr + 3, 0);
                    system->sysWrite(status_bank, status_addr + 4, 0);
                }

                cpu->X.B.L = 5;
                cpu->Y.B.L = 0;
                cpu->A.B.L = 0;
            }

            break;
        case 0x01:
            cpu->A.B.L = 0x21;

            break;
        case 0x02:
            cpu->A.B.L = 0x21;

            break;
        case 0x03:
            if (unit == nullptr) {
                system->sysWrite(status_bank, status_addr, 0xEC);
                system->sysWrite(status_bank, status_addr + 1, 0);
                system->sysWrite(status_bank, status_addr + 2, 0);
                system->sysWrite(status_bank, status_addr + 3, 0);
                system->sysWrite(status_bank, status_addr + 4, 0);
            }
            else {
                system->sysWrite(status_bank, status_addr, unit->locked? 0xFC: 0xF8);
                system->sysWrite(status_bank, status_addr + 1, unit->num_chunks & 0xFF);
                system->sysWrite(status_bank, status_addr + 2, unit->num_chunks >> 8);
                system->sysWrite(status_bank, status_addr + 3, 0);
                system->sysWrite(status_bank, status_addr + 4, 0);
            }

            system->sysWrite(0, status_addr + 5, 0x10);

            for (unsigned int i = 0 ; i < 16 ; ++i) {
                system->sysWrite(0, status_addr + 6 + i, id_string[i]);
            }

            system->sysWrite(0, status_addr + 22, 0x02);
            system->sysWrite(0, status_addr + 23, 0xC0);
            system->sysWrite(0, status_addr + 24, 0x00);
            system->sysWrite(0, status_addr + 25, 0x00);

            cpu->X.B.L = 26;
            cpu->Y.B.L = 0;
            cpu->A.B.L = 0;

            break;
        default :
            cpu->A.B.L = 0x21;

            break;
    }
}

void Smartport::readBlockCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    unsigned int unit_num = system->sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        cpu->A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        cpu->A.B.L = 0x2F;

        return;
    }

    uint16_t buffer_addr = system->sysRead(pbank, paddr + 2) | (system->sysRead(pbank, paddr + 3) << 8);
    uint8_t  buffer_bank = system->sysRead(pbank, paddr + 4);

    uint32_t block = system->sysRead(pbank, paddr + 6)
                        | (system->sysRead(pbank, paddr + 7) << 8)
                        | (system->sysRead(pbank, paddr + 8) << 16)
                        | (system->sysRead(pbank, paddr + 9) << 24);

    if (block > unit->num_chunks) {
        cpu->A.B.L = 0x2D;

        return;
    }

    try {
        unit->read(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        cpu->A.B.L = 0x27;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        system->sysWrite(buffer_bank, buffer_addr + i, disk_buffer[i]);
    }

    cpu->X.B.L = 0x00;
    cpu->Y.B.L = 0x02;
    cpu->A.B.L = 0;
}

void Smartport::writeBlockCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    unsigned int unit_num = system->sysRead(pbank, paddr + 1);

    if ((unit_num < 0) || (unit_num > kSmartportUnits)) {
        cpu->A.B.L = 0x28;

        return;
    }

    VirtualDisk *unit = unit_num? units[unit_num - 1] : nullptr;

    if (unit == nullptr) {
        cpu->A.B.L = 0x2F;

        return;
    }

    if (unit->locked) {
        cpu->A.B.L = 0x2B;

        return;
    }

    uint16_t buffer_addr = system->sysRead(pbank, paddr + 2) | (system->sysRead(pbank, paddr + 3) << 8); 
    uint8_t  buffer_bank = system->sysRead(pbank, paddr + 4);

    uint32_t block = system->sysRead(pbank, paddr + 6)
                        | (system->sysRead(pbank, paddr + 7) << 8)
                        | (system->sysRead(pbank, paddr + 8) << 16)
                        | (system->sysRead(pbank, paddr + 9) << 24);

    if (block > unit->num_chunks) {
        cpu->A.B.L = 0x2D;

        return;
    }

    for (unsigned int i = 0 ; i < sizeof(disk_buffer) ; ++i) {
        disk_buffer[i] = system->sysRead(buffer_bank, buffer_addr + i);
    }

    try {
        unit->write(disk_buffer, block, 1);
    }
    catch (std::runtime_error& e) {
        cpu->A.B.L = 0x27;

        return;
    }

    cpu->X.B.L = 0x00;
    cpu->Y.B.L = 0x02;
    cpu->A.B.L = 0;
}

void Smartport::formatCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;

    cpu->A.B.L = 0;
}

void Smartport::controlCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::initCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::openCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::closeCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::readCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}

void Smartport::writeCmdExt(const uint8_t pbank, const uint16_t paddr)
{
    M65816::Processor *cpu = system->cpu;
}
