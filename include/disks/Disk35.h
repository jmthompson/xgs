#pragma once

/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include <cstdint>
#include <cstdlib>
#include "disks/DiskTrack.h"
#include "disks/VirtualDisk.h"
#include "m65816.h"

class Disk35
{
public:
  static constexpr unsigned int kNumTracks = 80;
  static constexpr unsigned int kMaxSectorsPerTrack = 12;
  static constexpr unsigned int kSectorSize = 512;
  static constexpr unsigned int kNibblesPerSector = 800;

  VirtualDisk *vdisk = nullptr;
  DiskTrack tracks[2][kNumTracks];

  bool motor_on = false;
  bool disk_switched = false;

  int current_track = 0;

  unsigned int step = 0;
  unsigned int head = 0;
  unsigned int nib_pos = 0;

  Disk35();
  ~Disk35();

  uint8_t status(const unsigned int);

  uint8_t read(const m65816::cycles_t);
  void write(const m65816::cycles_t, uint8_t);
  void flush();

  void action(const unsigned int);

  void load(VirtualDisk *);
  void unload();

  void loadTrack(DiskTrack &);
  void flushTrack(DiskTrack &);

private:
  uint8_t track_buffer[kMaxSectorsPerTrack * kSectorSize];
};