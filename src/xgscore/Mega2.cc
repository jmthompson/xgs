/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the memory-management functionality of the
 * MegaII and the FPI/CYA. It listens for accesses to the memory
 * control softswitches and rebuilds the System's memory mapping as
 * needed.
 */

#include <cstdlib>

#include "ADB.h"
#include "Mega2.h"
#include "video/VGC.h"
#include "System.h"
#include "M65816/Processor.h"

using std::uint8_t;

/**
 * Reset the memory switches to their powerup state.
 */
void Mega2::reset()
{
    vgc = (VGC *) system->getDevice("vgc");
    adb = (ADB *) system->getDevice("adb");

    sw_80store = false;
    sw_auxrd   = false;
    sw_auxwr   = false;
    sw_altzp   = false;

    sw_lcbank2  = false;
    sw_lcread   = false;
    sw_lcwrite  = false;

    sw_intcxrom = false;

    last_access = 0;

    sw_shadow_text   = true;
    sw_shadow_text2  = false;
    sw_shadow_hires1 = true;
    sw_shadow_hires2 = true;
    sw_shadow_super  = false;
    sw_shadow_aux    = true;
    sw_shadow_lc     = true;

    for (int i = 0 ; i < 5 ; i++) sw_slot_reg[i] = false;

    sw_slot_reg[7] = true;

    sw_fastmode    = true;
    sw_slot7_motor = false;
    sw_slot6_motor = false;
    sw_slot5_motor = false;
    sw_slot4_motor = false;

    updateMemoryMaps();
}

void Mega2::updateMemoryMaps()
{
    unsigned int page;

    /*
     * Legacy main/aux memory Bank switching
     */

    for (page = 0x0000 ; page < 0x0002 ; page++) {
        system->mapRead(page, sw_altzp? page + 0x0100 : page);
        system->mapWrite(page, sw_altzp? page + 0x0100 : page);
    }
    for (page = 0x0002 ; page < 0x0003 ; page++) {
        system->mapRead(page,  sw_auxrd? page + 0x0100 : page);
        system->mapWrite(page, sw_auxwr? page + 0x0100 : page);
    }
    for (page = 0x0004 ; page < 0x0008 ; page++) {
        if (sw_80store) {
            system->mapRead(page,  vgc->sw_page2? page + 0x0100 : page);
            system->mapWrite(page, vgc->sw_page2? page + 0x0100 : page);
        }
        else {
            system->mapRead(page,  sw_auxrd? page + 0x0100 : page);
            system->mapWrite(page, sw_auxwr? page + 0x0100 : page);
        }
    }
    for (page = 0x0008 ; page < 0x0020 ; page++) {
        system->mapRead(page,  sw_auxrd? page + 0x0100 : page);
        system->mapWrite(page, sw_auxwr? page + 0x0100 : page);
    }
    for (page = 0x0020 ; page < 0x0040 ; page++) {
        if (sw_80store && vgc->sw_hires) {
            system->mapRead(page,  vgc->sw_page2? page + 0x0100 : page);
            system->mapWrite(page, vgc->sw_page2? page + 0x0100 : page);
        }
        else {
            system->mapRead(page,  sw_auxrd? page + 0x0100 : page);
            system->mapWrite(page, sw_auxwr? page + 0x0100 : page);
        }
    }
    for (page = 0x0040 ; page < 0x00C0 ; page++) {
        system->mapRead(page,  sw_auxrd? page + 0x0100 : page);
        system->mapWrite(page, sw_auxwr? page + 0x0100 : page);
    }

    // Language cards

    buildLanguageCard(0xE0, 0xE0);
    buildLanguageCard(0xE1, 0xE1);

    if (sw_shadow_lc) {
        buildLanguageCard(0x00, sw_altzp? 0x01 : 0x00);
        buildLanguageCard(0x01, 0x01);
    }
    else {
        for (page = 0x00C0 ; page <= 0x00FF ; page++) {
            system->mapRead(page,  sw_auxrd? page + 0x0100 : page);
            system->mapWrite(page, sw_auxwr? page + 0x0100 : page);
        }
        for (page = 0x01C0 ; page <= 0x01FF ; page++) {
            system->mapRead(page, page);
            system->mapWrite(page, page);
        }
    }

    /*
     * Shadowing
     * FIXME: support shadowing in all banks
     */

    for (page = 0x0004 ; page < 0x0008 ; page++) {
        system->setShadowed(page, sw_shadow_text);
    }
    for (page = 0x0008 ; page < 0x000C ; page++) {
        system->setShadowed(page, sw_shadow_text2);
    }
    for (page = 0x0020 ; page < 0x0040 ; page++) {
        system->setShadowed(page, sw_shadow_hires1);
    }
    for (page = 0x0060 ; page < 0x0080 ; page++) {
        system->setShadowed(page, sw_shadow_hires2);
    }
    for (page = 0x0104 ; page < 0x0108 ; page++) {
        system->setShadowed(page, sw_shadow_text);
    }
    for (page = 0x0108 ; page < 0x010C ; page++) {
        system->setShadowed(page, sw_shadow_text2);
    }
    for (page = 0x0120 ; page < 0x0140 ; page++) {
        system->setShadowed(page, (sw_shadow_hires1 && sw_shadow_aux) || sw_shadow_super);
    }
    for (page = 0x0140 ; page < 0x0160 ; page++) {
        system->setShadowed(page, (sw_shadow_hires2 && sw_shadow_aux) || sw_shadow_super);
    }
    for (page = 0x0160 ; page < 0x01A0 ; page++) {
        system->setShadowed(page, sw_shadow_aux || sw_shadow_super);
    }
}

/**
 * Build a 16k language card in dst_bank, using RAM pages from src_bank. The
 * separate src_bank is used to account for the ALTZP softswitch when building
 * the bank 0 language card.
 */
void Mega2::buildLanguageCard(unsigned int dst_bank, unsigned int src_bank)
{
    unsigned int page;

    dst_bank <<= 8;
    src_bank <<= 8;

    system->mapIO(dst_bank|0xC0);

    for (page = 0xC1 ; page <= 0xCF ; page++) {
        system->mapRead(dst_bank|page,  0xFF00|page);
        system->mapWrite(dst_bank|page, 0xFF00|page);
    }

    unsigned int offset = sw_lcbank2? 0 : 0x10;

    for (page = 0xD0 ; page <= 0xDF ; page++) {
        system->mapRead(dst_bank|page,  sw_lcread?  src_bank|(page - offset): 0xFF00|page);
        system->mapWrite(dst_bank|page, sw_lcwrite? src_bank|(page - offset): 0xFF00|page);
    }

    for (page = 0xE0 ; page <= 0xFF ; page++) {
        system->mapRead(dst_bank|page,  sw_lcread?  src_bank|page : 0xFF00|page);
        system->mapWrite(dst_bank|page, sw_lcwrite? src_bank|page : 0xFF00|page);
    }
}

uint8_t Mega2::read(const unsigned int& offset)
{
    uint8_t val = 0;

    switch (offset) {
        case 0x01:
            sw_80store = true;
            updateMemoryMaps();
            break;

        case 0x02:
            sw_auxrd = false;
            updateMemoryMaps();
            break;

        case 0x03:
            sw_auxrd = true;
            updateMemoryMaps();
            break;

        case 0x04:
            sw_auxwr = false;
            updateMemoryMaps();
            break;

        case 0x05:
            sw_auxwr = true;
            updateMemoryMaps();
            break;

        case 0x06:
            sw_intcxrom = false;
            break;

        case 0x07:
            sw_intcxrom = true;
            break;

        case 0x08:
            sw_altzp = false;
            updateMemoryMaps();
            break;

        case 0x09:
            sw_altzp = true;
            updateMemoryMaps();
            break;

        case 0x0A:
            sw_slotc3rom = false;
            break;

        case 0x0B:
            sw_slotc3rom = true;
            break;

        case 0x11:
            val = sw_lcbank2? 0x80 : 0x00;
            break;

        case 0x12:
            val = sw_lcread? 0x80 : 0x00;
            break;

        case 0x13:
            val = sw_auxwr? 0x80 : 0x00;
            break;

        case 0x14:
            val = sw_auxwr? 0x80 : 0x00;
            break;

        case 0x15:
            val = sw_intcxrom? 0x80 : 0x00;
            break;

        case 0x16:
            val = sw_altzp? 0x80 : 0x00;
            break;

        case 0x17:
            val = sw_slotc3rom? 0x80 : 0x00;
            break;

        case 0x18:
            val = sw_80store? 0x80 : 0x00;
            break;

        case 0x2D:
            for (int i = 7 ; i >= 0 ; --i) {
                val <<= 1;
                val |= sw_slot_reg[i];
            }

            break;

        case 0x35:
            if (!sw_shadow_text)   val |= 0x01;
            if (!sw_shadow_hires1) val |= 0x02;
            if (!sw_shadow_hires2) val |= 0x04;
            if (!sw_shadow_super)  val |= 0x08;
            if (!sw_shadow_aux)    val |= 0x10;
            //if (!sw_shadow_text2)  val |= 0x20;
            if (!sw_shadow_lc)     val |= 0x40;

            break;

        case 0x36:
            if (sw_fastmode)    val |= 0x80;
            if (sw_slot7_motor) val |= 0x08;
            if (sw_slot6_motor) val |= 0x04;
            if (sw_slot5_motor) val |= 0x02;
            if (sw_slot4_motor) val |= 0x01;

            break;

        case 0x41:
            if (vgc->sw_qtrsecirq_enable) val |= 0x10;
            if (vgc->sw_vblirq_enable)    val |= 0x08;
            if (adb->sw_m2mouseswirq)     val |= 0x04;
            if (adb->sw_m2mousemvirq)     val |= 0x02;
            if (adb->sw_m2mouseenable)    val |= 0x01;

            break;

        case 0x46:
            val = sw_diagtype;

            break;

        case 0x68:
            if (sw_intcxrom)   val |= 0x01;
            if (sw_rombank)    val |= 0x02;
            if (sw_lcbank2)    val |= 0x04;
            if (!sw_lcread)    val |= 0x08;
            if (sw_auxwr)      val |= 0x10;
            if (sw_auxrd)      val |= 0x20;
            if (vgc->sw_page2) val |= 0x40;
            if (sw_altzp)      val |= 0x80;
            break;

        case 0x80:
            sw_lcbank2 = true;
            sw_lcread  = true;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F:
            val = system->getPage(0xFFC0).read[offset];
            break;
        case 0x81:
            sw_lcbank2 = true;
            sw_lcread  = false;

            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        case 0x82:
            sw_lcbank2 = true;
            sw_lcread  = false;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x83:
            sw_lcbank2 = true;
            sw_lcread  = true;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        case 0x88:
            sw_lcbank2 = false;
            sw_lcread  = true;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x89:
            sw_lcbank2 = false;
            sw_lcread  = false;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        case 0x8A:
            sw_lcbank2 = false;
            sw_lcread  = false;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x8B:
            sw_lcbank2 = false;
            sw_lcread  = true;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        default:
            break;
    }

    last_access = offset;

    return val;
}

void Mega2::write(const unsigned int& offset, const uint8_t& val)
{
    switch (offset) {
        case 0x00:
            sw_80store = false;
            updateMemoryMaps();
            break;

        case 0x01:
            sw_80store = true;
            updateMemoryMaps();
            break;

        case 0x02:
            sw_auxrd = false;
            updateMemoryMaps();
            break;

        case 0x03:
            sw_auxrd = true;
            updateMemoryMaps();
            break;

        case 0x04:
            sw_auxwr = false;
            updateMemoryMaps();
            break;

        case 0x05:
            sw_auxwr = true;
            updateMemoryMaps();
            break;

        case 0x06:
            sw_intcxrom = false;
            break;

        case 0x07:
            sw_intcxrom = true;
            break;

        case 0x08:
            sw_altzp = false;
            updateMemoryMaps();
            break;

        case 0x09:
            sw_altzp = true;
            updateMemoryMaps();
            break;

        case 0x0A:
            sw_slotc3rom = false;
            break;

        case 0x0B:
            sw_slotc3rom = true;
            break;

        case 0x2D:
            for (int i = 0 ; i < 8 ; ++i) {
                sw_slot_reg[i] = val & (1 << i);
            }

            updateMemoryMaps();

            break;

        case 0x35:
            sw_shadow_text   = !(val & 0x01);
            sw_shadow_hires1 = !(val & 0x02);
            sw_shadow_hires2 = !(val & 0x04);
            sw_shadow_super  = !(val & 0x08);
            sw_shadow_aux    = !(val & 0x10);
            sw_shadow_text2  = !(val & 0x20);
            sw_shadow_lc     = !(val & 0x40);

            updateMemoryMaps();

            break;

        case 0x36:
            sw_fastmode    = val & 0x80;
            sw_slot7_motor = val & 0x08;
            sw_slot6_motor = val & 0x04;
            sw_slot5_motor = val & 0x02;
            sw_slot4_motor = val & 0x01;
#if 0
            if (sw_fastmode) {
                schedulerSetTargetSpeed(g_fast_mhz);
            }
            else {
                schedulerSetTargetSpeed(1.0);
            }
#endif
            break;

        case 0x41:
            adb->sw_m2mouseenable    = val & 0x01;
            adb->sw_m2mousemvirq     = val & 0x02;
            adb->sw_m2mouseswirq     = val & 0x04;
            vgc->sw_vblirq_enable    = val & 0x08;
            vgc->sw_qtrsecirq_enable = val & 0x10;

            break;

        case 0x47:
            if (sw_diagtype & 0x10) system->cpu->lowerInterrupt();
            if (sw_diagtype & 0x08) system->cpu->lowerInterrupt();

            sw_diagtype &= ~0x18;

            break;

        case 0x68:
            sw_intcxrom   = val & 0x01;
            sw_rombank    = val & 0x02;
            sw_lcbank2    = val & 0x04;
            sw_lcread     = !(val & 0x08);
            sw_auxwr      = val & 0x10;
            sw_auxrd      = val & 0x20;
            vgc->sw_page2 = val & 0x40;
            sw_altzp      = val & 0x80;

            // FIXME: tell vgc that page2 changed
            updateMemoryMaps();

            break;

        case 0x80:
            sw_lcbank2 = true;
            sw_lcread  = true;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x81:
            sw_lcbank2 = true;
            sw_lcread  = false;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        case 0x82:
            sw_lcbank2 = true;
            sw_lcread  = false;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x83:
            sw_lcbank2 = true;
            sw_lcread  = true;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        case 0x88:
            sw_lcbank2 = false;
            sw_lcread  = true;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x89:
            sw_lcbank2 = false;
            sw_lcread  = false;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;

        case 0x8A:
            sw_lcbank2 = false;
            sw_lcread  = false;
            sw_lcwrite = false;

            updateMemoryMaps();

            break;

        case 0x8B:
            sw_lcbank2 = false;
            sw_lcread  = true;
            if (last_access == offset) sw_lcwrite = true;

            updateMemoryMaps();

            break;
    }

    last_access = offset;
}
