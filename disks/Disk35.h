#ifndef DISK35_H_
#define DISK35_H_

#include <cstdlib>

#include "disks/DiskTrack.h"

class VirtualDisk;

class Disk35 {
    public:
        static const unsigned int kNumTracks  = 80;
        static const unsigned int kMaxSectorsPerTrack = 12;
        static const unsigned int kSectorSize = 512;

        static const unsigned int kNibblesPerSector = 800;

        VirtualDisk *vdisk = nullptr;
        DiskTrack tracks[2][kNumTracks];

        bool motor_on      = false;
        bool disk_switched = false;

        int current_track = 0;

        unsigned int step = 0;
        unsigned int head = 0;
        unsigned int nib_pos = 0;

        Disk35();
        ~Disk35();

        uint8_t status(const unsigned int);

        uint8_t read(const cycles_t);
        void write(const cycles_t, uint8_t);
        void flush();

        void action(const unsigned int);

        void load(VirtualDisk *);
        void unload();

        void loadTrack(DiskTrack&);
        void flushTrack(DiskTrack&);

private:
        uint8_t track_buffer[kMaxSectorsPerTrack * kSectorSize];
};

#endif // DISK35_H_
