/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This file provides implementations for the ioRead and ioWrite
 * methods, which handle access to the $C0xx I/O space.
 */

#include <cstdint>

#include "System.h"

using std::uint8_t;

uint8_t System::ioRead(uint8_t io_offset)
{
    uint8_t val;

    switch (io_offset) {
        case 0x00:
            // ADB_readKey
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

        case 0x0C:
            // VID_clear80col
            break;

        case 0x0D:
            // VID_set80col
            break;

        case 0x0E:
            // VID_clearAltCh
            break;

        case 0x0F:
            // VID_setAltCh
            break;

        case 0x10:
            // ADB_clearKey
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

        case 0x19:
            // hardwareInVBL
            break;

        case 0x1A:
            // VID_getText
            break;

        case 0x1B:
            // VID_getMixed
            break;

        case 0x1C:
            // VID_getPage2
            break;

        case 0x1D:
            // VID_getHires
            break;

        case 0x1E:
            // VID_getAltCh
            break;

        case 0x1F:
            // VID_get80col
            break;

        case 0x22:
            // VID_getColorReg
            break;

        case 0x23:
            // VID_getVGCIntReg
            break;

        case 0x24:
            // ADB_readMouse
            break;

        case 0x25:
            // ADB_readModifiers
            break;

        case 0x26:
            // ADB_readCommand
            break;

        case 0x27:
            // ADB_readStatus
            break;

        case 0x29:
            // VID_getNewVideo
            break;

        case 0x2D:
            // MEM_getSlotReg
            break;

        case 0x2E:
            // VID_getVertCnt
            break;

        case 0x2F:
            // VID_getHorzCnt
            break;

        case 0x30:
            // soundClickSpeaker
            break;

        case 0x31:
            // IWM_getDiskReg
            break;

        case 0x33:
            // clockGetData
            break;

        case 0x34:
            // clockGetControl
            break;

        case 0x35:
            if (!sw_shadow_text)   val |= 0x01;
            if (!sw_shadow_hires1) val |= 0x02;
            if (!sw_shadow_hires2) val |= 0x04;
            if (!sw_shadow_super)  val |= 0x08;
            if (!sw_shadow_aux)    val |= 0x10;
            if (!sw_shadow_text2)  val |= 0x10;
            if (!sw_shadow_lc)     val |= 0x40;

            break;

        case 0x36:
            // MEM_getCYAReg
/*
            if (g_fastmode) val |= 0x80;
            if (iwm_slot7_motor) val |= 0x08;
            if (iwm_slot6_motor) val |= 0x04;
            if (iwm_slot5_motor) val |= 0x02;
            if (iwm_slot4_motor) val |= 0x01;
*/
            break;

        case 0x3C:
            // SND_readSoundCtl
            break;

        case 0x3D:
            // SND_readSoundData
            break;

        case 0x3E:
            // SND_readSoundAddrL
            break;

        case 0x3F:
            // SND_readSoundAddrH
            break;

        case 0x41:
            // MEM_getIntEnReg
            break;

        case 0x44:
            // ADB_readM2MouseX
            break;

        case 0x45:
            // ADB_readM2MouseY
            break;

        case 0x46:
            // MEM_getDiagType
            break;

        case 0x50:
            // VID_clearText
            break;

        case 0x51:
            // VID_setText
            break;

        case 0x52:
            // VID_clearMixed
            break;

        case 0x53:
            // VID_setMixed
            break;

        case 0x54:
            // VID_clearPage2
            break;

        case 0x55:
            // VID_setPage2
            break;

        case 0x56:
            // VID_clearHires
            break;

        case 0x57:
            // VID_setHires
            break;

        case 0x5E:
            // VID_setDblRes
            break;

        case 0x5F:
            // VID_clearDblRes
            break;

        case 0x61:
            // ADB_readCommandKey
            break;

        case 0x62:
            // ADB_readOptionKey
            break;

        case 0x64:
            // ADB_readPaddle0
            break;

        case 0x65:
            // ADB_readPaddle1
            break;

        case 0x66:
            // ADB_readPaddle2
            break;

        case 0x67:
            // ADB_readPaddle3
            break;

        case 0x68:
            if (sw_intcxrom) val |= 0x01;
            if (sw_rombank)  val |= 0x02;
            if (sw_lcbank2)  val |= 0x04;
            if (!sw_lcread)  val |= 0x08;
            if (sw_auxwr)    val |= 0x10;
            if (sw_auxrd)    val |= 0x20;
            if (vid_page2)   val |= 0x40;
            if (sw_altzp)    val |= 0x80;
            break;

        case 0x70:
            // ADB_triggerPaddles
            break;

        case 0x71:
            // MEM_getC071
            break;

        case 0x72:
            // MEM_getC072
            break;

        case 0x73:
            // MEM_getC073
            break;

        case 0x74:
            // MEM_getC074
            break;

        case 0x75:
            // MEM_getC075
            break;

        case 0x76:
            // MEM_getC076
            break;

        case 0x77:
            // MEM_getC077
            break;

        case 0x78:
            // MEM_getC078
            break;

        case 0x79:
            // MEM_getC079
            break;

        case 0x7A:
            // MEM_getC07A
            break;

        case 0x7B:
            // MEM_getC07B
            break;

        case 0x7C:
            // MEM_getC07C
            break;

        case 0x7D:
            // MEM_getC07D
            break;

        case 0x7E:
            // MEM_getC07E
            break;

        case 0x7F:
            // MEM_getC07F
            break;

        case 0x80:
            // MEM_setLCx80
            break;

        case 0x81:
            // MEM_setLCx81
            break;

        case 0x82:
            // MEM_setLCx82
            break;

        case 0x83:
            // MEM_setLCx83
            break;

        case 0x88:
            // MEM_setLCx88
            break;

        case 0x89:
            // MEM_setLCx89
            break;

        case 0x8A:
            // MEM_setLCx8A
            break;

        case 0x8B:
            // MEM_setLCx8B
            break;

        case 0xE0:
            // IWM_readC0E0
            break;

        case 0xE1:
            // IWM_readC0E1
            break;

        case 0xE2:
            // IWM_readC0E2
            break;

        case 0xE3:
            // IWM_readC0E3
            break;

        case 0xE4:
            // IWM_readC0E4
            break;

        case 0xE5:
            // IWM_readC0E5
            break;

        case 0xE6:
            // IWM_readC0E6
            break;

        case 0xE7:
            // IWM_readC0E7
            break;

        case 0xE8:
            // IWM_readC0E8
            break;

        case 0xE9:
            // IWM_readC0E9
            break;

        case 0xEA:
            // IWM_readC0EA
            break;

        case 0xEB:
            // IWM_readC0EB
            break;

        case 0xEC:
            // IWM_readC0EC
            break;

        case 0xED:
            // IWM_readC0ED
            break;

        case 0xEE:
            // IWM_readC0EE
            break;

        case 0xEF:
            // IWM_readC0EF
            break;

        case 0xFF:
            // MEM_ioUnimp           
            break;

        default:
            break;
    }

    return val;
}

void System::ioWrite(uint8_t io_offset, uint8_t val)
{
    switch (io_offset) {
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

        case 0x0C:
            // VID_clear80col
            break;

        case 0x0D:
            // VID_set80col
            break;

        case 0x0E:
            // VID_clearAltCh
            break;

        case 0x0F:
            // VID_setAltCh
            break;

        case 0x10:
            // ADB_clearKey
            break;

        case 0x22:
            // VID_setColorReg
            break;

        case 0x23:
            // VID_setVGCIntReg
            break;

        case 0x26:
            // ADB_setCommand
            break;

        case 0x27:
            // ADB_setStatus
            break;

        case 0x29:
            // VID_setNewVideo
            break;

        case 0x2D:
            // MEM_setSlotReg
            break;

        case 0x30:
            // soundClickSpeaker
            break;

        case 0x31:
            // IWM_setDiskReg
            break;

        case 0x32:
            // VID_clearVGCInt
            break;

        case 0x33:
            // clockSetData
            break;

        case 0x34:
            // clockSetControl
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
            // MEM_setCYAReg
            break;

        case 0x3C:
            // SND_writeSoundCtl
            break;

        case 0x3D:
            // SND_writeSoundData
            break;

        case 0x3E:
            // SND_writeSoundAddrL
            break;

        case 0x3F:
            // SND_writeSoundAddrH
            break;

        case 0x41:
            // MEM_setIntEnReg
            break;

        case 0x47:
            // MEM_setVBLClear
            break;

        case 0x50:
            // VID_clearText
            break;

        case 0x51:
            // VID_setText
            break;

        case 0x52:
            // VID_clearMixed
            break;

        case 0x53:
            // VID_setMixed
            break;

        case 0x54:
            // VID_clearPage2
            break;

        case 0x55:
            // VID_setPage2
            break;

        case 0x56:
            // VID_clearHires
            break;

        case 0x57:
            // VID_setHires
            break;

        case 0x5E:
            // VID_setDblRes
            break;

        case 0x5F:
            // VID_clearDblRes
            break;

        case 0x68:
            sw_intcxrom = val & 0x01;
            sw_rombank  = val & 0x02;
            sw_lcbank2  = val & 0x04;
            sw_lcread   = val & 0x08;
            sw_auxwr    = val & 0x10;
            sw_auxrd    = val & 0x20;
            vid_page2   = val & 0x40;
            sw_altzp    = val & 0x80;

            updateMemoryMaps();

            break;

        case 0x70:
            // ADB_triggerPaddles
            break;

        case 0x80:
            // MEM_setLCx80
            break;

        case 0x81:
            // MEM_setLCx81
            break;

        case 0x82:
            // MEM_setLCx82
            break;

        case 0x83:
            // MEM_setLCx83
            break;

        case 0x88:
            // MEM_setLCx88
            break;

        case 0x89:
            // MEM_setLCx89
            break;

        case 0x8A:
            // MEM_setLCx8A
            break;

        case 0x8B:
            // MEM_setLCx8B
            break;

        case 0xE0:
            // IWM_writeC0E0
            break;

        case 0xE1:
            // IWM_writeC0E1
            break;

        case 0xE2:
            // IWM_writeC0E2
            break;

        case 0xE3:
            // IWM_writeC0E3
            break;

        case 0xE4:
            // IWM_writeC0E4
            break;

        case 0xE5:
            // IWM_writeC0E5
            break;

        case 0xE6:
            // IWM_writeC0E6
            break;

        case 0xE7:
            // IWM_writeC0E7
            break;

        case 0xE8:
            // IWM_writeC0E8
            break;

        case 0xE9:
            // IWM_writeC0E9
            break;

        case 0xEA:
            // IWM_writeC0EA
            break;

        case 0xEB:
            // IWM_writeC0EB
            break;

        case 0xEC:
            // IWM_writeC0EC
            break;

        case 0xED:
            // IWM_writeC0ED
            break;

        case 0xEE:
            // IWM_writeC0EE
            break;

        case 0xEF:
            // IWM_writeC0EF
            break;

        case 0xFF:
            // MEM_ioUnimp         
            break;

        default:
            break;
    }
}

