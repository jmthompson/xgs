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
 * File: imgconv.c
 *
 * Converts old proprietary XGS image files to the new
 * "2IMG" Universal Image Format.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disks.h"

/* Description of an XGS image file header.		*/
/* Note that multi-byte values are in LSB order!	*/

struct {
	char	magic[4];	/* Should be "XGS!"                */
	byte	version;	/* Format version (currently 0x00) */
	byte	pad1;		/* padding (x86 compatibility)     */
	word16	num_blocks;	/* Number of blocks in image       */
	byte	protected;	/* Nonzero if image is protected   */
	char	name[32];	/* Name of this image              */
	char	desc[1024];	/* Description of this image       */
	byte	pad2[3];	/* padding (x86 compatibility)     */
	byte	spare[8];	/* Spare bytes                     */
} xgs_image;

image_header	new_image;

byte		block[512];

int main(int argc, char *argv[])
{
	FILE	*fp1, *fp2;
	word32	i;

	if (argc != 3) {
		fprintf(stderr,"Usage: %s source dest\n",argv[0]);
		exit(-1);
	}

	fp1 = fopen(argv[1],"rb");
	if (!fp1) {
		perror("Couldn't open source image");
		exit(-2);
	}

	fp2 = fopen(argv[2],"wb");
	if (!fp2) {
		perror("Couldn't open destination image");
		fclose(fp1);
		exit(-2);
	}

	if (fread(&xgs_image, sizeof(xgs_image), 1, fp1) != 1) {
		perror("Error reading XGS image header");
		fclose(fp1);
		fclose(fp2);
		exit(-3);
	}

	if (strncmp(xgs_image.magic, "XGS!", 4)) {
		perror("Error reading XGS image header");
		fclose(fp1);
		fclose(fp2);
		exit(-4);
	}

#ifdef WORDS_BIGENDIAN
	DSK_swapField(&xgs_image.num_blocks,2);
#endif /* WORDS_BIGENDIAN */

	memset((void *) &new_image, 0, sizeof(new_image));

	strcpy(new_image.magic,IMAGE_MAGIC);
	strcpy(new_image.creator, IMAGE_CR_XGS);
	new_image.header_len = sizeof(new_image);
	new_image.version = 0x0000;
	new_image.image_format = IMAGE_IF_RAW_PO;
	new_image.flags = xgs_image.protected? IMAGE_FL_LOCKED : 0;
	new_image.num_blocks = xgs_image.num_blocks;
	new_image.data_offset = sizeof(new_image);
	new_image.data_len = new_image.num_blocks * 512;

	if (DSK_writeUnivImageHeader(fp2, &new_image)) {
		perror("Error writing 2IMG header");
		fclose(fp1);
		fclose(fp2);
		exit(-5);
	}

	printf("Converting image: ");
	for (i = 0 ; i < new_image.num_blocks ; i++) {
		if (fread(block, 512, 1, fp1) != 1) {
			perror("Error reading block from XGS image");
			fclose(fp1);
			fclose(fp2);
			exit(-6);
		}
		if (fwrite(block, 512, 1, fp2) != 1) {
			perror("Error writing block to 2IMG image");
			fclose(fp1);
			fclose(fp2);
			exit(-6);
		}
		if (!(i & 0x0F)) printf(".");
	}
	printf(".Done\n");
	fclose(fp1);
	fclose(fp2);
	return 0;
}
