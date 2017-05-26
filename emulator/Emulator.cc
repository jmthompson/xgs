/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * The Emulator class handles the task of instantiating and running
 * a copy of the emulator.
 */

#include <sys/time.h>
#include <sys/timerfd.h>

#include <iostream>
#include <stdexcept>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/program_options.hpp>

#include <SDL.h>
#include "imgui/imgui.h"

#include "Emulator.h"
#include "GUI.h"
#include "System.h"
#include "Video.h"

#include "adb/ADB.h"
#include "doc/DOC.h"
#include "mega2/Mega2.h"
#include "vgc/VGC.h"

#include "disks/IWM.h"
#include "disks/Smartport.h"
#include "disks/VirtualDisk.h"

#include "debugger/Debugger.h"
#include "M65816/Processor.h"

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using std::cerr;
using std::endl;
using std::string;

// avoid having to constantly reallocate this
static struct timeval tv;

static long now()
{
    gettimeofday(&tv, NULL);

    return (long) (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

Emulator::Emulator()
{
}

Emulator::~Emulator()
{
    close(timer_fd);

    delete cpu;
    delete sys;
    delete mega2;
    delete vgc;

    delete [] rom;
    delete [] fast_ram;
    delete [] slow_ram;
}

bool Emulator::setup(const int argc, const char** argv)
{
    if (!loadConfig(argc, argv)) {
        return false;
    }

    struct timespec ts;

    clock_getres(CLOCK_MONOTONIC, &ts);
    cerr << "Native timer resolution is " << ts.tv_nsec << " ns" << endl;

    if ((timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) < 0) {
        throw std::runtime_error("Failed to create timer");
    }

    last_time = now();

    maximum_speed = 2.8;
    framerate = pal? 50 : 60;

    for (int i = 0 ; i < framerate; ++i) {
        times[i] = 0.0;
        cycles[i] = 0;
    }

    total_time   = 0.0;
    total_cycles = 0;
    last_cycles  = 0;

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK) < 0) {
        throw std::runtime_error("Failed to initialize SDL");
    }

    video = new Video(VGC::kPixelsPerLine, VGC::kLinesPerFrame * 2);

    GUI::initialize();

    cpu = new M65816::Processor();
    sys = new System(rom03);

    mega2 = new Mega2();

    vgc = new VGC();
    vgc->setMemory(slow_ram);
    vgc->setFont40(font_40col, font_40col + kFont40Bytes);
    vgc->setFont80(font_80col, font_80col + kFont80Bytes);

    adb  = new ADB();
    iwm  = new IWM();
    smpt = new Smartport();

    doc  = new DOC();
    doc->setOutputDevice(nullptr);

    sys->installProcessor(cpu);

    sys->installMemory(rom, rom_start_page, rom_pages, ROM);
    sys->installMemory(fast_ram, 0, fast_ram_pages, FAST);
    sys->installMemory(slow_ram, 0xE000, 512, SLOW);

    sys->installDevice("mega2", mega2);
    sys->installDevice("vgc", vgc);
    sys->installDevice("adb", adb);
    sys->installDevice("doc", doc);
    sys->installDevice("iwm", iwm);
    sys->installDevice("smpt",smpt);

    sys->setWdmHandler(0xC7, smpt);
    sys->setWdmHandler(0xC8, smpt);

#ifdef ENABLE_DEBUGGER
    Debugger *dbg = new Debugger();

    if (debugger.trace) {
        dbg->enableTrace();
    }

    sys->installDebugger(dbg);
#endif

    sys->reset();

    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        if (hd[i].length()) {
            VirtualDisk *vd = new VirtualDisk(hd[i]);
            smpt->mountImage(i, vd);
        }
    }

    return true;
}

void Emulator::run()
{
    uint64_t exp;

    current_frame = 0;

    timer_interval = 1000000000 / framerate;
    cerr << boost::format("Timer period is %d Hz (%d ns)\n") % framerate % timer_interval;

    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_nsec = timer_interval;
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_nsec    = 1000000;

    if (timerfd_settime(timer_fd, 0, &timer, NULL) < 0) {
        throw std::runtime_error("Failed to set timer");
    }

    running = true;

    while (running) {
        ssize_t len = read(timer_fd, &exp, sizeof(exp));

        if (len != sizeof(exp)) {
            cerr << boost::format("Failed to read timer: %s") % strerror(errno) << endl;
        }

        tick();
        pollForEvents();
    }
}

void Emulator::tick()
{
    target_speed = mega2->sw_fastmode? maximum_speed : 1.0;

    unsigned int cycles_per = (1000000/(VGC::kLinesPerFrame * framerate)) * target_speed;

    for (unsigned int line = 0; line < VGC::kLinesPerFrame ; ++line) {
        unsigned int num_cycles = cpu->runUntil(cycles_per);

        sys->cycle_count += num_cycles;

        for (unsigned int dt = 0 ; dt < doc_ticks[line % 19] ; ++dt) {
            doc->microtick(0);
        }

        vgc->microtick(line);
        mega2->microtick(line);
    }

    mega2->tick(current_frame);
    vgc->tick(current_frame);
    iwm->tick(current_frame);

    sys->vbl_count++;

    if (++current_frame == framerate) {
        current_frame = 0;
    }

    cycles_t diff_cycles = sys->cycle_count - last_cycles;

    last_cycles = sys->cycle_count;

    video->startFrame();
    video->drawFrame(vgc->frame_buffer, vgc->video_width, vgc->video_height);

    GUI::newFrame(video->window);

    if (show_status_bar) {
        GUI::drawStatusBar(*this);
    }

    if (show_menu) {
        GUI::drawMenu(*this);
    }

    ImGui::Render();
    video->endFrame();

    this_time = now();

    long diff_time = this_time - last_time;

    last_time = this_time;

    total_time -= times[current_frame];
    times[current_frame] = diff_time;
    total_time += diff_time;

    total_cycles -= cycles[current_frame];
    cycles[current_frame] = diff_cycles;
    total_cycles += diff_cycles;

    actual_speed = ((float) total_cycles / (float) total_time) / 1000.0;
}

void Emulator::pollForEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {

        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_HOME: // Control-Home
                    if (event.key.keysym.mod & KMOD_LCTRL) {
                        sys->reset();
                    }

                    continue;
                case SDLK_RCTRL:
                    //mouse_grabbed = !mouse_grabbed;

                    //SDL_SetRelativeMouseMode(mouse_grabbed? SDL_TRUE : SDL_FALSE);

                    continue;
                case SDLK_F1:
                    show_menu = !show_menu;

                    continue;
                case SDLK_F3:
                    show_status_bar = !show_status_bar;

                    continue;
                case SDLK_F11:
                    fullscreen = !fullscreen;

                    video->setFullscreen(fullscreen);

                    continue;
#ifdef ENABLE_DEBUGGER
                case SDLK_PAUSE:
                    sys->debugger->toggleTrace();

                    continue;

                case SDLK_F12:
                    sys->cpu->nmi();

                    continue;
#endif
                default:
                    break;
            }
        }

        if ((event.type == SDL_WINDOWEVENT) && (event.window.event == SDL_WINDOWEVENT_RESIZED)) {
            video->onResize((unsigned int) event.window.data1, (unsigned int) event.window.data2);
        }
        else if ((event.type == SDL_QUIT) ||
            ((event.type == SDL_WINDOWEVENT) && (event.window.event == SDL_WINDOWEVENT_CLOSE))) {

            running = false;

            break;
        }
        else {
            adb->processEvent(event);
        }
    }
}

/**
 * Load the entirety of a file into a uint8_t buffer.
 */
unsigned int Emulator::loadFile(const std::string& filename, const unsigned int expected_size, uint8_t *buffer)
{
    fs::path p = data_dir / filename;

    if (!fs::exists(p)) {
        p = filename;
    }

    boost::uintmax_t bytes = fs::file_size(p);
    fs::ifstream ifs;

    //*buffer = new uint8_t[bytes];

    ifs.open(p, std::ifstream::binary);
    ifs.read((char *) buffer, bytes);
    ifs.close();

    if (bytes != expected_size) {
        string err = (boost::format("Error loading %s: expected %d bytes, but read %d\n") % filename % expected_size % bytes).str();

        throw std::runtime_error(err);
    }

    return bytes;
}

bool Emulator::loadConfig(const int argc, const char **argv)
{
    const char *p;
    unsigned int ram_size;
    string rom_file;
    string font40_file;
    string font80_file;

    if (p = std::getenv("XGS_DATA_DIR")) {
        data_dir = path(p);
    }
    else if (p = std::getenv("HOME")) {
        data_dir = path(p) / ".xgs";
    }
    else {
        return false;
    }

    cerr << "Using " << data_dir << " as XGS home directory" << endl;

    config_file = data_dir / "xgs.conf";

    po::options_description generic("Generic Options");
    generic.add_options()
        ("help",      "Print help message")
        ("version,v", "Print version string");

    po::options_description emulator("Emulator Options");
    emulator.add_options()
        ("trace",    po::bool_switch(&debugger.trace)->default_value(false), "Enable trace")
        ("rom03,3",  po::bool_switch(&rom03)->default_value(false),          "Enable ROM 03 emulation")
        ("pal",      po::bool_switch(&pal)->default_value(false),            "Enable PAL (50 Hz) mode")
        ("romfile",  po::value<string>(&rom_file)->default_value("xgs.rom"),        "Name of ROM file to load")
        ("ram",      po::value<unsigned int>(&ram_size)->default_value(1024),       "Set RAM size in KB")
        ("font40",   po::value<string>(&font40_file)->default_value("xgs40.fnt"),   "Name of 40-column font to load")
        ("font80",   po::value<string>(&font80_file)->default_value("xgs80.fnt"),   "Name of 80-column font to load");

    po::options_description vdisks("Virtual Disk Options");

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
        
    try { 
        fs::ifstream cfs{config_file};

        if (cfs.is_open()) {
        	po::store(po::parse_config_file(cfs, config_file_options), vm);

            cfs.close();
        }

        po::store(po::command_line_parser(argc, argv).options(cli_options).run(), vm);
 
        if (vm.count("help")) { 
            cerr << cli_options << endl << endl;

            return false;
        }
        else {
            po::notify(vm);
        } 

        rom_pages      = rom03? 1024 : 512;
        rom_start_page = 0x10000 - rom_pages;
        rom = new uint8_t[rom_pages * 256];

        loadFile(rom_file, rom_pages * 256, rom);

        slow_ram = new uint8_t[65536*2];
        fast_ram = new uint8_t[ram_size * 1024];
        fast_ram_pages = ram_size << 2;

        loadFile(font40_file, sizeof(font_40col), font_40col);
        loadFile(font80_file, sizeof(font_80col), font_80col);
    }
    catch (std::exception& e) { 
        cerr << "ERROR: " << e.what() << endl << endl;

        return false;
    } 

    return true;
}
