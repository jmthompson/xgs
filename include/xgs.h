#pragma once

#include "common.h"

extern vbls_t vbl_count;

extern bool setup(const int, const char **);
extern void reset(void);
extern void run(void);
extern void tick(void);

extern float getSpeed(void);
extern float getMaxSpeed(void);
extern void setMaxSpeed(float);
extern void handleWdm(std::uint8_t);
