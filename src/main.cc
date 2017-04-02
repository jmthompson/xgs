#include <iostream>
#include <stdexcept>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>

#include "xgscore/Config.h"
#include "xgscore/Emulator.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using std::cerr;
using std::endl;
using std::string;
using fs::path;

static fs::path data_dir;

/**
 * Load the entirety of a file into a uint8_t buffer.
 */
unsigned int loadFile(std::string& filename, uint8_t **buffer, const unsigned int expected_size)
{
    fs::path p = data_dir / filename;

    if (!fs::exists(p)) {
        p = filename;
    }

    boost::uintmax_t bytes = fs::file_size(p);
    fs::ifstream ifs;

    *buffer = new uint8_t[bytes];

    ifs.open(p, std::ifstream::binary);
    ifs.read((char *) *buffer, bytes);
    ifs.close();

    if (bytes != expected_size) {
        string err = (boost::format("Error loading %s: expected %d bytes, but read %d\n") % filename % expected_size % bytes).str();

        throw std::runtime_error(err);
    }

    return bytes;
}

bool buildConfig(Config& config, unsigned int argc, char **argv)
{
    const char *p;
    unsigned int ram_size;
    string rom_file;
    string font40_file;
    string font80_file;
    string hd0;

    if (p = std::getenv("XGS_DATA_DIR")) {
        data_dir = path(p);
    }
    else if (p = std::getenv("HOME")) {
        data_dir = path(p) / ".xgs";
    }
    else {
        cerr << "Neither XGS_HOME nor HOME is set. Unable to determine the XGS data directory." << endl;

        return false;
    }

    cerr << "Using " << data_dir << " as XGS home directory" << endl;

    po::options_description generic("Generic Options");
    generic.add_options()
        ("help",      "Print help message")
        ("version,v", "Print version string");

    po::options_description emulator("Emulator Options");
    emulator.add_options()
        ("trace",    po::bool_switch(&config.debugger.trace)->default_value(false), "Enable trace")
        ("rom03,3",  po::bool_switch(&config.rom03)->default_value(false),          "Enable ROM 03 emulation")
        ("pal",      po::bool_switch(&config.pal)->default_value(false),            "Enable PAL (50 Hz) mode")
        ("romfile",  po::value<string>(&rom_file)->default_value("xgs.rom"),        "Name of ROM file to load")
        ("ram",      po::value<unsigned int>(&ram_size)->default_value(1024),       "Set RAM size in KB")
        ("font40",   po::value<string>(&font40_file)->default_value("xgs40.fnt"),   "Name of 40-column font to load")
        ("font80",   po::value<string>(&font80_file)->default_value("xgs80.fnt"),   "Name of 80-column font to load");

    po::options_description vdisks("Virtual Disk Options");
    vdisks.add_options()
        ("hd0", po::value(&hd0), "Set HD #0 image");

    po::options_description cli_options("Allowed Options");
    cli_options.add(generic);
    cli_options.add(emulator);
    cli_options.add(vdisks);

    po::variables_map vm; 

    try { 
        po::store(po::command_line_parser(argc, argv).options(cli_options).run(), vm);
 
        if (vm.count("help")) { 
            cerr << cli_options << endl << endl;

            return false;
        }
        else {
            po::notify(vm);
        } 

        unsigned int bytes;

        bytes = loadFile(rom_file, &config.rom, config.rom03? kRom03Bytes : kRom01Bytes);
        config.rom_pages      = bytes >> 8;
        config.rom_start_page = 0x10000 - config.rom_pages;

        config.slow_ram = new uint8_t[65536*2];
        config.fast_ram = new uint8_t[ram_size * 1024];
        config.fast_ram_pages = ram_size << 2;

        bytes = loadFile(font40_file, &config.font_40col[0], kFont40Bytes * 2);
        config.font_40col[1] = config.font_40col[0] + kFont40Bytes;

        bytes = loadFile(font80_file, &config.font_80col[0], kFont80Bytes * 2);
        config.font_80col[1] = config.font_80col[0] + kFont80Bytes;

        config.vdisks.smartport[0] = hd0;
    }
    catch (std::exception& e) { 
        cerr << "ERROR: " << e.what() << endl << endl;

        return false;
    } 

    return true;
}

int main (int argc, char **argv)
{
    Config config;

    if (!buildConfig(config, argc, argv)) {
        exit(1);
    }

    Emulator *e = new Emulator(&config);
    e->run();
}
