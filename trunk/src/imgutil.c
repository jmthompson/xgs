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
 * File: imgutil.c
 *
 * This is the all-in-one image utility for manipulating XGS image
 * files.
 *
 * Patched by Joel Sutton 17th April, 1997
 *       Removed all of those nasty little gets's. Also fixed what 
 *       appeared to be a comments typo.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disks.h"

byte	block_buffer[512];
byte	track_buffer[4096];
char	buffer[256];

char	sourcefile[256],destfile[256];

image_header	image;
diskcopy_header	dcimage;

/* Function prototypes */

int	getSource(void);
int	getDest(void);
int	getSize(void);
void	waitForReturn(void);
int	newImage(char *, int);
int	lockImage(char *);
int	unlockImage(char *);
int	imageInfo(char *);
int	diskCopyToImage(char *, char *);
int	dosOrderToImage(char *, char *);
int	rawToImage(char *, char *);

int getSource()
{
	printf("\nEnter the source filename below or press RETURN to abort.\n: ");
	fgets(sourcefile,256,stdin);
	sourcefile[strlen(sourcefile)-1]='\0';
	return strlen(sourcefile);
}

int getDest()
{
	printf("\nEnter the destination filename below or press RETURN to abort.\n: ");
	fgets(destfile,256,stdin);
	destfile[strlen(destfile)-1]='\0';
	return strlen(destfile);
}

void waitForReturn()
{
	printf("\n\nPress RETURN to continue: ");
	fgets(buffer,256,stdin);
}

int getSize()
{
	char	last;
	int	size;

	printf("Enter a size for this image in bytes. You may also specify the size in\n");
    printf("kilobytes or megabytes by adding a 'K' or 'M' to the end of your input.\n");
    printf("You may enter any size between 16K and 32M.  To abort, just press RETURN.\n");
    printf(": ");
	fgets(buffer,256,stdin);
	if (!strlen(buffer)) return 0;
	size = atoi(buffer);
	last = buffer[strlen(buffer)-2];
	if ((last == 'k') || (last == 'K')) {
		size *= 1024;
	} else if ((last == 'm') || (last == 'M')) {
		size *= 1048576;
	}
	if (size == 33554431) size -= 1;
	if ((size < 16384) || (size > 33554430)) {
		printf("You have entered an invalid size.\n");
		size = 0;
	}
	return size;
}

int newImage(char *filename, int size)
{
	int	i;
	word16	blocks;
	FILE	*fp;

	if ((fp = fopen(filename,"wb")) == NULL) {
		perror("Can't create output file");
		return 1;
	}

	memset(&image, 0, sizeof(image));

	blocks = size / 512;
	printf("** Creating an image file of %hu blocks.\n",blocks);

	strncpy(image.magic,IMAGE_MAGIC,4);
	strncpy(image.creator,IMAGE_CR_XGS,4);
	image.header_len = sizeof(image);
	image.version = 0x0000;
	image.image_format = IMAGE_IF_RAW_PO;
	image.flags = 0;
	image.num_blocks = blocks;
	image.data_offset = sizeof(image);
	image.data_len = blocks * 512;

	DSK_writeUnivImageHeader(fp, &image);

	printf("\nZeroing image data: ");

	memset(block_buffer, 0, sizeof(block_buffer));

	for (i = 0 ; i < blocks ; i++) {
		fwrite(block_buffer,1,512,fp);
		if (!(i & 0xFF)) {
			printf(".");
			fflush(stdout);
		}
	}

	printf("\n\nDone.\n");

	fclose(fp);
	return 0;
}

int lockImage(char *filename)
{
	FILE		*fp;

	if ((fp = fopen(filename,"rb+")) == NULL) {
		perror("Can't open image file");
		return 1;
	}

	DSK_readUnivImageHeader(fp, &image);

	if (strncmp(image.magic,IMAGE_MAGIC,4)) {
		printf("\nImage header is corrupt.\n");
		fclose(fp);
		return 2;
	}

	if (image.version != 0x00) {
		printf("Unknown image format. Cannot continue.\n");
		fclose(fp);
		return 3;
	}

	image.flags |= IMAGE_FL_LOCKED;

	rewind(fp);
	DSK_writeUnivImageHeader(fp, &image);
	fclose(fp);

	printf("Image locked.\n");

	return 0;
}

int unlockImage(char *filename)
{
	FILE		*fp;

	if ((fp = fopen(filename,"rb+")) == NULL) {
		perror("Can't open image file");
		return 1;
	}

	DSK_readUnivImageHeader(fp, &image);

	if (strncmp(image.magic,IMAGE_MAGIC,4)) {
		printf("\nImage header is corrupt.\n");
		fclose(fp);
		return 2;
	}

	if (image.version != 0x00) {
		printf("Unknown image format. Cannot continue.\n");
		fclose(fp);
		return 3;
	}

	image.flags &= ~IMAGE_FL_LOCKED;

	rewind(fp);
	DSK_writeUnivImageHeader(fp, &image);
	fclose(fp);

	printf("Image unlocked.\n");

	return 0;
}

int imageInfo(char *filename)
{
	FILE		*fp;

	if ((fp = fopen(filename,"rb")) == NULL) {
		perror("Can't open image file");
		return 1;
	}

	DSK_readUnivImageHeader(fp, &image);

	if (strncmp(image.magic,IMAGE_MAGIC,4)) {
		printf("\nImage header is corrupt.\n");
		fclose(fp);
		return 2;
	}

	if (image.version != 0x00) {
		printf("Unknown image format. Cannot continue.\n");
		fclose(fp);
		return 3;
	}

	printf("\nImage statistics:\n\n");

	printf("     Filename : %s\n",filename);
	printf("No. of blocks : %hu\n",(unsigned int) image.num_blocks);
	printf("Write Protect : ");
	if (image.flags & IMAGE_FL_LOCKED) {
		printf("Enabled\n");
	} else {
		printf("Disabled\n");
	}

/*
	printf("\nDescription   : \n%s\n",image.desc);
*/

	printf("\n");

	fclose(fp);
	return 0;
}

int diskCopyToImage(char *source, char *dest)
{
	int	i,j;
	char	*cp;
	word32	checksum,ctmp;
	word16	blocks;
	word16	*wp;
	FILE	*fp1,*fp2;

	if ((fp1 = fopen(source,"rb")) == NULL) {
		perror("Can't open source file");
		return 1;
	}

	if ((fp2 = fopen(dest,"wb")) == NULL) {
		perror("Can't open destination file");
		fclose(fp1);
		return 1;
	}

	fread((void *) &dcimage,1,sizeof(dcimage),fp1);

#ifndef WORDS_BIGENDIAN
	DSK_swapField(&dcimage.dataSize,4);
	DSK_swapField(&dcimage.tagSize,4);
	DSK_swapField(&dcimage.dataChecksum,4);
	DSK_swapField(&dcimage.tagChecksum,4);
	DSK_swapField(&dcimage.private,2);
#endif

	if (dcimage.private != 0x0100) {
		printf("Unknown DiskCopy format. Cannot continue.\n");
		fclose(fp1);
		fclose(fp2);
		return 4;
	}

	strncpy(buffer,dcimage.diskName+1,dcimage.diskName[0]);
	buffer[(int) dcimage.diskName[0]] = 0;

	printf("\nDiskCopy image statistics:\n\n");

	printf("    Disk name : %s\n\n",buffer);

	printf("    Data size : %ld bytes\n",dcimage.dataSize);
	printf("Data checksum : $%08lX\n\n",dcimage.dataChecksum);

	printf("     Tag size : %ld bytes\n",dcimage.tagSize);
	printf(" Tag checksum : $%08lX\n\n",dcimage.tagChecksum);

	switch (dcimage.diskFormat) {
		case 0x00 :	cp = "400K";
				break;
		case 0x01 :	cp = "800K";
				break;
		case 0x02 :	cp = "720K";
				break;
		case 0x03 :	cp = "1440K";
				break;
		default :	cp = "Unknown";
				break;
	}
	printf("    Disk type : %s\n",cp);
	switch(dcimage.formatByte) {
		case 0x12 :	cp = "400K Macintosh";
				break;
		case 0x22 :	cp = ">400K Macintosh";
				break;
		case 0x24 :	cp = "800K Apple II";
				break;
		default :	cp = "Unknown";
				break;
	}
	printf("  Disk format : %s\n\n",cp);

	memset(&image, 0, sizeof(image));

	blocks = dcimage.dataSize / 512;
	printf("** Creating an image file of %hu blocks.\n",blocks);

	strncpy(image.magic,IMAGE_MAGIC,4);
	strncpy(image.creator,IMAGE_CR_XGS,4);
	image.header_len = sizeof(image);
	image.version = 0x0000;
	image.image_format = IMAGE_IF_RAW_PO;
	image.flags = IMAGE_FL_LOCKED;
	image.num_blocks = blocks;
	image.data_offset = sizeof(image);
	image.data_len = blocks * 512;

	cp = strdup(buffer);	/* save this in case we need it */

	DSK_writeUnivImageHeader(fp2, &image);

	printf("\nCopying data: ");
	checksum = 0;
	for (i = 0 ; i < blocks ; i++) {
		fread(block_buffer,1,512,fp1);
		fwrite(block_buffer,1,512,fp2);
		for (j = 0 ; j < 256 ; j++) {
			wp = ((word16 *) block_buffer) + j;
#ifndef WORDS_BIGENDIAN
			DSK_swapField(wp,2);
#endif
			checksum += *wp;
			ctmp = checksum & 0x01;
			checksum = (checksum >> 1) | (ctmp << 31);
		}
		printf(".");
		fflush(stdout);
	}

	fclose(fp1);
	fclose(fp2);

	if (checksum == dcimage.dataChecksum) {
		printf("\n\nChecksums match. Image is OK.\n");
		return 0;
	} else {
		printf("\n\nWARNING: data checksum mismatch (wanted $%08lX, got $%08lX). Image may be corrupt.\n",dcimage.dataChecksum,checksum);
		return 11;
	}
}

int imageToDiskCopy(char *source, char *dest)
{
	int	i,j;
	word32	checksum,ctmp;
	word16	blocks;
	word16	*wp;
	FILE	*fp1,*fp2;

	imageInfo(source);

	if ((fp1 = fopen(source,"rb")) == NULL) {
		perror("Can't open source file");
		return 1;
	}

	if ((fp2 = fopen(dest,"wb")) == NULL) {
		perror("Can't open destination file");
		fclose(fp1);
		return 1;
	}

	DSK_readUnivImageHeader(fp1, &image);

	if (strncmp(image.magic,IMAGE_MAGIC,4)) {
		printf("\nImage header is corrupt.\n");
		fclose(fp1);
		return 2;
	}

	if (image.version != 0x0000) {
		printf("Unknown image format. Cannot continue.\n");
		fclose(fp1);
		return 3;
	}

	if (image.image_format != IMAGE_IF_RAW_PO) {
		printf("Image not in ProDOS order. Cannot continue.\n");
		fclose(fp1);
		return 4;
	}

	bzero(&dcimage, sizeof(dcimage));

	printf("\nGenerated DiskCopy image statistics:\n\n");

	dcimage.diskName[0] = 7;
	strcpy(dcimage.diskName+1, "UNKNOWN");

	dcimage.tagSize = 0;
	dcimage.tagChecksum = 0;
	dcimage.diskFormat = 0xFF;
	dcimage.formatByte = 0xFF;
	dcimage.private = 0x0100;
	dcimage.dataSize = 512 * image.num_blocks;
	printf("    Data size : %ld bytes\n",dcimage.dataSize);

	fwrite(&dcimage,1,sizeof(dcimage),fp2);

	printf("\nCopying data: ");
	checksum = 0;
	blocks = dcimage.dataSize / 512;

	fseek(fp1,image.data_offset,SEEK_SET);
	for (i = 0 ; i < blocks ; i++) {
		fread(block_buffer,1,512,fp1);
		fwrite(block_buffer,1,512,fp2);
		for (j = 0 ; j < 256 ; j++) {
			wp = ((word16 *) block_buffer) + j;
#ifndef WORDS_BIGENDIAN
			DSK_swapField(wp,2);
#endif
			checksum += *wp;
			ctmp = checksum & 0x01;
			checksum = (checksum >> 1) | (ctmp << 31);
		}
		printf(".");
		/* way too many periods, flush less often. */
		{static loopi=0;
			if (loopi++%80==79)
				fflush(stdout);
		}
	}
	fflush(stdout);

	dcimage.dataChecksum = checksum;

#ifndef WORDS_BIGENDIAN
	DSK_swapField(&dcimage.dataSize,4);
	DSK_swapField(&dcimage.tagSize,4);
	DSK_swapField(&dcimage.dataChecksum,4);
	DSK_swapField(&dcimage.tagChecksum,4);
	DSK_swapField(&dcimage.private,2);
#endif

	rewind(fp2);
	fwrite(&dcimage,1,sizeof(dcimage),fp2);

	fclose(fp1);
	fclose(fp2);
	return 0;
}

int dosOrderToImage(char *source, char *dest)
{
	int	i;
	word16	blocks;
	FILE	*fp1,*fp2;
	word32	dataSize;

	if ((fp1 = fopen(source,"rb")) == NULL) {
		perror("Can't open source file");
		return 1;
	}

	if ((fp2 = fopen(dest,"wb")) == NULL) {
		perror("Can't open destination file");
		fclose(fp1);
		return 1;
	}

	if(fseek(fp1, 0L, SEEK_END)) {
		perror("Can't fseek to the end of the source file");	
		return 1;
	}
	
	dataSize = ftell(fp1);
	rewind(fp1);

	printf("Image size : %ld bytes\n",dataSize);

	memset(&image, 0, sizeof(image));

	blocks = dataSize / 512;
	printf("** Creating an image file of %hu blocks.\n",blocks);

	strncpy(image.magic,IMAGE_MAGIC,4);
	strncpy(image.creator,IMAGE_CR_XGS,4);
	image.header_len = sizeof(image);
	image.version = 0x0000;
	image.image_format = IMAGE_IF_RAW_PO;
	image.flags = IMAGE_FL_LOCKED;
	image.num_blocks = blocks;
	image.data_offset = sizeof(image);
	image.data_len = blocks * 512;

	DSK_writeUnivImageHeader(fp2, &image);

	printf("\nCopying data: ");
	for (i = 0 ; i < blocks/8 ; i++) {
		fread(track_buffer,1,4096,fp1);
		fwrite(track_buffer,1,256,fp2);
		fwrite(track_buffer+0xE00,1,256,fp2);
		fwrite(track_buffer+0xD00,1,256,fp2);
		fwrite(track_buffer+0xC00,1,256,fp2);
		fwrite(track_buffer+0xB00,1,256,fp2);
		fwrite(track_buffer+0xA00,1,256,fp2);
		fwrite(track_buffer+0x900,1,256,fp2);
		fwrite(track_buffer+0x800,1,256,fp2);
		fwrite(track_buffer+0x700,1,256,fp2);
		fwrite(track_buffer+0x600,1,256,fp2);
		fwrite(track_buffer+0x500,1,256,fp2);
		fwrite(track_buffer+0x400,1,256,fp2);
		fwrite(track_buffer+0x300,1,256,fp2);
		fwrite(track_buffer+0x200,1,256,fp2);
		fwrite(track_buffer+0x100,1,256,fp2);
		fwrite(track_buffer+0xF00,1,256,fp2);
		printf(".");
		fflush(stdout);
	}

	fclose(fp1);
	fclose(fp2);

	printf("\n\nImage created successfully.\n");

	return 0;
}

int rawToImage(char *source, char *dest)
{
	word32	blocks,i;
	FILE	*fp1,*fp2;
	word32	dataSize;

	if ((fp1 = fopen(source,"rb")) == NULL) {
		perror("Can't open source file");
		return 1;
	}

	if ((fp2 = fopen(dest,"wb")) == NULL) {
		perror("Can't open destination file");
		fclose(fp1);
		return 1;
	}


	if(fseek(fp1, 0L, SEEK_END)) {
		perror("Can't fseek to the end of the source file");	
		return 1;
	}
	
	dataSize = ftell(fp1);
	rewind(fp1);

	printf("Image size : %ld bytes\n",dataSize);

	memset(&image, 0, sizeof(image));

	blocks = dataSize / 512;
	printf("** Creating an XGS image file of %hu blocks.\n",(unsigned int) blocks);

	strncpy(image.magic,IMAGE_MAGIC,4);
	strncpy(image.creator,IMAGE_CR_XGS,4);
	image.header_len = sizeof(image);
	image.version = 0x0000;
	image.image_format = IMAGE_IF_RAW_PO;
	image.flags = IMAGE_FL_LOCKED;
	image.num_blocks = blocks;
	image.data_offset = sizeof(image);
	image.data_len = blocks * 512;

	DSK_writeUnivImageHeader(fp2, &image);

	printf("\nCopying data: ");
	for (i = 0 ; i < blocks ; i++) {
		fread(block_buffer,1,512,fp1);
		fwrite(block_buffer,1,512,fp2);
		printf(".");
		fflush(stdout);
	}

	fclose(fp1);
	fclose(fp2);

	printf("\n\nImage created successfully.\n");

	return 0;
}

int main(int argc, char *argv[]) {
	int	done,valid,size;

	if (argc != 1) {
		fprintf(stderr,"Usage: %s\n",argv[0]);
		exit(-1);
	}

	done = 0;

	while (!done) {
		printf("\n******************\nXGS Image Utility\n%s\n******************\n\
\n1. Create a blank image file\
\n2. Lock an image file\
\n3. Unlock an image file\
\n4. Display information about an\
\n   image file\
\n5. Convert a DiskCopy 4.2 image\
\n   to 2IMG format\
\n6. Convert a DOS 3.3-order image\
\n   (*.DSK file) to 2IMG format\
\n7. Convert a ProDOS-order image\
\n   (*.PO file) to 2IMG format\
\n8. Convert a 2IMG image to\
\n   DiskCopy format.\
\n\
\nQ. Quit the Image Utility\n"
		, VERSION);
		valid = 0;
		while(!valid) {
			printf("\nSelection [ 1-7, Q ]: ");
			fgets(buffer,256,stdin);
			switch(buffer[0]) {
				case '1' :	if (!getDest()) break;
						if (!(size = getSize())) break;
						newImage(destfile, size);
						waitForReturn();
						valid = 1;
						break;
				case '2' :	if (!getSource()) break;
						lockImage(sourcefile);
						waitForReturn();
						valid = 1;
						break;
				case '3' :	if (!getSource()) break;
						unlockImage(sourcefile);
						waitForReturn();
						valid = 1;
						break;
				case '4' :	if (!getSource()) break;
						imageInfo(sourcefile);
						waitForReturn();
						valid = 1;
						break;
				case '5' :	if (!getSource()) break;
						if (!getDest()) break;
						diskCopyToImage(sourcefile,
								destfile);
						waitForReturn();
						valid = 1;
						break;
				case '6' :	if (!getSource()) break;
						if (!getDest()) break;
						dosOrderToImage(sourcefile,
								destfile);
						waitForReturn();
						valid = 1;
						break;
				case '7' :	if (!getSource()) break;
						if (!getDest()) break;
						rawToImage(sourcefile,
							   destfile);
						waitForReturn();
						valid = 1;
						break;
				case '8' :	if (!getSource()) break;
						if (!getDest()) break;
						imageToDiskCopy(sourcefile,
								destfile);
						waitForReturn();
						valid = 1;
						break;
				case 'q' :
				case 'Q' :	valid = 1;
						done = 1;
						break;
				default :	break;
			} /* switch */
		} /* while (!valid) */
	} /* while (!done) */
	return 0;
}
