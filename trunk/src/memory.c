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
 * File: memory.c
 *
 * Memory management routines.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef EMX_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif 	/* end of #ifdef EMX_HAVE_SYS_TYPES_H */
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "adb.h"
#include "disks.h"
#include "emul.h"
#include "iwm.h"
#include "smtport.h"
#include "video.h"

int	mem_ramsize;

int	mem_shadow_text;
int	mem_shadow_hires1;
int	mem_shadow_hires2;
int	mem_shadow_super;
int	mem_shadow_aux;
int	mem_shadow_lc;

int	mem_slot_reg[8];

int	mem_lcbank2;
int	mem_lcread;
int	mem_lcwrite;
int	mem_lcsecond;

int	mem_auxrd;
int	mem_auxwrt;
int	mem_altzp;
int	mem_80store;

int	mem_intcxrom;
int	mem_slotc3rom;
int	mem_rombank;

int	mem_diagtype;

int	mem_rom03;

byte	*slow_memory;
byte	*fast_memory;
byte	*rom_memory;
byte	*save_memory;
byte	*c07x_memory;

byte	junk_memory[256];

mem_pagestruct	mem_pages[65536];

int MEM_init(void)
{
	int		i,rom_base;
	char		*path;
	FILE		*fp;
	struct stat	stats;

	if (mem_ramsize < 1) mem_ramsize = 1;
	if (mem_ramsize > 8) mem_ramsize = 8;

	printf("    - Allocating 128 KB for slow RAM: ");
	slow_memory = malloc(2*65536);
	if (slow_memory == NULL) {
		printf("Failed\n");
		return 1;
	}
	memset(slow_memory, 0, 2*65536);
	printf("Done\n");

	printf("    - Allocating %d MB for fast RAM: ",mem_ramsize);
	fast_memory = malloc(mem_ramsize * 1048576);
	if (fast_memory == NULL) {
		printf("Failed\n");
		return 1;
	}
	memset(fast_memory, 0, mem_ramsize * 1048576);
	printf("Done\n");

	printf("    - Allocating 128 KB save RAM: ");
	save_memory = malloc(2*65536);
	if (save_memory == NULL) {
		printf("Failed\n");
		return 1;
	}
	memset(save_memory, 0, 2*65536);
	printf("Done\n");

	printf("    - Loading ROM image: ");
	path = EMUL_expandPath(ROM_FILE);
	if (stat(path, &stats)) {
		printf("Failed\n");
		return 2;
	}
	if (stats.st_size == 131072) {
		mem_rom03 = 0;
	} else if (stats.st_size == 262144) {
		mem_rom03 = 1;
	} else {
		printf("Failed\n");
		return 2;
	}
	rom_memory = malloc(stats.st_size);
	if (rom_memory == NULL) {
		printf("Failed\n");
		return 2;
	}
	
	if ((fp = fopen(path,"rb")) == NULL) {
		printf("Failed\n");
		return 2;
	}
	if (fread(rom_memory,1,stats.st_size,fp) != stats.st_size) {
		printf("Failed\n");
		return 2;
	}
	fclose(fp);
	printf("Done\n");

	if (mem_rom03) {
		printf("    - ROM 03 emulation mode\n");
		rom_base = 0xFC00;
		c07x_memory = rom_memory + 0x03C070;
	} else {
		printf("    - ROM 01 emulation mode\n");
		rom_base = 0xFE00;
		c07x_memory = rom_memory + 0x01C070;
	}

	/* Initialize all pages */

	printf("\nInitializing emulator memory\n");
	for (i = 0x0000 ; i < mem_ramsize * 4096 ; i++) {
		mem_pages[i].readPtr = mem_pages[i].writePtr = fast_memory + (i * 256);
		mem_pages[i].readFlags = mem_pages[i].writeFlags = 0;
	}
	for (i = mem_ramsize * 4096 ; i < 0xE000 ; i++) {
		mem_pages[i].readPtr = mem_pages[i].writePtr = junk_memory;
		mem_pages[i].readFlags = mem_pages[i].writeFlags = MEM_FLAG_INVALID;
	}
	for (i = 0xE000 ; i < 0xE200 ; i++) {
		mem_pages[i].readPtr = mem_pages[i].writePtr = slow_memory + ((i-0xE000)*256);
		mem_pages[i].readFlags = mem_pages[i].writeFlags = MEM_FLAG_SPECIAL;
	}

	for (i = 0xE200 ; i < rom_base ; i++) {
		mem_pages[i].readPtr = mem_pages[i].writePtr = junk_memory;
		mem_pages[i].readFlags = mem_pages[i].writeFlags = MEM_FLAG_INVALID;
	}
	for (i = rom_base ; i < 0x10000 ; i++) {
		mem_pages[i].readPtr = rom_memory + ((i - rom_base) * 256);
		mem_pages[i].writePtr = junk_memory;
		mem_pages[i].readFlags = 0;
		mem_pages[i].writeFlags = MEM_FLAG_INVALID;
	}

	for (i = 0xE800 ; i < 0xEA00 ; i++) {
		mem_pages[i].readPtr = mem_pages[i].writePtr = save_memory + ((i-0xE800) * 256);
		mem_pages[i].readFlags = mem_pages[i].writeFlags = 0;
	}

	return 0;
}

void MEM_update()
{
}

/* Memory manager reset routine. Here we reset the memory soft swithches to a	*/
/* reasonable power-up/reset state, and then rebuild the mem_pages[] array.	*/

void MEM_reset()
{
	int	i;

	mem_80store = 0;
	mem_auxrd = 0;
	mem_auxwrt = 0;
	mem_altzp = 0;

	mem_lcbank2 = 0;
	mem_lcread = 0;
	mem_lcwrite = 0;
	mem_lcsecond = 0;

	mem_shadow_text = 1;
	mem_shadow_hires1 = 1;
	mem_shadow_hires2 = 1;
	mem_shadow_super = 0;
	mem_shadow_aux = 1;
	mem_shadow_lc = 1;

	for (i = 0 ; i < 5 ; i++) mem_slot_reg[i] = 0;
	mem_slot_reg[7] = 1;

	MEM_rebuildMainMem();
	MEM_rebuildAltZpMem();
}

void MEM_shutdown()
{
	printf("\nShutting down emulator memory\n");
	if (slow_memory) free(save_memory);
	if (slow_memory) free(fast_memory);
	if (slow_memory) free(slow_memory);
	if (slow_memory) free(rom_memory);
}

void MEM_rebuildMainMem()
{
	int	i,offset1,offset2;

	offset1 = mem_auxrd? 0x10000 : 0;
	offset2 = mem_auxwrt? 0x10000 : 0;

	for (i = 0x0002 ; i < 0x0004 ; i++) {
		mem_pages[i].readPtr = fast_memory + (i*256) + offset1;
		mem_pages[i].writePtr = fast_memory + (i*256) + offset2;
	}

	if (mem_80store) {
		offset1 = offset2 = vid_page2? 0x10000 : 0;
	}

	for (i = 0x0004 ; i < 0x0008 ; i++) {
		mem_pages[i].readPtr = fast_memory + (i*256) + offset1;
		mem_pages[i].writePtr = fast_memory + (i*256) + offset2;
	}

	offset1 = mem_auxrd? 0x10000 : 0;
	offset2 = mem_auxwrt? 0x10000 : 0;

	for (i = 0x0008 ; i < 0x0020 ; i++) {
		mem_pages[i].readPtr = fast_memory + (i*256) + offset1;
		mem_pages[i].writePtr = fast_memory + (i*256) + offset2;
	}

	if (mem_80store && vid_hires) {
		offset1 = offset2 = vid_page2? 0x10000 : 0;
	}

	for (i = 0x0020 ; i < 0x0040 ; i++) {
		mem_pages[i].readPtr = fast_memory + (i*256) + offset1;
		mem_pages[i].writePtr = fast_memory + (i*256) + offset2;
	}

	offset1 = mem_auxrd? 0x10000 : 0;
	offset2 = mem_auxwrt? 0x10000 : 0;

	for (i = 0x0040 ; i < 0x00C0 ; i++) {
		mem_pages[i].readPtr = fast_memory + (i*256) + offset1;
		mem_pages[i].writePtr = fast_memory + (i*256) + offset2;
	}
	MEM_rebuildShadowMem();
}

void MEM_rebuildAltZpMem()
{
	int	i,offset;

	offset = mem_altzp? 0x10000 : 0;

	for (i = 0x0000 ; i < 0x0002 ; i++) {
		mem_pages[i].readPtr = mem_pages[i].writePtr = fast_memory + (i * 256) + offset;
	}
	MEM_rebuildLangCardMem();
}

void MEM_rebuildLangCardMem()
{
	int	i,offset1,offset2;

	MEM_buildLanguageCard(0xE000,0,slow_memory);
	MEM_buildLanguageCard(0xE100,0,slow_memory+0x10000);
	offset1 = mem_altzp? 0x10000 : 0;
	if (mem_shadow_lc) {
		MEM_buildLanguageCard(0x0000,offset1,fast_memory);
		MEM_buildLanguageCard(0x0100,0x10000,fast_memory);
	} else {
		offset1 = mem_auxrd? 0x10000 : 0;
		offset2 = mem_auxwrt? 0x10000 : 0;
		for (i = 0x00C0 ; i < 0x0100 ; i++) {
			mem_pages[i].readPtr = fast_memory + (i*256) + offset1;
			mem_pages[i].writePtr = fast_memory + (i*256) + offset2;
		}
		for (i = 0x01C0 ; i < 0x0200 ; i++) {
			mem_pages[i].readPtr = fast_memory + (i*256);
			mem_pages[i].writePtr = fast_memory + (i*256);
		}
	}
}

void MEM_rebuildShadowMem()
{
	int	i,flag,flag2;

	if (mem_80store) {
		flag = vid_page2? MEM_FLAG_SHADOW_E1 : MEM_FLAG_SHADOW_E0;
	} else {
		flag = mem_auxwrt? MEM_FLAG_SHADOW_E1 : MEM_FLAG_SHADOW_E0;
	}

	for (i = 0x0004 ; i < 0x0008 ; i++) {
		mem_pages[i].writeFlags = mem_shadow_text? flag : 0;
	}

	if (mem_80store && vid_hires) {
		flag = vid_page2? MEM_FLAG_SHADOW_E1 : MEM_FLAG_SHADOW_E0;
	} else {
		flag = mem_auxwrt? MEM_FLAG_SHADOW_E1 : MEM_FLAG_SHADOW_E0;
	}

	if (flag == MEM_FLAG_SHADOW_E1) {
		flag2 = (mem_shadow_hires1 && mem_shadow_aux) || mem_shadow_super;
	} else {
		flag2 = mem_shadow_hires1;
	}

	for (i = 0x0020 ; i < 0x0040 ; i++) {
		mem_pages[i].writeFlags = flag2? flag : 0;
	}

	flag = mem_auxwrt? MEM_FLAG_SHADOW_E1 : MEM_FLAG_SHADOW_E0;

	if (flag == MEM_FLAG_SHADOW_E1) {
		flag2 = (mem_shadow_hires2 && mem_shadow_aux) || mem_shadow_super;
	} else {
		flag2 = mem_shadow_hires2;
	}

	for (i = 0x0040 ; i < 0x0060 ; i++) {
		mem_pages[i].writeFlags = flag2? flag : 0;
	}

	if (mem_shadow_super && mem_auxwrt) {
		flag = MEM_FLAG_SHADOW_E1;
	} else {
		flag = 0;
	}

	for (i = 0x0060 ; i < 0x00A0 ; i++) {
		mem_pages[i].writeFlags = flag;
	}

	for (i = 0x0104 ; i < 0x0108 ; i++) {
		mem_pages[i].writeFlags = mem_shadow_text? MEM_FLAG_SHADOW_E1 : 0;
	}

	flag = (mem_shadow_hires1 && mem_shadow_aux) || mem_shadow_super;

	for (i = 0x0120 ; i < 0x0140 ; i++) {
		mem_pages[i].writeFlags = flag? MEM_FLAG_SHADOW_E1 : 0;
	}

	flag = (mem_shadow_hires2 && mem_shadow_aux) || mem_shadow_super;

	for (i = 0x0140 ; i < 0x0160 ; i++) {
		mem_pages[i].writeFlags = flag? MEM_FLAG_SHADOW_E1 : 0;
	}

	for (i = 0x0160 ; i < 0x01A0 ; i++) {
		mem_pages[i].writeFlags = mem_shadow_super? MEM_FLAG_SHADOW_E1 : 0;
	}
}

/* This routine builds a language card in the given bank.  This	*/
/* means building all mapping info from $C000 up to $FFFF.	*/

void MEM_buildLanguageCard(int bank,int offset,byte *ram)
{
	int	i,lcbank,rom_lastbank;

	rom_lastbank = mem_rom03? 0x030000 : 0x010000;

	mem_pages[bank+0xC0].readFlags = MEM_FLAG_IO;
	mem_pages[bank+0xC0].writeFlags = MEM_FLAG_IO;

	for (i = 0xC1 ; i < 0xC7 ; i++) {
		if (mem_slot_reg[i - 0xC0]) {
			mem_pages[bank+i].readPtr = junk_memory;
			mem_pages[bank+i].readFlags = MEM_FLAG_INVALID;
		} else {
			mem_pages[bank+i].readPtr = rom_memory + rom_lastbank + (i * 256);
			mem_pages[bank+i].readFlags = 0;
		}
		mem_pages[bank+i].writePtr = junk_memory;
		mem_pages[bank+i].writeFlags = MEM_FLAG_INVALID;
	}

	mem_pages[bank+0xC7].readPtr = smpt_rom;
	mem_pages[bank+0xC7].readFlags = 0;

	for (i = 0xC8 ; i < 0xD0 ; i++) {
		mem_pages[bank+i].readPtr = rom_memory + rom_lastbank + (i * 256);
		mem_pages[bank+i].readFlags = 0;
		mem_pages[bank+i].writePtr = junk_memory;
		mem_pages[bank+i].writeFlags = MEM_FLAG_INVALID;
	}

	lcbank = mem_lcbank2? 0x1000 : 0;

	for (i = 0xD0 ; i < 0xE0 ; i++) {
		if (mem_lcread) {
			mem_pages[bank+i].readPtr = ram + (i*256) + offset - lcbank;
		} else {
			mem_pages[bank+i].readPtr = rom_memory + rom_lastbank + (i * 256);
		}
		if (mem_lcwrite) {
			mem_pages[bank+i].writePtr = ram + (i*256) + offset - lcbank;
			mem_pages[bank+i].writeFlags = 0;
		} else {
			mem_pages[bank+i].writePtr = junk_memory;
			mem_pages[bank+i].writeFlags = MEM_FLAG_INVALID;
		}
	}
	for (i = 0xE0 ; i < 0x100 ; i++) {
		if (mem_lcread) {
			mem_pages[bank+i].readPtr = ram + (i*256) + offset;
		} else {
			mem_pages[bank+i].readPtr = rom_memory + rom_lastbank + (i * 256);
		}
		if (mem_lcwrite) {
			mem_pages[bank+i].writePtr = ram + (i*256) + offset;
			mem_pages[bank+i].writeFlags = 0;
		} else {
			mem_pages[bank+i].writePtr = junk_memory;
			mem_pages[bank+i].writeFlags = MEM_FLAG_INVALID;
		}
	}
}

byte MEM_clear80store(byte val)
{
	mem_80store = 0;
	MEM_rebuildMainMem();
	return 0;
}

byte MEM_set80store(byte val)
{
	mem_80store = 1;
	MEM_rebuildMainMem();
	return 0;
}

byte MEM_get80store(byte val)
{
	return mem_80store? 0x80 : 0x00;
}

byte MEM_clearAuxRd(byte val)
{
	mem_auxrd = 0;
	MEM_rebuildMainMem();
	return 0;
}

byte MEM_setAuxRd(byte val)
{
	mem_auxrd = 1;
	MEM_rebuildMainMem();
	return 0;
}

byte MEM_getAuxRd(byte val)
{
	return mem_auxrd? 0x80 : 0x00;
}

byte MEM_clearAuxWrt(byte val)
{
	mem_auxwrt = 0;
	MEM_rebuildMainMem();
	return 0;
}

byte MEM_setAuxWrt(byte val)
{
	mem_auxwrt = 1;
	MEM_rebuildMainMem();
	return 0;
}

byte MEM_getAuxWrt(byte val)
{
	return mem_auxwrt? 0x80 : 0x00;
}

byte MEM_clearIntCXROM(byte val)
{
	mem_intcxrom = 0;
	return 0;
}

byte MEM_setIntCXROM(byte val)
{
	mem_intcxrom = 1;
	return 0;
}

byte MEM_getIntCXROM(byte val)
{
	return mem_intcxrom? 0x80 : 0x00;
}

byte MEM_clearAltZP(byte val)
{
	mem_altzp = 0;
	MEM_rebuildAltZpMem();
	return 0;
}

byte MEM_setAltZP(byte val)
{
	mem_altzp = 1;
	MEM_rebuildAltZpMem();
	return 0;
}

byte MEM_getAltZP(byte val)
{
	return mem_altzp? 0x80 : 0x00;
}

byte MEM_clearSlotC3ROM(byte val)
{
	mem_slotc3rom = 0;
	return 0;
}

byte MEM_setSlotC3ROM(byte val)
{
	mem_slotc3rom = 1;
	return 0;
}

byte MEM_getSlotC3ROM(byte val)
{
	return mem_slotc3rom? 0x80 : 0x00;
}

byte MEM_getLCbank(byte val)
{
	return mem_lcbank2? 0x80 : 0x00;
}

byte MEM_getLCread(byte val)
{
	return mem_lcread? 0x80 : 0x00;
}

byte MEM_getSlotReg(byte val)
{
	register int	i;

	val = 0;
	for (i = 7 ; i >= 0 ; i--) {
		val = val << 1;
		val |= mem_slot_reg[i];
	}
	return val;
}

byte MEM_setSlotReg(byte val)
{
	register int	i;

	for (i = 0 ; i < 8 ; i++) {
		mem_slot_reg[i] = val & 0x01;
		val = val >> 1;
	}
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_getShadowReg(byte val)
{
	val = 0;
	if (!mem_shadow_text) val |= 0x01;
	if (!mem_shadow_hires1) val |= 0x02;
	if (!mem_shadow_hires2) val |= 0x04;
	if (!mem_shadow_super) val |= 0x08;
	if (!mem_shadow_aux) val |= 0x10;
	if (!mem_shadow_lc) val |= 0x40;
	return val;
}

byte MEM_setShadowReg(byte val)
{
	mem_shadow_text = (val & 0x01)? 0 : 1;
	mem_shadow_hires1 = (val & 0x02)? 0 : 1;
	mem_shadow_hires2 = (val & 0x04)? 0 : 1;
	mem_shadow_super = (val & 0x08)? 0 : 1;
	mem_shadow_aux = (val & 0x10)? 0 : 1;
	mem_shadow_lc = (val & 0x40)? 0 : 1;
	MEM_rebuildLangCardMem();
	MEM_rebuildShadowMem();
	return 0;
}

byte MEM_getCYAReg(byte val)
{
	val = 0;
	if (emul_speed) val |= 0x80;
	if (iwm_slot7_motor) val |= 0x08;
	if (iwm_slot6_motor) val |= 0x04;
	if (iwm_slot5_motor) val |= 0x02;
	if (iwm_slot4_motor) val |= 0x01;
	return val;
}

byte MEM_setCYAReg(byte val)
{
	emul_speed = (val & 0x80)? 1 : 0;
	iwm_slot7_motor = (val & 0x08)? 1 : 0;
	iwm_slot6_motor = (val & 0x04)? 1 : 0;
	iwm_slot5_motor = (val & 0x02)? 1 : 0;
	iwm_slot4_motor = (val & 0x01)? 1 : 0;
	if (emul_speed) {
        emul_speed2 = 0;
		if (emul_speed2) {
			emul_target_cycles = 0;
			emul_target_speed = 0.0;
		} else {
			emul_target_cycles = 2500000;
			emul_target_speed = 2.5;
		}
	} else {
		emul_target_cycles = 1000000;
		emul_target_speed = 1.0;
	}
	return 0;
}

byte MEM_getIntEnReg(byte val)
{
	val = 0;
	if (emul_qtrsecirq) val |= 0x10;
	if (emul_vblirq) val |= 0x08;
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
	emul_vblirq = (val & 0x08)? 1 : 0;
	emul_qtrsecirq = (val & 0x10)? 1 : 0;
	return 0;
}

byte MEM_getDiagType(byte val)
{
	return mem_diagtype;
}

byte MEM_setVBLClear(byte val)
{
	if (mem_diagtype & 0x10) m65816_clearIRQ();
	if (mem_diagtype & 0x08) m65816_clearIRQ();
	mem_diagtype &= ~0x18;
	return 0;
}

byte MEM_getStateReg(byte val)
{
	val = 0;
	if (mem_intcxrom) val |= 0x01;
	if (mem_rombank) val |= 0x02;
	if (mem_lcbank2) val |= 0x04;
	if (!mem_lcread) val |= 0x08;
	if (mem_auxwrt) val |= 0x10;
	if (mem_auxrd) val |= 0x20;
	if (vid_page2) val |= 0x40;
	if (mem_altzp) val |= 0x80;
	return val;
}

byte MEM_setStateReg(byte val)
{
	mem_intcxrom = (val & 0x01)? 1:0;
	mem_rombank = (val & 0x02)? 1:0;
	mem_lcbank2 = (val & 0x04)? 1:0;
	mem_lcread = (val & 0x08)? 0:1;
	mem_auxwrt = (val & 0x10)? 1:0;
	mem_auxrd = (val & 0x20)? 1:0;
	(val & 0x40)? VID_setPage2(0) : VID_clearPage2(0);
	mem_altzp = (val & 0x80)? 1:0;
	MEM_rebuildMainMem();
	MEM_rebuildAltZpMem();
	return 0;
}

byte MEM_setLCx80(byte val)
{
	mem_lcbank2 = 1;
	mem_lcread = 1;
	mem_lcwrite = 0;
	mem_lcsecond = 0x80;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx81(byte val)
{
	mem_lcbank2 = 1;
	mem_lcread = 0;
	if (mem_lcsecond == 0x81) mem_lcwrite = 1;
	mem_lcsecond = 0x81;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx82(byte val)
{
	mem_lcbank2 = 1;
	mem_lcread = 0;
	mem_lcwrite = 0;
	mem_lcsecond = 0x82;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx83(byte val)
{
	mem_lcbank2 = 1;
	mem_lcread = 1;
	if (mem_lcsecond == 0x83) mem_lcwrite = 1;
	mem_lcsecond = 0x83;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx88(byte val)
{
	mem_lcbank2 = 0;
	mem_lcread = 1;
	mem_lcwrite = 0;
	mem_lcsecond = 0x88;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx89(byte val)
{
	mem_lcbank2 = 0;
	mem_lcread = 0;
	if (mem_lcsecond == 0x89) mem_lcwrite = 1;
	mem_lcsecond = 0x89;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx8A(byte val)
{
	mem_lcbank2 = 0;
	mem_lcread = 0;
	mem_lcwrite = 0;
	mem_lcsecond = 0x8A;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_setLCx8B(byte val)
{
	mem_lcbank2 = 0;
	mem_lcread = 1;
	if (mem_lcsecond == 0x8B) mem_lcwrite = 1;
	mem_lcsecond = 0x8B;
	MEM_rebuildLangCardMem();
	return 0;
}

byte MEM_getC071(byte val)
{
	return c07x_memory[0x01];
}

byte MEM_getC072(byte val)
{
	return c07x_memory[0x02];
}

byte MEM_getC073(byte val)
{
	return c07x_memory[0x03];
}

byte MEM_getC074(byte val)
{
	return c07x_memory[0x04];
}

byte MEM_getC075(byte val)
{
	return c07x_memory[0x05];
}

byte MEM_getC076(byte val)
{
	return c07x_memory[0x06];
}

byte MEM_getC077(byte val)
{
	return c07x_memory[0x07];
}

byte MEM_getC078(byte val)
{
	return c07x_memory[0x08];
}

byte MEM_getC079(byte val)
{
	return c07x_memory[0x09];
}

byte MEM_getC07A(byte val)
{
	return c07x_memory[0x0A];
}

byte MEM_getC07B(byte val)
{
	return c07x_memory[0x0B];
}

byte MEM_getC07C(byte val)
{
	return c07x_memory[0x0C];
}

byte MEM_getC07D(byte val)
{
	return c07x_memory[0x0D];
}

byte MEM_getC07E(byte val)
{
	return c07x_memory[0x0E];
}

byte MEM_getC07F(byte val)
{
	return c07x_memory[0x0F];
}
