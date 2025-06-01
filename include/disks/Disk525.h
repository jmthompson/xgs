#pragma once

/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "disks/DiskTrack.h"
#include "disks/VirtualDisk.h"
#include "m65816.h"
#include <cstdint>
#include <cstdlib>

class Disk525
{
public:
  static constexpr unsigned int kNumTracks = 35;
  static constexpr unsigned int kLastTrack = (kNumTracks - 1) * 4;

  static constexpr unsigned int kSectorSize = 256;
  static constexpr unsigned int kSectorsPerTrack = 16;
  static constexpr unsigned int kBlocksPerTrack = kSectorsPerTrack / 2;
  static constexpr unsigned int kBytesPerTrack = kSectorsPerTrack * kSectorSize;
  static constexpr unsigned int kNibblesPerSector = 342;
  static constexpr unsigned int kNibblesPerTrack = 6656;

  Disk525();
  ~Disk525();

  uint8_t status();

  uint8_t read(const m65816::cycles_t);
  void write(const m65816::cycles_t, const uint8_t);
  void flush();

  void load(VirtualDisk *);
  void unload();

  void loadTrack(DiskTrack &);
  void flushTrack(DiskTrack &);

  void phaseChange(const unsigned int);

private:
  VirtualDisk *vdisk = nullptr;
  DiskTrack tracks[kNumTracks * 4];

  // The cpu cycle count at which the disk was read or written.
  m65816::cycles_t last_access = 0;

  // The fractional track # over which the virtual disk head currently rests.
  int current_track = 0;
  unsigned int vol_num = 254;

  unsigned int last_phase = 0;
  unsigned int nib_pos = 0;

  uint8_t track_buffer[kSectorsPerTrack * kSectorSize];
  uint8_t nib_buff[kNibblesPerSector + 1];

  void advance(DiskTrack &);
};