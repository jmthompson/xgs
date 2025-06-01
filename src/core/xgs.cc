/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
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

#include <SDL.h>
#include <boost/format.hpp>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/timerfd.h>
#include <time.h>

#include "common.h"
#include "config.h"
#include "debugger.h"
#include "gui.h"
#include "m65816.h"
#include "memory.h"
#include "soft_switches.h"
#include "video.h"
#include "xgs.h"

#include "m65816.h"

#include "adb.h"
#include "doc.h"
#include "iwm.h"
#include "mega2.h"
#include "scc.h"
#include "smartport.h"
#include "vgc.h"

using std::cerr;
using std::endl;
using std::string;

vbls_t vbl_count = 0;

static bool running;
static bool fullscreen;
static bool show_status_bar = true;
static bool show_menu = false;

static float maximum_speed;
static float actual_speed;
static float target_speed;

// The timer we use for scheduling
static struct itimerspec timer;
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
static unsigned int doc_ticks[19] = {2, 1, 2, 1, 3, 1, 2, 1, 2, 1,
                                     3, 1, 2, 1, 2, 1, 3, 1, 2};

static long times[60];
static long last_time;
static long this_time;
static long total_time;

static cycles_t cycles[60];
static cycles_t last_cycles;
static cycles_t total_cycles;

static inline long now() { return SDL_GetTicks(); }

bool setup(const int argc, const char **argv)
{
  try {
    if (!configure(argc, argv)) return true;
  } catch (std::exception &e) {
    cerr << "ERROR: " << e.what() << std::endl << std::endl;

    return false;
  }

  struct timespec ts;

  clock_getres(CLOCK_MONOTONIC, &ts);
  cerr << "Native timer resolution is " << ts.tv_nsec << " ns" << endl;

  if ((timer_fd = timerfd_create(CLOCK_MONOTONIC, 0)) < 0) {
    throw std::runtime_error("Failed to create timer");
  }

  last_time = now();

  // maximum_speed = 2.8;
  maximum_speed = 512.0;
  framerate = pal ? 50 : 60;

  for (int i = 0; i < framerate; ++i) {
    times[i] = 0.0;
    cycles[i] = 0;
  }

  total_time = 0.0;
  total_cycles = 0;
  last_cycles = 0;

  int err;

  if ((err = SDL_Init(
           SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK
       )) < 0) {
    std::cout << "error = " << err << std::endl;
    throw std::runtime_error(SDL_GetError());
  }

  setupMemory();

  Video::setup(Video::kPixelsPerLine, Video::kLinesPerFrame * 2);
  Gui::initialize(Video::window, Video::context);

  Mega2::start();
  Scc::start();
  Vgc::start();

  Adb::start();
  Doc::start();
  Iwm::start();
  Smartport::start();

  // Must come last so that the Mega2 device has a change to build the
  // language card, otherwise the CPU will fetch an invalid reset vector
  m65816::reset();

  for (unsigned int i = 0; i < kSmartportUnits; ++i) {
    if (hd[i].length()) {
      Smartport::mountImage(i, new VirtualDisk(hd[i]));
    }
  }

  if (s5d1.length()) {
    Iwm::loadDrive(5, 0, new VirtualDisk(s5d1));
  }
  if (s5d2.length()) {
    Iwm::loadDrive(5, 1, new VirtualDisk(s5d2));
  }
  if (s6d1.length()) {
    Iwm::loadDrive(6, 0, new VirtualDisk(s6d1));
  }
  if (s6d2.length()) {
    Iwm::loadDrive(6, 1, new VirtualDisk(s6d2));
  }

  return true;
}

void shutdown()
{
  close(timer_fd);

  Smartport::stop();
  Iwm::stop();
  Doc::stop();
  Adb::stop();
  Vgc::stop();
  Scc::stop();
  Mega2::stop();

  freeMemory();
}

/**
 * Reset the system to its powerup state.
 */
void reset()
{
  Mega2::reset();
  Scc::reset();
  Vgc::reset();
  Adb::reset();
  Doc::reset();
  Iwm::reset();
  Smartport::reset();

  // Must come last so that the Mega2 device has a change to build the
  // language card, otherwise the CPU will fetch an invalid reset vector
  m65816::reset();
}

void tick()
{
  target_speed = sw_fastmode ? maximum_speed : 1.0f;

  unsigned int cycles_per =
      (1000000 / (Video::kLinesPerFrame * framerate)) * target_speed;

  for (unsigned int line = 0; line < Video::kLinesPerFrame; ++line) {
    m65816::runUntil(cycles_per);
    for (unsigned int dt = 0; dt < doc_ticks[line % 19]; ++dt) {
      // Doc::microtick(0);
    }

    Vgc::microtick(line);
    Mega2::microtick(line);
  }

  Mega2::tick(current_frame);
  Vgc::tick(current_frame);
  Iwm::tick(current_frame);

  vbl_count++;

  if (++current_frame == framerate) {
    current_frame = 0;
  }

  cycles_t diff_cycles = m65816::total_cycles - last_cycles;

  last_cycles = m65816::total_cycles;

  Video::startFrame();
  Video::drawFrame(Video::frame_buffer, Vgc::video_width, Vgc::video_height);

  Gui::newFrame(Video::window);

  if (show_status_bar) {
    Gui::drawStatusBar();
  }

  if (show_menu) {
    Gui::drawMenu();
  }

  Gui::render();
  Video::endFrame();

  this_time = now();

  long diff_time = this_time - last_time;

  last_time = this_time;

  total_time -= times[current_frame];
  times[current_frame] = diff_time;
  total_time += diff_time;

  total_cycles -= cycles[current_frame];
  cycles[current_frame] = diff_cycles;
  total_cycles += diff_cycles;

  actual_speed = ((float)total_cycles / (float)total_time) / 1000.0f;
}

void pollForEvents()
{
  SDL_Event event;
  static bool mouse_grabbed = false;

  while (SDL_PollEvent(&event)) {

    Gui::processEvent(event);

    if (event.type == SDL_KEYDOWN) {
      switch (event.key.keysym.sym) {
      case SDLK_HOME: // Control-Home
        if (event.key.keysym.mod & KMOD_LCTRL) {
          reset();
        }

        continue;
      case SDLK_RCTRL:
        mouse_grabbed = !mouse_grabbed;

        SDL_SetRelativeMouseMode(mouse_grabbed ? SDL_TRUE : SDL_FALSE);

        continue;
      case SDLK_F1:
        show_menu = !show_menu;

        continue;
      case SDLK_F2:
        show_status_bar = !show_status_bar;

        continue;
#ifdef ENABLE_DEBUGGER
      case SDLK_F3:
        enable_trace = !enable_trace;

        continue;

      case SDLK_F12:
        m65816::nmi();

        continue;
#endif
      default:
        break;
      }
    }

    if ((event.type == SDL_WINDOWEVENT) &&
        (event.window.event == SDL_WINDOWEVENT_RESIZED)) {
      Video::onResize();
    } else if ((event.type == SDL_QUIT) ||
               ((event.type == SDL_WINDOWEVENT) &&
                (event.window.event == SDL_WINDOWEVENT_CLOSE))) {

      running = false;

      break;
    } else {
      Adb::processEvent(event);
    }
  }
}

void run()
{
  current_frame = 0;

  timer_interval = 1000000000 / framerate;
  cerr << boost::format("Timer period is %d Hz (%d ns)\n") % framerate %
              timer_interval;

  uint64_t exp;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_nsec = timer_interval;
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_nsec = 1000000;

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

void handleWdm(uint8_t command)
{
  if ((command == 0xC7) || (command == 0xC8)) Smartport::wdm(command);
}

float getSpeed(void) { return actual_speed; }
float getMaxSpeed(void) { return maximum_speed; }
void setMaxSpeed(float speed) { maximum_speed = speed; }
