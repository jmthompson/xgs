/*********************************************************************
 *                                                                   *
 *                M65816: Portable 65816 CPU Emulator                *
 *                                                                   *
 *        Written and Copyright (C)1996 by Joshua M. Thompson        *
 *                                                                   *
 *  You are free to distribute this code for non-commercial purposes *
 * I ask only that you notify me of any changes you make to the code *
 *     Commercial use is prohibited without my written permission    *
 *                                                                   *
 *********************************************************************/

#include "config.h"

#ifdef DEBUG

#include <stdio.h>
#include "m65816.h"
#include "cpumicro.h"
#include "memory.h"
#include "emul.h"

/* 65816 debugger module */

char *mnemonics[256] = {
"BRK", "ORA", "COP", "ORA", "TSB", "ORA", "ASL", "ORA",
"PHP", "ORA", "ASL", "PHD", "TSB", "ORA", "ASL", "ORA",
"BPL", "ORA", "ORA", "ORA", "TRB", "ORA", "ASL", "ORA",
"CLC", "ORA", "INC", "TCS", "TRB", "ORA", "ASL", "ORA",
"JSR", "AND", "JSL", "AND", "BIT", "AND", "ROL", "AND",
"PLP", "AND", "ROL", "PLD", "BIT", "AND", "ROL", "AND",
"BMI", "AND", "AND", "AND", "BIT", "AND", "ROL", "AND",
"SEC", "AND", "DEC", "TSC", "BIT", "AND", "ROL", "AND",
"RTI", "EOR", "???", "EOR", "MVP", "EOR", "LSR", "EOR",
"PHA", "EOR", "LSR", "PHK", "JMP", "EOR", "LSR", "EOR",
"BVC", "EOR", "EOR", "EOR", "MVN", "EOR", "LSR", "EOR",
"CLI", "EOR", "PHY", "TCD", "JMP", "EOR", "LSR", "EOR",
"RTS", "ADC", "PER", "ADC", "STZ", "ADC", "ROR", "ADC",
"PLA", "ADC", "ROR", "RTL", "JMP", "ADC", "ROR", "ADC",
"BVS", "ADC", "ADC", "ADC", "STZ", "ADC", "ROR", "ADC",
"SEI", "ADC", "PLY", "TDC", "JMP", "ADC", "ROR", "ADC",
"BRA", "STA", "BRL", "STA", "STY", "STA", "STX", "STA",
"DEY", "BIT", "TXA", "PHB", "STY", "STA", "STX", "STA",
"BCC", "STA", "STA", "STA", "STY", "STA", "STX", "STA",
"TYA", "STA", "TXS", "TXY", "STZ", "STA", "STZ", "STA",
"LDY", "LDA", "LDX", "LDA", "LDY", "LDA", "LDX", "LDA",
"TAY", "LDA", "TAX", "PLB", "LDY", "LDA", "LDX", "LDA",
"BCS", "LDA", "LDA", "LDA", "LDY", "LDA", "LDX", "LDA",
"CLV", "LDA", "TSX", "TYX", "LDY", "LDA", "LDX", "LDA",
"CPY", "CMP", "REP", "CMP", "CPY", "CMP", "DEC", "CMP",
"INY", "CMP", "DEX", "WAI", "CPY", "CMP", "DEC", "CMP",
"BNE", "CMP", "CMP", "CMP", "PEI", "CMP", "DEC", "CMP",
"CLD", "CMP", "PHX", "STP", "JML", "CMP", "DEC", "CMP",
"CPX", "SBC", "SEP", "SBC", "CPX", "SBC", "INC", "SBC",
"INX", "SBC", "NOP", "XBA", "CPX", "SBC", "INC", "SBC",
"BEQ", "SBC", "SBC", "SBC", "PEA", "SBC", "INC", "SBC",
"SED", "SBC", "PLX", "XCE", "JSR", "SBC", "INC", "SBC"
};

int addrmodes[256] = {
21,  9, 21, 22,  4,  4,  4, 19, 21, 49,  5, 21,  2,  2,  2,  3,	
15,  7, 18, 23,  4, 10, 10,  8,  6, 13,  5,  6,  2, 12, 12, 14,
 2,  9,  3, 22,  4,  4,  4, 19, 21, 49,  5, 21,  2,  2,  2,  3,
15,  7, 18, 23, 10, 10, 10,  8,  6, 13,  5,  6, 12, 12, 12, 14,
 6,  9,  6, 22, 24,  4,  4, 19, 21, 49,  5, 21,  2,  2,  2,  3,
15,  7, 18, 23, 24, 10, 10,  8,  6, 13, 21,  6,  3, 12, 12, 14,
21,  9, 33, 22,  4,  4,  4, 19, 21, 49,  5, 21, 17,  2,  2,  3,
15,  7, 18, 23, 10, 10, 10,  8,  6, 13, 21,  6, 20, 12, 12, 14,
15,  9, 16, 22,  4,  4,  4, 19,  6, 49,  6, 21,  2,  2,  2,  3,
15,  7, 18, 23, 10, 10, 10,  8,  6, 13,  6,  6,  2, 12, 12, 14,
65,  9, 65, 22,  4,  4,  4, 19,  6, 49,  6, 21,  2,  2,  2,  3,
15,  7, 18, 23, 10, 10, 11,  8,  6, 13,  6,  6, 12, 12, 13, 14,
65,  9,  1, 22,  4,  4,  4, 19,  6, 49,  6,  6,  2,  2,  2,  3,
15,  7, 18, 23,  4, 10, 10,  8,  6, 13, 21, 06, 17, 12, 12, 14,
65,  9,  1, 22,  4,  4,  4, 19,  6, 49,  6,  6,  2,  2,  2,  3,
15,  7, 18, 23, 33, 10, 10,  8,  6, 13, 21,  6, 20, 12, 12, 14
};

extern int cpu_irq;

void m65816_debug(void) {
	int	opcode;
	int	mode;
	int	operand;

	opcode = M_READ(PC.A);
	mode = addrmodes[opcode];
	printf("%02X/%04X    %s  ",(int) PC.B.PB,(int) PC.W.PC,mnemonics[opcode]);
	switch (mode) {
		case 0x01:	operand = M_READ(PC.A+1);
				printf("#$%02X        ", operand);
				break;
		case 0x21:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
				printf("#$%04X      ", operand);
				break;
		case 0x31:	if (F_getM) {
					operand = M_READ(PC.A+1);
					printf("#$%02X        ", operand);
				} else {
					operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
					printf("#$%04X      ", operand);
				}
				break;
		case 0x41:	if (F_getX) {
					operand = M_READ(PC.A+1);
					printf("#$%02X        ", operand);
				} else {
					operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
					printf("#$%04X      ", operand);
				}
				break;
		case 0x02:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
				printf("$%04X       ", operand);
				break;
		case 0x03:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8) | (M_READ(PC.A+3) << 16);
				printf("$%06X     ", operand);
				break;
		case 0x04:	operand = M_READ(PC.A+1);
				printf("$%02X         ", operand);
				break;
		case 0x05:	printf("            ");
				break;
		case 0x06:	printf("            ");
				break;
		case 0x07:	operand = M_READ(PC.A+1);
				printf("($%02X),y     ", operand);
				break;
		case 0x08:	operand = M_READ(PC.A+1);
				printf("[$%02X],y     ", operand);
				break;
		case 0x09:	operand = M_READ(PC.A+1);
				printf("($%02X,x)     ", operand);
				break;
		case 0x0A:	operand = M_READ(PC.A+1);
				printf("$%02X,x       ", operand);
				break;
		case 0x0B:	operand = M_READ(PC.A+1);
				printf("$%02X,y       ", operand);
				break;
		case 0x0C:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
				printf("$%04X,x     ", operand);
				break;
		case 0x0D:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
				printf("$%04X,y     ", operand);
				break;
		case 0x0E:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8) | (M_READ(PC.A+3) << 16);
				printf("$%06X,x   ", operand);
				break;
		case 0x0F:	operand = PC.W.PC + 2 + (offset_s) M_READ(PC.A+1);
				printf("$%04X       ", operand & 0xFFFF);
				break;
		case 0x10:	operand = PC.W.PC + 3 + (offset_l) (M_READ(PC.A+1) | (M_READ(PC.A+2) << 8));
				printf("$%04X       ", operand & 0xFFFF);
				break;
		case 0x11:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
				printf("($%04X)     ", operand);
				break;
		case 0x12:	operand = M_READ(PC.A+1);
				printf("($%02X)       ", operand);
				break;
		case 0x13:	operand = M_READ(PC.A+1);
				printf("[$%02X]       ", operand);
				break;
		case 0x14:	operand = M_READ(PC.A+1) | (M_READ(PC.A+2) << 8);
				printf("($%04X,x)   ", operand);
				break;
		case 0x15:	printf("            ");
				break;
		case 0x16:	operand = M_READ(PC.A+1);
				printf("$%02X,s       ", operand);
				break;
		case 0x17:	operand = M_READ(PC.A+1);
				printf("($%02X,s),y   ", operand);
				break;
		case 0x18:	operand = M_READ(PC.A+1);
				printf("$%02X,",operand);
				operand = M_READ(PC.A+2);
				printf("$%02X     ",operand);
				break;
	}
	printf("A=%04X X=%04X Y=%04X S=%04X D=%04X DB=%02X P=%02X E=%1d\n",(int) A.W, (int) X.W,
									   (int) Y.W, (int) S.W,
									   (int) D.W, (int) DB,
									   (int) P, (int) E);
	fflush(stdout);
}

#endif
