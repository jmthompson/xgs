#pragma once

#include <filesystem>
#include <string>

// Maximum number of Smartport units
constexpr unsigned int kSmartportUnits = 8;
// Expected size of ROM 01 image
constexpr unsigned int kRom01Bytes = 131072;
// Expected size of ROM 03 image
constexpr unsigned int kRom03Bytes = 262144;

// Version number of this build
constexpr unsigned int kVersionMajor = VERSION_MAJOR;
constexpr unsigned int kVersionMinor = VERSION_MINOR;

extern std::filesystem::path data_dir;
extern std::filesystem::path config_file;
extern std::string font40_file;
extern std::string font80_file;
extern std::string rom_file;
extern std::string s5d1;
extern std::string s5d2;
extern std::string s6d1;
extern std::string s6d2;
extern std::string hd[kSmartportUnits];

extern bool rom03;
extern unsigned int ram_size;
extern bool pal;
extern bool enable_trace;

extern bool configure(const int, const char **);