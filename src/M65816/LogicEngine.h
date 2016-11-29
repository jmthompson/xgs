/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

template <typename MemSizeType, typename IndexSizeType, typename StackSizeType, const uint16_t StackOffset>
class LogicEngine {
    private:
        Processor *cpu;
        SystemBoard *board;

        const MemSizeType n_bit = 1 << ((sizeof(MemSizeType) * 8) - 1);
        const MemSizeType v_bit = 1 << ((sizeof(MemSizeType) * 8) - 2);

        const unsigned int m_max = sizeof(MemSizeType) == 2? 0xFFFF : 0xFF;

        /**
        * Should be 0 for native mode or 0x0100 for emulation mode.
        * This provides the upper 8 bits of the (bank 0) stack pointer
        * when in emulation mode with an 8-bit SP.
        */
        const uint16_t stack_offset = StackOffset;

        /**
         * References to the CPU registers in the parent object, for faster
         * access. In addition the A, X, Y, and S registers are adjusted for
         * the specific CPU mode we are trying to emulate.
         */

        StatusRegister& SR;
        uint16_t&       D;
        StackSizeType&  S;
        uint16_t&       PC;
        uint8_t&        PBR;
        uint8_t&        DBR;

        MemSizeType&   A;
        IndexSizeType& X;
        IndexSizeType& Y;

        /**
         * The address and bank of the operand address
         */
        uint16_t operand_addr;
        uint8_t  operand_bank;

        /**
         * The operand union allows working with an operand
         * that is the size of the memory width, the index
         * width, or a specific with (byte or word).
         */
        union {
            MemSizeType m;
            IndexSizeType x;
            uint8_t b;
            uint16_t w;
        } operand;

#include "AddressingModes.hxx"
#include "Operations.hxx"

    public:
        LogicEngine(Processor *parent) : cpu(parent), board(parent->board), SR(parent->SR), D(parent->D), S(parent->S), PC(parent->PC), PBR(parent->PBR), DBR(parent->DBR), A(parent->A), X(parent->X), Y(parent->Y) { }
        ~LogicEngine() = default;


#include "ExecuteOpcode.hxx"

};
