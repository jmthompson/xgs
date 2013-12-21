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
 * File: text.c
 *
 * Contains the routines responsible for managing the text-mode
 * displays, including page 1 and page 2 in both 40- and 80-col
 * mode.
 */

#include "xgs.h"
#include "video.h"

/* Base address table for text/lores pages 1. */

word32 vid_textbases1[24] ={
    0x0400, 0x0480, 0x0500, 0x0580, 0x0600, 0x0680, 0x0700, 0x0780,
    0x0428, 0x04A8, 0x0528, 0x05A8, 0x0628, 0x06A8, 0x0728, 0x07A8,
    0x0450, 0x04D0, 0x0550, 0x05D0, 0x0650, 0x06D0, 0x0750, 0x07D0 
};

/* Base address table for text/lores pages 2. */

word32 vid_textbases2[24] ={
    0x0800, 0x0880, 0x0900, 0x0980, 0x0A00, 0x0A80, 0x0B00, 0x0B80,
    0x0828, 0x08A8, 0x0928, 0x09A8, 0x0A28, 0x0AA8, 0x0B28, 0x0BA8,
    0x0850, 0x08D0, 0x0950, 0x09D0, 0x0A50, 0x0AD0, 0x0B50, 0x0BD0 
};

#define fixer(value) ((value) == 253? vid_textbg : vid_textfg)

static void
VID_refreshText40Row (int row, word32 addr)
{
    int    col,i;
    byte    *font;
    pixel_t    *scrn;

    for (col = 0 ; col < 40 ; col++, addr++) {
        font = vid_font40[vid_altcharset] + (slow_memory[addr] * 224);
        for (i = 0 ; i < 16 ; i++) {
            scrn = vid_lines[row * 16 + i] + (col * 14);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
        }
    }
}

void VID_refreshText40Page1()
{
    int    row,first;

    first = vid_text? 0 : 20;

    for (row = first ; row < 24 ; row++) {
        VID_refreshText40Row (row, vid_textbases1[row]);
    }
    mem_slowram_changed[0x0004] = 0;
    mem_slowram_changed[0x0005] = 0;
    mem_slowram_changed[0x0006] = 0;
    mem_slowram_changed[0x0007] = 0;
}

void VID_refreshText40Page2()
{
    int    row,first;

    first = vid_text? 0 : 20;

    for (row = first ; row < 24 ; row++) {
        VID_refreshText40Row (row, vid_textbases2[row]);
    }
    mem_slowram_changed[0x0008] = 0;
    mem_slowram_changed[0x0009] = 0;
    mem_slowram_changed[0x000A] = 0;
    mem_slowram_changed[0x000B] = 0;
}

static void
VID_refreshText80Row (int row, word32 addr)
{
    int    col,i,upd0,upd1;
    byte    *font;
    pixel_t    *scrn;

    for (col = 0 ; col < 40 ; col++, addr++) {
        font = vid_font80[vid_altcharset] + (slow_memory[addr] * 112);
        for (i = 0 ; i < 16 ; i++) {
            scrn = vid_lines[row * 16 + i] + (col * 14 + 7);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
        }

        font = vid_font80[vid_altcharset] + (slow_memory[addr + 0x10000] * 112);
        for (i = 0 ; i < 16 ; i++) {
            scrn = vid_lines[row * 16 + i] + (col * 14);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
            *scrn++ = fixer(*font++);
        }
    }
}

void VID_refreshText80Page1()
{
    int    row,first;

    first = vid_text? 0 : 20;

    for (row = first ; row < 24 ; row++) {
        VID_refreshText80Row (row, vid_textbases1[row]);
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

void VID_refreshText80Page2()
{
    int    row,first;

    first = vid_text? 0 : 20;

    for (row = first ; row < 24 ; row++) {
        VID_refreshText80Row (row, vid_textbases2[row]);
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

