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
 * File: arch/win32/adb-drv.c
 *
 * Win32 ADB driver.
 */

#define WIN32_LEAN_AND_MEAN

#include "xgs.h"

#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <windows.h>
#include <windowsx.h>

#include "main.h"

#include "adb.h"
#include "adb-drv.h"
#include "emul.h"
#include "video.h"
#include "vid-drv.h"

extern HINSTANCE	hInst;
extern HWND			hWnd;
extern HANDLE		hAccelTable;

int ADB_inputInit()
{
	return 0;
}

void ADB_inputUpdate()
{
	MSG	msg;

	if (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

void ADB_inputShutdown()
{
}

void ADB_inputMotionNotify(int X, int Y) {
	POINT Offset;

	if (adb_grab_mode == 1) {
 		adb_pdl0 = (int) (X / 3.0);
 		adb_pdl1 = (int) (Y / 2.1);
		if (adb_pdl0 > 255) adb_pdl0 = 255;
		if (adb_pdl1 > 255) adb_pdl1 = 255;
	} else if (adb_grab_mode == 2) {
		if (ski_status_reg & 0x80) return;	// Mouse reg still full
 		ski_xdelta = X - ski_lastx;
 		ski_ydelta = Y - ski_lasty;
		Offset.x = ski_lastx;
		Offset.y = ski_lasty;
		ClientToScreen(hWnd, &Offset);

		if (ski_xdelta || ski_ydelta)
			SetCursorPos(Offset.x, Offset.y);
 		if (ski_status_reg & 0x40) CPU_addIRQ();
 		ski_status_reg |= 0x80;
 		ski_status_reg &= ~0x02;
	}
}

void ADB_inputKeyDown(int keycode, int isvirt) {
	POINT	Offset;
	char	keyout;

	if (isvirt) {
		keyout = 0;
		switch(keycode) {
			case VK_LEFT:
				keyout = 0x08;
				break;
			case VK_RIGHT:
				keyout = 0x15;
				break;
			case VK_UP:
				keyout = 0x0B;
				break;
			case VK_DOWN:
				keyout = 0x0A;
				break;
			case VK_F5:
				if (adb_grab_mode == 1) {
 					adb_grab_mode = 0;
 					ReleaseCapture();
					ShowCursor(1);
 				} else if (adb_grab_mode == 0) {
 					adb_grab_mode = 1;
					ShowCursor(0);
					SetCapture(hWnd);
					ski_lastx = VID_WIDTH/2;
					ski_lasty = VID_HEIGHT/2;
					Offset.x = ski_lastx;
					Offset.y = ski_lasty;
					ClientToScreen(hWnd, &Offset);
					SetCursorPos(Offset.x, Offset.y);
 				}			
				break;
			case VK_F6:
				if (adb_grab_mode == 2) {
 					adb_grab_mode = 0;
 					ReleaseCapture();
					ShowCursor(1);
 				} else if (adb_grab_mode == 0) {
 					adb_grab_mode = 2;
					ShowCursor(0);
					SetCapture(hWnd);
					ski_lastx = VID_WIDTH/2;
					ski_lasty = VID_HEIGHT/2;
					Offset.x = ski_lastx;
					Offset.y = ski_lasty;
					ClientToScreen(hWnd, &Offset);
					SetCursorPos(Offset.x, Offset.y);
 				}			
				break;
			case VK_PRIOR:
				if (ski_modifier_reg & 0x02) EMUL_trace(1);
				break;
			case VK_NEXT:
				if (ski_modifier_reg & 0x02) EMUL_trace(0);
				break;
			case VK_HOME:
				if (ski_modifier_reg & 0x02) EMUL_reset();
				break;
			case VK_END:
				if (ski_modifier_reg & 0x02) EMUL_shutdown();
				break;
			case VK_PAUSE:
				EMUL_nmi();
				break;
			case VK_CAPITAL:
				ski_modifier_reg |= 0x04;
				break;
			case VK_SHIFT:
				ski_modifier_reg |= 0x01;
				break;
			case VK_CONTROL:
				ski_modifier_reg |= 0x02;
				break;
			case VK_F1:
				if ((ski_modifier_reg & 0x82) == 0x82) {
					if (!ski_status_irq) {
						ski_status_irq = 0x20;
						keycode = 0x1B;
						CPU_addIRQ();
					}
				} else {
					keyout = 0x1B;
				}
				break;
			case VK_F3:
				ski_modifier_reg |= 0x80;
				break;
			case VK_F4:
				ski_modifier_reg |= 0x40;
				break;
			case VK_F12:
				keyout = 0x8D; /* keypad enter key */
				break;
			default:
				break;
		}
	} else {
		keyout = keycode & 0x7F;
	}
	if (keyout) {
		ski_input_buffer[ski_input_index++] = keyout;
		if (ski_input_index == ADB_INPUT_BUFFER) ski_input_index = 0;
	}
}

void ADB_inputKeyRelease(int keycode, int isvirt) {
	if (isvirt) {
		switch(keycode) {
		 	case VK_CAPITAL:
 				ski_modifier_reg &= ~0x04;
				break;
	 		case VK_SHIFT:
 				ski_modifier_reg &= ~0x01;
				break;
			case VK_CONTROL:
 				ski_modifier_reg &= ~0x02;
	 			break;
			case VK_F3:
				ski_modifier_reg &= ~0x80;
				break;
			case VK_F4:
				ski_modifier_reg &= ~0x40;
				break;
		}
	}
}

void ADB_inputRightMouseDown() {
	if (adb_grab_mode == 1) {
 		ski_modifier_reg |= 0x40;
	} else if (adb_grab_mode == 2) {
		ski_button1 = 1;
 		if (ski_status_reg & 0x80) return;	// Mouse reg still full
 		ski_xdelta = 0;
 		ski_ydelta = 0;
 		if (ski_status_reg & 0x40) CPU_addIRQ();
 		ski_status_reg |= 0x80;
 		ski_status_reg &= ~0x02;
	}
}

void ADB_inputRightMouseUp() {
 	if (adb_grab_mode == 1) {
		ski_modifier_reg &= ~0x40;
	} else if (adb_grab_mode == 2) {
		ski_button1 = 0;
 		if (ski_status_reg & 0x80) return;	// Mouse reg still full
 		ski_xdelta = 0;
 		ski_ydelta = 0;
 		if (ski_status_reg & 0x40) CPU_addIRQ();
 		ski_status_reg |= 0x80;
 		ski_status_reg &= ~0x02;
	}
}

void ADB_inputLeftMouseDown() {
	if (adb_grab_mode == 1) {
 		ski_modifier_reg |= 0x80;
	} else if (adb_grab_mode == 2) {
		ski_button0 = 1;
 		if (ski_status_reg & 0x80) return;	// Mouse reg still full
 		ski_xdelta = 0;
 		ski_ydelta = 0;
 		if (ski_status_reg & 0x40) CPU_addIRQ();
 		ski_status_reg |= 0x80;
 		ski_status_reg &= ~0x02;
	}
}

void ADB_inputLeftMouseUp() {
	if (adb_grab_mode == 1) {
		ski_modifier_reg &= ~0x80;
	} else if (adb_grab_mode == 2) {
		ski_button0 = 0;
 		if (ski_status_reg & 0x80) return;	// Mouse reg still full
 		ski_xdelta = 0;
 		ski_ydelta = 0;
 		if (ski_status_reg & 0x40) CPU_addIRQ();
 		ski_status_reg |= 0x80;
 		ski_status_reg &= ~0x02;
	}
}
