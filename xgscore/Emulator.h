#ifndef EMULATOR_H_
#define EMULATOR_H_

#include <stdexcept>
#include <SDL.h>

#include "common.h"

class Config;
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
    private:
        Config *config;
        System *sys;
        M65816::Processor *cpu;

        ADB   *adb;
        DOC   *doc;
        IWM   *iwm;
        Mega2 *mega2;
        Smartport *smpt;
        VGC   *vgc;

        bool running;

        // The timer we use for scheduling
        struct itimerspec timer;
        unsigned int timer_interval;
        int timer_fd;

        // The number of frames to produce every second
        unsigned int framerate;

        float target_speed;
        float actual_speed;

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

        long getTime();

        void handleWindowEvent(SDL_WindowEvent *);
        void pollForEvents();

    public:
        Emulator(Config *);
        ~Emulator();

        void run();
        void tick();

        float getTargetSpeed() { return target_speed; }
        float getActualSpeed() { return actual_speed; }

        void setTargetSpeed(float mhz) { target_speed = mhz; }
};

class EmulatorTimerException : public std::runtime_error {};

#endif // EMULATOR_H_
