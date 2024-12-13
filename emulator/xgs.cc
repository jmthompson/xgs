/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the functionality of both the FPI/CYA and
 * the Mega II, since they are pretty tightly entwined. As such it
 * emulates the bus interface used the CPU to access the system bus.
 */

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <SDL.h>

#ifndef _WIN32
    //windows doesn't have these
    #include <sys/time.h>
    #include <sys/timerfd.h>
#else
    //use chrono instead
    #include <chrono>
    #include <thread>
#endif

#include "common.h"
#include "Video.h"
#include "GUI.h"

#include "adb/ADB.h"
#include "doc/DOC.h"
#include "mega2/Mega2.h"
#include "scc/Zilog8530.h"
#include "vgc/VGC.h"
#include "disks/IWM.h"
#include "disks/Smartport.h"
#include "disks/VirtualDisk.h"

#include "debugger/Debugger.h"
#include "M65816/m65816.h"
#include "xgs.h"

using std::cerr;
using std::endl;
using std::uint8_t;
using std::string;

namespace xgs {

constexpr unsigned int kRom01Bytes = 131072;
constexpr unsigned int kRom03Bytes = 262144;
constexpr unsigned int kFont40Bytes = 28672;
constexpr unsigned int kFont80Bytes = 14336;
constexpr unsigned int kPageSize = 256;
constexpr unsigned int kBankSize = 65536;
constexpr unsigned int kNumPages = 65536;
constexpr unsigned int kMaxPage  = kNumPages - 1;

// The I/O page sits above addressable memory and can
// only be accessed via a read or write mapping.
constexpr unsigned int kIOPage = kMaxPage + 1;

vbls_t vbl_count = 0;

static bool is_rom03;

static Video* video;
static Mega2* mega2;
static ADB* adb;
static IWM* iwm;
static Smartport* smpt;
static Zilog8530* scc;
static DOC* doc;
static VGC* vgc;

static unsigned int read_map[kNumPages];
static unsigned int write_map[kNumPages];

static MemoryPage memory[kNumPages];

static std::map<std::string, Device *> devices;

static Device *io_read[kPageSize];
static Device *io_write[kPageSize];
static Device *wdm_handler[256];

static bool enable_trace = false;
static bool irq_states[16];

#if __has_include(<filesystem>)
    namespace fs = std::filesystem;
#else
    namespace fs = std::experimental::filesystem;
#endif

namespace po = boost::program_options;

static fs::path data_dir;
static fs::path config_file;

static uint8_t *rom;
static unsigned int rom_start_page;
static unsigned int rom_pages;

static uint8_t *fast_ram;
static unsigned int fast_ram_pages;

static uint8_t *slow_ram;

static bool rom03;
static bool pal;

static uint8_t font_40col[kFont40Bytes * 2];
static uint8_t font_80col[kFont80Bytes * 2];

static std::string s5d1;
static std::string s5d2;
static std::string s6d1;
static std::string s6d2;
static std::string hd[kSmartportUnits];

static bool running;
static bool fullscreen;
static bool show_status_bar = true;
static bool show_menu = false;

static float maximum_speed;
static float actual_speed;
static float target_speed;

// The timer we use for scheduling
#ifndef _WIN32
    static struct itimerspec timer;
#endif
static unsigned int timer_interval;
static int timer_fd;

// The number of frames to produce every second
static unsigned int framerate;

static unsigned int current_frame;

/**
 * To maintain sync we need 32 DOC cycles (38us) 
 * for every 19 microticks (63.7u). Each integer
 * in this area is how many DOC ticks to run
 * during that microtick.
 */
static unsigned int doc_ticks[19] = {
    2, 1, 2, 1, 3, 1,
    2, 1, 2, 1, 3, 1,
    2, 1, 2, 1, 3, 1,
    2
};

static long times[60];
static long last_time;
static long this_time;
static long total_time;

static cycles_t cycles[60];
static cycles_t last_cycles;
static cycles_t total_cycles;

static inline long now() { return SDL_GetTicks(); }

void setWdmHandler(const unsigned int& command, Device *device)
{
    wdm_handler[command] = device;
}

/**
 * Load the entirety of a file into a uint8_t buffer.
 */
static unsigned int loadFile(const std::string& filename, const unsigned int expected_size, uint8_t *buffer)
{
    fs::path p = data_dir / filename;

    if (!fs::exists(p)) {
        p = filename;
    }

    std::uintmax_t bytes = fs::file_size(p);
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

static bool loadConfig(const int argc, const char **argv)
{
    const char *p;
    unsigned int ram_size;
    string rom_file;
    string font40_file;
    string font80_file;

    if ((p = std::getenv("XGS_DATA_DIR"))) {
        data_dir = fs::path(p);
    }
    else if ((p = std::getenv("HOME"))) {
        data_dir = fs::path(p) / ".xgs";
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
        ("trace",    po::bool_switch(&enable_trace)->default_value(false), "Enable trace")
        ("rom03,3",  po::bool_switch(&rom03)->default_value(false),          "Enable ROM 03 emulation")
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
        
    try { 
        std::ifstream cfs{config_file};

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

void installMemory(uint8_t *mem, const unsigned int start_page, const unsigned int num_pages, mem_page_t type)
{
    unsigned int i, page;
    uint8_t *p;

    for (i = 0, page = start_page, p = mem; i < num_pages ; ++i, ++page, p += kPageSize) {
        memory[page].type  = type;
        memory[page].read  = p;
        memory[page].write = (type == ROM? nullptr : p);
    }
}

void installDevice(const string& name, Device *d)
{
    devices[name] = d;
    d->start();
}

bool setup(const int argc, const char** argv)
{
    if (!loadConfig(argc, argv)) {
        return false;
    }

#ifndef _WIN32
    struct timespec ts;

    clock_getres(CLOCK_MONOTONIC, &ts);
    cerr << "Native timer resolution is " << ts.tv_nsec << " ns" << endl;

    if ((timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) < 0) {
        throw std::runtime_error("Failed to create timer");
    }
#endif

    last_time = now();

    //maximum_speed = 2.8;
    maximum_speed = 512.0;
    framerate = pal? 50 : 60;

    for (int i = 0 ; i < framerate; ++i) {
        times[i] = 0.0;
        cycles[i] = 0;
    }

    total_time   = 0.0;
    total_cycles = 0;
    last_cycles  = 0;

    int err;

    if ((err = SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS|SDL_INIT_JOYSTICK)) < 0) {
        std::cout << "error = " << err << std::endl;
        throw std::runtime_error(SDL_GetError());
    }

    video = new Video(VGC::kPixelsPerLine, VGC::kLinesPerFrame * 2);

    GUI::initialize(video->window, video->context);

    mega2 = new Mega2();
    scc = new Zilog8530();

    vgc = new VGC();
    vgc->setMemory(slow_ram);
    vgc->setFont40(font_40col, font_40col + kFont40Bytes);
    vgc->setFont80(font_80col, font_80col + kFont80Bytes);

    adb  = new ADB();
    iwm  = new IWM();
    smpt = new Smartport();
    scc  = new Zilog8530();
    doc  = new DOC();

    adb->start();
    iwm->start();
    smpt->start();
    scc->start();

    doc->start();
    doc->setOutputDevice(nullptr);

    installMemory(rom, rom_start_page, rom_pages, ROM);
    installMemory(fast_ram, 0, fast_ram_pages, FAST);
    installMemory(slow_ram, 0xE000, 512, SLOW);

    installDevice("mega2", mega2);
    installDevice("scc", scc);
    installDevice("vgc", vgc);
    installDevice("adb", adb);
    installDevice("doc", doc);
    installDevice("iwm", iwm);
    installDevice("smpt", smpt);

    setWdmHandler(0xC7, smpt);
    setWdmHandler(0xC8, smpt);

    if (enable_trace) {
        Debugger::enableTrace();
    }

    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        if (hd[i].length()) {
            smpt->mountImage(i, new VirtualDisk(hd[i]));
        }
    }

    if (s5d1.length()) {
        iwm->loadDrive(5, 0, new VirtualDisk(s5d1));
    }
    if (s5d2.length()) {
        iwm->loadDrive(5, 1, new VirtualDisk(s5d2));
    }
    if (s6d1.length()) {
        iwm->loadDrive(6, 0, new VirtualDisk(s6d1));
    }
    if (s6d2.length()) {
        iwm->loadDrive(6, 1, new VirtualDisk(s6d2));
    }

    for (unsigned int page = 0 ; page < kNumPages;  page++) {
        read_map[page] = write_map[page] = page;
    }

    return true;
}

void shutdown()
{
#ifndef _WIN32
    close(timer_fd);
#endif
    delete mega2;
    delete scc;
    delete vgc;
    delete video;

    delete [] rom;
    delete [] fast_ram;
    delete [] slow_ram;
}

/**
 * Reset the system to its powerup state.
 */
void reset()
{
    for (auto const& iter : devices) {
        iter.second->reset();
    }

    // Must come last so that the Mega2 device has a change to build the
    // language card, otherwise the CPU will fetch an invalid reset vector
    m65816::reset();
}

void tick()
{
    target_speed = mega2->sw_fastmode? maximum_speed : 1.0f;

    unsigned int cycles_per = (1000000/(VGC::kLinesPerFrame * framerate)) * target_speed;

    for (unsigned int line = 0; line < VGC::kLinesPerFrame ; ++line) {
        m65816::runUntil(cycles_per);
#if 0
        for (unsigned int dt = 0 ; dt < doc_ticks[line % 19] ; ++dt) {
            doc->microtick(0);
        }

        vgc->microtick(line);
        mega2->microtick(line);
#endif
    }

    mega2->tick(current_frame);
    vgc->tick(current_frame);
    iwm->tick(current_frame);

    vbl_count++;

    if (++current_frame == framerate) {
        current_frame = 0;
    }

    cycles_t diff_cycles = m65816::total_cycles - last_cycles;

    last_cycles = m65816::total_cycles;

    video->startFrame();
    video->drawFrame(vgc->frame_buffer, vgc->video_width, vgc->video_height);

    GUI::newFrame(video->window);

    if (show_status_bar) {
        GUI::drawStatusBar();
    }

    if (show_menu) {
        GUI::drawMenu();
    }

    GUI::render();
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

    actual_speed = ((float) total_cycles / (float) total_time) / 1000.0f;
}

void pollForEvents()
{
    SDL_Event event;
    static bool mouse_grabbed =false;

    while (SDL_PollEvent(&event)) {

        GUI::processEvent(event);

        if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_HOME: // Control-Home
                    if (event.key.keysym.mod & KMOD_LCTRL) {
                        reset();
                    }

                    continue;
                case SDLK_RCTRL:
                    mouse_grabbed = !mouse_grabbed;

                    SDL_SetRelativeMouseMode(mouse_grabbed? SDL_TRUE : SDL_FALSE);

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
                    Debugger::toggleTrace();

                    continue;

                case SDLK_F12:
                    m65816::nmi();

                    continue;
#endif
                default:
                    break;
            }
        }

        if ((event.type == SDL_WINDOWEVENT) && (event.window.event == SDL_WINDOWEVENT_RESIZED)) {
            video->onResize();
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

void run()
{
    current_frame = 0;

    timer_interval = 1000000000 / framerate;
    cerr << boost::format("Timer period is %d Hz (%d ns)\n") % framerate % timer_interval;

#ifndef _WIN32
    uint64_t exp;
    timer.it_interval.tv_sec  = 0;
    timer.it_interval.tv_nsec = timer_interval;
    timer.it_value.tv_sec     = 0;
    timer.it_value.tv_nsec    = 1000000;

    if (timerfd_settime(timer_fd, 0, &timer, NULL) < 0) {
        throw std::runtime_error("Failed to set timer");
    }
#else
    using clk = std::chrono::steady_clock; 
    auto next_tick = clk::now() + std::chrono::nanoseconds(timer_interval);
#endif

    running = true;

    while (running) {

#ifndef _WIN32
        ssize_t len = read(timer_fd, &exp, sizeof(exp));

        if (len != sizeof(exp)) {
            cerr << boost::format("Failed to read timer: %s") % strerror(errno) << endl;
        }
#else
        next_tick = clk::now() + std::chrono::nanoseconds(timer_interval);
#endif

        tick();
        pollForEvents();
#ifdef _WIN32
    std::this_thread::sleep_until(next_tick);
#endif
    }
}

void handleWdm(uint8_t command)
{
    if (Device *dev = wdm_handler[command]) {
        return dev->wdm(command);
    }
}

void mapRead(const unsigned int src_page, const unsigned int dst_page)
{
    read_map[src_page] = dst_page;
}

void mapWrite(const unsigned int src_page, const unsigned int dst_page)
{
    write_map[src_page] = dst_page;
}

void mapIO(const unsigned int src_page)
{
    read_map[src_page] = write_map[src_page] = kIOPage;
}

void setShadowed(const unsigned int page, const bool isShadowed)
{
    memory[page].swrite = isShadowed? memory[(page & 0x01FF) | 0xE000].write : nullptr;
}

void setIoRead(const unsigned int& offset, Device *device)
{
    io_read[offset] = device;
}

void setIoWrite(const unsigned int& offset, Device *device)
{
    io_write[offset] = device;
}

MemoryPage& getPage(const unsigned int page)
{
    return memory[page];
}

/**
 * Read the value from a memory location, honoring page remapping, but
 * ignoring I/O areas. Used internally by the emulator to access memory.
 */
uint8_t sysRead(const uint8_t bank, const uint16_t address)
{
    const unsigned int page_no = read_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = memory[page_no];

    if (page.read) {
        return page.read[offset];
    }
    else {
        return 0;
    }
}

/**
 * Write a value from a memory location, honoring page remapping, but
 * ignoring I/O areas. Used internally by the emulator to access memory.
 */
void sysWrite(const uint8_t bank, const uint16_t address, uint8_t val)
{
    const unsigned int page_no = write_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = memory[page_no];

    if (page.write) {
        page.write[offset] = val;

        if (page.swrite) {
            page.swrite[offset] = val;
        }
    }
}

uint8_t cpuRead(const uint8_t bank, const uint16_t address, const m65816::mem_access_t type)
{
    const unsigned int page_no = read_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = (type == m65816::VECTOR)? memory[page_no|0xFF00] : memory[page_no];
    uint8_t val;

    if (page_no == kIOPage) {
        if (Device *dev = io_read[offset]) {
            val = dev->read(offset);
        }
        else {
            val = 0; // FIXME: should be random
        }
    }
    else if (page.read) {
        val = page.read[offset];
    }
    else {
        val = 0;
    }

    if (enable_trace) {
        return Debugger::memoryRead(bank, address, val, type);
    }
    else {
        return val;
    }
}

void cpuWrite(const uint8_t bank, const uint16_t address, uint8_t val, const m65816::mem_access_t type)
{
    const unsigned int page_no = write_map[(bank << 8) | (address >> 8)];
    const unsigned int offset  = address & 0xFF;
    MemoryPage& page = memory[page_no];

    if (enable_trace) { val = Debugger::memoryWrite(bank, address, val, type); }

    if (page_no == kIOPage) {
        if (Device *dev = io_write[offset]) {
            dev->write(offset, val);
        }
    }
    else if (page.write) {
        page.write[offset] = val;

        if (page.swrite) {
            page.swrite[offset] = val;
        }
    }
}

static void updateIRQ()
{
    bool raised = false;

    for (unsigned int i = 0 ; i < 16 ; i++) {
        raised |= irq_states[i];
    }

    m65816::setIRQ(raised);
}

void raiseInterrupt(irq_source_t source)
{
    irq_states[source] = true;

//    cerr << boost::format("raiseInterrupt(%d)\n") % source;

    updateIRQ();
}

void lowerInterrupt(irq_source_t source)
{
    irq_states[source] = false;

//    cerr << boost::format("lowerInterrupt(%d)\n") % source;

    updateIRQ();
}

Video* getVideo(void) { return video; }
Mega2* getMega2(void) { return mega2; }
ADB* getAdb(void) { return adb; }
DOC* getDoc(void) { return doc; }
IWM* getIwm(void) { return iwm; }
Zilog8530* getScc(void) { return scc; }
Smartport* getSmartport(void) { return smpt; }
VGC* getVgc(void) { return vgc; }

float getSpeed(void) { return actual_speed; }
float getMaxSpeed(void) { return maximum_speed; }
void setMaxSpeed(float speed) { maximum_speed = speed; }

}; // namespace xgs
