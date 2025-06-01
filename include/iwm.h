#pragma once

#include <cstdlib>
#include "disks/VirtualDisk.h"

namespace Iwm {

void start(void);
void stop(void);
void reset(void);
void tick(const unsigned int);
void loadDrive(const unsigned int, const unsigned int, VirtualDisk *);
void unloadDrive(const unsigned int, const unsigned int);
const unsigned int getMotorState(void);

} //namespace Iwm
