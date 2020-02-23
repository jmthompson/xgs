#ifndef IMAGEFILE_H_
#define IMAGEFILE_H_

#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using fs::path;

const uint32_t kTwoImgMagic   = 0x474d4932; // "2IMG"
const uint32_t kTwoImgCreator = 0x21534758; // "XGS!"
const uint32_t kTwoImgVersion = 0x0100;     // Version 1.00

// Image data format numbers. Do not change ordering.

enum ImageFormat : uint32_t {
    DOS    = 0,
    PRODOS = 1,
    NIBBLE = 2
};

// Header structure for 2IMG files

struct TwoImgHeader {
    uint32_t    magic;          // "2IMG"
    uint32_t    creator;        // Creator signature
    uint16_t    header_len;     // Length of header in bytes
    uint16_t    version;        // Image version
    ImageFormat format;         // Image data format (see below)
    uint32_t    flags;          // Format flags (see below)
    uint32_t    blocks;         // Number of 512-byte blocks in this image
    uint32_t    data_offset;    // File offset to start of data
    uint32_t    data_len;       // Length of data in bytes
    uint32_t    comment_offset; // File offset to start of comment
    uint32_t    comment_len;    // Length of comment in bytes
    uint32_t    creator_offset; // File offset to start of creator data
    uint32_t    creator_len;    // Length of creator data in bytes
    uint32_t    spare[4];       // Spare uint32_t (pads header to 64 bytes)
};

/* Image flags */
const uint32_t kTwoImgLocked = 0x80000000;

// The DiskCopy 4.2 header structure

struct DiskCopyHeader {
    unsigned char   disk_name[64];  // Disk name, as a Pascal string
    uint32_t        data_size;      // Size of block data in bytes
    uint32_t        tag_size;       // Size of tag information in bytes
    uint32_t        data_checksum;  // Checksum of block data
    uint32_t        tag_checksum;   // Checksum of tag data
    uint8_t         disk_type;      // 0 = 400K, 1 = 800K, 2 = 720K, 3 = 1440K
    uint8_t         disk_format;    // 0x12 = 400K, 0x22 = >400K Mac, 0x24 = 800k Apple II
    uint16_t        unused;         // reserved
};

enum ImageType {
    TWOIMG,
    RAW,
    DISKCOPY42
};

class VirtualDisk {
    private:
        path image_path;

        std::fstream file;

        bool writeable; // True if the ifstream is writeable

        uintmax_t data_offset;
        uintmax_t data_len;

        uintmax_t comment_offset;
        uintmax_t comment_len;

        uintmax_t creator_offset;
        uintmax_t creator_len;

        void openTwoImgFile();
        void openRawFile(ImageFormat);
        void openDiskCopyFile();

    public:
        ImageType   type;
        ImageFormat format;

        bool locked; // True if the image is soft locked

        unsigned int num_chunks;
        unsigned int chunk_size;

        VirtualDisk(path filename) : image_path(filename) {}
        ~VirtualDisk();

        void open();
        void close();
        void read(uint8_t *, const unsigned int, const unsigned int);
        void write(uint8_t *, const unsigned int, const unsigned int);
};

#endif
