/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#pragma once

#include "platform.h"
#include "registers.h"

namespace m65816 {

extern unsigned int num_cycles;
extern bool stopped;
extern bool waiting;
extern bool nmi_pending;
extern bool abort_pending;
extern bool irq_pending;

/**
 * The address and bank of the operand address. These are common to all cores.
 */
static uint16_t operand_addr;
static uint8_t  operand_bank;

extern void modeSwitch();
extern void loadVector(const uint16_t);

using registers::D;
using registers::DBR;
using registers::PBR;
using registers::PC;
using registers::SR;

/**
 * The core class encapsulates the code and registers for a specific processor mode (M/X/e).
 * This is never instantiated; all members and methods are static.
 */
template <typename MemType, typename IndexType, typename StackType, const uint16_t StackOffset>
class core {
    using mem_type = MemType;
    using index_type = IndexType;
    using stack_type = StackType;

    static constexpr uint16_t stack_offset = StackOffset;

    /**
     * Value of the N and V bits in the current accmulator size
     */
    static constexpr mem_type n_bit = 1 << ((sizeof(mem_type) * 8) - 1);
    static constexpr mem_type v_bit = 1 << ((sizeof(mem_type) * 8) - 2);

    /**
     * Maximum value that can be stored in mem_type
     */
    static constexpr unsigned int m_max = sizeof(mem_type) == 2? 0xFFFF : 0xFF;

    /**
     * Number of nibbles in mem_type. Used by the BCD routines
     */
    static constexpr unsigned int m_nibbles = sizeof(mem_type) * 2;

public:
    /**
     *
     * Type-casted references to the processor registers. These are initialized
     * at startup in m65816.cc.
     */
    static mem_type& A;
    static index_type& X;
    static index_type& Y;
    static stack_type& S;

    /**
     * The operand union allows working with an operand that is the size of the memory width,
     * the index width, or a specific with (byte or word).
     */
    typedef union {
        mem_type m;
        index_type x;
        uint8_t b;
        uint16_t w;
    } operand_type;

    static operand_type operand;

    /**
     * If the CPU is in emulation mode, and the low byte of D is 0x00,
     * then wrap the address at the DP boundary.
     */
    static uint16_t wrapDirectPage(uint16_t address)
    {
        if (!(D & 0xFF) && stack_offset) {
            return (D & 0xFF00) | (address & 0xFF);
        }
        else {
            return address;
        }
    }

    /**** The getAddress methods retrieve the operand address for a given addressing mode. ****/

    static void getAddress_a()
    {
        operand_addr = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);
        operand_bank = DBR;
        PC += 2;
    }

    static void getAddress_al()
    {
        operand_addr = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);
        operand_bank = MREAD(PBR, PC + 2, INSTR);
        PC += 3;
    }

    static void getAddress_d()
    {
        checkDirectPageAlignment();

        operand_addr = wrapDirectPage(D + MREAD(PBR, PC++, INSTR));
        operand_bank = 0;
    }

    static void getAddress_dix()
    {
        checkDirectPageAlignment();

        uint16_t tmp = D + MREAD(PBR, PC++, INSTR);
        uint8_t lo = MREAD(0, wrapDirectPage(tmp), OPADDR);
        uint8_t hi = MREAD(0, wrapDirectPage(tmp + 1), OPADDR);

        operand_addr = lo | (hi << 8);
        operand_bank = DBR;

        tmp = operand_addr;
        operand_addr += Y;

        if (operand_addr < tmp) operand_bank++;

        checkDataPageCross(tmp);
    }

    static void getAddress_dixl()
    {
        uint16_t tmp = D + MREAD(PBR, PC++, INSTR);

        checkDirectPageAlignment();

        operand_addr = MREAD(0, tmp, OPADDR) | (MREAD(0, tmp + 1, OPADDR) << 8);
        operand_bank = MREAD(0, tmp + 2, OPADDR);

        tmp = operand_addr;
        operand_addr += Y;

        if (operand_addr < tmp) operand_bank++;
    }

    // (DIRECT,X)
    static void getAddress_dxi()
    {
        uint16_t tmp = D + X + MREAD(PBR, PC++, INSTR);

        checkDirectPageAlignment();

        uint8_t lo = MREAD(0, wrapDirectPage(tmp), OPADDR);
        uint8_t hi = MREAD(0, wrapDirectPage(tmp + 1), OPADDR);

        operand_addr = lo | (hi << 8);
        operand_bank = DBR;
    }

    // DIRECT,X
    static void getAddress_dxx()
    {
        checkDirectPageAlignment();

        uint8_t loc = MREAD(PBR, PC++, INSTR);

        operand_addr = wrapDirectPage(D + loc + X);
        operand_bank = 0;
    }

    // DIRECT,Y
    static void getAddress_dxy()
    {
        checkDirectPageAlignment();

        uint8_t loc = MREAD(PBR, PC++, INSTR);

        operand_addr = wrapDirectPage(D + loc + Y);
        operand_bank = 0;
    }

    static void getAddress_axx()
    {
        operand_addr = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);
        operand_bank = DBR;

        uint16_t tmp = operand_addr;
        operand_addr += X;

        if (operand_addr < tmp) operand_bank++;

        checkDataPageCross(tmp);

        PC += 2;
    }

    static void getAddress_axy()
    {
        operand_addr = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);
        operand_bank = DBR;

        uint16_t tmp = operand_addr;

        operand_addr += Y;

        if (operand_addr < tmp) operand_bank++;

        checkDataPageCross(tmp);

        PC += 2;
    }

    static void getAddress_alxx()
    {
        uint32_t address = MREAD(PBR, PC, INSTR)
                            | (MREAD(PBR, PC + 1, INSTR) << 8)
                            | (MREAD(PBR, PC + 2, INSTR) << 16);

        address += X;

        operand_addr = address;
        operand_bank = address >> 16;

        PC += 3;
    }

    static void getAddress_pcr()
    {
        int8_t offset = MREAD(PBR, PC++, INSTR);

        operand_addr = PC + offset;
        operand_bank = PBR;
    }

    static void getAddress_pcrl()
    {
        int16_t offset = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);

        PC += 2;

        operand_addr = PC + offset;
        operand_bank = PBR;
    }

    static void getAddress_ai()
    {
        uint16_t tmp = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);

        PC += 2;

        operand_addr = MREAD(0, tmp, OPADDR) | (MREAD(0, tmp + 1, OPADDR) << 8);
        operand_bank = 0;
    }

    static void getAddress_ail()
    {
        uint16_t tmp = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);

        PC += 2;

        operand_addr = MREAD(0, tmp, OPADDR) | (MREAD(0, tmp + 1, OPADDR) << 8);
        operand_bank = MREAD(0, tmp + 2, OPADDR);
    }

    // (DIRECT)
    static void getAddress_di()
    {
        uint16_t tmp = D + MREAD(PBR, PC++, INSTR);

        checkDirectPageAlignment();

        uint8_t lo = MREAD(0, tmp, OPADDR);
        uint8_t hi = MREAD(0, wrapDirectPage(tmp + 1), OPADDR);

        operand_addr = lo | (hi << 8);
        operand_bank = DBR;
    }

    static void getAddress_dil()
    {
        uint16_t tmp = D + MREAD(PBR, PC++, INSTR);

        checkDirectPageAlignment();

        operand_addr = MREAD(0, tmp, OPADDR) | (MREAD(0, tmp + 1, OPADDR) << 8);
        operand_bank = MREAD(0, tmp + 2, OPADDR);
    }

    static void getAddress_axi()
    {
        uint16_t tmp = (MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8)) + X;

        PC += 2;

        operand_addr = MREAD(PBR, tmp, OPADDR) | (MREAD(PBR, tmp + 1, OPADDR) << 8);
        operand_bank = PBR;
    }

    static void getAddress_sr()
    {
        operand_addr = S + MREAD(PBR, PC++, INSTR) + stack_offset;
        operand_bank = 0;
    }

    static void getAddress_srix()
    {
        uint16_t tmp = S + MREAD(PBR, PC++, INSTR) + stack_offset;

        operand_addr = MREAD(0, tmp, OPADDR) | (MREAD(0, tmp + 1, OPADDR) << 8);
        operand_bank = DBR;

        tmp = operand_addr;
        operand_addr += Y;

        if (operand_addr < tmp) operand_bank++;
    }

    inline static void stackPush(const uint8_t& v)
    {
        MWRITE(0, S-- + stack_offset, v, STACK);
    }

    inline static void stackPush(const uint16_t& v)
    {
        MWRITE(0, S-- + stack_offset, v >> 8, STACK);
        MWRITE(0, S-- + stack_offset, v, STACK);
    }

    inline static void stackPull(uint8_t& v)
    {
        v = MREAD(0, ++S + stack_offset, STACK);
    }

    inline static void stackPull(uint16_t& v)
    {
        v = MREAD(0, S + 1 + stack_offset, STACK) | (MREAD(0, S + 2 + stack_offset, STACK) << 8);
        S += 2;
    }

    inline static void jumpTo(const uint16_t& addr)
    {
        PC = addr;
    }

    inline static void jumpTo(const uint8_t& bank, const uint16_t& addr)
    {
        PBR = bank;
        PC  = addr;
    }

    inline static void checkDataPageCross(const uint16_t& check)
    {
        if (stack_offset && ((check & 0xFF00) != (operand_addr & 0xFF00)))
            num_cycles++;
    }

    inline static void checkProgramPageCross()
    {
        if (stack_offset && ((PC & 0xFF00) != (operand_addr & 0xFF00))) 
            num_cycles++;
    }

    inline static void checkDirectPageAlignment()
    {
        if (D & 0xFF) num_cycles++;
    }

    inline static void fetchImmediateOperand(uint8_t &op)
    {
        op = MREAD(PBR, PC++, INSTR);
    }

    inline static void fetchImmediateOperand(uint16_t &op)
    {
        op = MREAD(PBR, PC, INSTR) | (MREAD(PBR, PC + 1, INSTR) << 8);
        PC += 2;
    }

    inline static void fetchOperand(uint8_t &op)
    {
        op = MREAD(operand_bank, operand_addr, OPERAND);
    }

    inline static void fetchOperand(uint16_t &op)
    {
        op = MREAD(operand_bank, operand_addr, OPERAND)
             | (MREAD(operand_bank, operand_addr + 1, OPERAND) << 8);
    }

    inline static void storeOperand(uint8_t &op)
    {
        MWRITE(operand_bank, operand_addr, op, OPERAND);
    }

    inline static void storeOperand(uint16_t &op)
    {
        MWRITE(operand_bank, operand_addr, op, OPERAND);
        MWRITE(operand_bank, operand_addr + 1, op >> 8, OPERAND);
    }

    inline static void checkIfNegative(const uint8_t &v) { SR.N = v & 0x80; }
    inline static void checkIfNegative(const uint16_t &v) { SR.N = v & 0x8000; }

    inline static void checkIfZero(const uint8_t &v) { SR.Z = (v == 0); }
    inline static void checkIfZero(const uint16_t &v) { SR.Z = (v == 0); }

    inline static void op_AND()
    {
        A &= operand.m;

        checkIfNegative(A);
        checkIfZero(A);
    }

    inline static void op_BIT()
    {
        SR.Z = !(operand.m & A);
        SR.N = operand.m & n_bit;
        SR.V = operand.m & v_bit;
    }

    inline static void op_EOR()
    {
        A ^= operand.m;

        checkIfNegative(A);
        checkIfZero(A);
    }

    inline static void op_ORA()
    {
        A |= operand.m;

        checkIfNegative(A);
        checkIfZero(A);
    }

    inline static void op_TRB()
    {
        SR.Z = !(operand.m & A);

        operand.m &= ~A;
    }

    inline static void op_TSB()
    {
        SR.Z = !(operand.m & A);

        operand.m |= A;
    }

    // Increment/decrement

    inline static void op_DEC()
    {
        --operand.m;

        checkIfNegative(operand.m);
        checkIfZero(operand.m);
    }

    inline static void op_INC()
    {
        ++operand.m;

        checkIfNegative(operand.m);
        checkIfZero(operand.m);
    }

    // Bit shifts/rotates

    inline static void op_ASL()
    {
        SR.C = operand.m & n_bit;

        operand.m <<= 1;

        checkIfNegative(operand.m);
        checkIfZero(operand.m);
    }

    inline static void op_LSR()
    {
        SR.C = operand.m & 1;

        operand.m >>= 1;

        checkIfNegative(operand.m);
        checkIfZero(operand.m);
    }

    inline static void op_ROL()
    {
        bool c = SR.C;

        SR.C = operand.m & n_bit;

        operand.m = (operand.m << 1) | c;

        checkIfNegative(operand.m);
        checkIfZero(operand.m);
    }

    inline static void op_ROR()
    {
        bool c = SR.C;

        SR.C = operand.m & 1;
        operand.m >>= 1;
        if (c) operand.m |= n_bit;

        checkIfNegative(operand.m);
        checkIfZero(operand.m);
    }

    // Compares

    inline static void op_CMP()
    {
        mem_type tmp = A - operand.m;

        checkIfNegative(tmp);
        checkIfZero(tmp);

        SR.C = (A >= operand.m);
    }

    inline static void op_CPX()
    {
        index_type tmp = X - operand.x;

        checkIfNegative(tmp);
        checkIfZero(tmp);

        SR.C = (X >= operand.x);
    }

    inline static void op_CPY()
    {
        index_type tmp = Y - operand.x;

        checkIfNegative(tmp);
        checkIfZero(tmp);

        SR.C = (Y >= operand.x);
    }

    // Block moves

    static void op_MVP()
    {
        uint8_t src_bank = MREAD(PBR, PC + 1, INSTR);

        DBR = MREAD(PBR, PC, INSTR);

        if (registers::A.W != 0xFFFF) {
            MWRITE(DBR, registers::Y.W, MREAD(src_bank, registers::X.W, DATA), DATA);

            --registers::A.W;
            --registers::X.W;
            --registers::Y.W;

            --PC;
        }
        else {
            PC += 2;
        }
    }

    static void op_MVN()
    {
        uint8_t src_bank = MREAD(PBR, PC + 1, INSTR);

        DBR = MREAD(PBR, PC, INSTR);

        if (registers::A.W != 0xFFFF) {
            MWRITE(DBR, registers::Y.W, MREAD(src_bank, registers::X.W, DATA), DATA);

            --registers::A.W;
            ++registers::X.W;
            ++registers::Y.W;

            --PC;
        }
        else {
            PC += 2;
        }
    }

    // Loads and stores

    inline static void op_LDA()
    {
        A = operand.m;

        checkIfNegative(A);
        checkIfZero(A);
    }

    inline static void op_LDX()
    {
        X = operand.x;

        checkIfNegative(X);
        checkIfZero(X);
    }

    inline static void op_LDY()
    {
        Y = operand.x;

        checkIfNegative(Y);
        checkIfZero(Y);
    }

    inline static void op_STA()
    {
        MWRITE(operand_bank, operand_addr, A, DATA);

        if (sizeof(mem_type) == 2) {
            MWRITE(operand_bank, operand_addr + 1, A >> 8, DATA);
        }
    }

    inline static void op_STX()
    {
        MWRITE(operand_bank, operand_addr, X, DATA);

        if (sizeof(index_type) == 2) {
            MWRITE(operand_bank, operand_addr + 1, X >> 8, DATA);
        }
    }

    inline static void op_STY()
    {
        MWRITE(operand_bank, operand_addr, Y, DATA);

        if (sizeof(index_type) == 2) {
            MWRITE(operand_bank, operand_addr + 1, Y >> 8, DATA);
        }
    }

    inline static void op_STZ()
    {
        MWRITE(operand_bank, operand_addr, 0, DATA);

        if (sizeof(mem_type) == 2) {
            MWRITE(operand_bank, operand_addr + 1, 0, DATA);
        }
    }

    // Register transfers

    inline static void op_TCD()
    {
        D = registers::A.W;

        checkIfNegative(D);
        checkIfZero(D);
    }

    inline static void op_TDC()
    {
        registers::A.W = D;

        checkIfNegative(registers::A.W);
        checkIfZero(registers::A.W);
    }

    // Addition and subtraction

    inline static void op_ADC()
    {
        unsigned int sum = 0;

        if (SR.D) {
            int AL, tempA = A, tempB = operand.m, tempC = SR.C;

            sum = 0;

            for (unsigned int i = 0 ; i < m_nibbles ; ++i) {
                AL = (tempA & 0x0F) + (tempB & 0x0F) + tempC;

                if (AL >= 0x0A) {
                    AL = (AL + 0x06) & 0x0F;

                    tempC = 1;
                }
                else {
                    tempC = 0;
                }

                sum |= (AL << (4 * i));

                tempA >>= 4;
                tempB >>= 4;
            }

            SR.C = tempC;
        }
        else {
            sum = A + operand.m + SR.C;

            SR.C = (sum > m_max);
        }

        SR.V = (A ^ sum) & (operand.m ^ sum) & n_bit;

        A = sum;

        checkIfNegative(A);
        checkIfZero(A);
    }

    inline static void op_SBC()
    {
        int diff = 0;
        bool borrow = !SR.C;

        if (SR.D) {
            int AL, tempA = A, tempB = operand.m, tempC = SR.C;

            for (unsigned int i = 0 ; i < m_nibbles ; ++i) {
                AL = (tempA & 0x0F) - (tempB & 0x0F) + tempC - 1;

                if (AL < 0) {
                    AL = (AL - 0x06) & 0x0F;

                    tempC = 0;
                }
                else {
                    tempC = 1;
                }

                diff |= (AL << (4 * i));

                tempA >>= 4;
                tempB >>= 4;
            }

            SR.C = tempC;
        }
        else {
            diff = A - operand.m - borrow;

            SR.C = !(diff > m_max);
        }

        SR.V = (A ^ diff) & (~operand.m ^ diff) & n_bit;

        A = diff;

        checkIfNegative(A);
        checkIfZero(A);
    }

    static void executeOpcode(unsigned int opcode)
    {
        switch (opcode) {
            case 0x00:  /* BRK s */
                ++PC;

                if (stack_offset) {
                    stackPush(PC);
                    stackPush(uint8_t(SR | 0x10));  // set B bit on stack
                }
                else {
                    stackPush(PBR);
                    stackPush(PC);
                    stackPush(SR);
                }

                SR.D = false;
                SR.I = true;

                loadVector(stack_offset? 0xFFFE : 0xFFE6);

                break;

            case 0x01:  /* ORA (d,x) */
                getAddress_dxi();
                fetchOperand(operand.m);

                op_ORA();

                break;

            case 0x02:  /* COP s */
                ++PC;

                if (!stack_offset) {
                    stackPush(PBR);
                }

                stackPush(PC);
                stackPush(SR);

                SR.D = false;
                SR.I = true;

                loadVector(stack_offset? 0xFFF4 : 0xFFE4);

                break;

            case 0x03:  /* ORA d,s */
                getAddress_sr();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x04:  /* TSB d */
                getAddress_d();

                fetchOperand(operand.m);
                op_TSB();
                storeOperand(operand.m);

                break;

            case 0x05:  /* ORA d */
                getAddress_d();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x06:  /* ASL d */
                getAddress_d();

                fetchOperand(operand.m);
                op_ASL();
                storeOperand(operand.m);

                break;

            case 0x07:  /* ORA [d] */
                getAddress_dil();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x08:  /* PHP s */
                stackPush(SR);

                break;

            case 0x09:  /* ORA # */
                fetchImmediateOperand(operand.m);
                op_ORA();

                break;

            case 0x0A:  /* ASL A */
                operand.m = A;
                op_ASL();
                A = operand.m;

                break;

            case 0x0B:  /* PHD s */
                stackPush(D);

                break;

            case 0x0C:  /* TSB a */
                getAddress_a();

                fetchOperand(operand.m);
                op_TSB();
                storeOperand(operand.m);

                break;

            case 0x0D:  /* ORA a */
                getAddress_a();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x0E:  /* ASL a */
                getAddress_a();

                fetchOperand(operand.m);
                op_ASL();
                storeOperand(operand.m);

                break;

            case 0x0F:  /* ORA al */
                getAddress_al();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x10:  /* BPL r */
                getAddress_pcr();

                if (!SR.N) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0x11:  /* ORA (d),y */
                getAddress_dix();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x12:  /* ORA (d) */
                getAddress_di();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x13:  /* ORA (d,s),y */
                getAddress_srix();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x14:  /* TRB d */
                getAddress_d();

                fetchOperand(operand.m);
                op_TRB();
                storeOperand(operand.m);

                break;

            case 0x15:  /* ORA d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x16:  /* ASL d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_ASL();
                storeOperand(operand.m);

                break;

            case 0x17:  /* ORA [d],y */
                getAddress_dixl();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x18:  /* CLC i */
                SR.C = false;

                break;

            case 0x19:  /* ORA a,y */
                getAddress_axy();

                fetchOperand(operand.m);
                op_ORA();

                break;

            case 0x1A:  /* INC A */
                operand.m = A;
                op_INC();
                A = operand.m;

                break;

            case 0x1B:  /* TCS i */
                if (stack_offset) {
                    S = A;
                }
                else {
                    // native mode ignores M bit
                    registers::S.W = registers::A.W;
                }

                break;

            case 0x1C:  /* TRB a */
                getAddress_a();
                fetchOperand(operand.m);

                op_TRB();
                storeOperand(operand.m);

                break;

            case 0x1D:  /* ORA a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_ORA();

                break;

            case 0x1E:  /* ASL a,x */
                getAddress_axx();

                fetchOperand(operand.m);
                op_ASL();
                storeOperand(operand.m);

                break;

            case 0x1F:  /* ORA al,x */
                getAddress_alxx();
                fetchOperand(operand.m);

                op_ORA();

                break;

            case 0x20:  /* JSR a */
                getAddress_a();

                --PC;

                stackPush(PC);
                jumpTo(operand_addr);

                break;

            case 0x21:  /* AND (d,x) */
                getAddress_dxi();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x22:  /* JSL al */
                getAddress_al();

                --PC;

                stackPush(PBR);
                stackPush(PC);

                jumpTo(operand_bank, operand_addr);

                break;

            case 0x23:  /* AND d,s */
                getAddress_sr();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x24:  /* BIT d */
                getAddress_d();
                fetchOperand(operand.m);

                op_BIT();

                break;

            case 0x25:  /* AND d */
                getAddress_d();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x26:  /* ROL d */
                getAddress_d();

                fetchOperand(operand.m);
                op_ROL();
                storeOperand(operand.m);

                break;

            case 0x27:  /* AND [d] */
                getAddress_dil();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x28:  /* PLP s */
                {
                    uint8_t v;

                    stackPull(v);

                    SR = v;

                    modeSwitch();
                }

                break;

            case 0x29:  /* AND # */
                fetchImmediateOperand(operand.m);

                op_AND();

                break;

            case 0x2A:  /* ROL A */
                operand.m = A;
                op_ROL();
                A = operand.m;

                break;

            case 0x2B:  /* PLD s */
                stackPull(D);

                checkIfNegative(D);
                checkIfZero(D);

                break;

            case 0x2C:  /* BIT a */
                getAddress_a();
                fetchOperand(operand.m);

                op_BIT();

                break;

            case 0x2D:  /* AND a */
                getAddress_a();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x2E:  /* ROL a */
                getAddress_a();

                fetchOperand(operand.m);
                op_ROL();
                storeOperand(operand.m);

                break;

            case 0x2F:  /* AND al */
                getAddress_al();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x30:  /* BMI r */
                getAddress_pcr();

                if (SR.N) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0x31:  /* AND (d),y */
                getAddress_dix();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x32:  /* AND (d) */
                getAddress_di();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x33:  /* AND (d,s),y */
                getAddress_srix();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x34:  /* BIT d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_BIT();

                break;

            case 0x35:  /* AND d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x36:  /* ROL d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_ROL();
                storeOperand(operand.m);

                break;

            case 0x37:  /* AND [d],y */
                getAddress_dixl();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x38:  /* SEC i */
                SR.C = true;

                break;

            case 0x39:  /* AND a,y */
                getAddress_axy();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x3A:  /* DEC A */
                operand.m = A;
                op_DEC();
                A = operand.m;

                break;

            case 0x3B:  /* TSC i */
                // ignore M bit
                registers::A.W = registers::S.W;

                if (stack_offset) {
                    checkIfNegative(A);
                    checkIfZero(A);
                }
                else {
                    checkIfNegative(registers::A.W);
                    checkIfZero(registers::A.W);
                }

                break;

            case 0x3C:  /* BIT a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_BIT();

                break;

            case 0x3D:  /* AND a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x3E:  /* ROL a,x */
                getAddress_axx();

                fetchOperand(operand.m);
                op_ROL();
                storeOperand(operand.m);

                break;

            case 0x3F:  /* AND al,x */
                getAddress_alxx();
                fetchOperand(operand.m);

                op_AND();

                break;

            case 0x40:  /* RTI */
                {
                    uint8_t v;

                    stackPull(v);

                    SR = v;

                    modeSwitch();

                    stackPull(PC);

                    // cannot use stack_offset check here because we may have changed
                    // out of the mode this version of the template is compiled for.
                    if (!SR.E) {
                        stackPull(PBR);
                    }
                }

                break;

            case 0x41:  /* EOR (d,x) */
                getAddress_dxi();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x42:  /* WDM */
                fetchImmediateOperand(operand.b);

                WDM(operand.b);

                break;

            case 0x43:  /* EOR d,s */
                getAddress_sr();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x44:  /* MVP xyc */
                op_MVP();

                break;

            case 0x45:  /* EOR d */
                getAddress_d();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x46:  /* LSR d */
                getAddress_d();
                fetchOperand(operand.m);

                op_LSR();
                storeOperand(operand.m);

                break;

            case 0x47:  /* EOR [d] */
                getAddress_dil();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x48:  /* PHA */
                stackPush(A);

                break;

            case 0x49:  /* EOR # */
                fetchImmediateOperand(operand.m);

                op_EOR();

                break;

            case 0x4A:  /* LSR A */
                operand.m = A;
                op_LSR();
                A = operand.m;

                break;

            case 0x4B:  /* PHK */
                stackPush(PBR);

                break;

            case 0x4C:  /* JMP a */
                getAddress_a();

                jumpTo(operand_addr);

                break;

            case 0x4D:  /* EOR a */
                getAddress_a();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x4E:  /* LSR a */
                getAddress_a();
                fetchOperand(operand.m);

                op_LSR();
                storeOperand(operand.m);

                break;

            case 0x4F:  /* EOR al */
                getAddress_al();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x50:  /* BVC r */
                getAddress_pcr();

                if (!SR.V) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0x51:  /* EOR (d),y */
                getAddress_dix();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x52:  /* EOR (d) */
                getAddress_di();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x53:  /* EOR (d,s),y */
                getAddress_srix();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x54:  /* MVN xyc */
                op_MVN();

                break;

            case 0x55:  /* EOR d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x56:  /* LSR d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_LSR();
                storeOperand(operand.m);

                break;

            case 0x57:  /* EOR [d],y */
                getAddress_dixl();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x58:  /* CLI i */
                SR.I = 0;

                break;

            case 0x59:  /* EOR a,y */
                getAddress_axy();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x5A:  /* PHY s */
                stackPush(Y);

                break;

            case 0x5B:  /* TCD i */
                op_TCD();

                break;

            case 0x5C:  /* JMP al */
                getAddress_al();

                jumpTo(operand_bank, operand_addr);

                break;

            case 0x5D:  /* EOR a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x5E:  /* LSR a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_LSR();
                storeOperand(operand.m);

                break;

            case 0x5F:  /* EOR al,x */
                getAddress_alxx();
                fetchOperand(operand.m);

                op_EOR();

                break;

            case 0x60:  /* RTS s */
                stackPull(PC);

                ++PC;

                break;

            case 0x61:  /* ADC (d,x) */
                getAddress_dxi();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x62:  /* PER s */
                getAddress_pcrl();

                stackPush(operand_addr);

                break;

            case 0x63:  /* ADC d,s */
                getAddress_sr();

                fetchOperand(operand.m);
                op_ADC();

                break;

            case 0x64:  /* STZ d */
                getAddress_d();

                op_STZ();

                break;

            case 0x65:  /* ADC d */
                getAddress_d();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x66:  /* ROR d */
                getAddress_d();

                fetchOperand(operand.m);
                op_ROR();
                storeOperand(operand.m);

                break;

            case 0x67:  /* ADC [d] */
                getAddress_dil();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x68:  /* PLA s */
                stackPull(A);
                checkIfNegative(A);
                checkIfZero(A);

                break;

            case 0x69:  /* ADC # */
                fetchImmediateOperand(operand.m);

                op_ADC();

                break;

            case 0x6A:  /* ROR A */
                operand.m = A;
                op_ROR();
                A = operand.m;

                break;

            case 0x6B:  /* RTL s */
                stackPull(PC);
                stackPull(PBR);

                ++PC;

                break;

            case 0x6C:  /* JMP (a) */
                getAddress_ai();

                jumpTo(operand_addr);

                break;

            case 0x6D:  /* ADC a */
                getAddress_a();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x6E:  /* ROR a */
                getAddress_a();

                fetchOperand(operand.m);
                op_ROR();
                storeOperand(operand.m);

                break;

            case 0x6F:  /* ADC al */
                getAddress_al();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x70:  /* BVS r */
                getAddress_pcr();

                if (SR.V) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0x71:  /* ADC (d),y */
                getAddress_dix();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x72:  /* ADC (d) */
                getAddress_di();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x73:  /* ADC (d,s),y */
                getAddress_srix();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x74:  /* STZ d,x */
                getAddress_dxx();

                op_STZ();

                break;

            case 0x75:  /* ADC d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x76:  /* ROR d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_ROR();
                storeOperand(operand.m);

                break;

            case 0x77:  /* ADC [d],y */
                getAddress_dixl();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x78:  /* SEI i */
                SR.I = true;

                break;

            case 0x79:  /* ADC a,y */
                getAddress_axy();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x7A:  /* PLY */
                stackPull(Y);
                checkIfNegative(Y);
                checkIfZero(Y);

                break;

            case 0x7B:  /* TDC i */
                op_TDC();

                break;

            case 0x7C:  /* JMP (a,x) */
                getAddress_axi();

                jumpTo(operand_addr);

                break;

            case 0x7D:  /* ADC a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x7E:  /* ROR a,x */
                getAddress_axx();

                fetchOperand(operand.m);
                op_ROR();
                storeOperand(operand.m);

                break;

            case 0x7F:  /* ADC al,x */
                getAddress_alxx();
                fetchOperand(operand.m);

                op_ADC();

                break;

            case 0x80:  /* BRA r */
                getAddress_pcr();

                checkProgramPageCross();
                jumpTo(operand_addr);

                break;

            case 0x81:  /* STA (d,x) */
                getAddress_dxi();

                op_STA();

                break;

            case 0x82:  /* BRL rl */
                getAddress_pcrl();

                jumpTo(operand_bank, operand_addr);

                break;

            case 0x83:  /* STA d,s */
                getAddress_sr();

                op_STA();

                break;

            case 0x84:  /* STY d */
                getAddress_d();

                op_STY();

                break;

            case 0x85:  /* STA d */
                getAddress_d();

                op_STA();

                break;

            case 0x86:  /* STX d */
                getAddress_d();

                op_STX();

                break;

            case 0x87:  /* STA [d] */
                getAddress_dil();

                op_STA();

                break;

            case 0x88:  /* DEY i */
                --Y;

                checkIfNegative(Y);
                checkIfZero(Y);

                break;

            case 0x89:  /* BIT # */
                fetchImmediateOperand(operand.m);

                SR.Z = !(operand.m & A);

                break;

            case 0x8A:  /* TXA i */
                A = static_cast<mem_type> (X);

                checkIfNegative(A);
                checkIfZero(A);

                break;

            case 0x8B:  /* PHB */
                stackPush(DBR);

                break;

            case 0x8C:  /* STY a */
                getAddress_a();

                op_STY();

                break;

            case 0x8D:  /* STA a */
                getAddress_a();

                op_STA();

                break;

            case 0x8E:  /* STX a */
                getAddress_a();

                op_STX();

                break;

            case 0x8F:  /* STA al */
                getAddress_al();

                op_STA();

                break;

            case 0x90:  /* BCC r */
                getAddress_pcr();

                if (!SR.C) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0x91:  /* STA (d),y */
                getAddress_dix();

                op_STA();

                break;

            case 0x92:  /* STA (d) */
                getAddress_di();

                op_STA();

                break;

            case 0x93:  /* STA (d,s),y */
                getAddress_srix();

                op_STA();

                break;

            case 0x94:  /* STY d,x */
                getAddress_dxx();

                op_STY();

                break;

            case 0x95:  /* STA d,x */
                getAddress_dxx();

                op_STA();

                break;

            case 0x96:  /* STX d,y */
                getAddress_dxy();

                op_STX();

                break;

            case 0x97:  /* STA [d],y */
                getAddress_dixl();

                op_STA();

                break;

            case 0x98:  /* TYA i */
                A = static_cast<mem_type> (Y);

                checkIfNegative(A);
                checkIfZero(A);

                break;

            case 0x99:  /* STA a,y */
                getAddress_axy();

                op_STA();

                break;

            case 0x9A:  /* TXS i */
                S = X;

                break;

            case 0x9B:  /* TXY i */
                Y = X;

                checkIfNegative(Y);
                checkIfZero(Y);

                break;

            case 0x9C:  /* STZ a */
                getAddress_a();

                op_STZ();

                break;

            case 0x9D:  /* STA a,x */
                getAddress_axx();

                op_STA();

                break;

            case 0x9E:  /* STZ a,x */
                getAddress_axx();

                op_STZ();

                break;

            case 0x9F:  /* STA al,x */
                getAddress_alxx();

                op_STA();

                break;

            case 0xA0:  /* LDY # */
                fetchImmediateOperand(operand.x);

                op_LDY();

                break;

            case 0xA1:  /* LDA (d,x) */
                getAddress_dxi();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xA2:  /* LDX # */
                fetchImmediateOperand(operand.x);

                op_LDX();

                break;

            case 0xA3:  /* LDA d,s */
                getAddress_sr();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xA4:  /* LDY d */
                getAddress_d();
                fetchOperand(operand.x);

                op_LDY();

                break;

            case 0xA5:  /* LDA d */
                getAddress_d();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xA6:  /* LDX d */
                getAddress_d();
                fetchOperand(operand.x);

                op_LDX();

                break;

            case 0xA7:  /* LDA [d] */
                getAddress_dil();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xA8:  /* TAY i */
                // transfer all bits when x=0, even if m=1
                Y = static_cast<index_type> (A);

                checkIfNegative(Y);
                checkIfZero(Y);

                break;

            case 0xA9:  /* LDA # */
                fetchImmediateOperand(operand.m);

                op_LDA();

                break;

            case 0xAA:  /* TAX i */
                // transfer all bits when x=0, even if m=1
                X = static_cast<index_type> (A);

                checkIfNegative(X);
                checkIfZero(X);

                break;

            case 0xAB:  /* PLB s */
                stackPull(DBR);

                checkIfNegative(DBR);
                checkIfZero(DBR);

                break;

            case 0xAC:  /* LDY a */
                getAddress_a();
                fetchOperand(operand.x);

                op_LDY();

                break;

            case 0xAD:  /* LDA a */
                getAddress_a();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xAE:  /* LDX a */
                getAddress_a();
                fetchOperand(operand.x);

                op_LDX();

                break;

            case 0xAF:  /* LDA al */
                getAddress_al();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xB0:  /* BCS r */
                getAddress_pcr();

                if (SR.C) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0xB1:  /* LDA (d),y */
                getAddress_dix();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xB2:  /* LDA (d) */
                getAddress_di();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xB3:  /* LDA (d,s),y */
                getAddress_srix();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xB4:  /* LDY d,x */
                getAddress_dxx();
                fetchOperand(operand.x);

                op_LDY();

                break;

            case 0xB5:  /* LDA d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xB6:  /* LDX d,y */
                getAddress_dxy();
                fetchOperand(operand.x);

                op_LDX();

                break;

            case 0xB7:  /* LDA [d],y */
                getAddress_dixl();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xB8:  /* CLV i */
                SR.V = false;

                break;

            case 0xB9:  /* LDA a,y */
                getAddress_axy();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xBA:  /* TSX i */
                X = S;

                checkIfNegative(X);
                checkIfZero(X);

                break;

            case 0xBB:  /* TYX i */
                X = Y;

                checkIfNegative(X);
                checkIfZero(X);

                break;

            case 0xBC:  /* LDY a,x */
                getAddress_axx();
                fetchOperand(operand.x);

                op_LDY();

                break;

            case 0xBD:  /* LDA a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xBE:  /* LDX a,y */
                getAddress_axy();
                fetchOperand(operand.x);

                op_LDX();

                break;

            case 0xBF:  /* LDA al,x */
                getAddress_alxx();
                fetchOperand(operand.m);

                op_LDA();

                break;

            case 0xC0:  /* CPY # */
                fetchImmediateOperand(operand.x);
                op_CPY();

                break;

            case 0xC1:  /* CMP (d,x) */
                getAddress_dxi();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xC2:  /* REP # */
                fetchImmediateOperand(operand.b);
                SR &= ~operand.b;

                modeSwitch();

                break;

            case 0xC3:  /* CMP d,s */
                getAddress_sr();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xC4:  /* CPY d */
                getAddress_d();

                fetchOperand(operand.x);
                op_CPY();

                break;

            case 0xC5:  /* CMP d */
                getAddress_d();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xC6:  /* DEC d */
                getAddress_d();
                fetchOperand(operand.m);

                op_DEC();
                storeOperand(operand.m);

                break;

            case 0xC7:  /* CMP [d] */
                getAddress_dil();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xC8:  /* INY */
                ++Y;

                checkIfNegative(Y);
                checkIfZero(Y);

                break;

            case 0xC9:  /* CMP # */
                fetchImmediateOperand(operand.m);
                op_CMP();

                break;

            case 0xCA:  /* DEX i */
                --X;

                checkIfNegative(X);
                checkIfZero(X);

                break;

            case 0xCB:  /* WAI */
                waiting = true;

                break;

            case 0xCC:  /* CPY a */
                getAddress_a();

                fetchOperand(operand.x);
                op_CPY();

                break;

            case 0xCD:  /* CMP a */
                getAddress_a();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xCE:  /* DEC a */
                getAddress_a();
                fetchOperand(operand.m);

                op_DEC();
                storeOperand(operand.m);

                break;

            case 0xCF:  /* CMP al */
                getAddress_al();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xD0:  /* BNE r */
                getAddress_pcr();

                if (!SR.Z) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0xD1:  /* CMP (d),y */
                getAddress_dix();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xD2:  /* CMP (d) */
                getAddress_di();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xD3:  /* CMP (d,s),y */
                getAddress_srix();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xD4:  /* PEI d */
                getAddress_d();
                fetchOperand(operand.w);

                stackPush(operand.w);

                break;

            case 0xD5:  /* CMP d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xD6:  /* DEC d,x */
                getAddress_dxx();
                fetchOperand(operand.m);

                op_DEC();
                storeOperand(operand.m);

                break;

            case 0xD7:  /* CMP [d],y */
                getAddress_dixl();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xD8:  /* CLD i */
                SR.D = false;

                break;

            case 0xD9:  /* CMP a,y */
                getAddress_axy();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xDA:  /* PHX */
                stackPush(X);

                break;

            case 0xDB:  /* STP */
                stopped = true;

                break;

            case 0xDC:  /* JML (a) */
                getAddress_ail();

                jumpTo(operand_bank, operand_addr);

                break;

            case 0xDD:  /* CMP a,x */
                getAddress_axx();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xDE:  /* DEC a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_DEC();
                storeOperand(operand.m);

                break;

            case 0xDF:  /* CMP al,x */
                getAddress_alxx();

                fetchOperand(operand.m);
                op_CMP();

                break;

            case 0xE0:  /* CPX # */
                fetchImmediateOperand(operand.x);
                op_CPX();

                break;

            case 0xE1: /* SBC (d,x) */
                getAddress_dxi();
                fetchOperand(operand.m);

                op_SBC();

                break;

            case 0xE2:  /* SEP # */
                fetchImmediateOperand(operand.b);
                SR |= operand.b;

                modeSwitch();

                break;

            case 0xE3:  /* SBC d,s */
                getAddress_sr();
                fetchOperand(operand.m);

                op_SBC();

                break;

            case 0xE4:  /* CPX d */
                getAddress_d();

                fetchOperand(operand.x);
                op_CPX();

                break;

            case 0xE5:  /* SBC d */
                getAddress_d();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xE6:  /* INC d */
                getAddress_d();

                fetchOperand(operand.m);
                op_INC();
                storeOperand(operand.m);

                break;

            case 0xE7:  /* SBC [d] */
                getAddress_di();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xE8:  /* INX */
                ++X;

                checkIfNegative(X);
                checkIfZero(X);

                break;

            case 0xE9:  /* SBC # */
                fetchImmediateOperand(operand.m);

                op_SBC();

                break;

            case 0xEA:
                break;

            case 0xEB:  /* XBA i */
                operand.b = registers::A.B.H;
                registers::A.B.H = registers::A.B.L;
                registers::A.B.L = operand.b;

                checkIfNegative(registers::A.B.L);
                checkIfZero(registers::A.B.L);

                break;

            case 0xEC:  /* CPX a */
                getAddress_a();

                fetchOperand(operand.x);
                op_CPX();

                break;

            case 0xED: /* SBC a */
                getAddress_a();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xEE:  /* INC a */
                getAddress_a();

                fetchOperand(operand.m);
                op_INC();
                storeOperand(operand.m);

                break;

            case 0xEF:  /* SBC al */
                getAddress_al();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xF0:  /* BEQ r */
                getAddress_pcr();

                if (SR.Z) {
                    checkProgramPageCross();

                    jumpTo(operand_addr);

                    ++num_cycles;
                }

                break;

            case 0xF1:  /* SBC (d),y */
                getAddress_dix();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xF2:  /* SBC (d) */
                getAddress_di();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xF3:  /* SBC (d,s),y */
                getAddress_srix();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xF4:  /* PEA s */
                fetchImmediateOperand(operand.w);
                stackPush(operand.w);

                break;

            case 0xF5:  /* SBC d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xF6:  /* INC d,x */
                getAddress_dxx();

                fetchOperand(operand.m);
                op_INC();
                storeOperand(operand.m);

                break;

            case 0xF7:  /* SBC [d],y */
                getAddress_dixl();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xF8:  /* SED i */
                SR.D = true;

                break;

            case 0xF9:  /* SBC a,y */
                getAddress_axy();

                fetchOperand(operand.m);
                op_SBC();

                break;

            case 0xFA:  /* PLX s */
                stackPull(X);
                checkIfNegative(X);
                checkIfZero(X);

                break;

            case 0xFB:   /* XCE i */
                operand.b = SR.E;
                SR.E = SR.C;
                SR.C = operand.b;

                modeSwitch();

                break;

            case 0xFC:  /* JSR (a,x) */
                getAddress_axi();

                --PC;

                stackPush(PC);
                jumpTo(operand_addr);

                break;

            case 0xFD:  /* SBC a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_SBC();

                break;

            case 0xFE:  /* INC a,x */
                getAddress_axx();
                fetchOperand(operand.m);

                op_INC();

                storeOperand(operand.m);

                break;

            case 0xFF:  /* SBC al,x */
                getAddress_alxx();
                fetchOperand(operand.m);

                op_SBC();

                break;

            case 0x100: /* irq */
                waiting = false;

                if (stack_offset) {
                    stackPush(PC);
                    stackPush((uint8_t) (SR & ~0x10));
                }
                else {
                    stackPush(PBR);
                    stackPush(PC);
                    stackPush(SR);
                }

                SR.D = false;
                SR.I = true;

                loadVector(stack_offset ? 0xFFFE : 0xFFEE);

                break;

            case 0x101: /* nmi */
                nmi_pending = waiting = false;

                if (!stack_offset) {
                    stackPush(PBR);
                }

                stackPush(PC);
                stackPush(SR);

                SR.D = false;
                SR.I = true;

                loadVector(stack_offset? 0xFFFA : 0xFFEA);

                break;

            case 0x102: /* abort */
                abort_pending = waiting = false;

                if (!stack_offset) {
                    stackPush(PBR);
                }

                stackPush(PC);
                stackPush(SR);

                SR.D = false;
                SR.I = true;

                loadVector(stack_offset? 0xFFF8 : 0xFFE8);

                break;

            default:
                break;
        }
    }
};

} // namespace m65816