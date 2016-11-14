/**
 * M65816: Portable 65816 Emulator
 * Written and Copyright (C) 1996 - 2016 by Joshua M. Thompson
 *
 * You are free to distribute this code for non-commercial purposes
 * I ask only that you notify me of any changes you make to the code
 * Commercial use is prohibited without my written permission
 */

#include "M65816.h"

M65816::M65816(SystemBoard *theBoard )
{
    board = theBoard;
}

int M65816::runUntil(int max_cycles)
{
    int num_cycles = 0;

    while (num_cycles < max_cycles) {
        num_cycles += nextInstruction();
    }

    return num_cycles;
}

void M65816::reset(void)
{
    stopped = 0;
    waiting = 0;

    nmi_pending   = 0;
    irq_pending   = 0;
    abort_pending = 0;

    P.E = 1;
    P.M = 1;
    P.X = 1;
    P.I = 1;

    D.W = 0;
    DB  = 0;
    S.W = 0x01FF;

    A.W = 0;
    X.W = 0;
    Y.W = 0;

    PC.B.B = 0;
    PC.B.L = board->readMemory(0xFFFC);
    PC.B.H = board->readMemory(0xFFFD);
}

int M65816::nextInstruction(void)
{
    if (trace) {
    }

    if (abort_pending) {
        opcode = 0x102;
    }
    else if (nmi_pending) {
        opcode = 0x101;
    }
    else if (irq_pending) {
        opcode = 0x100;
    }
    else if (waiting) {
        return 0;
    }
    else {
        opcode = board->readMemory(PC.A);

        PC.W.L++;
    }

    if (P.E) {
        return executeOpcode_e1m1x1();
    }
    else if (P.M) {
        if (P.X) {
            return executeOpcode_e0m1x1();
        }
        else {
            return executeOpcode_e0m1x0();
        }
    }
    else {
        if (P.X) {
            return executeOpcode_e0m0x1();
        }
        else {
            return executeOpcode_e0m0x0();
        }
    }
}

void M65816::modeSwitch()
{
    if (P.E) {
        P.M = 1;
        P.X = 1;

        S.B.H = 0x01;
    }

    if (P.X) {
        X.B.H = 0x00;
        Y.B.H = 0x00;
    }
}

void M65816::bcdToNibbles(word16 in, int *out, int digits)
{
    for (int i = 0 ; i < digits ; i++) {
        out[i] = in & 0x0F;

        in >>= 4;
    }
}

word32 M65816::decimalAdd(word16 operand1, word16 operand2)
{
    int op1[4];
    int op2[4];
    int digits = P.M? 2 : 4;
    int carry = P.C;
    word32 out;

    bcdToNibbles(operand1, op1, digits);
    bcdToNibbles(operand2, op2, digits);

    out = 0;

    for (int i = 0 ; i < digits; i++) {
        int sum = op1[i] + op2[i] + carry;

        if (sum >= 10) {
            sum -= 10;

            carry = 1;
        }
        else {
            carry = 0;
        }

        out = (out << 4) | (sum & 0x0F);
    }

    P.C = carry;

    return out;
}

word32 M65816::decimalSubtract(word16 operand1, word16 operand2)
{
    int op1[4];
    int op2[4];
    int digits = P.M? 2 : 4;
    int borrow = !P.C;
    word32 out;

    bcdToNibbles(operand1, op1, digits);
    bcdToNibbles(operand2, op2, digits);

    out = 0;

    for (int i = 0 ; i < digits; i++) {
        int sum = op2[i] - op1[i] - borrow;

        if (sum < 0) {
            sum += 10;

            borrow = 1;
        }
        else {
            borrow = 0;
        }

        out = (out << 4) | (sum & 0x0F);
    }

    P.C = borrow;

    return out;
}
