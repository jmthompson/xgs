/*********************************************************************
 *                                                                   *
 *                     XGS : Apple IIGS Emulator                     *
 *                                                                   *
 *        Written and Copyright (C)1996 by Joshua M. Thompson        *
 *                                                                   *
 *  You are free to distribute this code for non-commercial purposes *
 * I ask only that you notify me of any changes you make to the code *
 *     Commercial use is prohibited without my written permission    *
 *                                                                   *
 *********************************************************************/

/*
 * File: rw.c
 *
 * This is the top-level code for reading and writing bytes from
 * emulator RAM, calling the appropriate fault routines for accesses
 * to special memory areas.
 */

#include "xgs.h"

#include "adb.h"
#include "clock.h"
#include "disks.h"
#include "hardware.h"
#include "iwm.h"
#include "smtport.h"
#include "sound.h"
#include "video.h"

const word32    mem_change_masks[256] = {
    0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001, 0x00000001,
    0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 0x00000002, 
    0x00000004, 0x00000004, 0x00000004, 0x00000004, 0x00000004, 0x00000004, 0x00000004, 0x00000004,
    0x00000008, 0x00000008, 0x00000008, 0x00000008, 0x00000008, 0x00000008, 0x00000008, 0x00000008,
    0x00000010, 0x00000010, 0x00000010, 0x00000010, 0x00000010, 0x00000010, 0x00000010, 0x00000010,
    0x00000020, 0x00000020, 0x00000020, 0x00000020, 0x00000020, 0x00000020, 0x00000020, 0x00000020,
    0x00000040, 0x00000040, 0x00000040, 0x00000040, 0x00000040, 0x00000040, 0x00000040, 0x00000040,
    0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080, 0x00000080,
    0x00000100, 0x00000100, 0x00000100, 0x00000100, 0x00000100, 0x00000100, 0x00000100, 0x00000100,
    0x00000200, 0x00000200, 0x00000200, 0x00000200, 0x00000200, 0x00000200, 0x00000200, 0x00000200,
    0x00000400, 0x00000400, 0x00000400, 0x00000400, 0x00000400, 0x00000400, 0x00000400, 0x00000400,
    0x00000800, 0x00000800, 0x00000800, 0x00000800, 0x00000800, 0x00000800, 0x00000800, 0x00000800,
    0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000, 0x00001000,
    0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x00002000, 0x00002000,
    0x00004000, 0x00004000, 0x00004000, 0x00004000, 0x00004000, 0x00004000, 0x00004000, 0x00004000,
    0x00008000, 0x00008000, 0x00008000, 0x00008000, 0x00008000, 0x00008000, 0x00008000, 0x00008000,
    0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000, 0x00010000,
    0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000, 0x00020000,
    0x00040000, 0x00040000, 0x00040000, 0x00040000, 0x00040000, 0x00040000, 0x00040000, 0x00040000,
    0x00080000, 0x00080000, 0x00080000, 0x00080000, 0x00080000, 0x00080000, 0x00080000, 0x00080000,
    0x00100000, 0x00100000, 0x00100000, 0x00100000, 0x00100000, 0x00100000, 0x00100000, 0x00100000,
    0x00200000, 0x00200000, 0x00200000, 0x00200000, 0x00200000, 0x00200000, 0x00200000, 0x00200000,
    0x00400000, 0x00400000, 0x00400000, 0x00400000, 0x00400000, 0x00400000, 0x00400000, 0x00400000,
    0x00800000, 0x00800000, 0x00800000, 0x00800000, 0x00800000, 0x00800000, 0x00800000, 0x00800000,
    0x01000000, 0x01000000, 0x01000000, 0x01000000, 0x01000000, 0x01000000, 0x01000000, 0x01000000,
    0x02000000, 0x02000000, 0x02000000, 0x02000000, 0x02000000, 0x02000000, 0x02000000, 0x02000000,
    0x04000000, 0x04000000, 0x04000000, 0x04000000, 0x04000000, 0x04000000, 0x04000000, 0x04000000,
    0x08000000, 0x08000000, 0x08000000, 0x08000000, 0x08000000, 0x08000000, 0x08000000, 0x08000000,
    0x10000000, 0x10000000, 0x10000000, 0x10000000, 0x10000000, 0x10000000, 0x10000000, 0x10000000,
    0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000, 0x20000000,
    0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000, 0x40000000,
    0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000, 0x80000000,
};

word32    mem_slowram_changed[512];

/* Dispatch table for reads to page $C0xx I/O space    */

byte (*mem_io_read_dispatch[256])(byte) = {
    ADB_readKey,           /* $C000 */
    MEM_set80store,        /* $C001 */
    MEM_clearAuxRd,        /* $C002 */
    MEM_setAuxRd,          /* $C003 */
    MEM_clearAuxWrt,       /* $C004 */
    MEM_setAuxWrt,         /* $C005 */
    MEM_clearIntCXROM,     /* $C006 */
    MEM_setIntCXROM,       /* $C007 */
    MEM_clearAltZP,        /* $C008 */
    MEM_setAltZP,          /* $C009 */
    MEM_clearSlotC3ROM,    /* $C00A */
    MEM_setSlotC3ROM,      /* $C00B */
    VID_clear80col,        /* $C00C */
    VID_set80col,          /* $C00D */
    VID_clearAltCh,        /* $C00E */
    VID_setAltCh,          /* $C00F */
    ADB_clearKey,          /* $C010 */
    MEM_getLCbank,         /* $C011 */
    MEM_getLCread,         /* $C012 */
    MEM_getAuxRd,          /* $C013 */
    MEM_getAuxWrt,         /* $C014 */
    MEM_getIntCXROM,       /* $C015 */
    MEM_getAltZP,          /* $C016 */
    MEM_getSlotC3ROM,      /* $C017 */
    MEM_get80store,        /* $C018 */
    hardwareInVBL,         /* $C019 */
    VID_getText,           /* $C01A */
    VID_getMixed,          /* $C01B */
    VID_getPage2,          /* $C01C */
    VID_getHires,          /* $C01D */
    VID_getAltCh,          /* $C01E */
    VID_get80col,          /* $C01F */
    MEM_ioUnimp,           /* $C020 */
    MEM_ioUnimp,           /* $C021 */
    VID_getColorReg,       /* $C022 */
    VID_getVGCIntReg,      /* $C023 */
    ADB_readMouse,         /* $C024 */
    ADB_readModifiers,     /* $C025 */
    ADB_readCommand,       /* $C026 */
    ADB_readStatus,        /* $C027 */
    MEM_ioUnimp,           /* $C028 */
    VID_getNewVideo,       /* $C029 */
    MEM_ioUnimp,           /* $C02A */
    MEM_ioUnimp,           /* $C02B */
    MEM_ioUnimp,           /* $C02C */
    MEM_getSlotReg,        /* $C02D */
    VID_getVertCnt,        /* $C02E */
    VID_getHorzCnt,        /* $C02F */
    soundClickSpeaker,     /* $C030 */
    IWM_getDiskReg,        /* $C031 */
    MEM_ioUnimp,           /* $C032 */
    clockGetData,          /* $C033 */
    clockGetControl,       /* $C034 */
    MEM_getShadowReg,      /* $C035 */
    MEM_getCYAReg,         /* $C036 */
    MEM_ioUnimp,           /* $C037 */
    MEM_ioUnimp,           /* $C038 */
    MEM_ioUnimp,           /* $C039 */
    MEM_ioUnimp,           /* $C03A */
    MEM_ioUnimp,           /* $C03B */
    SND_readSoundCtl,      /* $C03C */
    SND_readSoundData,     /* $C03D */
    SND_readSoundAddrL,    /* $C03E */
    SND_readSoundAddrH,    /* $C03F */
    MEM_ioUnimp,           /* $C040 */
    MEM_getIntEnReg,       /* $C041 */
    MEM_ioUnimp,           /* $C042 */
    MEM_ioUnimp,           /* $C043 */
    ADB_readM2MouseX,      /* $C044 */
    ADB_readM2MouseY,      /* $C045 */
    MEM_getDiagType,       /* $C046 */
    MEM_ioUnimp,           /* $C047 */
    MEM_ioUnimp,           /* $C048 */
    MEM_ioUnimp,           /* $C049 */
    MEM_ioUnimp,           /* $C04A */
    MEM_ioUnimp,           /* $C04B */
    MEM_ioUnimp,           /* $C04C */
    MEM_ioUnimp,           /* $C04D */
    MEM_ioUnimp,           /* $C04E */
    MEM_ioUnimp,           /* $C04F */
    VID_clearText,         /* $C050 */
    VID_setText,           /* $C051 */
    VID_clearMixed,        /* $C052 */
    VID_setMixed,          /* $C053 */
    VID_clearPage2,        /* $C054 */
    VID_setPage2,          /* $C055 */
    VID_clearHires,        /* $C056 */
    VID_setHires,          /* $C057 */
    MEM_ioUnimp,           /* $C058 */
    MEM_ioUnimp,           /* $C059 */
    MEM_ioUnimp,           /* $C05A */
    MEM_ioUnimp,           /* $C05B */
    MEM_ioUnimp,           /* $C05C */
    MEM_ioUnimp,           /* $C05D */
    VID_setDblRes,         /* $C05E */
    VID_clearDblRes,       /* $C05F */
    MEM_ioUnimp,           /* $C060 */
    ADB_readCommandKey,    /* $C061 */
    ADB_readOptionKey,     /* $C062 */
    MEM_ioUnimp,           /* $C063 */
    ADB_readPaddle0,       /* $C064 */
    ADB_readPaddle1,       /* $C065 */
    ADB_readPaddle2,       /* $C066 */
    ADB_readPaddle3,       /* $C067 */
    MEM_getStateReg,       /* $C068 */
    MEM_ioUnimp,           /* $C069 */
    MEM_ioUnimp,           /* $C06A */
    MEM_ioUnimp,           /* $C06B */
    MEM_ioUnimp,           /* $C06C */
    MEM_ioUnimp,           /* $C06D */
    MEM_ioUnimp,           /* $C06E */
    MEM_ioUnimp,           /* $C06F */
    ADB_triggerPaddles,    /* $C070 */
    MEM_getC071,           /* $C071 */
    MEM_getC072,           /* $C072 */
    MEM_getC073,           /* $C073 */
    MEM_getC074,           /* $C074 */
    MEM_getC075,           /* $C075 */
    MEM_getC076,           /* $C076 */
    MEM_getC077,           /* $C077 */
    MEM_getC078,           /* $C078 */
    MEM_getC079,           /* $C079 */
    MEM_getC07A,           /* $C07A */
    MEM_getC07B,           /* $C07B */
    MEM_getC07C,           /* $C07C */
    MEM_getC07D,           /* $C07D */
    MEM_getC07E,           /* $C07E */
    MEM_getC07F,           /* $C07F */
    MEM_setLCx80,          /* $C080 */
    MEM_setLCx81,          /* $C081 */
    MEM_setLCx82,          /* $C082 */
    MEM_setLCx83,          /* $C083 */
    MEM_ioUnimp,           /* $C084 */
    MEM_ioUnimp,           /* $C085 */
    MEM_ioUnimp,           /* $C086 */
    MEM_ioUnimp,           /* $C087 */
    MEM_setLCx88,          /* $C088 */
    MEM_setLCx89,          /* $C089 */
    MEM_setLCx8A,          /* $C08A */
    MEM_setLCx8B,          /* $C08B */
    MEM_ioUnimp,           /* $C08C */
    MEM_ioUnimp,           /* $C08D */
    MEM_ioUnimp,           /* $C08E */
    MEM_ioUnimp,           /* $C08F */
    MEM_ioUnimp,           /* $C090 */
    MEM_ioUnimp,           /* $C091 */
    MEM_ioUnimp,           /* $C092 */
    MEM_ioUnimp,           /* $C093 */
    MEM_ioUnimp,           /* $C094 */
    MEM_ioUnimp,           /* $C095 */
    MEM_ioUnimp,           /* $C096 */
    MEM_ioUnimp,           /* $C097 */
    MEM_ioUnimp,           /* $C098 */
    MEM_ioUnimp,           /* $C099 */
    MEM_ioUnimp,           /* $C09A */
    MEM_ioUnimp,           /* $C09B */
    MEM_ioUnimp,           /* $C09C */
    MEM_ioUnimp,           /* $C09D */
    MEM_ioUnimp,           /* $C09E */
    MEM_ioUnimp,           /* $C09F */
    MEM_ioUnimp,           /* $C0A0 */
    MEM_ioUnimp,           /* $C0A1 */
    MEM_ioUnimp,           /* $C0A2 */
    MEM_ioUnimp,           /* $C0A3 */
    MEM_ioUnimp,           /* $C0A4 */
    MEM_ioUnimp,           /* $C0A5 */
    MEM_ioUnimp,           /* $C0A6 */
    MEM_ioUnimp,           /* $C0A7 */
    MEM_ioUnimp,           /* $C0A8 */
    MEM_ioUnimp,           /* $C0A9 */
    MEM_ioUnimp,           /* $C0AA */
    MEM_ioUnimp,           /* $C0AB */
    MEM_ioUnimp,           /* $C0AC */
    MEM_ioUnimp,           /* $C0AD */
    MEM_ioUnimp,           /* $C0AE */
    MEM_ioUnimp,           /* $C0AF */
    MEM_ioUnimp,           /* $C0B0 */
    MEM_ioUnimp,           /* $C0B1 */
    MEM_ioUnimp,           /* $C0B2 */
    MEM_ioUnimp,           /* $C0B3 */
    MEM_ioUnimp,           /* $C0B4 */
    MEM_ioUnimp,           /* $C0B5 */
    MEM_ioUnimp,           /* $C0B6 */
    MEM_ioUnimp,           /* $C0B7 */
    MEM_ioUnimp,           /* $C0B8 */
    MEM_ioUnimp,           /* $C0B9 */
    MEM_ioUnimp,           /* $C0BA */
    MEM_ioUnimp,           /* $C0BB */
    MEM_ioUnimp,           /* $C0BC */
    MEM_ioUnimp,           /* $C0BD */
    MEM_ioUnimp,           /* $C0BE */
    MEM_ioUnimp,           /* $C0BF */
    MEM_ioUnimp,           /* $C0C0 */
    MEM_ioUnimp,           /* $C0C1 */
    MEM_ioUnimp,           /* $C0C2 */
    MEM_ioUnimp,           /* $C0C3 */
    MEM_ioUnimp,           /* $C0C4 */
    MEM_ioUnimp,           /* $C0C5 */
    MEM_ioUnimp,           /* $C0C6 */
    MEM_ioUnimp,           /* $C0C7 */
    MEM_ioUnimp,           /* $C0C8 */
    MEM_ioUnimp,           /* $C0C9 */
    MEM_ioUnimp,           /* $C0CA */
    MEM_ioUnimp,           /* $C0CB */
    MEM_ioUnimp,           /* $C0CC */
    MEM_ioUnimp,           /* $C0CD */
    MEM_ioUnimp,           /* $C0CE */
    MEM_ioUnimp,           /* $C0CF */
    MEM_ioUnimp,           /* $C0D0 */
    MEM_ioUnimp,           /* $C0D1 */
    MEM_ioUnimp,           /* $C0D2 */
    MEM_ioUnimp,           /* $C0D3 */
    MEM_ioUnimp,           /* $C0D4 */
    MEM_ioUnimp,           /* $C0D5 */
    MEM_ioUnimp,           /* $C0D6 */
    MEM_ioUnimp,           /* $C0D7 */
    MEM_ioUnimp,           /* $C0D8 */
    MEM_ioUnimp,           /* $C0D9 */
    MEM_ioUnimp,           /* $C0DA */
    MEM_ioUnimp,           /* $C0DB */
    MEM_ioUnimp,           /* $C0DC */
    MEM_ioUnimp,           /* $C0DD */
    MEM_ioUnimp,           /* $C0DE */
    MEM_ioUnimp,           /* $C0DF */
    IWM_readC0E0,          /* $C0E0 */
    IWM_readC0E1,          /* $C0E1 */
    IWM_readC0E2,          /* $C0E2 */
    IWM_readC0E3,          /* $C0E3 */
    IWM_readC0E4,          /* $C0E4 */
    IWM_readC0E5,          /* $C0E5 */
    IWM_readC0E6,          /* $C0E6 */
    IWM_readC0E7,          /* $C0E7 */
    IWM_readC0E8,          /* $C0E8 */
    IWM_readC0E9,          /* $C0E9 */
    IWM_readC0EA,          /* $C0EA */
    IWM_readC0EB,          /* $C0EB */
    IWM_readC0EC,          /* $C0EC */
    IWM_readC0ED,          /* $C0ED */
    IWM_readC0EE,          /* $C0EE */
    IWM_readC0EF,          /* $C0EF */
    MEM_ioUnimp,           /* $C0F0 */
    MEM_ioUnimp,           /* $C0F1 */
    MEM_ioUnimp,           /* $C0F2 */
    MEM_ioUnimp,           /* $C0F3 */
    MEM_ioUnimp,           /* $C0F4 */
    MEM_ioUnimp,           /* $C0F5 */
    MEM_ioUnimp,           /* $C0F6 */
    MEM_ioUnimp,           /* $C0F7 */
    MEM_ioUnimp,           /* $C0F8 */
    MEM_ioUnimp,           /* $C0F9 */
    MEM_ioUnimp,           /* $C0FA */
    MEM_ioUnimp,           /* $C0FB */
    MEM_ioUnimp,           /* $C0FC */
    MEM_ioUnimp,           /* $C0FD */
    MEM_ioUnimp,           /* $C0FE */
    MEM_ioUnimp            /* $C0FF */
};

/* Dispatch table for writes to page $C0xx I/O space    */

byte (*mem_io_write_dispatch[256])(byte) = {
    MEM_clear80store,    /* $C000 */
    MEM_set80store,      /* $C001 */
    MEM_clearAuxRd,      /* $C002 */
    MEM_setAuxRd,        /* $C003 */
    MEM_clearAuxWrt,     /* $C004 */
    MEM_setAuxWrt,       /* $C005 */
    MEM_clearIntCXROM,   /* $C006 */
    MEM_setIntCXROM,     /* $C007 */
    MEM_clearAltZP,      /* $C008 */
    MEM_setAltZP,        /* $C009 */
    MEM_clearSlotC3ROM,  /* $C00A */
    MEM_setSlotC3ROM,    /* $C00B */
    VID_clear80col,      /* $C00C */
    VID_set80col,        /* $C00D */
    VID_clearAltCh,      /* $C00E */
    VID_setAltCh,        /* $C00F */
    ADB_clearKey,        /* $C010 */
    MEM_ioUnimp,         /* $C011 */
    MEM_ioUnimp,         /* $C012 */
    MEM_ioUnimp,         /* $C013 */
    MEM_ioUnimp,         /* $C014 */
    MEM_ioUnimp,         /* $C015 */
    MEM_ioUnimp,         /* $C016 */
    MEM_ioUnimp,         /* $C017 */
    MEM_ioUnimp,         /* $C018 */
    MEM_ioUnimp,         /* $C019 */
    MEM_ioUnimp,         /* $C01A */
    MEM_ioUnimp,         /* $C01B */
    MEM_ioUnimp,         /* $C01C */
    MEM_ioUnimp,         /* $C01D */
    MEM_ioUnimp,         /* $C01E */
    MEM_ioUnimp,         /* $C01F */
    MEM_ioUnimp,         /* $C020 */
    MEM_ioUnimp,         /* $C021 */
    VID_setColorReg,     /* $C022 */
    VID_setVGCIntReg,    /* $C023 */
    MEM_ioUnimp,         /* $C024 */
    MEM_ioUnimp,         /* $C025 */
    ADB_setCommand,      /* $C026 */
    ADB_setStatus,       /* $C027 */
    MEM_ioUnimp,         /* $C028 */
    VID_setNewVideo,     /* $C029 */
    MEM_ioUnimp,         /* $C02A */
    MEM_ioUnimp,         /* $C02B */
    MEM_ioUnimp,         /* $C02C */
    MEM_setSlotReg,      /* $C02D */
    MEM_ioUnimp,         /* $C02E */
    MEM_ioUnimp,         /* $C02F */
    soundClickSpeaker,   /* $C030 */
    IWM_setDiskReg,      /* $C031 */
    VID_clearVGCInt,     /* $C032 */
    clockSetData,        /* $C033 */
    clockSetControl,     /* $C034 */
    MEM_setShadowReg,    /* $C035 */
    MEM_setCYAReg,       /* $C036 */
    MEM_ioUnimp,         /* $C037 */
    MEM_ioUnimp,         /* $C038 */
    MEM_ioUnimp,         /* $C039 */
    MEM_ioUnimp,         /* $C03A */
    MEM_ioUnimp,         /* $C03B */
    SND_writeSoundCtl,   /* $C03C */
    SND_writeSoundData,  /* $C03D */
    SND_writeSoundAddrL, /* $C03E */
    SND_writeSoundAddrH, /* $C03F */
    MEM_ioUnimp,         /* $C040 */
    MEM_setIntEnReg,     /* $C041 */
    MEM_ioUnimp,         /* $C042 */
    MEM_ioUnimp,         /* $C043 */
    MEM_ioUnimp,         /* $C044 */
    MEM_ioUnimp,         /* $C045 */
    MEM_ioUnimp,         /* $C046 */
    MEM_setVBLClear,     /* $C047 */
    MEM_ioUnimp,         /* $C048 */
    MEM_ioUnimp,         /* $C049 */
    MEM_ioUnimp,         /* $C04A */
    MEM_ioUnimp,         /* $C04B */
    MEM_ioUnimp,         /* $C04C */
    MEM_ioUnimp,         /* $C04D */
    MEM_ioUnimp,         /* $C04E */
    MEM_ioUnimp,         /* $C04F */
    VID_clearText,       /* $C050 */
    VID_setText,         /* $C051 */
    VID_clearMixed,      /* $C052 */
    VID_setMixed,        /* $C053 */
    VID_clearPage2,      /* $C054 */
    VID_setPage2,        /* $C055 */
    VID_clearHires,      /* $C056 */
    VID_setHires,        /* $C057 */
    MEM_ioUnimp,         /* $C058 */
    MEM_ioUnimp,         /* $C059 */
    MEM_ioUnimp,         /* $C05A */
    MEM_ioUnimp,         /* $C05B */
    MEM_ioUnimp,         /* $C05C */
    MEM_ioUnimp,         /* $C05D */
    VID_setDblRes,       /* $C05E */
    VID_clearDblRes,     /* $C05F */
    MEM_ioUnimp,         /* $C060 */
    MEM_ioUnimp,         /* $C061 */
    MEM_ioUnimp,         /* $C062 */
    MEM_ioUnimp,         /* $C063 */
    MEM_ioUnimp,         /* $C064 */
    MEM_ioUnimp,         /* $C065 */
    MEM_ioUnimp,         /* $C066 */
    MEM_ioUnimp,         /* $C067 */
    MEM_setStateReg,     /* $C068 */
    MEM_ioUnimp,         /* $C069 */
    MEM_ioUnimp,         /* $C06A */
    MEM_ioUnimp,         /* $C06B */
    MEM_ioUnimp,         /* $C06C */
    MEM_ioUnimp,         /* $C06D */
    MEM_ioUnimp,         /* $C06E */
    MEM_ioUnimp,         /* $C06F */
    ADB_triggerPaddles,  /* $C070 */
    MEM_ioUnimp,         /* $C071 */
    MEM_ioUnimp,         /* $C072 */
    MEM_ioUnimp,         /* $C073 */
    MEM_ioUnimp,         /* $C074 */
    MEM_ioUnimp,         /* $C075 */
    MEM_ioUnimp,         /* $C076 */
    MEM_ioUnimp,         /* $C077 */
    MEM_ioUnimp,         /* $C078 */
    MEM_ioUnimp,         /* $C079 */
    MEM_ioUnimp,         /* $C07A */
    MEM_ioUnimp,         /* $C07B */
    MEM_ioUnimp,         /* $C07C */
    MEM_ioUnimp,         /* $C07D */
    MEM_ioUnimp,         /* $C07E */
    MEM_ioUnimp,         /* $C07F */
    MEM_setLCx80,        /* $C080 */
    MEM_setLCx81,        /* $C081 */
    MEM_setLCx82,        /* $C082 */
    MEM_setLCx83,        /* $C083 */
    MEM_ioUnimp,         /* $C084 */
    MEM_ioUnimp,         /* $C085 */
    MEM_ioUnimp,         /* $C086 */
    MEM_ioUnimp,         /* $C087 */
    MEM_setLCx88,        /* $C088 */
    MEM_setLCx89,        /* $C089 */
    MEM_setLCx8A,        /* $C08A */
    MEM_setLCx8B,        /* $C08B */
    MEM_ioUnimp,         /* $C08C */
    MEM_ioUnimp,         /* $C08D */
    MEM_ioUnimp,         /* $C08E */
    MEM_ioUnimp,         /* $C08F */
    MEM_ioUnimp,         /* $C090 */
    MEM_ioUnimp,         /* $C091 */
    MEM_ioUnimp,         /* $C092 */
    MEM_ioUnimp,         /* $C093 */
    MEM_ioUnimp,         /* $C094 */
    MEM_ioUnimp,         /* $C095 */
    MEM_ioUnimp,         /* $C096 */
    MEM_ioUnimp,         /* $C097 */
    MEM_ioUnimp,         /* $C098 */
    MEM_ioUnimp,         /* $C099 */
    MEM_ioUnimp,         /* $C09A */
    MEM_ioUnimp,         /* $C09B */
    MEM_ioUnimp,         /* $C09C */
    MEM_ioUnimp,         /* $C09D */
    MEM_ioUnimp,         /* $C09E */
    MEM_ioUnimp,         /* $C09F */
    MEM_ioUnimp,         /* $C0A0 */
    MEM_ioUnimp,         /* $C0A1 */
    MEM_ioUnimp,         /* $C0A2 */
    MEM_ioUnimp,         /* $C0A3 */
    MEM_ioUnimp,         /* $C0A4 */
    MEM_ioUnimp,         /* $C0A5 */
    MEM_ioUnimp,         /* $C0A6 */
    MEM_ioUnimp,         /* $C0A7 */
    MEM_ioUnimp,         /* $C0A8 */
    MEM_ioUnimp,         /* $C0A9 */
    MEM_ioUnimp,         /* $C0AA */
    MEM_ioUnimp,         /* $C0AB */
    MEM_ioUnimp,         /* $C0AC */
    MEM_ioUnimp,         /* $C0AD */
    MEM_ioUnimp,         /* $C0AE */
    MEM_ioUnimp,         /* $C0AF */
    MEM_ioUnimp,         /* $C0B0 */
    MEM_ioUnimp,         /* $C0B1 */
    MEM_ioUnimp,         /* $C0B2 */
    MEM_ioUnimp,         /* $C0B3 */
    MEM_ioUnimp,         /* $C0B4 */
    MEM_ioUnimp,         /* $C0B5 */
    MEM_ioUnimp,         /* $C0B6 */
    MEM_ioUnimp,         /* $C0B7 */
    MEM_ioUnimp,         /* $C0B8 */
    MEM_ioUnimp,         /* $C0B9 */
    MEM_ioUnimp,         /* $C0BA */
    MEM_ioUnimp,         /* $C0BB */
    MEM_ioUnimp,         /* $C0BC */
    MEM_ioUnimp,         /* $C0BD */
    MEM_ioUnimp,         /* $C0BE */
    MEM_ioUnimp,         /* $C0BF */
    MEM_ioUnimp,         /* $C0C0 */
    MEM_ioUnimp,         /* $C0C1 */
    MEM_ioUnimp,         /* $C0C2 */
    MEM_ioUnimp,         /* $C0C3 */
    MEM_ioUnimp,         /* $C0C4 */
    MEM_ioUnimp,         /* $C0C5 */
    MEM_ioUnimp,         /* $C0C6 */
    MEM_ioUnimp,         /* $C0C7 */
    MEM_ioUnimp,         /* $C0C8 */
    MEM_ioUnimp,         /* $C0C9 */
    MEM_ioUnimp,         /* $C0CA */
    MEM_ioUnimp,         /* $C0CB */
    MEM_ioUnimp,         /* $C0CC */
    MEM_ioUnimp,         /* $C0CD */
    MEM_ioUnimp,         /* $C0CE */
    MEM_ioUnimp,         /* $C0CF */
    MEM_ioUnimp,         /* $C0D0 */
    MEM_ioUnimp,         /* $C0D1 */
    MEM_ioUnimp,         /* $C0D2 */
    MEM_ioUnimp,         /* $C0D3 */
    MEM_ioUnimp,         /* $C0D4 */
    MEM_ioUnimp,         /* $C0D5 */
    MEM_ioUnimp,         /* $C0D6 */
    MEM_ioUnimp,         /* $C0D7 */
    MEM_ioUnimp,         /* $C0D8 */
    MEM_ioUnimp,         /* $C0D9 */
    MEM_ioUnimp,         /* $C0DA */
    MEM_ioUnimp,         /* $C0DB */
    MEM_ioUnimp,         /* $C0DC */
    MEM_ioUnimp,         /* $C0DD */
    MEM_ioUnimp,         /* $C0DE */
    MEM_ioUnimp,         /* $C0DF */
    IWM_writeC0E0,       /* $C0E0 */
    IWM_writeC0E1,       /* $C0E1 */
    IWM_writeC0E2,       /* $C0E2 */
    IWM_writeC0E3,       /* $C0E3 */
    IWM_writeC0E4,       /* $C0E4 */
    IWM_writeC0E5,       /* $C0E5 */
    IWM_writeC0E6,       /* $C0E6 */
    IWM_writeC0E7,       /* $C0E7 */
    IWM_writeC0E8,       /* $C0E8 */
    IWM_writeC0E9,       /* $C0E9 */
    IWM_writeC0EA,       /* $C0EA */
    IWM_writeC0EB,       /* $C0EB */
    IWM_writeC0EC,       /* $C0EC */
    IWM_writeC0ED,       /* $C0ED */
    IWM_writeC0EE,       /* $C0EE */
    IWM_writeC0EF,       /* $C0EF */
    MEM_ioUnimp,         /* $C0F0 */
    MEM_ioUnimp,         /* $C0F1 */
    MEM_ioUnimp,         /* $C0F2 */
    MEM_ioUnimp,         /* $C0F3 */
    MEM_ioUnimp,         /* $C0F4 */
    MEM_ioUnimp,         /* $C0F5 */
    MEM_ioUnimp,         /* $C0F6 */
    MEM_ioUnimp,         /* $C0F7 */
    MEM_ioUnimp,         /* $C0F8 */
    MEM_ioUnimp,         /* $C0F9 */
    MEM_ioUnimp,         /* $C0FA */
    MEM_ioUnimp,         /* $C0FB */
    MEM_ioUnimp,         /* $C0FC */
    MEM_ioUnimp,         /* $C0FD */
    MEM_ioUnimp,         /* $C0FE */
    MEM_ioUnimp          /* $C0FF */
};

byte MEM_ioUnimp(byte val)
{
    return 0;
}

void MEM_writeMem2(word32 addr, byte val)
{
    int    page;
    byte    offset;
    word16    flags;
    
    page = (addr >> 8);
    offset = (byte) addr;
    flags = mem_pages[page].writeFlags;
    if (flags & MEM_FLAG_INVALID) return;
    if (flags & MEM_FLAG_IO) {
        (*mem_io_write_dispatch[offset])(val);
        return;
    }
    if (flags & MEM_FLAG_SPECIAL) {
        if (*(mem_pages[page].writePtr + offset) != val) {
            mem_slowram_changed[page & 0x1FF] |= mem_change_masks[offset];
            *(mem_pages[page].writePtr + offset) = val;
        }
        return;
    }
    *(mem_pages[page].writePtr + offset) = val;
    if (flags & MEM_FLAG_SHADOW_E0) {
        page = (page & 0xFF) + 0xE000;
        if (*(mem_pages[page].writePtr + offset) != val) {
            mem_slowram_changed[page & 0x1FF] |= mem_change_masks[offset];
            *(mem_pages[page].writePtr + offset) = val;
        }
    } else if (flags & MEM_FLAG_SHADOW_E1) {
        page = (page & 0xFF) + 0xE100;
        if (*(mem_pages[page].writePtr + offset) != val) {
            mem_slowram_changed[page & 0x1FF] |= mem_change_masks[offset];
            *(mem_pages[page].writePtr + offset) = val;
        }
    }
}
