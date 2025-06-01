#pragma once

#include <cstdlib>
#include <filesystem>

using std::filesystem::path;

namespace Vgc
{
// Sizes of the 40/80 column fonts
constexpr unsigned int kFont40Bytes = 28672;
constexpr unsigned int kFont80Bytes = 14336;

/**
 * This is the difference (in seconds) between the IIGS's time
 * (secs since 1/1/04 00:00:00) and Unix time (secs since
 * 1/1/70 00:00:00).
 */
constexpr std::uint32_t kClockOffset = 2082826800;

// The current dimensions of the frame buffer
extern unsigned int video_width;
extern unsigned int video_height;

void start(void);
void stop(void);
void reset(void);
void tick(const unsigned int);
void microtick(const unsigned int);

} // namespace Vgc