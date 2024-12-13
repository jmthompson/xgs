/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This file defines macros used by the rest of
 * the code for interfacing to the host platform.
 */
#include "emulator/xgs.h"

#define MREAD(b,a,t) xgs::cpuRead(b, a, t)
#define MWRITE(b,a, v, t) xgs::cpuWrite(b, a, v, t)
#define WDM(v) xgs::handleWdm(v)
