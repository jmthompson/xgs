#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>

#include "common.h"
#include "config.h"

using std::cerr;
using std::string;
using boost::format;

namespace po = boost::program_options;

std::filesystem::path data_dir;
std::filesystem::path config_file;
string s5d1;
string s5d2;
string s6d1;
string s6d2;
string hd[kSmartportUnits];

bool enable_trace = false;
bool rom03 = false;
bool pal = false;
unsigned int ram_size;
string rom_file;
string font40_file;
string font80_file;

bool configure(const int argc, const char **argv)
{
    const char *p;

    if ((p = std::getenv("XGS_DATA_DIR"))) {
        data_dir = std::filesystem::path(p);
    }
    else if ((p = std::getenv("HOME"))) {
        data_dir = std::filesystem::path(p) / ".xgs";
    }
    else {
        data_dir = std::filesystem::current_path();
    }

    cerr << "Using " << data_dir << " as XGS home directory" << std::endl;

    config_file = data_dir / "xgs.conf";

    po::options_description generic("Generic Options");
    generic.add_options()
        ("help",      "Print help message")
        ("version,v", "Print version string");

    po::options_description emulator("Emulator Options");
    emulator.add_options()
        ("trace",    po::bool_switch(&enable_trace)->default_value(false), "Enable trace")
        ("rom03",    po::bool_switch(&rom03)->default_value(false),          "Enable ROM 03 emulation")
        ("pal",      po::bool_switch(&pal)->default_value(false),            "Enable PAL (50 Hz) mode")
        ("romfile",  po::value<string>(&rom_file)->default_value("xgs.rom"),        "Name of ROM file to load")
        ("ram",      po::value<unsigned int>(&ram_size)->default_value(1024),       "Set RAM size in KB")
        ("font40",   po::value<string>(&font40_file)->default_value("xgs40.fnt"),   "Name of 40-column font to load")
        ("font80",   po::value<string>(&font80_file)->default_value("xgs80.fnt"),   "Name of 80-column font to load");

    po::options_description vdisks("Virtual Disk Options");

    vdisks.add_options()
        ("s5d1", po::value<string>(&s5d1), "Mount disk image on S5,D1")
        ("s5d2", po::value<string>(&s5d2), "Mount disk image on S5,D2")
        ("s6d1", po::value<string>(&s6d1), "Mount disk image on S6,D1")
        ("s6d2", po::value<string>(&s6d2), "Mount disk image on S6,D2");

    for (unsigned int i = 1 ; i <= kSmartportUnits ; ++i) {
        string name = (format("hd%d") % i).str();
        string desc = (format("Set HD #%d image") % i).str();

        vdisks.add_options()
            (name.c_str(), po::value(&hd[i - 1]), desc.c_str());
    }

    po::options_description cli_options("Allowed Options");
    cli_options.add(generic);
    cli_options.add(emulator);
    cli_options.add(vdisks);

    po::options_description config_file_options("Config File Options");
    config_file_options.add(emulator);
    config_file_options.add(vdisks);

    po::variables_map vm; 
        
    std::ifstream cfs{config_file};

    if (cfs.is_open()) {
        po::store(po::parse_config_file(cfs, config_file_options), vm);

        cfs.close();
    }

    po::store(po::command_line_parser(argc, argv).options(cli_options).run(), vm);
 
    if (vm.count("help")) { 
        cerr << cli_options << std::endl << std::endl;

        return false;
        }
    else {
        po::notify(vm);
    } 

    return true;
}