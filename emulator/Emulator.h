#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdexcept>
#include <boost/filesystem/path.hpp>

#include <SDL.h>

#include "emulator/common.h"

class Config;
class Video;

class System;
class ADB;
class DOC;
class IWM;
class Mega2;
class Smartport;
class VGC;

namespace M65816 {
    class Processor;
}

const unsigned int kRom01Bytes = 131072;
const unsigned int kRom03Bytes = 262144;

const unsigned int kFont40Bytes = 28672;
const unsigned int kFont80Bytes = 14336;

class Emulator {
    public:
        Emulator();
        ~Emulator();

        bool setup(const int, const char **);
        void run();
        void tick();

        Video* getVideo() { return video; }

        M65816::Processor* getCpu() { return cpu; }
        System* getSys() { return sys; }
        ADB* getAdb() { return adb; }
        DOC* getDoc() { return doc; }
        IWM* getIwm() { return iwm; }
        Smartport* getSmartport() { return smpt; }

        float getSpeed() { return actual_speed; }
        float getMaxSpeed() { return maximum_speed; }

    private:
        boost::filesystem::path data_dir;
        boost::filesystem::path config_file;

        System* sys;
        M65816::Processor* cpu;

        Video *video;

        ADB*   adb;
        DOC*   doc;
        IWM*   iwm;
        Mega2* mega2;
        Smartport* smpt;
        VGC*   vgc;

        uint8_t *rom;
        unsigned int rom_start_page;
        unsigned int rom_pages;

        uint8_t *fast_ram;
        unsigned int fast_ram_pages;

        uint8_t *slow_ram;

        bool rom03;
        bool use_debugger;
        bool pal;

        uint8_t font_40col[kFont40Bytes * 2];
        uint8_t font_80col[kFont80Bytes * 2];

        std::string s5d1;
        std::string s5d2;
        std::string s6d1;
        std::string s6d2;
        std::string hd[kSmartportUnits];

        struct {
            bool trace;
        } debugger;

        bool running;
        bool fullscreen;
        bool show_status_bar = true;
        bool show_menu = false;

        float maximum_speed;
        float actual_speed;
        float target_speed;

        // The timer we use for scheduling
        struct itimerspec timer;
        unsigned int timer_interval;
        int timer_fd;

        // The number of frames to produce every second
        unsigned int framerate;

        unsigned int current_frame;

        /**
         * To maintain sync we need 32 DOC cycles (38us) 
         * for every 19 microticks (63.7u). Each integer
         * in this area is how many DOC ticks to run
         * during that microtick.
         */
        unsigned int doc_ticks[19] = {
            2, 1, 2, 1, 3, 1,
            2, 1, 2, 1, 3, 1,
            2, 1, 2, 1, 3, 1,
            2
        };

        long times[60];
        long last_time;
        long this_time;
        long total_time;

        cycles_t cycles[60];
        cycles_t last_cycles;
        cycles_t total_cycles;

        void pollForEvents();

        unsigned int loadFile(const std::string&, const unsigned int, uint8_t *);
        bool loadConfig(const int, const char **);
};

class EmulatorTimerException : public std::runtime_error {};

#endif // EMULATOR_H_
