#pragma once

#include <cstdint>
#include <cstdlib>
#include <SDL.h>

using std::uint16_t;
using std::uint32_t;
using std::uint8_t;

struct AudioSample {
  float left;
  float right;
};

namespace Doc
{

constexpr unsigned int kNumOscillators = 32;
constexpr unsigned int kInterruptStackSize = 256;
constexpr unsigned int kSampleRate = 26320;
constexpr unsigned int kAudioBufferSize = 4096;

void start(void);
void stop(void);
void reset(void);
void microtick(const unsigned int);
// void clickSpeaker();

} // namespace Doc