/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * The VirtualDisk class is used to load and save virtual disk images in a
 * variety of formats.
 */

#include <climits>
#include <stdexcept>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

#include "emulator/common.h"

#include "disks/VirtualDisk.h"

namespace fs = boost::filesystem;

using fs::path;
using boost::uintmax_t;

VirtualDisk::~VirtualDisk()
{
    if (file.is_open()) {
        file.close();
    }
}

/**
 * Attempt to open the disk image file
 */

void VirtualDisk::open()
{
    path ext = image_path.extension();

    // Try in r/w mode first, then try falling back to read-only before giving up

    locked    = false;
    writeable = true;

    file.open(image_path, std::fstream::in | std::fstream::out | std::fstream::binary);
    if (file.fail()) {
        locked    = true;
        writeable = false;

        file.open(image_path, std::fstream::in | std::fstream::binary);
    }

    if (file.fail()) {
        std::runtime_error("Error opening image file");
    }

    try {
        if (ext == ".2mg") {
            openTwoImgFile();
        }
        else if ((ext == ".do") || (ext == ".dsk")) {
            openRawFile(DOS);
        }
        else if ((ext == ".po") || (ext == ".raw")) {
            openRawFile(PRODOS);
        }
        else if (ext == ".nib") {
            openRawFile(NIBBLE);
        }
        else if (ext == ".dc") {
            openDiskCopyFile();
        }
        else {
            throw std::runtime_error("Unknown image extension");
        }

        if (format == DOS) {
            chunk_size = 256;
        }
        else if (format == PRODOS) {
            chunk_size = 512;
        }
        else if (format == NIBBLE) {
            chunk_size = 6656;
        }

        num_chunks = data_len / chunk_size;

        if (data_len % chunk_size) {
            throw std::runtime_error("Image data size is not a multiple of the chunk size");
        }
    }
    catch (std::runtime_error& e) {
        file.close();

        throw e;
    }
}

void VirtualDisk::openTwoImgFile()
{
    TwoImgHeader header;
    uintmax_t    bytes;

    file.read((char *) &header, sizeof(header));

    if (file.eof()) {
        throw std::runtime_error("Got EOF trying to read 2IMG header");
    }

#ifdef BIGENDIAN
    header.magic          = swap_endian(header.magic);
    header.creator        = swap_endian(header.creator);
    header.header_len     = swap_endian(header.header_len);
    header.version        = swap_endian(header.version);
    header.format         = swap_endian(header.format);
    header.flags          = swap_endian(header.flags);
    header.blocks         = swap_endian(header.blocks);
    header.data_offset    = swap_endian(header.data_offset);
    header.data_len       = swap_endian(header.data_len);
    header.comment_offset = swap_endian(header.comment_offset);
    header.comment_len    = swap_endian(header.comment_len);
    header.creator_offset = swap_endian(header.creator_offset);
    header.creator_len    = swap_endian(header.creator_len);
#endif

    if (header.magic != kTwoImgMagic) {
        std::cerr << boost::format("got magic %08X, expected %08X\n") % header.magic % kTwoImgMagic;

        throw std::runtime_error("Invalid magic number in 2IMG header");
    }

    if (header.version > kTwoImgVersion) {
        throw std::runtime_error("Unsupport 2IMG version");
    }

    if (header.flags & kTwoImgLocked) {
        locked = true;
    }

    type           = TWOIMG;
    format         = header.format;
    data_offset    = header.data_offset;
    data_len       = header.data_len;
    comment_offset = header.comment_offset;
    comment_len    = header.comment_len;
    creator_offset = header.creator_offset;
    creator_len    = header.creator_len;
}

void VirtualDisk::openRawFile(ImageFormat data_format)
{
    type           = RAW;
    format         = data_format;
    data_offset    = 0;
    data_len       = fs::file_size(image_path);
    comment_offset = 0;
    comment_len    = 0;
    creator_offset = 0;
    creator_len    = 0;
}

void VirtualDisk::openDiskCopyFile()
{
    DiskCopyHeader header;

    file.read((char *) &header, sizeof(header));

    if (file.eof()) {
        throw std::runtime_error("Got EOF trying to read DiskCopy header");
    }

#ifndef BIGENDIAN
    header.data_size     = swap_endian(header.data_size);
    header.tag_size      = swap_endian(header.tag_size);
    header.data_checksum = swap_endian(header.data_checksum);
    header.tag_checksum  = swap_endian(header.tag_checksum);
    header.unused        = swap_endian(header.unused);
#endif /* BIGENDIAN */

    if (header.unused != 0x0100) {
        throw std::runtime_error("Invalid DiskCopy header");
    }

    type           = DISKCOPY42;
    format         = PRODOS;
    data_offset    = sizeof(header);
    data_len       = header.data_size;
    comment_offset = 0;
    comment_len    = 0;
    creator_offset = 0;
    creator_len    = 0;
}

void VirtualDisk::close()
{
    file.close();
}

#if 0
/*
 * Write a 2IMG header to a file, automatically byte-swapping hte
 * fields before writing if necessary.
 */

int DSK_writeUnivImageHeader(FILE *fp, image_header *image)
{
    image_header    our_image;

    memcpy(&our_image,image,sizeof(image_header));
#ifdef BIGENDIAN
    our_image.header_len     = swap_endian(our_image.header_len);
    our_image.version        = swap_endian(our_image.version);
    our_image.format         = swap_endian(our_image.format);
    our_image.flags          = swap_endian(our_image.flags);
    our_image.blocks         = swap_endian(our_image.blocks);
    our_image.data_offset    = swap_endian(our_image.data_offset);
    our_image.data_len       = swap_endian(our_image.data_len);
    our_image.comment_offset = swap_endian(our_image.comment_offset);
    our_image.comment_len    = swap_endian(our_image.comment_len);
    our_image.creator_offset = swap_endian(our_image.creator_offset);
    our_image.creator_len    = swap_endian(our_image.creator_len);
#endif /* BIGENDIAN */
    fwrite(&our_image, sizeof(image_header), 1, fp);
    return errno;
}
#endif

/**
 * Read one or more chunks of data from the image file
 */
void VirtualDisk::read(uint8_t *buffer, const unsigned int first_chunk, const unsigned int num_chunks)
{
    file.seekg(data_offset + (first_chunk * chunk_size));
    file.read((char *) buffer, num_chunks * chunk_size);

    if (file.fail()) {
        throw std::runtime_error("Error reading from image file");
    }
}

/*
 * Write one or more chunks to an image file
 */
void VirtualDisk::write(uint8_t *buffer, const unsigned int first_chunk, const unsigned int num_chunks)
{
    if (locked) {
        throw std::runtime_error("Attempt to write to a locked disk");
    }

    file.seekg(data_offset + (first_chunk * chunk_size));
    file.write((char *) buffer, num_chunks * chunk_size);

    if (file.fail()) {
        throw std::runtime_error("Error writing to image file");
    }
}
