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
 * File: arch/x11/vid-drv.c
 *
 * X11 video driver.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#ifdef MITSHM
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>
#endif

#include "video.h"
#include "vid-drv.h"

Screen		*vid_screen;
Display		*vid_display;
Visual		*vid_visual;
XImage		*vid_image;
Window		vid_root,vid_main_window,vid_window;
GC		vid_gc;
Colormap	vid_colormap;
Atom		wm_delete;
XFontStruct	*vid_fontinfo;
#ifdef MITSHM
XShmSegmentInfo	vid_shminfo;
#endif

PIXEL		*vid_lines[VID_WIN_HEIGHT];
PIXEL		*vid_buffer;

int		vid_shm;
int		vid_mapped;

XColor	vid_supercolors[256];
XColor	vid_textcolors[16] = {
	{ 0, 0x0000, 0x0000, 0x0000, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xDDDD, 0x0000, 0x3333, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x0000, 0x0000, 0x9999, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xDDDD, 0x2222, 0xDDDD, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x0000, 0x7777, 0x2222, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x5555, 0x5555, 0x5555, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x2222, 0x2222, 0xFFFF, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x6666, 0xAAAA, 0xFFFF, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x8888, 0x5555, 0x0000, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xFFFF, 0x6666, 0x0000, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xAAAA, 0xAAAA, 0xAAAA, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xFFFF, 0x9999, 0x8888, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x0000, 0xDDDD, 0x0000, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xFFFF, 0xFFFF, 0x0000, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0x5555, 0xFFFF, 0x9999, DoRed | DoGreen | DoBlue, 0 },
	{ 0, 0xFFFF, 0xFFFF, 0xFFFF, DoRed | DoGreen | DoBlue, 0 },
};

#ifdef MITSHM
/*
 * Error handling.
 */

int vid_xerrorflag = 0;

int VID_handleXError(Display *dpy, XErrorEvent *event)
{
	vid_xerrorflag = 1;
	return 0;
}
#endif

/*
 * Check if the X Shared Memory extension is available.
 * Return:  0 = not available
 *          1 = shared XImage support available
 *          2 = shared Pixmap support available also
 */
int VID_checkForSHM(Display *display)
{
#ifdef MITSHM
	int	major, minor, ignore;
	Bool	pixmaps;

	if (XQueryExtension(display, "MIT-SHM", &ignore, &ignore, &ignore)) {
		if (XShmQueryVersion(display, &major, &minor, &pixmaps)==True) {
			return (pixmaps==True) ? 2 : 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
#else
	return 0;
#endif
}

/*
 * Allocate a shared memory XImage back buffer for the given context.
 * Return:  1 if success, 0 if error
 */

int VID_allocShmImage(void)
{
#ifdef MITSHM
   /*
    * We have to do a _lot_ of error checking here to be sure we can
    * really use the XSHM extension.  It seems different servers trigger
    * errors at different points if the extension won't work.  Therefore
    * we have to be very careful...
    */
	GC	gc;
	int	(*old_handler)(Display *, XErrorEvent *);

	vid_image = XShmCreateImage(vid_display, vid_visual, VID_DEPTH,
					ZPixmap, NULL, &vid_shminfo,
					VID_WIDTH, VID_HEIGHT);

	if (vid_image == NULL) {
		vid_shm = 0;
		return 0;
	}

	vid_shminfo.shmid = shmget(IPC_PRIVATE, vid_image->bytes_per_line
					*vid_image->height, IPC_CREAT|0777);
	if (vid_shminfo.shmid < 0) {
		perror("VID_allocShmImage");
		XDestroyImage(vid_image);
		vid_image = NULL;
		vid_shm = 0;
		return 0;
	}

	vid_shminfo.shmaddr = vid_image->data = (char*)shmat(vid_shminfo.shmid, 0, 0);

	if (vid_shminfo.shmaddr == (char *) -1) {
		XDestroyImage(vid_image);
		vid_image = NULL;
		vid_shm = 0;
		return 0;
	}

	vid_shminfo.readOnly = False;
	vid_xerrorflag = 0;
	old_handler = XSetErrorHandler(VID_handleXError);

	/* This may trigger the X protocol error we're ready to catch: */

	XShmAttach(vid_display, &vid_shminfo);
	XSync(vid_display, False);

	/* we are on a remote display, this error is normal, don't print it */

	if (vid_xerrorflag) {
		XFlush(vid_display);
		vid_xerrorflag = 0;
		XDestroyImage(vid_image);
		shmdt(vid_shminfo.shmaddr);
		shmctl(vid_shminfo.shmid, IPC_RMID, 0);
		vid_image = NULL;
		vid_shm = 0;
		XSetErrorHandler(old_handler);
		return 0;
	}

	shmctl(vid_shminfo.shmid, IPC_RMID, 0); /* nobody else needs it */

   /* Finally, try an XShmPutImage to be really sure the extension works */

	gc = XCreateGC(vid_display, vid_window, 0, NULL);
	XShmPutImage(vid_display, vid_window, gc,
			vid_image, 0, 0, 0, 0, 1, 1 /*one pixel*/, False);
	XSync(vid_display, False);
	XFreeGC(vid_display, gc);
	XSetErrorHandler(old_handler);
	if (vid_xerrorflag) {
		XFlush(vid_display);
		vid_xerrorflag = 0;
		XDestroyImage(vid_image);
		shmdt(vid_shminfo.shmaddr);
		shmctl(vid_shminfo.shmid, IPC_RMID, 0);
		vid_image = NULL;
		vid_shm = 0;
		return 0;
	}

	vid_buffer = (PIXEL *) vid_shminfo.shmaddr;
	return 1;
#else
	return 0;
#endif
}

int VID_outputInit(void)
{
	XSizeHints	size_hints;
	XWMHints	wm_hints;

	XVisualInfo	vTemplate;
	XVisualInfo	*visualList;
	int		visualsMatched;
	int		visual_chosen;
	XSetWindowAttributes win_attr;
	XGCValues	gcvals;

	int		i;

	printf("    - Opening display: ");
	vid_display = XOpenDisplay(NULL);
	if (!vid_display) {
		printf("Failed\n");
		return 1;
	}
	vid_screen = DefaultScreenOfDisplay(vid_display);
	vid_black = BlackPixelOfScreen(vid_screen);
	vid_white = WhitePixelOfScreen(vid_screen);
	printf("Done\n");

	printf("    - Picking a visual: ");
	vTemplate.screen = DefaultScreen(vid_display);
	vTemplate.depth = VID_DEPTH;
	visualList = XGetVisualInfo(vid_display,
		VisualScreenMask | VisualDepthMask,
		&vTemplate, &visualsMatched);
	if(visualsMatched == 0) {
		fprintf(stderr, "no visuals!\n");
		return 1;
	}
	visual_chosen = -1;
	for(i = 0; i < visualsMatched; i++) {
		if (visualList[i].class == VID_CLASS) {
			visual_chosen = i;
			break;
		}
	}
	if (visual_chosen < 0) {
		printf("Failed\n");
		return 1;
	} else {
		printf("id %08x, screen %d, depth %d, class %d\n", (int) visualList[visual_chosen].visualid, visualList[visual_chosen].screen, visualList[visual_chosen].depth, visualList[visual_chosen].class);
	}
	vid_visual = visualList[visual_chosen].visual;

	printf("    - Creating main window: ");
	vid_root = RootWindowOfScreen(vid_screen);
	vid_colormap = XCreateColormap(vid_display, vid_root, vid_visual, AllocAll);
	win_attr.border_pixel = 255;
	win_attr.background_pixel = 255;
	win_attr.colormap = vid_colormap;
	vid_mapped = 0;
	vid_main_window = XCreateWindow(vid_display, vid_root, 0, 0,
				VID_WIN_WIDTH,VID_WIN_HEIGHT, 0, vTemplate.depth, InputOutput,
				vid_visual, CWColormap | CWBackPixel | CWBorderPixel,
				&win_attr);
	if (!vid_main_window) {
		printf("Failed\n");
		return 1;
	}
	printf("Done\n");

	printf("    - Creating display subwindow: ");
	win_attr.border_pixel = vid_white;
	win_attr.background_pixel = vid_black;
	win_attr.colormap = vid_colormap;
	vid_window = XCreateWindow(vid_display, vid_main_window,
			(VID_WIN_WIDTH - VID_WIDTH) / 2, (VID_WIN_HEIGHT - VID_HEIGHT) / 2,
			VID_WIDTH, VID_HEIGHT, 0, vTemplate.depth, InputOutput,
			vid_visual, CWColormap | CWBackPixel | CWBorderPixel,
			&win_attr);
	if (!vid_window) {
		printf("Failed\n");
		return 1;
	}
	printf("Done\n");

	XStoreName(vid_display, vid_main_window, VERSION);
	size_hints.flags = PSize | PMinSize | PMaxSize;
	size_hints.base_width = VID_WIN_WIDTH;
	size_hints.min_width = VID_WIN_WIDTH;
	size_hints.max_width = VID_WIN_WIDTH;
	size_hints.base_height = VID_WIN_HEIGHT;
	size_hints.min_height = VID_WIN_HEIGHT;
	size_hints.max_height = VID_WIN_HEIGHT;
	wm_hints.input = True;
	wm_hints.flags = InputHint;
	XSetWMNormalHints(vid_display, vid_main_window, &size_hints);
	XSetWMHints(vid_display, vid_main_window, &wm_hints);
	/* We will handle WM_DELETE window manager messages */
	wm_delete = XInternAtom(vid_display,"WM_DELETE_WINDOW",True);
	if(wm_delete)
	  XSetWMProtocols(vid_display, vid_main_window, &wm_delete, 1);
	XSelectInput(vid_display,vid_main_window,ExposureMask|StructureNotifyMask);
	XSelectInput(vid_display,vid_window,ExposureMask|KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask);
	XMapRaised(vid_display, vid_main_window);
	XMapRaised(vid_display, vid_window);

	printf("    - Setting status font: ");
	vid_gc = XCreateGC(vid_display, vid_main_window, 0, NULL);
	vid_fontinfo = XLoadQueryFont(vid_display,VID_STATUS_FONT);
	gcvals.font = vid_fontinfo->fid;
	gcvals.fill_style = FillSolid;
	XChangeGC(vid_display, vid_gc, GCFillStyle | GCFont, &gcvals);
	printf("Done\n");

#ifdef MITSHM
	printf("    - Checking for MIT-SHM support: ");
	vid_shm = VID_checkForSHM(vid_display);
	if (vid_shm) {
		printf("Done\n");
	} else {
		printf("Failed\n");
	}
	printf("    - Creating shared image: ");
	if (VID_allocShmImage()) {
		printf("Done\n");
	} else {
		printf("Failed! SHM disabled.\n");
	}
#else
	vid_shm = 0;
#endif
	if (!vid_shm) {
		printf("    - Creating Image: ");
		vid_image = XCreateImage(vid_display, vid_visual, VID_DEPTH,
						ZPixmap, 0,   /* format, offset */
						NULL, VID_WIDTH, VID_HEIGHT,
						sizeof(PIXEL)*8, 0);  /* pad, bytes_per_line */
		if (!vid_image) {
			printf("Failed\n");
			return 1;
		}
		vid_image->data = (char *) malloc(vid_image->height
					       * vid_image->bytes_per_line);
		if (!vid_image->data) {
			printf("Failed\n");
			XDestroyImage(vid_image);
			return 1;
		}
	    
		vid_buffer = vid_image->data;
		printf("Done\n");
	}

	vid_lines[0] = vid_buffer;
	for (i = 1 ; i < VID_HEIGHT ; i++) {
		vid_lines[i] = vid_lines[i-1] + VID_WIDTH;
	}

	return 0;
}

void VID_outputImage()
{
	if (!vid_mapped) return;

	vid_xmin = VID_WIDTH;
	vid_ymin = VID_HEIGHT;
	vid_xmax = vid_ymax = 0;

	(*VID_updateRoutine)();

	if ((vid_xmax > vid_xmin) && (vid_ymax > vid_ymin)) {
#ifdef MITSHM
		if (vid_shm) {
		    XShmPutImage(vid_display, vid_window, vid_gc, vid_image, vid_xmin, vid_ymin, vid_xmin, vid_ymin, vid_xmax - vid_xmin, vid_ymax - vid_ymin, False);
		} else
#endif
		{
		    XPutImage(vid_display, vid_window, vid_gc, vid_image, vid_xmin, vid_ymin, vid_xmin, vid_ymin, vid_xmax - vid_xmin, vid_ymax - vid_ymin);
		}
		XSync(vid_display, False);
	}
}

void VID_outputShutdown()
{
	printf("    - Destroying image\n");
#ifdef MITSHM
	if (vid_shm) {
	    XShmDetach(vid_display, &vid_shminfo);
	    XDestroyImage(vid_image);
	    shmdt(vid_shminfo.shmaddr);
	    shmctl(vid_shminfo.shmid,IPC_RMID,0);
	} else
#endif
	{
	    XDestroyImage(vid_image);
	    vid_image = NULL;
	}
	printf("    - Closing display\n");
	XFreeColormap(vid_display, vid_colormap);
	XCloseDisplay(vid_display);
}

void VID_outputStatus1(char *msg)
{
	int	height,margin;

	height = vid_fontinfo->ascent + vid_fontinfo->descent;
	margin = vid_fontinfo->ascent;

	XSetForeground(vid_display, vid_gc, vid_white);
	XSetBackground(vid_display, vid_gc, vid_black);

	XDrawImageString(vid_display, vid_main_window, vid_gc, 0, VID_WIN_HEIGHT-height*2-margin, msg, strlen(msg));
}

void VID_outputStatus2(char *msg)
{
	int	height,margin;

	height = vid_fontinfo->ascent + vid_fontinfo->descent;
	margin = vid_fontinfo->ascent;

	XSetForeground(vid_display, vid_gc, vid_white);
	XSetBackground(vid_display, vid_gc, vid_black);
	XDrawImageString(vid_display, vid_main_window, vid_gc, 0, VID_WIN_HEIGHT-height-margin, msg, strlen(msg));
}

void VID_outputResize(int width, int height)
{
	XMoveResizeWindow(vid_display, vid_window,
			  (VID_WIN_WIDTH - width) / 2,
			  (VID_WIN_HEIGHT - height) / 2,
			  width, height);
	XClearWindow(vid_display, vid_main_window);
}

void VID_outputSetTextColors()
{
	vid_textcolors[vid_textbgcolor].pixel = 253;
	XStoreColor(vid_display, vid_colormap, &vid_textcolors[vid_textbgcolor]);
	vid_textcolors[vid_textfgcolor].pixel = 254;
	XStoreColor(vid_display, vid_colormap, &vid_textcolors[vid_textfgcolor]);
}

void VID_outputSetBorderColor()
{
	vid_textcolors[vid_bordercolor].pixel = 255;
	XStoreColor(vid_display, vid_colormap, &vid_textcolors[vid_bordercolor]);
}

void VID_outputSetStandardColors()
{
	int	i;

	vid_textcolors[vid_textbgcolor].pixel = 253;
	XStoreColor(vid_display, vid_colormap, &vid_textcolors[vid_textbgcolor]);
	vid_textcolors[vid_textfgcolor].pixel = 254;
	XStoreColor(vid_display, vid_colormap, &vid_textcolors[vid_textfgcolor]);
	vid_textcolors[vid_bordercolor].pixel = 255;
	XStoreColor(vid_display, vid_colormap, &vid_textcolors[vid_bordercolor]);
	for (i = 0 ; i <= 15 ; i++) {
		vid_textcolors[i].pixel = i;
	}
	XStoreColors(vid_display, vid_colormap, vid_textcolors, 16);
	vid_black = 0;
	vid_white = 15;
}

void VID_outputSetSuperColors()
{
	int	addr,i;

	addr = 0x019E00;
	for (i = 0 ; i < 256 ; i++) {
		if (mem_slowram_changed[addr >> 8] & mem_change_masks[addr & 0xFF]) {
			vid_supercolors[i].blue = (slow_memory[addr] & 0x0F) * 4369;
			vid_supercolors[i].green = ((slow_memory[addr] >> 4) & 0x0F) * 4369;
			vid_supercolors[i].red = (slow_memory[addr+1] & 0x0F) * 4369;
			vid_supercolors[i].flags = DoRed | DoGreen | DoBlue;
			vid_supercolors[i].pixel = i;
			XStoreColor(vid_display, vid_colormap, &vid_supercolors[i]);
		}
		addr += 2;
	}
	mem_slowram_changed[0x019E] = 0;
	mem_slowram_changed[0x019F] = 0;
}
