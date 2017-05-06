#ifndef DEVICE_H_
#define DEVICE_H_

#include <vector>

#include "emulator/common.h"

class System;

class Device {
    protected:
        System  *system = nullptr;

        virtual std::vector<unsigned int>& ioReadList() = 0;
        virtual std::vector<unsigned int>& ioWriteList() = 0;

    public:
        Device() = default;
        virtual ~Device() = default;

        virtual void reset() = 0;
        virtual uint8_t read(const unsigned int& offset) = 0;
        virtual void write(const unsigned int& offset, const uint8_t& value) = 0;

        virtual void cop(const uint8_t) {}
        virtual void wdm(const uint8_t) {}

        virtual void attach(System *theSystem);
        virtual void detach() { system = nullptr; }

        virtual void tick(const unsigned int) {}
        virtual void microtick(const unsigned int) {}
};

#endif // DEVICE_H_
