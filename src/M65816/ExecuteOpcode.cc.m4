/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "M65816.h"
divert(-1)

dnl This macro take the value of the E, M, and X bits, and produces a version
dnl of executeOpcode tailored for that specific CPU mode.

define(`EXECUTE_OPCODE', `
define(`EMULATION', `$1')
define(`SHORT_M', `$2')
define(`SHORT_X', `$3')
include(`macros.m4')
include(`opcodes.m4')

int M65816::executeOpcode_e$1m$2x$3()
{
    int num_cycles = cycle_counts_e$1m$2x$3[opcode];
    word32 tmp;
    dualw wtmp;
    duala atmp;

    switch (opcode) {
        case 0x00:  /* BRK s */
            PC.W.L++;

            PUSH_PC
            PUSH_b(STATUS_REG|BRK_BIT)

            P.D = 0;
            P.I = 1;

            LOAD_VECTOR(0xFFFE, 0xFFE6)

            break;

        case 0x01:  /* ORA (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x02:  /* COP s */
            PC.W.L++;

            PUSH_PC
            PUSH_b(STATUS_REG)

            P.D = 0;
            P.I = 1;

            LOAD_VECTOR(0xFFF4, 0xFFE4)

            break;

        case 0x03:  /* ORA d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x04:  /* TSB d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_TSB
            STORE_OPERAND_m

            break;

        case 0x05:  /* ORA d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x06:  /* ASL d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_ASL(operand)
            STORE_OPERAND_m

            break;

        case 0x07:  /* ORA [d] */
            GET_ADDRESS_dil
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x08:  /* PHP s */
            PUSH_b(STATUS_REG)

            break;

        case 0x09:  /* ORA # */
            FETCH_IMMEDIATE_OPERAND_m

            OPCODE_ORA

            break;

        case 0x0A:  /* ASL A */
            OPCODE_ASL(A)

            break;

        case 0x0B:  /* PHD s */
            PUSH_w(D)

            break;

        case 0x0C:  /* TSB a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_TSB
            STORE_OPERAND_m

            break;

        case 0x0D:  /* ORA a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x0E:  /* ASL a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_ASL(operand)
            STORE_OPERAND_m

            break;

        case 0x0F:  /* ORA al */
            GET_ADDRESS_al
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x10:  /* BPL r */
            GET_ADDRESS_pcr

            if (!P.N) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0x11:  /* ORA (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x12:  /* ORA (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x13:  /* ORA (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x14:  /* TRB d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_TRB
            STORE_OPERAND_m

            break;

        case 0x15:  /* ORA d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x16:  /* ASL d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_ASL(operand)
            STORE_OPERAND_m

            break;

        case 0x17:  /* ORA [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x18:  /* CLC i */
            P.C = 0;

            break;

        case 0x19:  /* ORA a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x1A:  /* INC A */
            OPCODE_INC(A)

            break;

        case 0x1B:  /* TCS i */
            REG_S = REG_A;

            break;

        case 0x1C:  /* TRB a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_TRB
            STORE_OPERAND_m

            break;

        case 0x1D:  /* ORA a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x1E:  /* ASL a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_ASL(operand)
            STORE_OPERAND_m

            break;

        case 0x1F:  /* ORA al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m

            OPCODE_ORA

            break;

        case 0x20:  /* JSR a */
            GET_ADDRESS_a

            OPCODE_JSR

            break;

        case 0x21:  /* AND (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x22:  /* JSL al */
            GET_ADDRESS_al

            PC.W.L--;
            PUSH_l(PC)

            JUMP_l

            break;

        case 0x23:  /* AND d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x24:  /* BIT d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_BIT

            break;

        case 0x25:  /* AND d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x26:  /* ROL d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_ROL(operand)
            STORE_OPERAND_m

            break;

        case 0x27:  /* AND [d] */
            GET_ADDRESS_dil
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x28:  /* PLP s */
            PULL_SR

            break;

        case 0x29:  /* AND # */
            FETCH_IMMEDIATE_OPERAND_m

            OPCODE_AND

            break;

        case 0x2A:  /* ROL A */
            OPCODE_ROL(A)

            break;

        case 0x2B:  /* PLD s */
            PULL_w(D)

            CHECK_IF_NEGATIVE_w(D.W)
            CHECK_IF_ZERO_w(D.W)

            break;

        case 0x2C:  /* BIT a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_BIT

            break;

        case 0x2D:  /* AND a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x2E:  /* ROL a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_ROL(operand)
            STORE_OPERAND_m

            break;

        case 0x2F:  /* AND al */
            GET_ADDRESS_al
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x30:  /* BMI r */
            GET_ADDRESS_pcr

            if (P.N) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0x31:  /* AND (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x32:  /* AND (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x33:  /* AND (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x34:  /* BIT d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_BIT

            break;

        case 0x35:  /* AND d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x36:  /* ROL d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_ROL(operand)
            STORE_OPERAND_m

            break;

        case 0x37:  /* AND [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x38:  /* SEC i */
            P.C = 1;

            break;

        case 0x39:  /* AND a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x3A:  /* DEC A */
            OPCODE_DEC(A)

            break;

        case 0x3B:  /* TSC i */
            A.W = S.W;

            CHECK_IF_NEGATIVE_w(A.W)
            CHECK_IF_ZERO_w(A.W)

            break;

        case 0x3C:  /* BIT a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_BIT

            break;

        case 0x3D:  /* AND a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x3E:  /* ROL a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_ROL(operand)
            STORE_OPERAND_m

            break;

        case 0x3F:  /* AND al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m

            OPCODE_AND

            break;

        case 0x40:  /* RTI */
            PULL_SR
            PULL_PC

            break;

        case 0x41:  /* EOR (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x42:  /* WDM */
            FETCH_IMMEDIATE_OPERAND_b

            board->wdmCallback(operand.B.L);    // FIXME: pass processor state

            break;

        case 0x43:  /* EOR d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x44:  /* MVP xyc */
            OPCODE_MVP

            break;

        case 0x45:  /* EOR d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x46:  /* LSR d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_LSR(operand)
            STORE_OPERAND_m

            break;

        case 0x47:  /* EOR [d] */
            GET_ADDRESS_dil
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x48:  /* PHA */
            ifelse(SHORT_M, `1', `PUSH_b(A.B.L)', `PUSH_w(A)')

            break;

        case 0x49:  /* EOR # */
            FETCH_IMMEDIATE_OPERAND_m

            OPCODE_EOR

            break;

        case 0x4A:  /* LSR A */
            OPCODE_LSR(A)

            break;

        case 0x4B:  /* PHK */
            PUSH_b(PC.B.B)

            break;

        case 0x4C:  /* JMP a */
            GET_ADDRESS_a

            JUMP_w

            break;

        case 0x4D:  /* EOR a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x4E:  /* LSR a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_LSR(operand)
            STORE_OPERAND_m

            break;

        case 0x4F:  /* EOR al */
            GET_ADDRESS_al
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x50:  /* BVC r */
            GET_ADDRESS_pcr

            if (!P.V) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0x51:  /* EOR (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x52:  /* EOR (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x53:  /* EOR (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x54:  /* MVN xyc */
            OPCODE_MVN

            break;

        case 0x55:  /* EOR d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x56:  /* LSR d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_LSR(operand)
            STORE_OPERAND_m

            break;

        case 0x57:  /* EOR [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x58:  /* CLI i */
            P.I = 0;

            break;

        case 0x59:  /* EOR a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x5A:  /* PHY s */
            ifelse(SHORT_X, `1', `PUSH_b(Y.B.L)', `PUSH_w(Y)')

            break;

        case 0x5B:  /* TCD i */
            OPCODE_TCD

            break;

        case 0x5C:  /* JMP al */
            GET_ADDRESS_al

            JUMP_l

            break;

        case 0x5D:  /* EOR a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x5E:  /* LSR a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_LSR(operand)
            STORE_OPERAND_m

            break;

        case 0x5F:  /* EOR al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m

            OPCODE_EOR

            break;

        case 0x60:  /* RTS s */
            PULL_w(PC)

            PC.W.L++;

            break;

        case 0x61:  /* ADC (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x62:  /* PER s */
            GET_ADDRESS_pcrl

            PUSH_w(address)

            break;

        case 0x63:  /* ADC d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x64:  /* STZ d */
            GET_ADDRESS_d

            OPCODE_STZ

            break;

        case 0x65:  /* ADC d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x66:  /* ROR d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_ROR(operand)
            STORE_OPERAND_m

            break;

        case 0x67:  /* ADC [d] */
            GET_ADDRESS_dil
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x68:  /* PLA s */
            OPCODE_PLA

            break;

        case 0x69:  /* ADC # */
            FETCH_IMMEDIATE_OPERAND_m

            OPCODE_ADC

            break;

        case 0x6A:  /* ROR A */
            OPCODE_ROR(A)

            break;

        case 0x6B:  /* RTL s */
            PULL_l(PC)

            PC.W.L++;

            break;

        case 0x6C:  /* JMP (a) */
            GET_ADDRESS_ai

            JUMP_w

            break;

        case 0x6D:  /* ADC a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x6E:  /* ROR a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_ROR(operand)
            STORE_OPERAND_m

            break;

        case 0x6F:  /* ADC al */
            GET_ADDRESS_al
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x70:  /* BVS r */
            GET_ADDRESS_pcr

            if (P.V) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0x71:  /* ADC (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x72:  /* ADC (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x73:  /* ADC (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x74:  /* STZ d,x */
            GET_ADDRESS_dxx

            OPCODE_STZ

            break;

        case 0x75:  /* ADC d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x76:  /* ROR d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_ROR(operand)
            STORE_OPERAND_m

            break;

        case 0x77:  /* ADC [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x78:  /* SEI i */
            P.I = 1;

            break;

        case 0x79:  /* ADC a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x7A:  /* PLY */
            OPCODE_PLY

            break;

        case 0x7B:  /* TDC i */
            OPCODE_TDC

            break;

        case 0x7C:  /* JMP (a,x) */
            GET_ADDRESS_axi
            JUMP_w

            break;

        case 0x7D:  /* ADC a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x7E:  /* ROR a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_ROR(operand)
            STORE_OPERAND_m

            break;

        case 0x7F:  /* ADC al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m

            OPCODE_ADC

            break;

        case 0x80:  /* BRA r */
            GET_ADDRESS_pcr

            CHECK_PC_PAGE_CROSS
            JUMP_w

            break;

        case 0x81:  /* STA (d,x) */
            GET_ADDRESS_dxi

            OPCODE_STA

            break;

        case 0x82:  /* BRL rl */
            GET_ADDRESS_pcrl
            JUMP_l

            break;

        case 0x83:  /* STA d,s */
            GET_ADDRESS_sr

            OPCODE_STA

            break;

        case 0x84:  /* STY d */
            GET_ADDRESS_d

            OPCODE_STY

            break;

        case 0x85:  /* STA d */
            GET_ADDRESS_d

            OPCODE_STA

            break;

        case 0x86:  /* STX d */
            GET_ADDRESS_d

            OPCODE_STX

            break;

        case 0x87:  /* STA [d] */
            GET_ADDRESS_dil

            OPCODE_STA

            break;

        case 0x88:  /* DEY i */
            REG_Y--;

            CHECK_IF_NEGATIVE_x(Y)
            CHECK_IF_ZERO_x(Y)

            break;

        case 0x89:  /* BIT # */
            FETCH_IMMEDIATE_OPERAND_m

            P.Z = (OPERAND_m & REG_A)? 0 : 1;

            break;

        case 0x8A:  /* TXA i */
            OPCODE_TXA

            break;

        case 0x8B:  /* PHB */
            PUSH_b(DB)

            break;

        case 0x8C:  /* STY a */
            GET_ADDRESS_a

            OPCODE_STY

            break;

        case 0x8D:  /* STA a */
            GET_ADDRESS_a

            OPCODE_STA

            break;

        case 0x8E:  /* STX a */
            GET_ADDRESS_a

            OPCODE_STX

            break;

        case 0x8F:  /* STA al */
            GET_ADDRESS_al

            OPCODE_STA

            break;

        case 0x90:  /* BCC r */
            GET_ADDRESS_pcr

            if (!P.C) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0x91:  /* STA (d),y */
            GET_ADDRESS_dix

            OPCODE_STA

            break;

        case 0x92:  /* STA (d) */
            GET_ADDRESS_di

            OPCODE_STA

            break;

        case 0x93:  /* STA (d,s),y */
            GET_ADDRESS_srix

            OPCODE_STA

            break;

        case 0x94:  /* STY d,x */
            GET_ADDRESS_dxx

            OPCODE_STY

            break;

        case 0x95:  /* STA d,x */
            GET_ADDRESS_dxx

            OPCODE_STA

            break;

        case 0x96:  /* STX d,y */
            GET_ADDRESS_dxy

            OPCODE_STX

            break;

        case 0x97:  /* STA [d],y */
            GET_ADDRESS_dixl

            OPCODE_STA

            break;

        case 0x98:  /* TYA i */
            OPCODE_TYA

            break;

        case 0x99:  /* STA a,y */
            GET_ADDRESS_axy

            OPCODE_STA

            break;

        case 0x9A:  /* TXS i */
            OPCODE_TXS

            break;

        case 0x9B:  /* TXY i */
            OPCODE_TXY

            break;

        case 0x9C:  /* STZ a */
            GET_ADDRESS_a

            OPCODE_STZ

            break;

        case 0x9D:  /* STA a,x */
            GET_ADDRESS_axx

            OPCODE_STA

            break;

        case 0x9E:  /* STZ a,x */
            GET_ADDRESS_axx

            OPCODE_STZ

            break;

        case 0x9F:  /* STA al,x */
            GET_ADDRESS_alxx

            OPCODE_STA

            break;

        case 0xA0:  /* LDY # */
            FETCH_IMMEDIATE_OPERAND_x
            
            OPCODE_LDY

            break;

        case 0xA1:  /* LDA (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xA2:  /* LDX # */
            FETCH_IMMEDIATE_OPERAND_x
            
            OPCODE_LDX

            break;

        case 0xA3:  /* LDA d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xA4:  /* LDY d */
            GET_ADDRESS_d
            FETCH_OPERAND_x
            
            OPCODE_LDY

            break;

        case 0xA5:  /* LDA d */
            GET_ADDRESS_d
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xA6:  /* LDX d */
            GET_ADDRESS_d
            FETCH_OPERAND_x
            
            OPCODE_LDX

            break;

        case 0xA7:  /* LDA [d] */
            GET_ADDRESS_dil
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xA8:  /* TAY i */
            OPCODE_TAY

            break;

        case 0xA9:  /* LDA # */
            FETCH_IMMEDIATE_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xAA:  /* TAX i */
            OPCODE_TAX

            break;

        case 0xAB:  /* PLB s */
            PULL_b(DB)

            CHECK_IF_NEGATIVE_b(DB)
            CHECK_IF_ZERO_b(DB)

            break;

        case 0xAC:  /* LDY a */
            GET_ADDRESS_a
            FETCH_OPERAND_x
            
            OPCODE_LDY

            break;

        case 0xAD:  /* LDA a */
            GET_ADDRESS_a
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xAE:  /* LDX a */
            GET_ADDRESS_a
            FETCH_OPERAND_x
            
            OPCODE_LDX

            break;

        case 0xAF:  /* LDA al */
            GET_ADDRESS_al
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xB0:  /* BCS r */
            GET_ADDRESS_pcr

            if (P.C) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0xB1:  /* LDA (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xB2:  /* LDA (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xB3:  /* LDA (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xB4:  /* LDY d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_x
            
            OPCODE_LDY

            break;

        case 0xB5:  /* LDA d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xB6:  /* LDX d,y */
            GET_ADDRESS_dxy
            FETCH_OPERAND_x
            
            OPCODE_LDX

            break;

        case 0xB7:  /* LDA [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xB8:  /* CLV i */
            P.V = 0;

            break;

        case 0xB9:  /* LDA a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xBA:  /* TSX i */
            OPCODE_TSX

            break;

        case 0xBB:  /* TYX i */
            OPCODE_TYX

            break;

        case 0xBC:  /* LDY a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_x
            
            OPCODE_LDY

            break;

        case 0xBD:  /* LDA a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xBE:  /* LDX a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_x
            
            OPCODE_LDX

            break;

        case 0xBF:  /* LDA al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m
            
            OPCODE_LDA

            break;

        case 0xC0:  /* CPY # */
            FETCH_IMMEDIATE_OPERAND_x

            OPCODE_CPY

            break;

        case 0xC1:  /* CMP (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xC2:  /* REP # */
            FETCH_IMMEDIATE_OPERAND_b

            OPCODE_REP

            break;

        case 0xC3:  /* CMP d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xC4:  /* CPY d */
            GET_ADDRESS_d
            FETCH_OPERAND_x

            OPCODE_CPY

            break;

        case 0xC5:  /* CMP d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xC6:  /* DEC d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_DEC(operand)
            STORE_OPERAND_m

            break;

        case 0xC7:  /* CMP [d] */
            GET_ADDRESS_dil
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xC8:  /* INY */
            REG_Y++;

            CHECK_IF_NEGATIVE_x(Y)
            CHECK_IF_ZERO_x(Y)

            break;

        case 0xC9:  /* CMP # */
            FETCH_IMMEDIATE_OPERAND_m

            OPCODE_CMP

            break;

        case 0xCA:  /* DEX i */
            REG_X--;

            CHECK_IF_NEGATIVE_x(X)
            CHECK_IF_ZERO_x(X)

            break;

        case 0xCB:  /* WAI */
            waiting = 1;

            break;

        case 0xCC:  /* CPY a */
            GET_ADDRESS_a
            FETCH_OPERAND_x

            OPCODE_CPY

            break;

        case 0xCD:  /* CMP a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xCE:  /* DEC a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_DEC(operand)
            STORE_OPERAND_m

            break;

        case 0xCF:  /* CMP al */
            GET_ADDRESS_al
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xD0:  /* BNE r */
            GET_ADDRESS_pcr

            if (!P.Z) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0xD1:  /* CMP (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xD2:  /* CMP (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xD3:  /* CMP (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xD4:  /* PEI d */
            GET_ADDRESS_d
            FETCH_OPERAND_w

            PUSH_w(operand)

            break;

        case 0xD5:  /* CMP d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xD6:  /* DEC d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_DEC(operand)
            STORE_OPERAND_m

            break;

        case 0xD7:  /* CMP [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xD8:  /* CLD i */
            P.D = 0;

            break;

        case 0xD9:  /* CMP a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xDA:  /* PHX */
            ifelse(SHORT_X, `1', `PUSH_b(X.B.L)', `PUSH_w(X)')

            break;

        case 0xDB:  /* STP */
            stopped = 1;

            break;

        case 0xDC:  /* JML (a) */
            GET_ADDRESS_ail

            JUMP_l

            break;

        case 0xDD:  /* CMP a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xDE:  /* DEC a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_DEC(operand)
            STORE_OPERAND_m

            break;

        case 0xDF:  /* CMP al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m

            OPCODE_CMP

            break;

        case 0xE0:  /* CPX # */
            FETCH_IMMEDIATE_OPERAND_x

            OPCODE_CPX

            break;

        case 0xE1: /* SBC (d,x) */
            GET_ADDRESS_dxi
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xE2:  /* SEP # */
            FETCH_IMMEDIATE_OPERAND_b

            OPCODE_SEP

            break;

        case 0xE3:  /* SBC d,s */
            GET_ADDRESS_sr
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xE4:  /* CPX d */
            GET_ADDRESS_d
            FETCH_OPERAND_x

            OPCODE_CPX

            break;

        case 0xE5:  /* SBC d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xE6:  /* INC d */
            GET_ADDRESS_d
            FETCH_OPERAND_m

            OPCODE_INC(operand)
            STORE_OPERAND_m

            break;

        case 0xE7:  /* SBC [d] */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xE8:  /* INX */
            REG_X++;

            CHECK_IF_NEGATIVE_x(X)
            CHECK_IF_ZERO_x(X)

            break;

        case 0xE9:  /* SBC # */
            FETCH_IMMEDIATE_OPERAND_m

            OPCODE_SBC

            break;

        case 0xEA:
            break;

        case 0xEB:  /* XBA i */
            OPCODE_XBA

            break;

        case 0xEC:  /* CPX a */
            GET_ADDRESS_a
            FETCH_OPERAND_x

            OPCODE_CPX

            break;

        case 0xED: /* SBC a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xEE:  /* INC a */
            GET_ADDRESS_a
            FETCH_OPERAND_m

            OPCODE_INC(operand)
            STORE_OPERAND_m

            break;

        case 0xEF:  /* SBC al */
            GET_ADDRESS_al
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xF0:  /* BEQ r */
            GET_ADDRESS_pcr

            if (P.Z) {
                CHECK_PC_PAGE_CROSS

                JUMP_w

                num_cycles++;
            }

            break;

        case 0xF1:  /* SBC (d),y */
            GET_ADDRESS_dix
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xF2:  /* SBC (d) */
            GET_ADDRESS_di
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xF3:  /* SBC (d,s),y */
            GET_ADDRESS_srix
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xF4:  /* PEA s */
            FETCH_IMMEDIATE_OPERAND_w

            PUSH_w(operand)

            break;

        case 0xF5:  /* SBC d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xF6:  /* INC d,x */
            GET_ADDRESS_dxx
            FETCH_OPERAND_m

            OPCODE_INC(operand)
            STORE_OPERAND_m

            break;

        case 0xF7:  /* SBC [d],y */
            GET_ADDRESS_dixl
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xF8:  /* SED i */
            P.D = 1;

            break;

        case 0xF9:  /* SBC a,y */
            GET_ADDRESS_axy
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xFA:  /* PLX s */
            OPCODE_PLX

            break;

        case 0xFB:   /* XCE i */
            OPCODE_XCE

            break;

        case 0xFC:  /* JSR (a,x) */
            GET_ADDRESS_axi

            OPCODE_JSR

            break;

        case 0xFD:  /* SBC a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0xFE:  /* INC a,x */
            GET_ADDRESS_axx
            FETCH_OPERAND_m

            OPCODE_INC(operand)
            STORE_OPERAND_m

            break;

        case 0xFF:  /* SBC al,x */
            GET_ADDRESS_alxx
            FETCH_OPERAND_m

            OPCODE_SBC

            break;

        case 0x100: /* irq */
            irq_pending = waiting = 0;

            PUSH_PC
            PUSH_b(STATUS_REG)

            P.D = 0;
            P.I = 1;

            LOAD_VECTOR(0xFFFE, 0xFFEE)

            break;

        case 0x101: /* nmi */
            nmi_pending = waiting = 0;

            PUSH_PC
            PUSH_b(STATUS_REG)

            P.D = 0;
            P.I = 1;

            LOAD_VECTOR(0xFFFA, 0xFFEA)

            break;

        case 0x102: /* abort */
            abort_pending = waiting = 0;

            PUSH_PC
            PUSH_b(STATUS_REG)

            P.D = 0;
            P.I = 1;

            LOAD_VECTOR(0xFFF8, 0xFFE8)

            break;

        default:
            break;
    }

    return num_cycles;
}
')

EXECUTE_OPCODE(0, 0, 0)
EXECUTE_OPCODE(0, 0, 1)
EXECUTE_OPCODE(0, 1, 0)
EXECUTE_OPCODE(0, 1, 1)
EXECUTE_OPCODE(1, 1, 1)

divert(0)
