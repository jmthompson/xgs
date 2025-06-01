#pragma once

#include <cstdlib>
#include <SDL.h>

namespace Adb {

// This is the version returned on a real ROM 03
constexpr unsigned int kADBVersion = 6;

// Size of the SKI buffer
constexpr unsigned int kInputBufferSize = 128;

/**
 * Structure for representing a SKI command in progress. The command will
 * begin execution after we have read read_bytes, and ends after write_bytes
 * are written.
 */

struct SKICommand {
    int command     = -1;
    int read_bytes  = -1;
    int write_bytes = -1;
};

void start(void);
void stop(void);
void reset(void);
void tick(const unsigned int);
void microtick(const unsigned int);
bool processEvent(SDL_Event&);

} // namespace Adb
