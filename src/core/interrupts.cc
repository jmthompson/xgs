/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2025 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "interrupts.h"
#include "m65816.h"

static bool irq_states[16];

static void updateIRQ()
{
    bool raised = false;

    for (unsigned int i = 0 ; i < 16 ; i++) {
        raised |= irq_states[i];
    }

    m65816::setIRQ(raised);
}

void raiseInterrupt(irq_source_t source)
{
    irq_states[source] = true;

//    cerr << boost::format("raiseInterrupt(%d)\n") % source;

    updateIRQ();
}

void lowerInterrupt(irq_source_t source)
{
    irq_states[source] = false;

//    cerr << boost::format("lowerInterrupt(%d)\n") % source;

    updateIRQ();
}

