/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "M65816/types.h"

class System;

class Debugger {
    private:
        const unsigned int kMaxInstLen = 4;

        System *system;

        /**
         * True when the CPU is fetching an instruction
         */
        bool ins_fetch = false;

        bool vector_fetch = false;
        uint16_t vector_addr;
        uint16_t vector_data;

        /**
         * The instruction being fetched
         */
        struct {
            std::uint8_t pbr;
            std::uint16_t pc;

            unsigned int len;

            std::vector<unsigned int> bytes;
        } cpu_instr;

        bool trace = false;

        void handleInstructionRead(const std::uint8_t, const std::uint16_t, const std::uint8_t);
        void handleVectorRead(const std::uint8_t, const std::uint16_t, const std::uint8_t);

        std::string renderArgument();
        std::string renderBytes();

    public:
        Debugger() = default;
        ~Debugger() = default;

        void attach(System *theSystem)
        {
            system = theSystem;
        }

        void enableTrace() { trace = true; }

        std::uint8_t memoryRead(const uint8_t bank, const uint16_t address, const uint8_t val, const M65816::mem_access_t type);
        std::uint8_t memoryWrite(const uint8_t bank, const uint16_t address, const uint8_t val, const M65816::mem_access_t type);
};

#endif // DEBUGGER_H
