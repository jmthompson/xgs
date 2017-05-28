#ifndef TESTSUITE_H_
#define TESTSUITE_H_

#include <stdexcept>

#include "emulator/common.h"

class System;
class Debugger;

namespace M65816 {
    class Processor;
}

const unsigned int kTickRate = 60;

class TestRunner {
    public:
        TestRunner();
        ~TestRunner();

        bool setup(const int, const char **);
        void run();

        M65816::Processor* getCpu() { return cpu; }
        System* getSys() { return sys; }

    private:
        System* sys;
        M65816::Processor* cpu;
        Debugger *dbg;

        uint8_t *ram;
        unsigned int ram_pages;

        uint32_t start_address;

        float target_speed = 1.0;

        // The timer we use for scheduling
        struct itimerspec timer;
        unsigned int timer_interval;
        int timer_fd;

        unsigned int loadFile(const std::string&, const unsigned int, uint8_t *);
        bool loadConfig(const int, const char **);
};

class TestRunnerTimerException : public std::runtime_error {};

#endif // TESTSUITE_H_
