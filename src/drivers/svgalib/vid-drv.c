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
 * File: arch/svgalib/vid-drv.c
 *
 * SVGAlib video driver.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <vga.h>
#include <vgagl.h>

#include "video.h"
#include "vid-drv.h"

int		vid_win_width;
int		vid_win_height;

GraphicsContext	*vid_screen;
PIXEL		*vid_buffer;
PIXEL		*vid_lines[2048];
int		vid_vgamode;

VGACOLOR vid_supercolors[256];
VGACOLOR vid_textcolors[16] = {
	{ 0x00, 0x00, 0x00 }, { 0xDD, 0x00, 0x33 },
	{ 0x00, 0x00, 0x99 }, { 0xDD, 0x22, 0xDD },
	{ 0x00, 0x77, 0x22 }, { 0x55, 0x55, 0x55 },
	{ 0x22, 0x22, 0xFF }, { 0x66, 0xAA, 0xFF },
	{ 0x88, 0x55, 0x00 }, { 0xFF, 0x66, 0x00 },
	{ 0xAA, 0xAA, 0xAA }, { 0xFF, 0x99, 0x88 },
	{ 0x00, 0xDD, 0x00 }, { 0xFF, 0xFF, 0x00 },
	{ 0x55, 0xFF, 0x99 }, { 0xFF, 0xFF, 0xFF }
};

int VID_outputInit(void)
{
	int		i;

	printf("    - Allocating display buffer: ");
	vid_buffer = (PIXEL *) malloc(VID_WIDTH * VID_HEIGHT);
	if (!vid_buffer) {
		printf("Failed\n");
		return 1;
	}
	printf("Done\n");

	printf("    - Initializing SVGA: ");
	if ((vid_vgamode = vga_getdefaultmode()) == -1)
		vid_vgamode = G800x600x256;

	if (!vga_hasmode(vid_vgamode)) {
		printf("Failed: selected mode not available.\n";
		return 1;
	}
	if (vga_getmodeinfo(vid_vgamode)->colors != 256) {
		printf("Failed: selected mode is not 256 colors.\n";
		return 1;
	}
	vga_init();
	vid_black = 0;
	vid_white = 15;

	vga_setmode(vid_vgamode);
	gl_setcontextvga(vid_vgamode);

	vid_screen = gl_allocatecontext();
	gl_getcontext(vid_screen);
	
	/* Get the screen width+height from the current context */

	vid_win_width = WIDTH;
	vid_win_height = HEIGHT;

	printf("Done\n");

	return 0;
}

void VID_outputImage()
{
	int		i;

	vid_lines[0] = vid_buffer + (vid_out_y * vid_win_width)
				  + vid_out_x;

	for (i = 1 ; i < VID_HEIGHT ; i++) {
		vid_lines[i] = vid_lines[i-1] + vid_win_width;
	}

	vid_xmin = VID_WIDTH;
	vid_ymin = VID_HEIGHT;
	vid_xmax = vid_ymax = 0;

	(*VID_updateRoutine)();

	if ((vid_xmax > vid_xmin) && (vid_ymax > vid_ymin)) {
		putbox(vid_xmin, vid_ymin, vid_xmax - vid_xmin, vid_ymax - vid_ymin, (void *) vid_buffer);
	}
}

void VID_outputShutdown()
{
	printf("    - Restoring text mode\n");
	vga_setmode(TEXT);
}

void VID_outputStatus1(char *msg)
{
}

void VID_outputStatus2(char *msg)
{
}

void VID_outputResize(int width, int height)
{

void VID_outputResize(int width, int height)
{
	int	i,y;

	vid_out_width = width;
	vid_out_height = height;
	vid_out_x = (vid_win_width - width) / 2;
	vid_out_y = (vid_win_height - height) / 2;

	vid_lines[0] = vid_buffer;
	for (i = 1 ; i < vid_win_height ; i++) {
		vid_lines[i] = vid_lines[i-1] + vid_win_width;
	}

	for (y = 0 ; y < vid_out_y ; y++) {
		memset(vid_lines[y], 255, vid_win_width);
	}

	for (y = vid_out_y ; y < vid_out_y + vid_out_height; y++) {
		memset(vid_lines[y], 255, vid_out_x);
		memset(vid_lines[y]+vid_out_width+vid_out_x, 255, vid_out_x);
	}

	for (y = vid_out_y + vid_out_height ; y < vid_win_height ; y++) {
		memset(vid_lines[y], 255, vid_win_width);
	}
}

void VID_outputSetTextColors()
{
	setpalettecolor(253, vid_textcolors[vid_textbgcolor].r,
			     vid_textcolors[vid_textbgcolor].g,
			     vid_textcolors[vid_textbgcolor].b);
	setpalettecolor(254, vid_textcolors[vid_textfgcolor].r,
			     vid_textcolors[vid_textfgcolor].g,
			     vid_textcolors[vid_textfgcolor].b);
}

void VID_outputSetBorderColor()
{
	setpalettecolor(255, vid_textcolors[vid_bordercolor].r,
			     vid_textcolors[vid_bordercolor].g,
			     vid_textcolors[vid_bordercolor].b);
}

void VID_outputSetStandardColors()
{
	int	i;

	setpalettecolor(253, vid_textcolors[vid_textbgcolor].r,
			     vid_textcolors[vid_textbgcolor].g,
			     vid_textcolors[vid_textbgcolor].b);
	setpalettecolor(254, vid_textcolors[vid_textfgcolor].r,
			     vid_textcolors[vid_textfgcolor].g,
			     vid_textcolors[vid_textfgcolor].b);
	setpalettecolor(255, vid_textcolors[vid_bordercolor].r,
			     vid_textcolors[vid_bordercolor].g,
			     vid_textcolors[vid_bordercolor].b);
	for (i = 0 ; i <= 15 ; i++) {
		setpalettecolor(i, vid_textcolors[i].r,
				   vid_textcolors[i].g,
				   vid_textcolors[i].b);
	}
}

void VID_outputSetSuperColors()
{
	int	addr,i;

	addr = 0x019E00;
	for (i = 0 ; i < 255 ; i++) {
		if (mem_slowram_changed[addr >> 8] & mem_change_masks[addr & 0xFF]) {
			setpalettecolor(i,
				(slow_memory[addr+1] & 0x0F) * 17,
				((slow_memory[addr] >> 4) & 0x0F) * 17,
				(slow_memory[addr] & 0x0F) * 17);
		}
		addr += 2;
	}
	mem_slowram_changed[0x019E] = 0;
	mem_slowram_changed[0x019F] = 0;
}
