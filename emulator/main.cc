/**
 * XGS: The Linux GS Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

/*
 * This file contains the startup code that loads the configuration
 * and instantiates an Emulator instance.
 */

#include "emulator/Emulator.h"

int main (int argc, char **argv)
{
    Emulator *e = new Emulator();

    if ( e->setup(argc, const_cast<const char**>(argv) ) ) {
        e->run();
    }
    delete e; //call delete to make destructors run
    return 0;
}
