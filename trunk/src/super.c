
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
 * File: super.c
 *
 * Contains the routines responsible for managing the super high-res
 * graphics page.
 */

#include "xgs.h"
#include "video.h"

void VID_refreshSuperHires()
{
    register byte    v,scb;
    register pixel_t    *scrn,*scrn2;
    register int    a,palette,pixel,lastpixel;
    register int    row,col,fillmode;

    VID_outputSetSuperColors();

    lastpixel = 0;
    for (row = 0 ; row < 200 ; row++) {
        scb = slow_memory[0x019D00 + row];
        fillmode = ((scb & 0x20) == 0x20);

        palette = (scb & 0x0F) << 4;

        if (scb & 0x80) {
            for (col = 0 ; col < 160 ; col++) {
                a = 0x012000 + (row * 160) + col;
                v = slow_memory[a];
                scrn  = vid_lines[row * 2] + (col * 4);
                scrn2 = vid_lines[row * 2 + 1] + (col * 4);

                scrn[3] = scrn2[3] = vid_supercolors[(v & 0x03) + palette + 12];
                scrn[2] = scrn2[2] = vid_supercolors[((v >> 2) & 0x03) + palette + 8];
                scrn[1] = scrn2[1] = vid_supercolors[((v >> 4) & 0x03) + palette + 4];
                scrn[0] = scrn2[0] = vid_supercolors[(v >> 6) + palette ];
            }
        } else {
            for (col = 0 ; col < 160 ; col++) {
                a = 0x012000 + (row * 160) + col;
                v = slow_memory[a];
                scrn = vid_lines[row * 2] + (col * 4);
                scrn2 = vid_lines[row * 2 + 1] + (col * 4);

                pixel = v >> 4;
                if (fillmode && !pixel)
                    pixel = lastpixel;

                lastpixel = pixel;

                scrn[0] = scrn[1] = scrn2[0] = scrn2[1] = vid_supercolors[pixel + palette];

                pixel = v & 0x0F;
                if (fillmode && !pixel)
                    pixel = lastpixel;

                lastpixel = pixel;

                scrn[2] = scrn[3] = scrn2[2] = scrn2[3] = vid_supercolors[pixel + palette];
            }
        }
    }
    for (a = 0x0120 ; a < 0x019E ; a++) mem_slowram_changed[a] = 0;
}
