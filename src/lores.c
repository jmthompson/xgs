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
 * File: lores.c
 *
 * Contains the routines responsible for managing the low-res
 * graphics pages 1 and 2 in both standard and double-resolution.
 */

#include "xgs.h"
#include "video.h"
#include "vid-drv.h"

/* No use duplicating this when we can borrow it from	*/
/* the text mode drivers!				*/

extern word32 vid_textbases1[24];
extern word32 vid_textbases2[24];

static void
VID_refreshLoresRow (int row, word32 addr)
{
	int	col,i,val;
	PIXEL	*scrn,top,btm;
	word32  mem_changed;

	mem_changed = mem_slowram_changed[addr >> 8];
	if (!mem_changed)
		return;
	for (col = 0 ; col < 40 ; col++, addr++) {
		if (!(mem_changed & mem_change_masks[addr & 0xFF]))
			continue;
		val = slow_memory[addr];
		top = val & 0x0F;
		btm = val >> 4;
		for (i = 0 ; i < 8 ; i++) {
			scrn = vid_lines[row * 16 + i] + (col * 14);
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
			*scrn++ = top;
		}
		for (i = 8 ; i < 16 ; i++) {
			scrn = vid_lines[row * 16 + i] + (col * 14);
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
			*scrn++ = btm;
		}
		if (vid_xmin > (col * 14))
			vid_xmin = col*14;
		if (vid_xmax < ((col+1)*14))
			vid_xmax = (col+1)*14;
		if (vid_ymin > (row * 16))
			vid_ymin = row*16;
		if (vid_ymax < ((row+1)*16))
			vid_ymax = (row+1)*16;
	}
}

void VID_refreshLoresPage1()
{
	int	row,max;

	max = vid_mixed? 20 : 24;

	for (row = 0 ; row < max ; row++) {
		VID_refreshLoresRow (row, vid_textbases1[row]);
	}
	if (vid_mixed) {
		vid_80col? VID_refreshText80Page1() : VID_refreshText40Page1();
	}
	mem_slowram_changed[0x0004] = 0;
	mem_slowram_changed[0x0005] = 0;
	mem_slowram_changed[0x0006] = 0;
	mem_slowram_changed[0x0007] = 0;
}

void VID_refreshLoresPage2()
{
	int	row,max;

	max = vid_mixed? 20 : 24;

	for (row = 0 ; row < max ; row++) {
		VID_refreshLoresRow (row, vid_textbases2[row]);
	}
	if (vid_mixed) {
		vid_80col? VID_refreshText80Page2() : VID_refreshText40Page2();
	}
	mem_slowram_changed[0x0008] = 0;
	mem_slowram_changed[0x0009] = 0;
	mem_slowram_changed[0x000A] = 0;
	mem_slowram_changed[0x000B] = 0;
}

static void
VID_refreshDLoresRow (int row, word32 addr)
{
	int	col,i,val,upd0,upd1;
	PIXEL	*scrn,top,btm;
	word32  mem_changed0, mem_changed1;

	mem_changed0 = mem_slowram_changed[addr >> 8];
	mem_changed1 = mem_slowram_changed[(addr >> 8) + 0x100];
	if (!mem_changed0 && !mem_changed1)
		return;
	for (col = 0 ; col < 40 ; col++, addr++) {
		upd0 = mem_changed0 & mem_change_masks[addr & 0xFF];
		upd1 = mem_changed1 & mem_change_masks[addr & 0xFF];
		if (!upd0 && !upd1) continue;
		if (upd0) {
			val = slow_memory[addr];
			top = val & 0x0F;
			btm = val >> 4;
			for (i = 0 ; i < 8 ; i++) {
				scrn = vid_lines[row * 16 + i] + (col * 14 + 7);
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
			}
			for (i = 8 ; i < 16 ; i++) {
				scrn = vid_lines[row * 16 + i] + (col * 14 + 7);
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
			}
		}
		if (upd1) {
			val = slow_memory[addr + 0x10000];
			top = val & 0x0F;
			btm = val >> 4;
			for (i = 0 ; i < 8 ; i++) {
				scrn = vid_lines[row * 16 + i] + (col * 14);
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
				*scrn++ = top;
			}
			for (i = 8 ; i < 16 ; i++) {
				scrn = vid_lines[row * 16 + i] + (col * 14);
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
				*scrn++ = btm;
			}
		}
		if (vid_xmin > (col * 14))
			vid_xmin = col*14;
		if (vid_xmax < ((col+1)*14))
			vid_xmax = (col+1)*14;
		if (vid_ymin > (row * 16))
			vid_ymin = row*16;
		if (vid_ymax < ((row+1)*16))
			vid_ymax = (row+1)*16;
	}
}

void VID_refreshDLoresPage1()
{
	int	row,max;

	max = vid_mixed? 20 : 24;

	for (row = 0 ; row < max ; row++) {
		VID_refreshDLoresRow (row, vid_textbases1[row]);
	}
	mem_slowram_changed[0x0004] = 0;
	mem_slowram_changed[0x0005] = 0;
	mem_slowram_changed[0x0006] = 0;
	mem_slowram_changed[0x0007] = 0;
	mem_slowram_changed[0x0104] = 0;
	mem_slowram_changed[0x0105] = 0;
	mem_slowram_changed[0x0106] = 0;
	mem_slowram_changed[0x0107] = 0;
}

void VID_refreshDLoresPage2()
{
	int	row,max;

	max = vid_mixed? 20 : 24;

	for (row = 0 ; row < max ; row++) {
		VID_refreshDLoresRow (row, vid_textbases2[row]);
	}
	mem_slowram_changed[0x0008] = 0;
	mem_slowram_changed[0x0009] = 0;
	mem_slowram_changed[0x000A] = 0;
	mem_slowram_changed[0x000B] = 0;
	mem_slowram_changed[0x0108] = 0;
	mem_slowram_changed[0x0109] = 0;
	mem_slowram_changed[0x010A] = 0;
	mem_slowram_changed[0x010B] = 0;
}
