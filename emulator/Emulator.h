#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdexcept>
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

class Emulator {
    public:
        Emulator(Config *);
        ~Emulator();

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
        Config* config;
        System* sys;
        M65816::Processor* cpu;

        Video *video;

        ADB*   adb;
        DOC*   doc;
        IWM*   iwm;
        Mega2* mega2;
        Smartport* smpt;
        VGC*   vgc;

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
};

class EmulatorTimerException : public std::runtime_error {};

#endif // EMULATOR_H_
