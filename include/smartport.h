#pragma once

#include <cstdint>
#include <cstdlib>

using std::uint8_t;

class VirtualDisk;

namespace Smartport {

struct SmartportCommand {
    int command     = -1;
    int read_bytes  = -1;
    int write_bytes = -1;
};

void start(void);
void stop(void);
void reset(void);
void wdm(const uint8_t);
void tick(const unsigned int);
void microtick(const unsigned int);
void mountImage(const unsigned int, VirtualDisk *);
void unmountImage(const unsigned int);

} // namespace Smartport