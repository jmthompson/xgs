#ifndef DISK525_H_
#define DISK525_H_

#include <cstdlib>

#include "DiskTrack.h"

class VirtualDisk;

class Disk525 {
    public:
        static const unsigned int kNumTracks = 35;
        static const unsigned int kLastTrack = (kNumTracks - 1) * 4;

        static const unsigned int kSectorSize      = 256;
        static const unsigned int kSectorsPerTrack = 16;
        static const unsigned int kBlocksPerTrack  = kSectorsPerTrack / 2;
        static const unsigned int kBytesPerTrack   = kSectorsPerTrack * kSectorSize;
        static const unsigned int kNibblesPerTrack = 6656;

        VirtualDisk *vdisk = nullptr;
        DiskTrack tracks[kNumTracks * 4];

        // The cpu cycle count at which the disk was read or written.
        cycles_t last_access = 0;

        // The fractional track # over which the virtual disk head currently rests.
        unsigned int current_track = 0;
        unsigned int vol_num       = 254;

        unsigned int last_phase = 0;
        unsigned int nib_pos    = 0;

        Disk525();
        ~Disk525();

        uint8_t status();

        uint8_t read(const cycles_t);
        void write(const cycles_t, const uint8_t);
        void flush();

        void load(VirtualDisk *);
        void unload();

        void loadTrack(DiskTrack&);
        void flushTrack(DiskTrack&);

        void phaseChange(const unsigned int);
};

#endif // DISK525_H_
