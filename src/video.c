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
 * File: video.c
 *
 * Video subsystem initialization and and core routines.
 */

#include "xgs.h"
#include "video.h"

byte        *vid_font40[2];
byte        *vid_font80[2];

void        (*VID_updateRoutine)(void);

int        vid_bordercolor;
int        vid_textfgcolor;
int        vid_textbgcolor;

int        vid_currmode;

int        vid_super;
int        vid_linear;
int        vid_a2mono;

int        vid_80col;
int        vid_altcharset;
int        vid_text;
int        vid_mixed;
int        vid_page2;
int        vid_hires;
int        vid_dblres;

int        vid_vgcint;

int        vid_vert_cnt;
int        vid_horiz_cnt;

pixel_t   vid_textfg, vid_textbg, vid_border;

pixel_t   vid_supercolors[256];
pixel_t   vid_stdcolors[16] = {
    0x000000,
    0xDD0033,
    0x000099,
    0xDD22DD,
    0x007722,
    0x555555,
    0x2222FF,
    0x66AAFF,
    0x885500,
    0xFF6600,
    0xAAAAAA,
    0xFF9988,
    0x00DD00,
    0xFFFF00,
    0x55FF99,
    0xFFFFFF
};

static SDL_Window   *vid_window;
static SDL_Renderer *vid_renderer;
static SDL_Surface  *vid_surface;
static SDL_Texture  *vid_texture;

pixel_t **vid_lines;

int videoInit(void)
{
    int fd, h, i, w;

    w = VID_WIDTH  + (VID_BORDER_SIZE * 2);
    h = VID_HEIGHT + (VID_BORDER_SIZE * 2);

    vid_lines = malloc(sizeof(pixel_t *) * h);
    if (!vid_lines) {
        perror("Couldn't allocate vid_lines");

        return -1;
    }

    vid_window = SDL_CreateWindow("XGS", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
    if (vid_window == NULL) {
        printSDLError("Failed to create window");

        return -1;
    }

    vid_renderer = SDL_CreateRenderer(vid_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (vid_renderer == NULL) {
        printSDLError("Failed to create renderer");

        return -1;
    }

    vid_surface = NULL;
    vid_texture = NULL;

    SDL_RenderSetLogicalSize(vid_renderer, w, h);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

    if (!(vid_font40[0] = malloc(57344))) {
        perror(NULL);

        return 1;
    }
    if (!(vid_font40[1] = malloc(57344))) {
        perror(NULL);

        return 1;
    }

    fd = openDataFile(FONT40_FILE);

    if (fd < 0) {
        perror("Error loading 40-column font");

        return 1;
    }
    if (read(fd, vid_font40[0], 57344) != 57344) {
        perror("Error loading 40-column font");

        close(fd);

        return 1;
    }
    if (read(fd, vid_font40[1], 57344) != 57344) {
        perror("Error loading 40-column font");

        close(fd);

        return 1;
    }

    close(fd);

    if (!(vid_font80[0] = malloc(28672))) {
        perror(NULL);

        return 1;
    }
    if (!(vid_font80[1] = malloc(28672))) {
        perror(NULL);

        return 1;
    }

    fd = openDataFile(FONT80_FILE);

    if (fd < 0) {
        perror("Error loading 80-column font");

        return 1;
    }
    if (read(fd, vid_font80[0], 28672) != 28672) {
        perror("Error loading 80-column font");

        close(fd);

        return 1;
    }
    if (read(fd, vid_font80[1], 28672) != 28672) {
        perror("Error loading 80-column font");

        close(fd);

        return 1;
    }

    close(fd);

    return 0;
}

/* Update the display by sending the current image. This routine is    */
/* called automatically by the EMUL_update() procedure.            */

void videoUpdate()
{
	(*VID_updateRoutine)();

    SDL_UpdateTexture(vid_texture, NULL, vid_surface->pixels, vid_surface->pitch);

    SDL_RenderClear(vid_renderer);
    SDL_RenderCopy(vid_renderer, vid_texture, NULL, NULL);
    SDL_RenderPresent(vid_renderer);
}

void videoReset()
{
    vid_vgcint = 0;

    vid_80col = 0;
    vid_altcharset = 0;
    vid_text = 1;
    vid_mixed = 0;
    vid_page2 = 0;
    vid_hires = 0;
    vid_dblres = 0;
    vid_currmode = VID_MODE_SUPER;

    vid_a2mono = 0;
    vid_linear = 0;
    vid_super = 0;

    vid_bordercolor = 0;
    vid_textbgcolor = 0;
    vid_textfgcolor = 15;

    vid_vert_cnt = 0;
    vid_horiz_cnt = 0;

    videoModeChanged();
}

void videoShutdown()
{
    if (vid_font80[1]) free(vid_font80[1]);
    if (vid_font80[0]) free(vid_font80[0]);
    if (vid_font40[1]) free(vid_font40[1]);
    if (vid_font40[0]) free(vid_font40[0]);
}

/* Handle the gruntwork of switching video modes. We update the vid_currmode    */
/* variable, add/remove the necessary fault handlers, and then redraw the     */
/* screen in the new mode.                            */

void videoModeChanged()
{
    int    oldmode,x,width,height;

    oldmode = vid_currmode;
    if (vid_super) {
        vid_currmode = VID_MODE_SUPER;
    } else {
        if (vid_text) {
            if (vid_80col) {
                vid_currmode = (!mem_80store && vid_page2)? VID_MODE_80TEXT2 : VID_MODE_80TEXT1;
            } else {
                vid_currmode = (!mem_80store && vid_page2)? VID_MODE_40TEXT2 : VID_MODE_40TEXT1;
            }
        } else {
            if (vid_hires) {
                if (vid_dblres && vid_80col) {
                    vid_currmode = (!mem_80store && vid_page2)? VID_MODE_DHIRES2 : VID_MODE_DHIRES1;
                } else {
                    vid_currmode = (!mem_80store && vid_page2)? VID_MODE_HIRES2 : VID_MODE_HIRES1;
                }
            } else {
                if (vid_dblres && vid_80col) {
                    vid_currmode = (!mem_80store && vid_page2)? VID_MODE_DLORES2 : VID_MODE_DLORES1;
                } else {
                    vid_currmode = (!mem_80store && vid_page2)? VID_MODE_LORES2 : VID_MODE_LORES1;
                }
            }
        }
    }
    if (oldmode != vid_currmode) {
        if ((oldmode == VID_MODE_HIRES1) && (vid_currmode == VID_MODE_HIRES2)) {
            VID_switchPage (0x40,0x20,0x20);
        } else if ((oldmode == VID_MODE_HIRES2) && (vid_currmode == VID_MODE_HIRES1)) {
            VID_switchPage (0x20,0x40,0x20);
        } else if ((oldmode == VID_MODE_DHIRES1) && (vid_currmode == VID_MODE_DHIRES2)) {
            VID_switchPage (0x40,0x20,0x20);
            VID_switchPage (0x140,0x120,0x20);
        } else if ((oldmode == VID_MODE_DHIRES2) && (vid_currmode == VID_MODE_DHIRES1)) {
            VID_switchPage (0x20,0x40,0x20);
            VID_switchPage (0x120,0x140,0x20);
        } else {
            for (x = 0 ; x < 512 ; x++) mem_slowram_changed[x] = 0xFFFFFFFF;
            if ((oldmode == VID_MODE_SUPER) || (vid_currmode == VID_MODE_SUPER)) {
                if (vid_currmode == VID_MODE_SUPER) {
                    width = 640;
                    height = 400;
                    VID_outputSetSuperColors();
                } else {
                    width = 560;
                    height = 384;
                    VID_outputSetStandardColors();
                }

                videoSetSize(width, height);
            }
        }
    }
    switch(vid_currmode) {
        case VID_MODE_40TEXT1:    VID_updateRoutine = VID_refreshText40Page1;
                    break;
        case VID_MODE_40TEXT2:    VID_updateRoutine = VID_refreshText40Page2;
                    break;
        case VID_MODE_80TEXT1:    VID_updateRoutine = VID_refreshText80Page1;
                    break;
        case VID_MODE_80TEXT2:    VID_updateRoutine = VID_refreshText80Page2;
                    break;
        case VID_MODE_LORES1:    VID_updateRoutine = VID_refreshLoresPage1;
                    break;
        case VID_MODE_LORES2:    VID_updateRoutine = VID_refreshLoresPage2;
                    break;
        case VID_MODE_DLORES1:    VID_updateRoutine = VID_refreshDLoresPage1;
                    break;
        case VID_MODE_DLORES2:    VID_updateRoutine = VID_refreshDLoresPage2;
                    break;
        case VID_MODE_HIRES1:    VID_updateRoutine = VID_refreshHiresPage1;
                    break;
        case VID_MODE_HIRES2:    VID_updateRoutine = VID_refreshHiresPage2;
                    break;
        case VID_MODE_DHIRES1:    VID_updateRoutine = VID_refreshDHiresPage1;
                    break;
        case VID_MODE_DHIRES2:    VID_updateRoutine = VID_refreshDHiresPage2;
                    break;
        case VID_MODE_SUPER:    VID_updateRoutine = VID_refreshSuperHires;
                    break;
    }
}

void videoSetFullscreen(int value) {
    if (SDL_SetWindowFullscreen(vid_window, value) < 0) {
        printSDLError("Failed to change window mode");
    }
}

void videoSetSize(int w, int h) {
    int full_height = h + (VID_BORDER_SIZE * 2);
    int full_width  = w + (VID_BORDER_SIZE * 2);
    int i;

    if (vid_surface != NULL) {
        SDL_FreeSurface(vid_surface);
    }

    vid_surface = SDL_CreateRGBSurface(0, full_width, full_height, 32, 0, 0, 0, 0);
    if (vid_surface == NULL) {
        printSDLError("Failed to create surface");

        globalShutdown();
    }

    vid_texture = SDL_CreateTexture(vid_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, full_width, full_height);
    if (vid_texture == NULL) {
        printSDLError("Failed to create texture");

        globalShutdown();
    }

    vid_lines[0] = (pixel_t *) vid_surface->pixels + (full_width * VID_BORDER_SIZE) + VID_BORDER_SIZE;

    for (i = 1 ; i < full_height ; i++) {
        vid_lines[i] = vid_lines[i-1] + full_width;
    }
}

void VID_switchPage (int new, int old, int len)
{
    int page, i;
    const int masksize = 8;

    for (page = 0; page<len; page++) {
        mem_slowram_changed[new+page] = mem_slowram_changed[old+page];
        for (i=0; i<0x100; i+= masksize) {
            if (memcmp(slow_memory+((new+page)<<8)+i,
                   slow_memory+((old+page)<<8)+i, masksize))
                mem_slowram_changed[new+page] |= mem_change_masks[i];
        }
    }
}

void VID_redrawScreen()
{
    int    i;

    for (i = 0 ; i < 512 ; i++) mem_slowram_changed[i] = 0xFFFFFFFF;
    vid_currmode = -1;
    videoModeChanged();
}

byte VID_clear80col(byte val)
{
    vid_80col = 0;
    videoModeChanged();
    return 0;
}

byte VID_set80col(byte val)
{
    vid_80col = 1;
    videoModeChanged();
    return 0;
}

byte VID_get80col(byte val)
{
    return vid_80col? 0x80 : 0x00;
}

byte VID_clearAltCh(byte val)
{
    vid_altcharset = 0;
    videoModeChanged();
    return 0;
}

byte VID_setAltCh(byte val)
{
    vid_altcharset = 1;
    videoModeChanged();
    return 0;
}

byte VID_getAltCh(byte val)
{
    return vid_altcharset? 0x80 : 0x00;
}

byte VID_getColorReg(byte val)
{
    return (vid_textfgcolor << 4) | vid_textbgcolor;
}

byte VID_setColorReg(byte val)
{
    vid_textfgcolor = (val >> 4) & 0x0F;
    vid_textbgcolor = val & 0x0F;
    VID_outputSetTextColors();
    VID_redrawScreen();
    return 0;
}

byte VID_getVGCIntReg(byte val)
{
    val = vid_vgcint & 0xF8;
    if (g_onesecirq_enable) val |= 0x04;
    if (g_scanirq_enable) val |= 0x02;
    return val;
}

byte VID_setVGCIntReg(byte val)
{
    g_onesecirq_enable = (val & 0x04)? 1 : 0;
    g_scanirq_enable = (val & 0x02)? 1 : 0;
    return 0;
}

byte VID_clearVGCInt(byte val)
{
    if (!(val & 0x40)) {
        if (vid_vgcint & 0x40) m65816_clearIRQ();
        vid_vgcint &= ~0x40;
    }
    if (!(val & 0x20)) {
        if (vid_vgcint & 0x20) m65816_clearIRQ();
        vid_vgcint &= ~0x20;
    }
    if (!(vid_vgcint & 0x60)) vid_vgcint &= ~0x80;
    return 0;
}

byte VID_getNewVideo(byte val)
{
    val = 0x01;
    if (vid_super) val |= 0x80;
    if (vid_linear) val |= 0x40;
    if (vid_a2mono) val |= 0x20;
    return val;
}

byte VID_setNewVideo(byte val)
{
    vid_super = (val & 0x80)? 1:0;
    vid_linear = (val & 0x40)? 1:0;
    vid_a2mono = (val & 0x20)? 1:0;
    videoModeChanged();
    return 0;
}

byte VID_getVertCnt(byte val)
{
    return ((vid_vert_cnt + 0xFA) >> 1);
}

byte VID_getHorzCnt(byte val)
{
    vid_horiz_cnt = random() & 0x7F;
    val = (vid_vert_cnt & 0x01) << 7;
    return val | vid_horiz_cnt;
}

byte VID_getBorder(byte val)
{
    return vid_bordercolor;
}

byte VID_setBorder(byte val)
{
    if (vid_bordercolor == val) return 0;
    vid_bordercolor = val;
    VID_outputSetBorderColor();
    VID_redrawScreen();
    return 0;
}

byte VID_clearText(byte val)
{
    vid_text = 0;
    videoModeChanged();
    return 0;
}

byte VID_setText(byte val)
{
    vid_text = 1;
    videoModeChanged();
    return 0;
}

byte VID_getText(byte val)
{
    return vid_text? 0x80 : 0x00;
}

byte VID_clearMixed(byte val)
{
    vid_mixed = 0;
    videoModeChanged();
    return 0;
}

byte VID_setMixed(byte val)
{
    vid_mixed = 1;
    videoModeChanged();
    return 0;
}

byte VID_getMixed(byte val)
{
    return vid_mixed? 0x80 : 0x00;
}

byte VID_clearPage2(byte val)
{
    vid_page2 = 0;
    if(mem_80store) {
        MEM_rebuildMainMem();
    } else {
        videoModeChanged();
    }
    return 0;
}

byte VID_setPage2(byte val)
{
    vid_page2 = 1;
    if(mem_80store) {
        MEM_rebuildMainMem();
    } else {
        videoModeChanged();
    }
    return 0;
}

byte VID_getPage2(byte val)
{
    return vid_page2? 0x80 : 0x00;
}

byte VID_clearHires(byte val)
{
    vid_hires = 0;
    videoModeChanged();
    return 0;
}

byte VID_setHires(byte val)
{
    vid_hires = 1;
    videoModeChanged();
    return 0;
}

byte VID_getHires(byte val)
{
    return vid_hires? 0x80 : 0x00;
}

byte VID_clearDblRes(byte val)
{
    vid_dblres = 0;
    videoModeChanged();
    return 0;
}

byte VID_setDblRes(byte val)
{
    vid_dblres = 1;
    videoModeChanged();
    return 0;
}

// FIXME: old pre-SDL stuff

void VID_outputSetTextColors()
{
    vid_textbg = vid_stdcolors[vid_textbgcolor];
    vid_textfg = vid_stdcolors[vid_textfgcolor];
}

void VID_outputSetBorderColor()
{
    vid_border = vid_stdcolors[vid_bordercolor];
}

void VID_outputSetStandardColors()
{
}

void VID_outputSetSuperColors()
{
	int	addr,i;
    int r,g,b;

	addr = 0x019E00;
	for (i = 0 ; i < 256 ; i++) {
		r = (slow_memory[addr+1] & 0x0F) * 17;
		g = ((slow_memory[addr] >> 4) & 0x0F) * 17;
		b = (slow_memory[addr] & 0x0F) * 17;
	    vid_supercolors[i] = (r << 16) | (g << 8) | b;

		addr += 2;
	}
	mem_slowram_changed[0x019E] = 0;
	mem_slowram_changed[0x019F] = 0;
}
