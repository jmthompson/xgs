#ifndef DISKTRACK_H_
#define DISKTRACK_H_

/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This is the DiskTrack class, a very simple class for emulating one
 * track of nibblized data. The data is read from and written to a
 * circular buffer.
 */

class DiskTrack {
    public:
        bool valid = false;
        bool dirty = false;

        int overflow_size = 0;
        int track_len = 0;

        unsigned int track_num;
        unsigned int nibble_count;

        std::uint8_t *nibble_size = nullptr;
        std::uint8_t *nibble_data = nullptr;

        DiskTrack()  = default;
        ~DiskTrack() = default;

        void allocate(unsigned int len)
        {
            track_len   = len;
            nibble_size = new uint8_t[track_len];
            nibble_data = new uint8_t[track_len];

            for (unsigned int i = 0 ; i < track_len ; ++i) {
                nibble_size[i] = 8;
                nibble_data[i] = 0xFF;
            }

            valid = dirty = false;
            overflow_size = 0;
        }

        void invalidate()
        {
            valid = false;

            delete[] nibble_size;
            delete[] nibble_data;

            nibble_size = nibble_data = nullptr;
        }

        /**
         * Return the next byte of track data from the buffer at pos.
         * The pos will be updated to the next buffer position, circling
         * around to the beginning if it passes the end of the track.
         */
        uint8_t read(unsigned int& pos)
        {
            uint8_t val, size = 0;

            while (!size) {
                size = nibble_size[pos];
                val  = nibble_data[pos];

                ++nibble_count;

                if (++pos > track_len) pos = 0;
            }

            return val;
        }

        /**
         * Like read(), but reads a single nibble encoded as four
         * disk bytes in the format used in sector headers.
         */
        uint8_t read_4x4(unsigned int& pos)
        {
            return ((read(pos) << 1) + 1) & read(pos);
        }

        /**
         * Writes a disk byte of size bits to the buffer at position pos.
         * The pos will be updated to the next buffer position, circling
         * around to the beginning if it passes the end of the track.
         */
        void write(const uint8_t val, const unsigned int size, unsigned int& pos)
        {
            nibble_size[pos] = size;
            nibble_data[pos] = val;

            if (++pos >= track_len) pos = 0;
        }

        /**
         * Like write(), but writes a single byte encoded as four
         * disk bytes in the format used in sector headers.
         */
        void write_4x4(const uint8_t val, unsigned int& pos)
        {
            write(0xAA | (val >> 1), 8, pos);
            write(0xAA | val, 8, pos);
        }
};

#endif // DISKTRACK_H_
