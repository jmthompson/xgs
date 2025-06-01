#pragma once

/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "m65816.h"

constexpr unsigned int kMaxInstLen = 4;
constexpr int kLenIsMemorySize = -1;
constexpr int kLenIsIndexSize  = -2;

// Addressing modes

constexpr unsigned int kAddrMode_immediate   = 1;
constexpr unsigned int kAddrMode_a           = 2;
constexpr unsigned int kAddrMode_al          = 3;
constexpr unsigned int kAddrMode_d           = 4;
constexpr unsigned int kAddrMode_accumulator = 5;
constexpr unsigned int kAddrMode_implied     = 6;
constexpr unsigned int kAddrMode_dix         = 7;
constexpr unsigned int kAddrMode_dixl        = 8;
constexpr unsigned int kAddrMode_dxi         = 9;
constexpr unsigned int kAddrMode_dxx         = 10;
constexpr unsigned int kAddrMode_dxy         = 11;
constexpr unsigned int kAddrMode_axx         = 12;
constexpr unsigned int kAddrMode_alxx        = 13;
constexpr unsigned int kAddrMode_axy         = 14;
constexpr unsigned int kAddrMode_pcr         = 15;
constexpr unsigned int kAddrMode_pcrl        = 16;
constexpr unsigned int kAddrMode_ai          = 17;
constexpr unsigned int kAddrMode_ail         = 171; // same as _ai in the data sheet but acts differently
constexpr unsigned int kAddrMode_di          = 18;
constexpr unsigned int kAddrMode_dil         = 19;
constexpr unsigned int kAddrMode_axi         = 20;
constexpr unsigned int kAddrMode_stack       = 21;
constexpr unsigned int kAddrMode_sr          = 22;
constexpr unsigned int kAddrMode_srix        = 23;
constexpr unsigned int kAddrMode_blockmove   = 24;

std::uint8_t debugRead(const uint8_t, const uint16_t, const uint8_t, const m65816::mem_access_t);
std::uint8_t debugWrite(const uint8_t, const uint16_t, const uint8_t, const m65816::mem_access_t);
