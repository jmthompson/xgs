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
 * File: arch/svgalib/adb-drv.c
 *
 * SVGAlib ADB driver.
 */

#include "xgs.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "adb.h"
#include "adb-drv.h"
#include "emul.h"
#include "video.h"
#include "vid-drv.h"

int ADB_inputInit()
{
	return 0;
}

void ADB_inputUpdate()
{
#if 0
	XEvent	event;
	int 	count,i,len;
	word16	key;
	char	buffer[4];
  
 	for( count = XPending( vid_display ); count > 0; count-- ) {
 		XNextEvent( vid_display, &event );
 		switch( event.xany.type ) {
 		case ClientMessage:
 			if((Atom)event.xclient.data.l[0] == wm_delete) EMUL_shutdown();
 			break;
 		case DestroyNotify:
 			EMUL_shutdown();
 			break;
		case MapNotify:
			if (event.xmap.window == vid_main_window)
				vid_mapped = 1;
			break;
		case UnmapNotify:
			if (event.xmap.window == vid_main_window)
				vid_mapped = 0;
			break;
 		case KeyPress:
 			key = XLookupKeysym((XKeyEvent *) &event, 0);
 			switch(key) {
 			case XK_F5 :
 				if (adb_grab_mode == 1) {
 					adb_grab_mode = 0;
 					XUngrabPointer(vid_display, CurrentTime);
 				} else if (adb_grab_mode == 0) {
 					adb_grab_mode = 1;
 					XGrabPointer(vid_display,vid_window,True,NoEventMask,
 						     GrabModeAsync, GrabModeAsync,
 						     vid_window, None, CurrentTime);
 				}
 				continue;
 			case XK_F6 :
 				if (adb_grab_mode == 2) {
 					adb_grab_mode = 0;
 					XUngrabPointer(vid_display, CurrentTime);
 				} else if (adb_grab_mode == 0) {
 					adb_grab_mode = 2;
 					XGrabPointer(vid_display,vid_window,True,NoEventMask,
 						     GrabModeAsync, GrabModeAsync,
 						     vid_window, None, CurrentTime);
					ski_lastx = VID_WIDTH/2;
					ski_lasty = VID_HEIGHT/2;
					XWarpPointer(vid_display, None, vid_window, 0, 0, 0, 0, ski_lastx, ski_lasty);
 				}
 				continue;
 			case XK_Caps_Lock :
 				ski_modifier_reg |= 0x04;
 				continue;
			case XK_Shift_L:
 			case XK_Shift_R:
 				ski_modifier_reg |= 0x01;
 				continue;
			case XK_Control_L:
 			case XK_Control_R:
 				ski_modifier_reg |= 0x02;
 				continue;
 			case XK_Alt_L:
 				ski_modifier_reg |= 0x80;
 				continue;
 			case XK_Alt_R:
 				ski_modifier_reg |= 0x40;
 				continue;
 			case XK_Left:
 				buffer[0] = 0x88;
 				break;
 			case XK_Right:
 				buffer[0] = 0x95;
 				break;
 			case XK_Up:
 				buffer[0] = 0x8B;
 				break;
 			case XK_Down:
 				buffer[0] = 0x8A;
 				break;
 			case XK_Escape:
 				if ((ski_modifier_reg & 0x82) == 0x82) {
					if (!ski_status_irq) {
						ski_status_irq = 0x20;
 						buffer[0] = 0x1B;
 						CPU_addIRQ();
					}
 				} else {
 					buffer[0] = 0x1B;
 				}
 				break;
 			default:
 				len = XLookupString((XKeyEvent *) &event,buffer,4,NULL,NULL);
 				if (!len)
 					continue;
 				break;
 			}
 			ski_input_buffer[ski_input_index++] = buffer[0] & 0x7F;
 			if (ski_input_index == ADB_INPUT_BUFFER) ski_input_index = 0;
 			break;
 		case KeyRelease:
 			key = XLookupKeysym((XKeyEvent *) &event, 0);
 			switch(key) {
 			case XK_Caps_Lock :
 				ski_modifier_reg &= ~0x04;
 				break;
			case XK_Shift_L:
 			case XK_Shift_R:
 				ski_modifier_reg &= ~0x01;
 				break;
			case XK_Control_L:
 			case XK_Control_R:
 				ski_modifier_reg &= ~0x02;
 				break;
 			case XK_Alt_L:	
 				ski_modifier_reg &= ~0x80;
 				break;
 			case XK_Alt_R:
 				ski_modifier_reg &= ~0x40;
 				break;
 			case XK_Prior:
 				if (ski_modifier_reg & 0x02) EMUL_trace(1);
 				break;
 			case XK_Next:
 				if (ski_modifier_reg & 0x02) EMUL_trace(0);
 				break;
 			case XK_Home:
 				if (ski_modifier_reg & 0x02) EMUL_reset();
 				break;
 			case XK_End:
 				if (ski_modifier_reg & 0x02) EMUL_shutdown();
 				break;
 			case XK_Pause:
				EMUL_nmi();
 				break;
 			default:
 				break;
 			}
 			break;
 		case ButtonPress:
 			if (adb_grab_mode == 1) {		/* Joystick */
 				switch(event.xbutton.button) {
					case Button1 :	ski_modifier_reg |= 0x80;
 							break;
					case Button3 :	ski_modifier_reg |= 0x40;
 							break;
					default :	break;
 				}
 			} else if (adb_grab_mode == 2) {	/* Mouse */
 				if (ski_status_reg & 0x80) return;	/* Mouse reg still full */
 				switch(event.xbutton.button) {
					case Button1 :	ski_button0 = 1;
 							break;
					case Button3 :	ski_button1 = 1;
 							break;
					default :	break;
 				}
 				ski_xdelta = 0;
 				ski_ydelta = 0;
 				if (ski_status_reg & 0x40) CPU_addIRQ();
 				ski_status_reg |= 0x80;
 				ski_status_reg &= ~0x02;
			}
 			break;
 		case ButtonRelease:
 			if (adb_grab_mode == 1) {		/* Joystick */
 				switch(event.xbutton.button) {
					case Button1 :	ski_modifier_reg &= ~0x80;
 							break;
					case Button3 :	ski_modifier_reg &= ~0x40;
 							break;
					default :	break;
 				}
 			} else if (adb_grab_mode == 2) {	/* Mouse */
 				if (ski_status_reg & 0x80) return;	/* Mouse reg still full */
 				switch(event.xbutton.button) {
					case Button1 :	ski_button0 = 0;
 							break;
					case Button3 :	ski_button1 = 0;
 							break;
					default :	break;
 				}
 				ski_xdelta = 0;
 				ski_ydelta = 0;
 				if (ski_status_reg & 0x40) CPU_addIRQ();
 				ski_status_reg |= 0x80;
 				ski_status_reg &= ~0x02;
			}
 			break;
 		case MotionNotify:
 			while (XCheckTypedWindowEvent(vid_display, vid_window,
 						      MotionNotify, &event))
 				count--;
 			if (adb_grab_mode == 1) {		/* Joystick */
 				adb_pdl0 = (int) (event.xmotion.x / 2.2);
 				adb_pdl1 = (int) (event.xmotion.y / 1.5);
				if (adb_pdl0 > 255) adb_pdl0 = 255;
				if (adb_pdl1 > 255) adb_pdl1 = 255;
 				if (event.xmotion.state & Button1Mask) {
 					ski_modifier_reg |= 0x80;
 				} else {
 					ski_modifier_reg &= ~0x80;
 				}
 				if (event.xmotion.state & Button3Mask) {
 					ski_modifier_reg |= 0x40;
 				} else {
 					ski_modifier_reg &= ~0x40;
 				}
 			} else if (adb_grab_mode == 2) {	/* Mouse */
 				if (ski_status_reg & 0x80) return;	/* Mouse reg still full */
 				ski_xdelta = event.xmotion.x - ski_lastx;
 				ski_ydelta = event.xmotion.y - ski_lasty;
				if (ski_xdelta || ski_ydelta)
					XWarpPointer(vid_display, None, vid_window, 0, 0, 0, 0, ski_lastx, ski_lasty);
 				ski_button0 = (event.xmotion.state & Button1Mask)? 1 : 0;
 				ski_button1 = (event.xmotion.state & Button3Mask)? 1 : 0;
 				if (ski_status_reg & 0x40) CPU_addIRQ();
 				ski_status_reg |= 0x80;
 				ski_status_reg &= ~0x02;
			}
 			break;
 		case Expose:
 			while (XCheckTypedEvent(vid_display, Expose, &event)) count--;
 			VID_redrawScreen();
 			break;
		}
	}
#endif
}

void ADB_inputShutdown()
{
}
