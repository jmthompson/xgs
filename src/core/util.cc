#include <fstream>
#include <filesystem>

#include "common.h"
#include "config.h"
#include "util.h"

using std::filesystem::path;
using std::filesystem::exists;
using std::string;

/**
 * Load the entirety of a file into a uint8_t buffer.
 */
unsigned int loadFile(const std::string& filename, const unsigned int expected_size, uint8_t *buffer)
{
    path p = data_dir / filename;

    if (!exists(p)) {
        p = filename;
    }
    std::uintmax_t bytes = std::filesystem::file_size(p);
    std::ifstream ifs;

    ifs.open(p, std::ifstream::binary);
    ifs.read((char *) buffer, bytes);
    ifs.close();

    if (bytes != expected_size) {
        string err = (boost::format("Error loading %s: expected %d bytes, but read %d\n") % filename % expected_size % bytes).str();

        throw std::runtime_error(err);
    }

    return bytes;
}