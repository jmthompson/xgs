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
 * File: arch/win32/vid-drv.cpp
 *
 * Win32/DirectX video driver
 */

#define WIN32_LEAN_AND_MEAN

#include <stdio.h>
#include <stdlib.h>

#include <windows.h>
#include <windowsx.h>
#include <ddraw.h>
#include "main.h"

extern "C" {
#include "xgs.h"
#include "memory.h"
#include "video.h"
#include "vid-drv.h"

extern HWND	hWnd;
}

PIXEL	*vid_buffer;
PIXEL	*vid_lines[VID_WIN_HEIGHT];

typedef struct {
	int	red,green,blue;
} vid_color;

vid_color vid_textcolors[16] = {
	{ 0x00, 0x00, 0x00 },
	{ 0xDD, 0x00, 0x33 },
	{ 0x00, 0x00, 0x99 },
	{ 0xDD, 0x22, 0xDD },
	{ 0x00, 0x77, 0x22 },
	{ 0x55, 0x55, 0x55 },
	{ 0x22, 0x22, 0xFF },
	{ 0x66, 0xAA, 0xFF },
	{ 0x88, 0x55, 0x00 },
	{ 0xFF, 0x66, 0x00 },
	{ 0xAA, 0xAA, 0xAA },
	{ 0xFF, 0x99, 0x88 },
	{ 0x00, 0xDD, 0x00 },
	{ 0xFF, 0xFF, 0x00 },
	{ 0x55, 0xFF, 0x99 },
	{ 0xFF, 0xFF, 0xFF }
};

vid_color vid_supercolors[256];

// DirectX Variables

LPDIRECTDRAW		vid_out_dd;
LPDIRECTDRAWSURFACE	vid_out_dds0;
LPDIRECTDRAWPALETTE	vid_out_ddp;

DDSURFACEDESC		vid_out_ddsd;
DDSCAPS				vid_out_ddscaps;
PALETTEENTRY		vid_palette[256];

int	vid_out_width,vid_out_height,vid_out_x,vid_out_y;

int VID_outputInit(void)
{
	HRESULT	err;

	/*
	printf("Allocating screen data buffer: ");
	vid_buffer = (char *) malloc(VID_WIDTH * VID_HEIGHT);
	if (!vid_buffeR) {
		printf("Failed\n");
		return 1;
	}
	printf("Done");
	*/

	// Create the IDirectDraw object and put DirectDraw in
	// full-screen exclusive access mode

	printf("    - IDirectDraw::DirectDrawCreate: ");
	err = DirectDrawCreate(NULL, &vid_out_dd, NULL);
	if (err != DD_OK) {
		printf("Failed (error #%X)\n", err);
		return 1;
	}
	printf("Done\n");

	printf("    - IDirectDraw::SetCooperativeLevel: ");
	err = vid_out_dd->SetCooperativeLevel(hWnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
	if (err != DD_OK) {
		printf("Failed (error #%X)\n", err);
		vid_out_dd->Release();
		return 1;
	}
	printf("Done\n");

	// Set the display mode

	printf("    - IDirectDraw::SetDisplayMode: ");
	err = vid_out_dd->SetDisplayMode(VID_WIN_WIDTH, VID_WIN_HEIGHT, 8);
	if (err != DD_OK) {
		printf("Failed (error #%X)\n", err);
		vid_out_dd->Release();
		return 1;
	}
	printf("Done\n");

	// Create the primary display surface w/ one backing buffer

	vid_out_ddsd.dwSize = sizeof(vid_out_ddsd);
	vid_out_ddsd.dwFlags = DDSD_CAPS;
	vid_out_ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
	printf("    - IDirectDraw::CreateSurface: ");
	err = vid_out_dd->CreateSurface(&vid_out_ddsd, &vid_out_dds0, NULL);
	if (err != DD_OK) {
		printf("Failed (error #%X)\n", err);
		vid_out_dd->RestoreDisplayMode();
		vid_out_dd->Release();
		return 1;
	}
	printf("Done\n");

	// Create a new palette and attach it to the primary surface

	printf("    - IDirectDraw::CreatePalette: ");
	err = vid_out_dd->CreatePalette(DDPCAPS_8BIT, vid_palette, &vid_out_ddp, NULL);
	if (err != DD_OK) {
		printf("Failed (error #%X)\n", err);
		vid_out_dds0->Release();
		vid_out_dd->RestoreDisplayMode();
		vid_out_dd->Release();
		return 1;
	}
	printf("Done\n");

	printf("    - IDirectDraw::SetPalette: ");
	err = vid_out_dds0->SetPalette(vid_out_ddp);
	if (err != DD_OK) {
		printf("Failed (error #%X)\n", err);
		vid_out_ddp->Release();
		vid_out_dds0->Release();
		vid_out_dd->RestoreDisplayMode();
		vid_out_dd->Release();
		return 1;
	}
	printf("Done\n");
	return 0;
}

void VID_outputImage()
{
	HRESULT	err;
	int		i;

	err = vid_out_dds0->Lock(NULL, &vid_out_ddsd, DDLOCK_WAIT, NULL);
	if (err != DD_OK) {
		if (err == DDERR_SURFACELOST) {
			err = vid_out_dds0->Restore();
			if (err != DD_OK) return;
			VID_outputResize(vid_out_width, vid_out_height);
			for (i = 0 ; i < 512 ; i++) mem_slowram_changed[i] = 0xFFFFFFFF;
			return;
		} else {
			printf("Couldn't lock surface: error #%8X\n",err);
			return;
		}
	}

	vid_buffer = (byte *) vid_out_ddsd.lpSurface;
	vid_lines[0] = vid_buffer + (vid_out_y * vid_out_ddsd.lPitch) + vid_out_x;
	for (i = 1 ; i < VID_HEIGHT ; i++) {
		vid_lines[i] = vid_lines[i-1] + vid_out_ddsd.lPitch;
	}

	vid_xmin = VID_WIDTH;
	vid_ymin = VID_HEIGHT;
	vid_xmax = vid_ymax = 0;

	(*VID_updateRoutine)();

	err = vid_out_dds0->Unlock(vid_out_ddsd.lpSurface);
	if (err != DD_OK) {
		printf("Couldn't unlock surface: error #%8X\n",err);
	}
}

void VID_outputShutdown()
{
	vid_out_ddp->Release();
	vid_out_dds0->Release();
	vid_out_dd->RestoreDisplayMode();
	vid_out_dd->Release();
}

void VID_outputStatus1(char *msg)
{
	printf("%s\n",msg);
}

void VID_outputStatus2(char *msg)
{
	printf("%s\n",msg);
}

void VID_outputResize(int width, int height)
{
	HRESULT	err;
	int		i,y;

	vid_out_width = width;
	vid_out_height = height;
	vid_out_x = (VID_WIN_WIDTH - width) / 2;
	vid_out_y = (VID_WIN_HEIGHT - height) / 2;

	err = vid_out_dds0->Lock(NULL, &vid_out_ddsd, DDLOCK_WAIT, NULL);
	if (err != DD_OK) {
		printf("Couldn't lock surface: error #%8X\n",err);
		return;
	}

	vid_buffer = (byte *) vid_out_ddsd.lpSurface;
	vid_lines[0] = vid_buffer;
	for (i = 1 ; i < VID_WIN_HEIGHT ; i++) {
		vid_lines[i] = vid_lines[i-1] + vid_out_ddsd.lPitch;
	}

	for (y = 0 ; y < vid_out_y ; y++) {
		memset(vid_lines[y], 252, VID_WIN_WIDTH);
	}

	for (y = vid_out_y ; y < vid_out_y + vid_out_height; y++) {
		memset(vid_lines[y], 252, vid_out_x);
		memset(vid_lines[y]+vid_out_width+vid_out_x, 252, vid_out_x);
	}

	for (y = vid_out_y + vid_out_height ; y < VID_WIN_HEIGHT ; y++) {
		memset(vid_lines[y], 252, VID_WIN_WIDTH);
	}

	err = vid_out_dds0->Unlock(vid_out_ddsd.lpSurface);
	if (err != DD_OK) {
		printf("Couldn't unlock surface: error #%8X\n",err);
	}
}

void VID_outputSetTextColors()
{
	vid_palette[253].peRed = vid_textcolors[vid_textbgcolor].red;
	vid_palette[253].peGreen = vid_textcolors[vid_textbgcolor].green;
	vid_palette[253].peBlue = vid_textcolors[vid_textbgcolor].blue;
	
	vid_palette[254].peRed = vid_textcolors[vid_textfgcolor].red;
	vid_palette[254].peGreen = vid_textcolors[vid_textfgcolor].green;
	vid_palette[254].peBlue = vid_textcolors[vid_textfgcolor].blue;

	vid_out_ddp->SetEntries(0, 0, 256, vid_palette);
}

void VID_outputSetBorderColor()
{
	vid_palette[252].peRed = vid_textcolors[vid_bordercolor].red;
	vid_palette[252].peGreen = vid_textcolors[vid_bordercolor].green;
	vid_palette[252].peBlue = vid_textcolors[vid_bordercolor].blue;

	vid_out_ddp->SetEntries(0, 0, 256, vid_palette);
}

void VID_outputSetStandardColors()
{
	int i;

	for (i = 0; i < 16; i++) {
		vid_palette[i].peRed = vid_textcolors[i].red;
		vid_palette[i].peGreen = vid_textcolors[i].green;
		vid_palette[i].peBlue = vid_textcolors[i].blue;
	}

	vid_palette[253].peRed = vid_textcolors[vid_textbgcolor].red;
	vid_palette[253].peGreen = vid_textcolors[vid_textbgcolor].green;
	vid_palette[253].peBlue = vid_textcolors[vid_textbgcolor].blue;
	
	vid_palette[254].peRed = vid_textcolors[vid_textfgcolor].red;
	vid_palette[254].peGreen = vid_textcolors[vid_textfgcolor].green;
	vid_palette[254].peBlue = vid_textcolors[vid_textfgcolor].blue;

	vid_palette[252].peRed = vid_textcolors[vid_bordercolor].red;
	vid_palette[252].peGreen = vid_textcolors[vid_bordercolor].green;
	vid_palette[252].peBlue = vid_textcolors[vid_bordercolor].blue;
	
	vid_out_ddp->SetEntries(0, 0, 256, vid_palette);

	vid_black = 0;
	vid_white = 15;
}

void VID_outputSetSuperColors()
{
	int addr, i;
	int requireupdate = 0;

	addr = 0x019E00;
	for (i = 0 ; i < 256 ; i++) {
		if (mem_slowram_changed[addr >> 8] & mem_change_masks[addr & 0xFF]) {
			vid_palette[i].peBlue = (slow_memory[addr] & 0x0F) * 17;
			vid_palette[i].peGreen = ((slow_memory[addr] >> 4) & 0x0F) * 17;
			vid_palette[i].peRed = (slow_memory[addr+1] & 0x0F) * 17;
			requireupdate = 1;
		}
		addr += 2;
	}
	mem_slowram_changed[0x019E] = 0;
	mem_slowram_changed[0x019F] = 0;
	
	if (requireupdate) vid_out_ddp->SetEntries(0, 0, 256, vid_palette);
}
