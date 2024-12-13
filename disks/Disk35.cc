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

#include "emulator/common.h"

#include "disks/IWM.h"
#include "disks/DiskTrack.h"
#include "disks/Disk35.h"
#include "disks/VirtualDisk.h"
#include "disks/nibbles.h"

#include "emulator/xgs.h"

using std::uint32_t;

const unsigned int kTagByte = 0x22;

static const unsigned int sectors_per_track[80] = {
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8
};

static const unsigned int track_start[80] = {
    0,    12,  24,  36,  48,  60 , 72,  84,
    96,  108, 120, 132, 144, 156, 168, 180,
    192, 203, 214, 225, 236, 247, 258, 269,
    280, 291, 302, 313, 324, 335, 346, 357,
    368, 378, 388, 398, 408, 418, 428, 438,
    448, 458, 468, 478, 488, 498, 508, 518,
    528, 537, 546, 555, 564, 573, 582, 591,
    600, 609, 618, 627, 636, 645, 654, 663,
    672, 680, 688, 696, 704, 712, 720, 728,
    736, 744, 752, 760, 768, 776, 784, 792
};

Disk35::Disk35()
{
    for (unsigned int i = 0 ; i < kNumTracks ; ++i) {
        tracks[0][i].track_num = tracks[1][i].track_num = i;
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
    if (vdisk == nullptr) return (cycle_count & 0xFF) | 0x80;

    DiskTrack& track = tracks[head][current_track];
    loadTrack(track);

    if (!track.valid) return (cycle_count & 0xFF) | 0x80;

    /* Assume everyone is reading 3.5" disks with latch mode enabled */

    uint8_t ret = track.nibble_data[nib_pos++];
    if (nib_pos >= track.track_len) nib_pos = 0;

    return ret;
}

void Disk35::write(const cycles_t cycle_count, const uint8_t val)
{
}

void Disk35::flush()
{
    for (unsigned int i = 0 ; i < 2 ; ++i) {
        for (unsigned int j = 0 ; j < kNumTracks ; ++j) {
            DiskTrack& track = tracks[i][j];

            if (track.valid) {
                flushTrack(track);

                if (j != current_track) track.invalidate();
            }
        }
    }
}

void Disk35::action(const unsigned int state)
{
    switch(state) {
        case 0x00:  // Set step direction inward (towards higher tracks)
            step = 0;

            break;
        case 0x01:  // Set step direction outward (towards lower tracks)
            step = 1;

            break;
        case 0x03:  // reset disk-switched flag
            disk_switched = false;

            break;
        case 0x04:    // step disk
            if (step) {
                if (current_track) --current_track;
            }
            else {
                if (current_track < (kNumTracks - 1)) ++current_track;
            }

            break;
        case 0x08:  // turn motor on
            motor_on = true;

            break;
        case 0x09:  // turn motor off
            motor_on = false;

            break;
        case 0x0D:  // eject disk
            unload();

            break;
        default:
            break;
    }
}

void Disk35::load(VirtualDisk *new_vdisk)
{
    new_vdisk->open();

    if ((new_vdisk->format != PRODOS) || (new_vdisk->num_chunks != 1600)) {
        throw std::runtime_error("3.5\" drives support only ProODS-order 800k disk imges");
    }

    vdisk = new_vdisk;
    disk_switched = true;
    nib_pos = 0;

    for (unsigned int i = 0 ; i < kNumTracks ; ++i) {
        tracks[0][i].valid = tracks[0][i].dirty = false;
        tracks[1][i].valid = tracks[1][i].dirty = false;
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
    // Do nothing if this track is already loaded
    if (track.nibble_data != nullptr) return;

    try {
        unsigned int num_sectors = sectors_per_track[track.track_num];
        unsigned int nibbles = num_sectors * kNibblesPerSector;
        unsigned int start = (track_start[track.track_num] * 2) + (head * num_sectors);

        track.allocate(nibbles);

        vdisk->read(track_buffer, start, num_sectors);

        for (unsigned int sec = 0 ; sec < num_sectors ; ++sec) {
            unsigned int side = head? 0x20 : 0x00;

            if (track.track_num & 0x40) side |= 0x01;

            unsigned int sum = (track.track_num ^ sec ^ side ^ kTagByte) & 0x3F;

            // pad
            track.write(0xFF, 8, nib_pos);

            // Sync
            for (unsigned int i = 0 ; i < 35 ; ++i) {
                track.write(0xFF, 10, nib_pos);
            }

            // header prolog
            track.write(0xD5, 8, nib_pos);
            track.write(0xAA, 8, nib_pos);
            track.write(0x96, 8, nib_pos);

            // header data
            track.write(nibble_to_disk[track.track_num & 0x3F], 8, nib_pos);
            track.write(nibble_to_disk[sec], 8, nib_pos);
            track.write(nibble_to_disk[side], 8, nib_pos);
            track.write(nibble_to_disk[kTagByte], 8, nib_pos);
            track.write(nibble_to_disk[sum], 8, nib_pos);

            // header epilog
            track.write(0xDE, 8, nib_pos);
            track.write(0xAA, 8, nib_pos);

            // pad
            track.write(0xFF, 8, nib_pos);

            // Sync
            for (unsigned int i = 0 ; i < 5 ; ++i) {
                track.write(0xFF, 10, nib_pos);
            }

            // Data prolog
            track.write(0xD5, 8, nib_pos);
            track.write(0xAA, 8, nib_pos);
            track.write(0xAD, 8, nib_pos);
            track.write(nibble_to_disk[sec], 8, nib_pos);

            // Data

            uint8_t val;
            uint8_t buffer1[175],buffer2[175],buffer3[175];

            // IIgs doesn't use the tag bytes so set them to zero
            for (unsigned int i = 171 ; i < 175 ; ++i) {
                buffer1[i] = buffer2[i] = buffer3[i] = 0;
            }

            /* Copy from the user's buffer to our buffer, while computing   */
            /* the three-byte data checksum.                */

            {
                unsigned int csum1 = 0, csum2 = 0, csum3 = 0, csum4;
                int i = sec * kSectorSize;
                int j = 170;

                while (true) {
                    csum1 = (csum1 & 0xFF) << 1;
                    if (csum1 & 0x0100) ++csum1;

                    val = track_buffer[i++];
                    csum3 += val;
                    if (csum1 & 0x0100) {
                        csum3++;
                        csum1 &= 0xFF;
                    }
                    buffer1[j] = (val ^ csum1) & 0xFF;

                    val = track_buffer[i++];
                    csum2 += val;
                    if (csum3 > 0xFF) {
                        csum2++;
                        csum3 &= 0xFF;
                    }
                    buffer2[j] = (val ^ csum3) & 0xFF;

                    if (--j < 0) break;

                    val = track_buffer[i++];
                    csum1 += val;
                    if (csum2 > 0xFF) {
                        csum1++;
                        csum2 &= 0xFF;
                    }
                    buffer3[j+1] = (val ^ csum2) & 0xFF;
                }

                csum4 =  ((csum1 & 0xC0) >> 6);
                csum4 |= ((csum2 & 0xC0) >> 4);
                csum4 |= ((csum3 & 0xC0) >> 2);

                i = 175;
                while(--i >= 0) {
                    uint8_t nibble1 = buffer1[i] & 0x3F;
                    uint8_t nibble2 = buffer2[i] & 0x3F;
                    uint8_t nibble3 = buffer3[i] & 0x3F;
                    uint8_t nibble4 = ((buffer1[i] & 0xC0) >> 2);

                    nibble4 |= ((buffer2[i] & 0xC0) >> 4);
                    nibble4 |= ((buffer3[i] & 0xC0) >> 6);

                    track.write(nibble_to_disk[nibble4], 8, nib_pos);
                    track.write(nibble_to_disk[nibble1], 8, nib_pos);
                    track.write(nibble_to_disk[nibble2], 8, nib_pos);
                    if (i) track.write(nibble_to_disk[nibble3], 8, nib_pos);
                }

                track.write(nibble_to_disk[csum4 & 0x3F], 8, nib_pos);
                track.write(nibble_to_disk[csum3 & 0x3F], 8, nib_pos);
                track.write(nibble_to_disk[csum2 & 0x3F], 8, nib_pos);
                track.write(nibble_to_disk[csum1 & 0x3F], 8, nib_pos);
            }

            // Data epilog
            track.write(0xDE, 8, nib_pos);
            track.write(0xAA, 8, nib_pos);
            track.write(0xFF, 8, nib_pos);
        }
    }
    catch (std::runtime_error& e) {
        track.invalidate();

        throw e;
    }

    track.valid = true;
    track.dirty = false;
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
