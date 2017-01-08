/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * A class for emulating 3.5" floppies
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>

#include <SDL.h>

#include "gstypes.h"
#include "IWM.h"
#include "DiskTrack.h"
#include "Disk35.h"
#include "System.h"
#include "VirtualDisk.h"

using std::uint32_t;

Disk35::Disk35()
{
    for (unsigned int i = 0 ; i < kNumTracks * 2 ; ++i) {
        tracks[i].track_num = i >> 1;
    }
}

Disk35::~Disk35()
{
    flush();
}

uint8_t Disk35::status(const unsigned int state)
{
    uint8_t result = 0;

    switch(state) {
        case 0x00:  // step direction
                result = step;

                break;
        case 0x01:  // disk in place: 0 = yes, 1 = no
                result = (vdisk == nullptr);

                break;
        case 0x02:  // disk is stepping
                result = 1;

                break;
        case 0x03:  // disk is locked
                result = vdisk? vdisk->locked : 0;

                break;
        case 0x04:  // motor on
                result = motor_on;

                break;
        case 0x05:  // at track 0: 0 = yes, 1 = no
                result = !current_track;

                break;
        case 0x06:  // disk switched
                result = disk_switched;

                break;
        case 0x07:  // tachometer
                result = rand() & 1;

                break;
        case 0x08:  // lower head activate
                head   = 0;
                result = 0;

                break;
        case 0x09:  // upper head activate
                head   = 1;
                result = 0;

                return 0;
        case 0x0C:  // number of sides
                result = 1;

                break;
        case 0x0D:  // disk ready
                result = 0;

                break;
        case 0x0F:  // drive installed
                result = 0;

                break;
        default:
                result = 1;

                break;
    }

    return result;
}

uint8_t Disk35::read(const cycles_t cycle_count)
{
}

void Disk35::write(const cycles_t cycle_count, const uint8_t val)
{
}

void Disk35::flush()
{
    for (unsigned int i = 0; i < kNumTracks * 2 ; ++i) {
        DiskTrack& track = tracks[i];
        if (!track.valid) continue;

        flushTrack(track);

        if (i != current_track) track.invalidate();
    }
}

void Disk35::load(VirtualDisk *new_vdisk)
{
    if ((new_vdisk->format != PRODOS) || (new_vdisk->num_chunks != 1600)) {
        throw std::runtime_error("3.5\" drives support only ProODS-order 800k disk imges");
    }

    vdisk = new_vdisk;
    disk_switched = true;
    nib_pos = 0;

    for (unsigned int i = 0 ; i < 80*2 ; ++i) {
        tracks[i].valid = false;
        tracks[i].dirty = false;
    }
}

void Disk35::unload()
{
    if (vdisk != nullptr) {
        vdisk->close();

        vdisk = nullptr;
    }
}

void Disk35::loadTrack(DiskTrack& track)
{
}

void Disk35::flushTrack(DiskTrack& track)
{
    if (!track.dirty) return;

#if 0
                if (trk->track_dirty) {
                    trk->track_dirty = 0;
                    ret = IWM_trackToUnix35(dsk2, trk, track_num, head, &(buffer[0]));
                    if (ret != 1) break;

                    len = iwm_35track_len[track_num];
                    start = iwm_35track_index[track_num] * 2 + (head * len);
                    ret = DSK_writeChunk(dsk2->disk, buffer, start, len);
                }
                if (j != (dsk2->cur_track * 2 + dsk2->head)) {
                    track.invalidate();
                }
#endif
}
