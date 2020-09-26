/**
 * XGS: The Linux GS TestRunner
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * The TestRunner class is a stripped down emulator designed to run
 * binary test suites.
 */
#include "TestRunner.h"

#include <sys/time.h>
#include <sys/timerfd.h>

#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>

#include <fstream>
#include <boost/program_options.hpp>

#include "emulator/System.h"


#include "debugger/Debugger.h"
#include "M65816/Processor.h"

#if __has_include(<filesystem>)
    #include <filesystem>
    namespace fs = std::filesystem;
    using std::filesystem::path;
#else
    #include <experimental/filesystem>
    namespace fs = std::experimental::filesystem;
    using std::experimental::filesystem::path;
#endif

namespace po = boost::program_options;

using std::cerr;
using std::endl;
using std::string;

TestRunner::TestRunner()
{
}

TestRunner::~TestRunner()
{
    delete cpu;
    delete sys;
    delete dbg;
    delete [] ram;
}

bool TestRunner::setup(const int argc, const char** argv)
{
    if (!loadConfig(argc, argv)) {
        return false;
    }

    cpu = new M65816::Processor();
    sys = new System(false);
    dbg = new Debugger();

    sys->installProcessor(cpu);
    sys->installMemory(ram, 0, ram_pages, FAST);
    sys->installMemory(ram + 0xFF00, 0xFFFF, 1, FAST); // hack until we fix the forcing of vector reads to page FFFF
    sys->installDebugger(dbg);
    sys->reset();

    return true;
}

void TestRunner::run()
{
    cpu->PBR = start_address >> 16;
    cpu->PC  = start_address & 0xFFFF;

    while (true) {
        uint8_t last_PBR = cpu->PBR;
        uint16_t last_PC = cpu->PC;

        unsigned int num_cycles = cpu->runUntil(1);

        sys->cycle_count += num_cycles;
        sys->vbl_count++;

        if ((cpu->PBR == last_PBR) && (cpu->PC == last_PC)) {
            cerr << "\nTest suite trap encountered:\n\n";

            dbg->enableTrace();
            cpu->runUntil(1);
            cpu->runUntil(1); // give debugger a chance to show that last instruction

            break;
        }
    }
}

/**
 * Load the entirety of a file into a uint8_t buffer.
 */
unsigned int TestRunner::loadFile(const string& filename, const unsigned int maximum_size, uint8_t *buffer)
{
    std::uintmax_t bytes = maximum_size;
    std::ifstream ifs;

    ifs.open(filename, std::ifstream::binary);
    ifs.read((char *) buffer, bytes);
    ifs.close();

    return ifs.gcount();
}

bool TestRunner::loadConfig(const int argc, const char **argv)
{
    string bin_file;
    string bin_origin;
    unsigned int ram_size;

    po::options_description cli_options("Test Suite Options");
    cli_options.add_options()
        ("help",      "Print help message")
        ("version,v", "Print version string")
        ("file,f",   po::value<string>(&bin_file)->default_value("testsuite.bin"),  "Name of binary file to load")
        ("origin,o", po::value<string>(&bin_origin)->default_value("0x400"),        "Origin address of binary file")
        ("ram",      po::value<unsigned int>(&ram_size)->default_value(64),         "Set RAM size in KB");

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

        ram = new uint8_t[ram_size * 1024];
        ram_pages = ram_size << 2;

/*
        std::stringstream ss;
        ss << std::hex << bin_origin;
        ss >> start_address;
*/
        start_address = 0x400;

        // load test suite machine code file
        unsigned int bytes = loadFile(bin_file, 65536, ram);
    }
    catch (std::exception& e) { 
        cerr << "ERROR: " << e.what() << endl << endl;

        return false;
    } 

    return true;
}
