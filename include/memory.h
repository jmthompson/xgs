#pragma once

#include <cstdint>
#include "m65816.h"

constexpr unsigned int kPageSize = 256;
constexpr unsigned int kBankSize = 65536;
constexpr unsigned int kNumPages = 65536;
constexpr unsigned int kNumBanks = 256;
constexpr unsigned int kPagesPerBank = 256;
constexpr unsigned int kMaxPage = kNumPages - 1;

// The I/O page sits above addressable memory and can
// only be accessed via a read or write mapping.
constexpr unsigned int kIOPage = kMaxPage + 1;

enum mem_page_t { UNMAPPED = 0, ROM, FAST, SLOW };

typedef uint8_t (ioHandler)(const uint8_t, const uint8_t);

void setupMemory(void);
void freeMemory(void);
void installRom(uint8_t *, const uint16_t, const uint16_t);
void setIoReadHandler(const uint8_t, ioHandler);
void setIoWriteHandler(const uint8_t, ioHandler);
void updateMemoryMaps(void);
uint8_t *getPageReadPointer(uint16_t);
uint8_t sysRead(const uint8_t, const uint16_t);
void sysWrite(const uint8_t, const uint16_t, uint8_t);
uint8_t cpuRead(const uint8_t, const uint16_t, const m65816::mem_access_t);
void cpuWrite(const uint8_t, const uint16_t, uint8_t, const m65816::mem_access_t);