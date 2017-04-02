/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * A class for emulating 5.25" floppies
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>

#include <SDL.h>

#include "common.h"

#include "disks/IWM.h"
#include "disks/Disk525.h"
#include "disks/VirtualDisk.h"
#include "disks/nibbles.h"

#include "xgscore/System.h"

using std::uint32_t;

static const uint8_t physical_to_logical[2][16] = {
    {
        0x00, 0x07, 0x0E, 0x06, 0x0D, 0x05, 0x0C, 0x04,
        0x0B, 0x03, 0x0A, 0x02, 0x09, 0x01, 0x08, 0x0F
    },
    {
        0x00, 0x08, 0x01, 0x09, 0x02, 0x0A, 0x03, 0x0B,
        0x04, 0x0C, 0x05, 0x0D, 0x06, 0x0E, 0x07, 0x0F
    }
};

Disk525::Disk525()
{
    for (unsigned int i = 0 ; i < kNumTracks * 4; ++i) {
        tracks[i].track_num = i >> 2;
    }
}

Disk525::~Disk525()
{
    flush();
}

uint8_t Disk525::status()
{
    return (vdisk == nullptr)? 1 : vdisk->locked;
}

uint8_t Disk525::read(const cycles_t cycle_count)
{
    if (vdisk == nullptr) return (cycle_count & 0xFF) | 0x80;

    DiskTrack& track = tracks[current_track];
    loadTrack(track);

    if (!track.valid) return (cycle_count & 0xFF) | 0x80;

    unsigned int bits_read = (cycle_count - last_access) >> 2;

    uint8_t size = track.nibble_size[nib_pos];

#if 0
    // should never need this; we always initialize new buffers
    // so there's no way to ever get a byte with 0 size.

    while (!size) {
        if (++nib_pos >= track.track_len) nib_pos = 0;

        size = track.nibble_size[nib_pos];
    }
#endif

    if (bits_read > (size + 2)) {
        bits_read -= (size + 2);

        unsigned int skip_nibs = bits_read >> 3;

        nib_pos += skip_nibs;

        if (nib_pos >= track.track_len) nib_pos %= track.track_len;

        last_access += (skip_nibs * 32);

        if ((bits_read >= 10) || (cycle_count - last_access) > 60) {
            /* We're way off, adjust last_access */
            last_access = (cycle_count - 32) & ~0x1FL;
        }

        bits_read = 8;

        size = track.nibble_data[nib_pos];
    }

#if 0
    while (!size) {
        ++nib_pos;
        if (pos >= track_len) pos = 0;
        size = trk->track_data[pos];
    }
#endif

    if (bits_read < size) {
        return track.nibble_data[nib_pos] >> (size - bits_read);
    }
    else {
        uint8_t val = track.nibble_data[nib_pos];

        if (++nib_pos >= track.track_len) nib_pos = 0;

        last_access += size*4;

        return val;
    }
}

void Disk525::write(const cycles_t cycle_count, const uint8_t val)
{
    if (!vdisk) return;

    DiskTrack& track = tracks[current_track];
    loadTrack(track);

    if (!track.valid) return;

    track.dirty = true;

    unsigned int bits_read = (cycle_count - last_access) >> 2;

    track.write(val? val : cycle_count & 0xFF, bits_read, nib_pos);

    last_access += (bits_read *4);
}

void Disk525::flush()
{
    for (unsigned int i = 0 ; i < kNumTracks * 4 ; ++i) {
        DiskTrack& track = tracks[i];
        if (!track.valid) continue;

        flushTrack(track);

        if (i != current_track) track.invalidate();
    }
}

void Disk525::load(VirtualDisk *new_vdisk)
{
    if (new_vdisk->num_chunks != (new_vdisk->format == NIBBLE? 280 : 280)) {
        throw std::runtime_error("5.25\" drives support only 140K images");
    }

    vdisk   = new_vdisk;
    nib_pos = 0;

    for (unsigned int i = 0 ; i < kNumTracks * 4 ; ++i) {
        tracks[i].valid = false;
        tracks[i].dirty = false;
    }
}

void Disk525::unload()
{
    if (vdisk != nullptr) {
        vdisk->close();

        vdisk = nullptr;
    }
}

void Disk525::loadTrack(DiskTrack& track)
{
    unsigned int pos;

    // Do nothing if this track is already loaded
    if (track.nibble_data != nullptr) return;

    try {
        int i;

        track.allocate(kNibblesPerTrack);

        if (vdisk->format == NIBBLE) {
            vdisk->read(track.nibble_data, track.track_num, 1);

            for (i = 0 ; i < track.track_len ; ++i) {
                track.nibble_size[i] = 8;
            }
        }
        else {
            uint8_t track_buffer[kNibblesPerTrack];
            uint8_t nib_buff[342];

            vdisk->read(track_buffer, track.track_num * kBlocksPerTrack, kBlocksPerTrack);

            pos = 0;

            for (unsigned int sector = 0; sector < kSectorsPerTrack; ++sector) {
                unsigned int num_sync = sector? 14 : 70;

                for (i = 0; i < num_sync; i++) {
                    track.write(0xFF, 10, pos);
                }

                // header prolog
                track.write(0xD5, 8, pos);
                track.write(0xAA, 8, pos);
                track.write(0x96, 8, pos);

                // header data
                track.write_4x4(vol_num, pos);
                track.write_4x4(track.track_num, pos);
                track.write_4x4(sector, pos);
                track.write_4x4(vol_num ^ track.track_num ^ sector, pos);

                // header epilog
                track.write(0xDE, 8, pos);
                track.write(0xAA, 8, pos);
                track.write(0xEB, 8, pos);

                // Inter-sector sync
                track.write(0xFF, 8, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);

                // data prolog
                track.write(0xD5, 8, pos);
                track.write(0xAA, 8, pos);
                track.write(0xAD, 8, pos);

                uint8_t *aux_buf = nib_buff;
                uint8_t *nib_out = nib_buff + 0x56;
                uint8_t *in      = track_buffer + (sector * kSectorSize);

                for (i = 0 ; i < 0x56 ; ++i) aux_buf[i] = 0;

                int x;

                for (i = 0x101, x = 0x55 ; i >= 0 ; --i) {
                    uint8_t v1 = (i >= 0x100)? 0 : in[i];
                    uint8_t v2 = (aux_buf[x] << 1) + (v1 & 1);

                    v1 >>= 1; 

                    v2 = (v2 << 1) + (v1 & 1);
                    v1 >>= 1; 

                    nib_out[i] = v1;
                    aux_buf[x] = v2;

                    if (x-- < 0) x = 0x55;
                }

                uint8_t val, last = 0;

                for (i = 0 ; i < 342 ; ++i) {
                    val = nib_buff[i];

                    track.write(nibble_to_disk[last ^ val], 8, pos);

                    last = val;
                }

                track.write(nibble_to_disk[last], 8, pos);

                // data epilog
                track.write(0xDE, 8, pos);
                track.write(0xAA, 8, pos);
                track.write(0xEB, 8, pos);
                track.write(0xFF, 8, pos);

                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
                track.write(0xFF, 10, pos);
            }
        }
    }
    catch (std::runtime_error& e) {
        track.invalidate();

        throw e;
    }

    track.valid = true;
    track.dirty = false;
}

/**
 * Convert a track from nibbles to bytes, placing the results into
 * the specified output buffer. The output buffer must be at least
 * kBytesPerTrack bytes long.
 */
void Disk525::flushTrack(DiskTrack& track)
{
    if (!track.dirty) return;
            
    if (vdisk->format == NIBBLE) {
        vdisk->write(track.nibble_data, track.track_num, 1);
    }
    else {
        uint8_t buffer[kBytesPerTrack];
        bool sector_done[kSectorsPerTrack];
        unsigned int sectors_done = 0, pos = 0;
        int  i;

        for (i = 0 ; i < kSectorsPerTrack ; ++i) {
            sector_done[i] = false;
        }

        track.nibble_count = 0;

        while (track.nibble_count < track.track_len) {
            int val;

            // look for start of a sector
            if ((val = track.read(pos)) != 0xD5) continue;
            if ((val = track.read(pos)) != 0xAA) continue;
            if ((val = track.read(pos)) != 0x96) continue;

            uint8_t vol_num    = track.read_4x4(pos);
            uint8_t track_num  = track.read_4x4(pos);
            uint8_t sector_num = track.read_4x4(pos);
            uint8_t cksum      = track.read_4x4(pos);

            // Verify checksum and track/sector numbers
            if (vol_num ^ track_num ^ sector_num ^ cksum) return;
            if ((track_num != track.track_num) || (sector_num >= kSectorsPerTrack)) return;
            if (sector_done[sector_num]) return;

            // Skip to start of data
            for (i = 0; i < 38; ++i) {
                val = track.read(pos);

                if (val == 0xD5) break;
            }

            if (val != 0xD5) return;
            if ((val = track.read(pos)) != 0xAA) return;
            if ((val = track.read(pos)) != 0xAD) return;

            uint8_t *buf = buffer + (sector_num * kSectorSize);
            uint8_t aux_buf[0x80];
            int prev_val = 0;

            for (i = 0x55 ; i >= 0; --i) {
                val = disk_to_nibble[track.read(pos)];
                if (val < 0) return;

                aux_buf[i] = (prev_val ^= val);
            }

            // data area
            for (i = 0 ; i < kSectorSize ; i++) {
                val = disk_to_nibble[track.read(pos)];
                if (val < 0) return;

                buf[i] = (prev_val ^= val);
            }

            // verify data checksum
            val = disk_to_nibble[track.read(pos)];
            if (val != prev_val) return;

            unsigned int x = 0x55;

            // Now merge aux_buf into buf
            for (i = 0 ; i < kSectorSize ; ++i) {
                uint8_t v1, v2;

                v1 = aux_buf[x];
                v2 = (buf[i] << 1) + (v1 & 1);

                v1 >>= 1;
                v2 = (v2 << 1) + (v1 & 1);

                buf[i]     = v2;
                aux_buf[x] = (val >>= 1);

                if (x-- < 0) x = 0x55;
            }

            sector_done[sector_num] = 1;
            sectors_done++;

            if (sectors_done >= kSectorsPerTrack) break;
        }

        vdisk->write(buffer, track.track_num * kBlocksPerTrack, kBlocksPerTrack);

        track.dirty = false;
    }
}

void Disk525::phaseChange(const unsigned int phase)
{
    unsigned int phase_up   = (phase - 1) & 3;
    unsigned int phase_down = (phase + 1) & 3;
    unsigned int delta      = 0;

    if (last_phase == phase_up) {
        delta = 2;

        last_phase = phase;
    }
    else if (last_phase == phase_down) {
        delta = -2;

        last_phase = phase;
    }

    current_track += delta;

    if (current_track < 1) {
        current_track = 0;
        last_phase    = 0;
    }
    else if (current_track > kLastTrack) {
        current_track = kLastTrack;
        last_phase    = 2;
    }
}
