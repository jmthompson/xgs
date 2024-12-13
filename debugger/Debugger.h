#pragma once

/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "M65816/m65816.h"

namespace xgs::Debugger {

constexpr unsigned int kMaxInstLen = 4;

extern void enableTrace();
extern void disableTrace();
extern void toggleTrace();
extern std::uint8_t memoryRead(const uint8_t, const uint16_t, const uint8_t, const m65816::mem_access_t);
extern std::uint8_t memoryWrite(const uint8_t, const uint16_t, const uint8_t, const m65816::mem_access_t);

} // namespace xgs::Debugger