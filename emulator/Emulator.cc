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
#include <boost/filesystem/path.hpp>

#include <SDL.h>

#include "Config.h"
#include "Emulator.h"
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

using std::cerr;
using std::endl;
using fs::path;

// avoid having to constantly reallocate this
static struct timeval tv;

static long now()
{
    gettimeofday(&tv, NULL);

    return (long) (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

Emulator::Emulator(Config *theConfig)
{
    config = theConfig;

    struct timespec ts;

    clock_getres(CLOCK_MONOTONIC, &ts);
    cerr << "Native timer resolution is " << ts.tv_nsec << " ns" << endl;

    if ((timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) < 0) {
        throw std::runtime_error("Failed to create timer");
    }

    last_time = now();

    framerate = config->pal? 50 : 60;

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

    Video::initialize(1280, 720);

    cpu = new M65816::Processor();
    sys = new System(config->rom03);

    mega2 = new Mega2();

    vgc = new VGC();
    vgc->setMemory(config->slow_ram);
    vgc->setFont40(config->font_40col[0], config->font_40col[1]);
    vgc->setFont80(config->font_80col[0], config->font_80col[1]);

    adb  = new ADB();
    iwm  = new IWM();
    smpt = new Smartport();

    doc  = new DOC();
    doc->setOutputDevice(nullptr);

    sys->installProcessor(cpu);

    sys->installMemory(config->rom, config->rom_start_page, config->rom_pages, ROM);
    sys->installMemory(config->fast_ram, 0, config->fast_ram_pages, FAST);
    sys->installMemory(config->slow_ram, 0xE000, 512, SLOW);

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

    if (config->debugger.trace) {
        dbg->enableTrace();
    }

    sys->installDebugger(dbg);
#endif

    sys->reset();

    for (unsigned int i = 0 ; i < kSmartportUnits ; ++i) {
        if (config->vdisks.smartport[i].length()) {
            VirtualDisk *hd = new VirtualDisk(config->vdisks.smartport[i]);
            smpt->mountImage(i, hd);
        }
    }
}

Emulator::~Emulator()
{
    close(timer_fd);

    delete cpu;
    delete sys;
    delete mega2;
    delete vgc;
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
    target_speed = mega2->sw_fastmode? 2.7 : 1.0;

    unsigned int cycles_per = (1024000/(VGC::kLinesPerFrame * framerate)) * target_speed;

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

    Video::drawFrame(vgc->frame_buffer, vgc->video_width, vgc->video_height);

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

    if (current_frame == 0) {
        cerr << boost::format("Current speed = %0.1f MHz (%ld cycles, %ld ms)\n") % actual_speed % total_cycles % total_time;
    }
}

void Emulator::handleWindowEvent(SDL_WindowEvent *event)
{
    switch (event->event) {
        case SDL_WINDOWEVENT_CLOSE:
            running = false;

            break;
        case SDL_WINDOWEVENT_ENTER:
            //SDL_ShowCursor(SDL_DISABLE);

            break;
        case SDL_WINDOWEVENT_LEAVE:
            //SDL_ShowCursor(SDL_ENABLE);

            break;
    }
}

void Emulator::pollForEvents()
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
            case SDL_KEYUP:
            case SDL_JOYBUTTONDOWN:
            case SDL_JOYBUTTONUP:
            case SDL_JOYAXISMOTION:
            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEMOTION:
                adb->processEvent(&event);

                break;
            case SDL_WINDOWEVENT:
                handleWindowEvent(&event.window);

                break;
            case SDL_QUIT:
                running = false;

                break;
        }
    }
}
