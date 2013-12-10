
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
#include "video-output.h"

void VID_refreshSuperHires()
{
	register byte	v,scb;
	register PIXEL	*scrn,*scrn2;
	register int	a,palette,pixel,lastpixel,line_changed;
	register int	row,col,fillmode;

	register int	myvidpixel;

	VID_outputSetSuperColors();

	lastpixel = 0;
	for (row = 0 ; row < 200 ; row++) {
		scb = slow_memory[0x019D00 + row];
		fillmode = ((scb & 0x20) == 0x20);

		palette = (scb & 0x0F) << 4;
		line_changed = (mem_slowram_changed[0x019D] &
				mem_change_masks[row]) || fillmode;

		if (scb & 0x80) {
			for (col = 0 ; col < 160 ; col++) {
				a = 0x012000 + (row * 160) + col;
				if (line_changed ||
				    (mem_slowram_changed[a >> 8] &
				     mem_change_masks[a & 0xFF])) {
					v = slow_memory[a];
					scrn = vid_lines[row * 2] + (col * 4);
					scrn2 = vid_lines[row * 2 + 1] + (col * 4);
#ifndef WORDS_BIGENDIAN
					pixel = (v & 0x03) + palette + 12;
					myvidpixel = (pixel << 24);

					pixel = ((v >> 2) & 0x03) + palette + 8;
					myvidpixel |= (pixel << 16);

					pixel = ((v >> 4) & 0x03) + palette + 4;
					myvidpixel |= (pixel << 8);

					pixel = (v >> 6) + palette;
					myvidpixel |= pixel;
#else /* WORDS_BIGENDIAN */
					pixel = (v & 0x03) + palette + 12;
					myvidpixel = pixel;

					pixel = ((v >> 2) & 0x03) + palette + 8;
					myvidpixel |= (pixel << 8);

					pixel = ((v >> 4) & 0x03) + palette + 4;
					myvidpixel |= (pixel << 16);

					pixel = (v >> 6) + palette;
					myvidpixel |= (pixel << 24);
#endif /* WORDS_BIGENDIAN */
					*((int *)scrn) = *((int *)scrn2) = myvidpixel;
					if (vid_xmin > (col * 4))
						vid_xmin = col*4;
					if (vid_xmax < ((col+1)*4))
						vid_xmax = (col+1)*4;
					if (vid_ymin > (row * 2))
						vid_ymin = row*2;
					if (vid_ymax < ((row+1)*2))
						vid_ymax = (row+1)*2;

				}
			}
		} else {
			for (col = 0 ; col < 160 ; col++) {
				a = 0x012000 + (row * 160) + col;
				if (line_changed ||
				    (mem_slowram_changed[a >> 8] &
				     mem_change_masks[a & 0xFF])) {
					v = slow_memory[a];
					scrn = vid_lines[row * 2] + (col * 4);
					scrn2 = vid_lines[row * 2 + 1] + (col *
4);
#ifndef WORDS_BIGENDIAN
					pixel = v >> 4;
					if (fillmode && !pixel)
						pixel = lastpixel;
					lastpixel = pixel;
					pixel += palette;

					myvidpixel = (pixel << 8);
					myvidpixel |= pixel;

					pixel = v & 0x0F;
					if (fillmode && !pixel)
						pixel = lastpixel;
					lastpixel = pixel;
					pixel += palette;

					myvidpixel |= (pixel << 24);
					myvidpixel |= (pixel << 16);
#else /* WORDS_BIGENDIAN */
					pixel = v >> 4;
					if (fillmode && !pixel)
						pixel = lastpixel;
					lastpixel = pixel;
					pixel += palette;

					myvidpixel = (pixel << 16);
					myvidpixel |= (pixel << 24);

					pixel = v & 0x0F;
					if (fillmode && !pixel)
						pixel = lastpixel;
					lastpixel = pixel;
					pixel += palette;

					myvidpixel |= pixel;
					myvidpixel |= pixel << 8;
#endif /* WORDS_BIGENDIAN */
					*((int *)scrn) = *((int *)scrn2) = myvidpixel;

					if (vid_xmin > (col*4)) 
						vid_xmin = col*4;
					if (vid_xmax < ((col+1)*4))
						vid_xmax = (col+1)*4;
					if (vid_ymin > (row*2))
						vid_ymin = row*2;
					if (vid_ymax < ((row+1)*2))
						vid_ymax = (row+1)*2;

				}
			}
		}
	}
	for (a = 0x0120 ; a < 0x019E ; a++) mem_slowram_changed[a] = 0;
}
