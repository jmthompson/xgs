/*********************************************************************
 *                                                                   *
 *                     XGS : Apple IIGS Emulator                     *
 *                                                                   *
 *        Written and Copyright (C)1996 by Joshua M. Thompson        *
 *                                                                   *
 *  You are free to distribute this code for non-commercial purposes *
 * I ask only that you notify me of any changes you make to the code *
 *     Commercial use is prohibited without my written permission    *
 *                                                                   *
 *********************************************************************/

/*
 * File: smtport.c
 *
 * Emulates a smartport device in slot 7, supporting up to 14 ProDOS
 * block devices of up to 32 MB each.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disks.h"
#include "emul.h"
#include "smtport.h"

extern byte	dsk_buffer[512];

disk_struct	*smpt_dev[NUM_SMPT_DEVS];

char		smpt_id_string[17] = "XGS SmartPort   ";

byte		smpt_rom[256] = {
	0xA9, 0x20, 0xA9, 0x00, 0xA9, 0x03, 0xA9, 0x00, 0xA9, 0x01, 0x85, 0x42, 0x64, 0x43, 0x64, 0x44,
	0xA9, 0x08, 0x85, 0x45, 0x64, 0x46, 0x64, 0x47, 0x42, 0xC7, 0xB0, 0x77, 0xA9, 0xC7, 0x8D, 0xF8,
	0x07, 0xA9, 0x07, 0xA2, 0x70, 0x4C, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x4C, 0x90, 0xC7, 0x42, 0xC8, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x42, 0xC7, 0x60, 0x4C, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x93, 0x80,
};

typedef struct {
	byte	cmd;
	byte	parms;
	void	(*func)(int);
} smpt_cmd_def;

smpt_cmd_def	smpt_commands[] = {
	{ 0x00, 0x03, SMPT_statusCmd		},
	{ 0x01, 0x03, SMPT_readBlockCmd		},
	{ 0x02, 0x03, SMPT_writeBlockCmd	},
	{ 0x03, 0x01, SMPT_formatCmd		},
	{ 0x04, 0x03, SMPT_controlCmd		},
	{ 0x05, 0x01, SMPT_initCmd		},
	{ 0x06, 0x01, SMPT_openCmd		},
	{ 0x07, 0x01, SMPT_closeCmd		},
	{ 0x08, 0x04, SMPT_readCmd		},
	{ 0x09, 0x04, SMPT_writeCmd		},
	{ 0x40, 0x03, SMPT_statusCmdExt		},
	{ 0x41, 0x03, SMPT_readBlockCmdExt	},
	{ 0x42, 0x03, SMPT_writeBlockCmdExt	},
	{ 0x43, 0x01, SMPT_formatCmdExt		},
	{ 0x44, 0x03, SMPT_controlCmdExt	},
	{ 0x45, 0x01, SMPT_initCmdExt		},
	{ 0x46, 0x01, SMPT_openCmdExt		},
	{ 0x47, 0x01, SMPT_closeCmdExt		},
	{ 0x48, 0x04, SMPT_readCmdExt		},
	{ 0x49, 0x04, SMPT_writeCmdExt		},
	{ 0xFF, 0xFF, NULL			}
};

int SMPT_init()
{
	int	i;

	printf("\nInitializing SmartPort emulation\n");
	for (i = 0 ; i < NUM_SMPT_DEVS ; i++) {
		smpt_dev[i] = NULL;
	}
	return 0;
}

void SMPT_update()
{
}

void SMPT_reset()
{
}

void SMPT_shutdown()
{
	int	i;
	printf("\nShutting down SmartPort emulation\n");
	for (i = 0 ; i < NUM_SMPT_DEVS ; i++) {
		if (smpt_dev[i]) free(smpt_dev[i]);
	}
}

int SMPT_loadDrive(int dev, char *path)
{
	disk_struct	*image;
	int		err;

	if ((dev < 0) || (dev >= NUM_SMPT_DEVS)) return -1;
	if (!path) {
		smpt_dev[dev] = NULL;
		return 0;
	}
	if ((err = DSK_openImage(path, &image))) {
		smpt_dev[dev] = NULL;
		return err;
	}
	if (image->header.image_format != IMAGE_IF_RAW_PO) {
		DSK_closeImage(image);
		smpt_dev[dev] = NULL;
		return -2;
	}
	smpt_dev[dev] = image;
	return 0;
}

int SMPT_unloadDrive(int dev)
{
	if ((dev < 0) || (dev >= NUM_SMPT_DEVS)) return -1;
	if (smpt_dev[dev]) DSK_closeImage(smpt_dev[dev]);
	smpt_dev[dev] = NULL;
	return 0;
}

/* Handle calls to the ProDOS block device entry point for slot 7 */

void SMPT_prodosEntry()
{
	int		cmd,i;
	dualw		block,buffer;
	disk_struct	*unit;

	cmd = MEM_readMem(0x000042);
	unit = (MEM_readMem(0x000043) & 0x80)? smpt_dev[1] : smpt_dev[0];
	buffer.B.L = MEM_readMem(0x000044);
	buffer.B.H = MEM_readMem(0x000045);
	block.B.L = MEM_readMem(0x000046);
	block.B.H = MEM_readMem(0x000047);

	if (!unit) {
		A.B.L = 0x028;	/* NO DEVICE CONNECTED */
		P |= 0x01;
		return;
	}

	A.B.L = 0;

	switch(cmd) {
		case 0x00 :	X.B.L = unit->header.num_blocks & 0xFF;
				Y.B.L = unit->header.num_blocks >> 8;
				break;
		case 0x01 :	if (DSK_readChunk(unit, dsk_buffer,
				    block.W, 1)) {
					A.B.L = 0x27;	/* I/O ERROR */
					break;
				}
				for (i = 0 ; i < 512 ; i++) {
					MEM_writeMem(buffer.W + i, dsk_buffer[i]);
				}
				break;
		case 0x02 :	if (unit->header.flags & IMAGE_FL_LOCKED) {
					A.B.L = 0x2B;	/* WRITE PROTECTED */
					break;
				}
				for (i = 0 ; i < 512 ; i++) {
					dsk_buffer[i] = MEM_readMem(buffer.W + i);
				}
				if (DSK_writeChunk(unit, dsk_buffer,
				    block.W, 1)) {
					A.B.L = 0x27;	/* I/O ERROR */
					break;
				}
				break;
		case 0x03 :	if (unit->header.flags & IMAGE_FL_LOCKED) {
					A.B.L = 0x2B;	/* WRITE PROTECTED */
				}
				break;
		default :	A.B.L = 0x01;	/* BAD CALL */
				break;
	}
	if (A.B.L) {
		P |= 0x01;
	} else {
		P &= ~0x01;
	}
}

/* Handle calls to the SmartPort entry point for slot 7 */

void SMPT_smartportEntry()
{
	duala		addr,parms;
	int		i,cmd,parm_cnt;

	addr.B.L = MEM_readMem(S.W + 1);
	addr.B.H = MEM_readMem(S.W + 2);
	addr.W.L++;
	addr.W.H = 0;

	cmd = MEM_readMem(addr.A);
	parms.B.L = MEM_readMem(addr.A + 1);
	parms.B.H = MEM_readMem(addr.A + 2);
	if (cmd & 0x40) {
		parms.B.B = MEM_readMem(addr.A + 3);
		parms.B.Z = MEM_readMem(addr.A + 4);
		addr.W.L += 4;
	} else {
		parms.W.H = 0;
		addr.W.L += 2;
	}
	
	MEM_writeMem(S.W + 1, addr.B.L);
	MEM_writeMem(S.W + 2, addr.B.H);

	parm_cnt = MEM_readMem(parms.A);

	A.B.L = 0x01;	/* Invalid command */

	i = 0;
	while (smpt_commands[i].func != NULL) {
		if ((smpt_commands[i].cmd == cmd) && (smpt_commands[i].parms == parm_cnt)) {
			(smpt_commands[i].func)(parms.A);
			break;
		}
		i++;
	}

	if (A.B.L) {
		P |= 0x01;
	} else {
		P &= ~0x01;
	}
}

void SMPT_statusCmd(int parms)
{
	int		unit_num,status_code,i;
	duala		status_ptr;
	disk_struct	*unit;

	unit_num = MEM_readMem(parms + 1);
	if ((unit_num < 0) || (unit_num > NUM_SMPT_DEVS)) {
		A.B.L = 0x28;
		return;
	}
	if (unit_num) {
		unit = smpt_dev[unit_num - 1];
	} else {
		unit = NULL;
	}
	status_ptr.B.L = MEM_readMem(parms + 2);
	status_ptr.B.H = MEM_readMem(parms + 3);
	status_ptr.W.H = 0;
	status_code = MEM_readMem(parms + 4);
	switch(status_code) {
		case 0x00 :	if (!unit_num) {
					MEM_writeMem(status_ptr.A, NUM_SMPT_DEVS);
					for (i = 1 ; i < 8 ; i++) MEM_writeMem(status_ptr.A + i, 0);
					X.B.L = 8;
					Y.B.L = 0;
					A.B.L = 0;
				} else {
					if (unit == NULL) {
						MEM_writeMem(status_ptr.A, 0xEC);
						MEM_writeMem(status_ptr.A + 1, 0);
						MEM_writeMem(status_ptr.A + 2, 0);
						MEM_writeMem(status_ptr.A + 3, 0);
					} else {
						if (unit->header.flags & IMAGE_FL_LOCKED) {
							MEM_writeMem(status_ptr.A, 0xFC);
						} else {
							MEM_writeMem(status_ptr.A, 0xF8);
						}
						MEM_writeMem(status_ptr.A + 1, (byte) (unit->header.num_blocks & 0xFF));
						MEM_writeMem(status_ptr.A + 2, (byte) (unit->header.num_blocks >> 8));
						MEM_writeMem(status_ptr.A + 3, 0);
					}
					X.B.L = 4;
					Y.B.L = 0;
					A.B.L = 0;
				}
				break;
		case 0x01 :	A.B.L = 0x21;
				break;
		case 0x02 :	A.B.L = 0x21;
				break;
		case 0x03 :	if (unit == NULL) {
					MEM_writeMem(status_ptr.A, 0xEC);
					MEM_writeMem(status_ptr.A + 1, 0);
					MEM_writeMem(status_ptr.A + 2, 0);
					MEM_writeMem(status_ptr.A + 3, 0);
				} else {
					if (unit->header.flags & IMAGE_FL_LOCKED) {
						MEM_writeMem(status_ptr.A, 0xF8);
					} else {
						MEM_writeMem(status_ptr.A, 0xFC);
					}
					MEM_writeMem(status_ptr.A + 1, (byte) (unit->header.num_blocks & 0xFF));
					MEM_writeMem(status_ptr.A + 2, (byte) (unit->header.num_blocks >> 8));
					MEM_writeMem(status_ptr.A + 3, 0);
				}
				MEM_writeMem(status_ptr.A + 4, 0x10);
				for (i = 0 ; i < 16 ; i++) {
					MEM_writeMem(status_ptr.A + 5 + i, smpt_id_string[i]);
				}
				MEM_writeMem(status_ptr.A + 21, 0x02);
				MEM_writeMem(status_ptr.A + 22, 0xC0);
				MEM_writeMem(status_ptr.A + 23, 0x00);
				MEM_writeMem(status_ptr.A + 24, 0x00);
				X.B.L = 25;
				Y.B.L = 0;
				A.B.L = 0;
				break;
		default :	A.B.L = 0x21;
				break;
	}
}

void SMPT_readBlockCmd(int parms)
{
	int		unit_num,i;
	duala		buffer,block;
	disk_struct	*unit;

	unit_num = MEM_readMem(parms + 1);

	if ((unit_num < 1) || (unit_num > NUM_SMPT_DEVS)) {
		A.B.L = 0x28;
		return;
	}
	if ((unit = smpt_dev[unit_num - 1]) == NULL) {
		A.B.L = 0x2F;
		return;
	}
	buffer.B.L = MEM_readMem(parms + 2);
	buffer.B.H = MEM_readMem(parms + 3);
	buffer.W.H = 0;
	block.B.L = MEM_readMem(parms + 4);
	block.B.H = MEM_readMem(parms + 5);
	block.B.B = MEM_readMem(parms + 6);
	block.B.Z = 0;

	if (block.A >= unit->header.num_blocks) {
		A.B.L = 0x2D;
		return;
	}

	if (DSK_readChunk(unit, dsk_buffer, block.A, 1)) {
		A.B.L = 0x27;
		return;
	}
	for (i = 0 ; i < 512 ; i++) {
		MEM_writeMem(buffer.A + i, dsk_buffer[i]);
	}
	X.B.L = 0x00;
	Y.B.L = 0x02;
	A.B.L = 0;
}

void SMPT_writeBlockCmd(int parms)
{
	int		unit_num,i;
	duala		buffer,block;
	disk_struct	*unit;

	unit_num = MEM_readMem(parms + 1);

	if ((unit_num < 1) || (unit_num > NUM_SMPT_DEVS)) {
		A.B.L = 0x28;
		return;
	}
	if ((unit = smpt_dev[unit_num - 1]) == NULL) {
		A.B.L = 0x2F;
		return;
	}

	if (unit->header.flags & IMAGE_FL_LOCKED) {
		A.B.L = 0x2B;
		return;
	}

	buffer.B.L = MEM_readMem(parms + 2);
	buffer.B.H = MEM_readMem(parms + 3);
	buffer.W.H = 0;
	block.B.L = MEM_readMem(parms + 4);
	block.B.H = MEM_readMem(parms + 5);
	block.B.B = MEM_readMem(parms + 6);
	block.B.Z = 0;

	if (block.A >= unit->header.num_blocks) {
		A.B.L = 0x2D;
		return;
	}

	for (i = 0 ; i < 512 ; i++) {
		dsk_buffer[i] = MEM_readMem(buffer.A+i);
	}
	if (DSK_writeChunk(unit, dsk_buffer, block.A, 1)) {
		A.B.L = 0x27;
		return;
	}
	X.B.L = 0x00;
	Y.B.L = 0x02;
	A.B.L = 0;
}

void SMPT_formatCmd(int parms)
{
	A.B.L = 0;
}

void SMPT_controlCmd(int parms)
{
#if 0
	unit_num = MEM_readMem((cmd_list+1) & mask, 0);
	ctl_ptr_lo = MEM_readMem((cmd_list+2) & mask, 0);
	ctl_ptr_hi = MEM_readMem((cmd_list+3) & mask, 0);
	ctl_code = MEM_readMem((cmd_list +4) & mask, 0);
	switch(ctl_code) {
	case 0x00:
		printf("Performing a reset on unit %d\n", unit);
		break;
	default:
		printf("control code: %02x unknown!\n", ctl_code);
	}
	engine.xreg = 0;
	engine.yreg = 2;
	engine.acc &= 0xff00;
#endif
	X.B.L = 0;
	Y.B.L = 0;
	A.B.L = 0;
}

void SMPT_initCmd(int parms)
{
}

void SMPT_openCmd(int parms)
{
}

void SMPT_closeCmd(int parms)
{
}

void SMPT_readCmd(int parms)
{
}

void SMPT_writeCmd(int parms)
{
}

void SMPT_statusCmdExt(int parms)
{
	int		unit_num,status_code,i;
	duala		status_ptr;
	disk_struct	*unit;

	unit_num = MEM_readMem(parms + 1);
	if ((unit_num < 0) || (unit_num > NUM_SMPT_DEVS)) {
		A.B.L = 0x28;
		return;
	}
	if (unit_num) {
		unit = smpt_dev[unit_num - 1];
	} else {
		unit = NULL;
	}
	status_ptr.B.L = MEM_readMem(parms + 2);
	status_ptr.B.H = MEM_readMem(parms + 3);
	status_ptr.B.B = MEM_readMem(parms + 4);
	status_ptr.B.Z = MEM_readMem(parms + 5);

	status_code = MEM_readMem(parms + 6);

	switch(status_code) {
		case 0x00 :	if (!unit_num) {
					MEM_writeMem(status_ptr.A, NUM_SMPT_DEVS);
					for (i = 1 ; i < 8 ; i++) MEM_writeMem(status_ptr.A + i, 0);
					X.B.L = 8;
					Y.B.L = 0;
					A.B.L = 0;
				} else {
					if (unit == NULL) {
						MEM_writeMem(status_ptr.A, 0xEC);
						MEM_writeMem(status_ptr.A + 1, 0);
						MEM_writeMem(status_ptr.A + 2, 0);
						MEM_writeMem(status_ptr.A + 3, 0);
						MEM_writeMem(status_ptr.A + 4, 0);
					} else {
						if (unit->header.flags & IMAGE_FL_LOCKED) {
							MEM_writeMem(status_ptr.A, 0xFC);
						} else {
							MEM_writeMem(status_ptr.A, 0xF8);
						}
						MEM_writeMem(status_ptr.A + 1, (byte) (unit->header.num_blocks & 0xFF));
						MEM_writeMem(status_ptr.A + 2, (byte) (unit->header.num_blocks >> 8));
						MEM_writeMem(status_ptr.A + 3, 0);
						MEM_writeMem(status_ptr.A + 4, 0);
					}
					X.B.L = 5;
					Y.B.L = 0;
					A.B.L = 0;
				}
				break;
		case 0x01 :	A.B.L = 0x21;
				break;
		case 0x02 :	A.B.L = 0x21;
				break;
		case 0x03 :	if (unit == NULL) {
					MEM_writeMem(status_ptr.A, 0xEC);
					MEM_writeMem(status_ptr.A + 1, 0);
					MEM_writeMem(status_ptr.A + 2, 0);
					MEM_writeMem(status_ptr.A + 3, 0);
					MEM_writeMem(status_ptr.A + 4, 0);
				} else {
					if (unit->header.flags & IMAGE_FL_LOCKED) {
						MEM_writeMem(status_ptr.A, 0xF8);
					} else {
						MEM_writeMem(status_ptr.A, 0xFC);
					}
					MEM_writeMem(status_ptr.A + 1, (byte) (unit->header.num_blocks & 0xFF));
					MEM_writeMem(status_ptr.A + 2, (byte) (unit->header.num_blocks >> 8));
					MEM_writeMem(status_ptr.A + 3, 0);
					MEM_writeMem(status_ptr.A + 4, 0);
				}
				MEM_writeMem(status_ptr.A + 5, 0x10);
				for (i = 0 ; i < 16 ; i++) {
					MEM_writeMem(status_ptr.A + 6 + i, smpt_id_string[i]);
				}
				MEM_writeMem(status_ptr.A + 22, 0x02);
				MEM_writeMem(status_ptr.A + 23, 0xC0);
				MEM_writeMem(status_ptr.A + 24, 0x00);
				MEM_writeMem(status_ptr.A + 25, 0x00);
				X.B.L = 26;
				Y.B.L = 0;
				A.B.L = 0;
				break;
		default :	A.B.L = 0x21;
				break;
	}
}

void SMPT_readBlockCmdExt(int parms)
{
	int		unit_num,i;
	duala		buffer,block;
	disk_struct	*unit;

	unit_num = MEM_readMem(parms + 1);

	if ((unit_num < 1) || (unit_num > NUM_SMPT_DEVS)) {
		A.B.L = 0x28;
		return;
	}
	if ((unit = smpt_dev[unit_num - 1]) == NULL) {
		A.B.L = 0x2F;
		return;
	}
	buffer.B.L = MEM_readMem(parms + 2);
	buffer.B.H = MEM_readMem(parms + 3);
	buffer.B.B = MEM_readMem(parms + 4);
	buffer.B.Z = MEM_readMem(parms + 5);

	block.B.L = MEM_readMem(parms + 6);
	block.B.H = MEM_readMem(parms + 7);
	block.B.B = MEM_readMem(parms + 8);
	block.B.Z = MEM_readMem(parms + 9);

	if (block.A >= unit->header.num_blocks) {
		A.B.L = 0x2D;
		return;
	}
	if (DSK_readChunk(unit, dsk_buffer, block.A, 1)) {
		A.B.L = 0x27;
		return;
	}
	for (i = 0 ; i < 512 ; i++) {
		MEM_writeMem(buffer.A+i,dsk_buffer[i]);
	}
	X.B.L = 0x00;
	Y.B.L = 0x02;
	A.B.L = 0;
}

void SMPT_writeBlockCmdExt(int parms)
{
	int		unit_num,i;
	duala		buffer,block;
	disk_struct	*unit;

	unit_num = MEM_readMem(parms + 1);

	if ((unit_num < 1) || (unit_num > NUM_SMPT_DEVS)) {
		A.B.L = 0x28;
		return;
	}
	if ((unit = smpt_dev[unit_num - 1]) == NULL) {
		A.B.L = 0x2F;
		return;
	}

	if (unit->header.flags & IMAGE_FL_LOCKED) {
		A.B.L = 0x2B;
		return;
	}

	buffer.B.L = MEM_readMem(parms + 2);
	buffer.B.H = MEM_readMem(parms + 3);
	buffer.B.B = MEM_readMem(parms + 4);
	buffer.B.Z = MEM_readMem(parms + 5);

	block.B.L = MEM_readMem(parms + 6);
	block.B.H = MEM_readMem(parms + 7);
	block.B.B = MEM_readMem(parms + 8);
	block.B.Z = MEM_readMem(parms + 9);

	if (block.A >= unit->header.num_blocks) {
		A.B.L = 0x2D;
		return;
	}

	for (i = 0 ; i < 512 ; i++) {
		dsk_buffer[i] = MEM_readMem(buffer.A+i);
	}
	if (DSK_writeChunk(unit, dsk_buffer, block.A, 1)) {
		A.B.L = 0x27;
		return;
	}
	X.B.L = 0x00;
	Y.B.L = 0x02;
	A.B.L = 0;
}

void SMPT_formatCmdExt(int parms)
{
	A.B.L = 0;
}

void SMPT_controlCmdExt(int parms)
{
}

void SMPT_initCmdExt(int parms)
{
}

void SMPT_openCmdExt(int parms)
{
}

void SMPT_closeCmdExt(int parms)
{
}

void SMPT_readCmdExt(int parms)
{
}

void SMPT_writeCmdExt(int parms)
{
}
