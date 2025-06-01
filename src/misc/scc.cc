/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2021 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This class implements the Zilog 8530 Serial Communications Controller.
 */

#include <cstdint>
#include <cstdlib>

#include "scc.h"

namespace Scc
{
void start(void) {}

void stop(void) {}

void reset(void) {}

void tick(const unsigned int frame_number) {}

void microtick(const unsigned int line_number) {}

} // namespace Scc