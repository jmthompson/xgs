#pragma once

#include <vector>

#include "emulator/common.h"

class Device {
protected:
    virtual std::vector<unsigned int>& ioReadList() = 0;
    virtual std::vector<unsigned int>& ioWriteList() = 0;

public:
    Device() = default;
    virtual ~Device() = default;

    virtual void start(void);
    virtual void stop(void);
    virtual void reset(void);

    virtual uint8_t read(const unsigned int& offset) = 0;
    virtual void write(const unsigned int& offset, const uint8_t& value) = 0;

    virtual void cop(const uint8_t) {}
    virtual void wdm(const uint8_t) {}

    virtual void tick(const unsigned int) {}
    virtual void microtick(const unsigned int) {}
};
