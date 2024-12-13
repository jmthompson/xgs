#pragma once

#include <iostream>
#include <map>
#include <boost/format.hpp>

#include "common.h"
#include "Device.h"
#include "Video.h"
#include "adb/ADB.h"
#include "doc/DOC.h"
#include "mega2/Mega2.h"
#include "scc/Zilog8530.h"
#include "vgc/VGC.h"
#include "disks/IWM.h"
#include "disks/Smartport.h"
#include "M65816/m65816.h"

using std::uint8_t;
using std::uint16_t;

namespace xgs {

extern vbls_t vbl_count;

extern bool setup(const int, const char **);
extern void reset(void);
extern void run(void);
extern void tick(void);

extern void installMemory(uint8_t *, const unsigned int, const unsigned int, mem_page_t);
extern void installDevice(const std::string&, Device *);

extern Video* getVideo(void);
extern Mega2* getMega2(void);
extern ADB* getAdb(void);
extern DOC* getDoc(void);
extern IWM* getIwm(void);
extern Zilog8530* getScc(void);
extern Smartport* getSmartport(void);
extern VGC* getVgc(void);
extern float getSpeed(void);
extern float getMaxSpeed(void);
extern void setMaxSpeed(float);
extern Device *getDevice(const std::string& name);
extern void handleWdm(uint8_t);
extern void mapRead(const unsigned int, const unsigned int);
extern void mapWrite(const unsigned int, const unsigned int);
extern void mapIO(const unsigned int);
extern void setShadowed(const unsigned int, const bool);
extern void setIoRead(const unsigned int&, Device *);
extern void setIoWrite(const unsigned int&, Device *);
extern void setWdmHandler(const unsigned int&, Device *);
extern MemoryPage& getPage(const unsigned int);
extern uint8_t sysRead(const uint8_t, const uint16_t);
extern void sysWrite(const uint8_t, const uint16_t, uint8_t);
extern uint8_t cpuRead(const uint8_t, const uint16_t, const m65816::mem_access_t);
extern void cpuWrite(const uint8_t, const uint16_t, uint8_t, const m65816::mem_access_t);
extern void raiseInterrupt(irq_source_t);
extern void lowerInterrupt(irq_source_t);

}; // namespace xgs
