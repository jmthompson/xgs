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
 * File: disks.c
 *
 * Core routines for manipulating disk image files.
 */

#include "xgs.h"

#include <ctype.h>
#include "disks.h"

/* Multi-purpose block buffer, shared by all drivers */

byte	dsk_buffer[512];

/*
 * Generic endianness-swap routine. Swaps any field containing
 * an even number of bytes.
 */

void DSK_swapField(void *field, int size)
{
	int	i,max;
	byte	*p,tmp;

	p = (byte *) field;
	max = size - 1;
	for (i = 0 ; i < (size / 2) ; i++) {
		tmp = p[i];
		p[i] = p[max - i];
		p[max - i] = tmp;
	}
}

/*
 * Open a disk image from the specified file, and return
 * a pointer to the image data.
 */

int DSK_openImage(char *pathname, disk_struct **disk)
{
	FILE		*fp;
	char		buff[4],*ext,*cp;
	int		err;

	*disk = (disk_struct *) malloc(sizeof(disk_struct));
	if (!*disk) {
		return -1;
	}

	/* Open the file read-write by default. If we can't,	*/
	/* try opening it read-only and forcing the loaded disk	*/
	/* to be write-protected.				*/

	(**disk).read_only = 0;
	fp = fopen(pathname,"rb+");
	if (!fp) {
		err = errno;
		if (err == EACCES) {
			(**disk).read_only = 1;
			fp = fopen(pathname,"rb");
			if (!fp) {
				err = errno;
				free(*disk);
				return err;
			}
		} else {
			free(*disk);
			return err;
		}
	}

	if (fread(buff,1,4,fp) != 4) {
		err = errno;
		free(*disk);
		fclose(fp);
		return err;
	}

	if (!strncmp(buff,IMAGE_MAGIC,4)) {
		return DSK_loadUnivImage(pathname, disk, fp);
	}

	/* Get the filename extension so we can try to	*/
	/* determine if it's one of the raw image	*/
	/* formats that we know how to handle.		*/

	ext = strrchr(pathname, '.');
	if (!ext) {
		fclose(fp);
		return -2;
	}

	/* Our buffer is only four bytes. */

	ext++;
	if (strlen(ext) > 3) {
		fclose(fp);
		free(*disk);
		return -2;
	}

	/* Make an all-lowercase copy of the extension	*/

	cp = buff;
	while (*ext) {
		*cp = *ext;
		if (isupper(*cp)) *cp += 0x20;
		ext++;
		cp++;
	}
	*cp = 0;

	/* Now check the extensions we know:
	 *
	 * .dsk = DOS 3.3-order raw image
	 * .do  = DOS 3.3-order raw image
	 * .po  = ProDOS-order raw image
	 * .raw = ProDOS-order raw image
	 * .nib = Nibblized 5.25" disk image, 6656 bytes/track
	 * .dc  = Macintosh DiskCopy 4.2 image
	 */

	if (!strcmp(buff,"dsk") || !strcmp(buff,"do")) {
		return DSK_loadDosOrder(pathname, disk, fp);
	} else if (!strcmp(buff,"po") || !strcmp(buff,"raw")) {
		return DSK_loadProdosOrder(pathname, disk, fp);
	} else if (!strcmp(buff,"nib")) {
		return DSK_loadNibblized(pathname, disk, fp);
	} else if (!strcmp(buff,"dc")) {
		return DSK_loadDiskCopy(pathname, disk, fp);
	} else {
		fclose(fp);
		free(*disk);
		return -2;
	}
}

int DSK_closeImage(disk_struct *disk)
{
	free(disk->pathname);
	fclose(disk->stream);
	free(disk);
	return 0;
}

int DSK_updateImage(disk_struct *disk)
{
	return 0;
}

/*
 * Read a "chunk" of data from an image.
 *
 * Chunk numbers and lengths are defined by the image type:
 *
 * IMAGE_IF_RAW_DO or
 * IMAGE_IF_RAW_PO : chunk_num = block_num;
 *                   len = 512;
 * IMAGE_IF_NIBBLE : chunk_num = track_num;
 *                   len = 6656;
 *
 * start = Starting chunk number
 * count = Number of chunks to read
 */

int DSK_readChunk(disk_struct *disk, byte *buffer, int start, int count)
{
	int	len,max;
	long	seek;

	switch(disk->header.image_format) {
		case IMAGE_IF_RAW_DO:
		case IMAGE_IF_RAW_PO:	len = 512;
					max = disk->header.num_blocks;
					break;
		case IMAGE_IF_NIBBLE:	len = 6656;
					max = 35;
					break;
		default:		return -666;
	}
	if ((start < 0) || (start > max) || ((start+count-1) > max)) {
		return -2;
	}
	seek = (start * len) + disk->header.data_offset;
	if (fseek(disk->stream, seek, SEEK_SET)) {
		return -4;
	}
	if (fread(buffer, len, count, disk->stream) != count) {
		return -5;
	}
	return 0;
}

/*
 * Write a "chunk" of data to an image.
 *
 * Chunk numbers and lengths are defined by the image type:
 *
 * IMAGE_IF_RAW_DO or
 * IMAGE_IF_RAW_PO : chunk_num = block_num;
 *                   len = 512;
 * IMAGE_IF_NIBBLE : chunk_num = track_num;
 *                   len = 6656;
 *
 * start = Starting chunk number
 * count = Number of chunks to write
 */

int DSK_writeChunk(disk_struct *disk, byte *buffer, int start, int count)
{
	int	len,max;
	long	seek;

	if (disk->header.flags & IMAGE_FL_LOCKED) {
		return -1;
	}
	switch(disk->header.image_format) {
		case IMAGE_IF_RAW_DO:
		case IMAGE_IF_RAW_PO:	len = 512;
					max = disk->header.num_blocks;
					break;
		case IMAGE_IF_NIBBLE:	len = 6656;
					max = 35;
					break;
		default:		return -666;
	}
	if ((start < 0) || (start > max) || ((start+count-1) > max)) {
		return -2;
	}
	seek = (start * len) + disk->header.data_offset;
	if (fseek(disk->stream, seek, SEEK_SET)) {
		return -4;
	}
	if (fwrite(buffer, len, count, disk->stream) != count) {
		return -5;
	}
	return 0;
}

int DSK_loadUnivImage(char *pathname, disk_struct **disk, FILE *fp)
{
	int	err;

	rewind(fp);
	if ((err = DSK_readUnivImageHeader(fp, &((**disk).header)))) {
		fclose(fp);
		free(*disk);
		return err;
	}

	/* We already know the magic number matches, so start	*/
	/* by checking the header version number.		*/

	if ((**disk).header.version > 0x0100) {
		fclose(fp);
		free(*disk);
		return -2;
	}

	if ((**disk).read_only) (**disk).header.flags |= IMAGE_FL_LOCKED;

	(**disk).pathname = strdup(pathname);
	(**disk).stream = fp;
	(**disk).image_type = IMAGE_TYPE_2IMG;
	return 0;
}

int DSK_loadDosOrder(char *pathname, disk_struct **disk, FILE *fp)
{
	long	size;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	if (size & 0x1FF) {
		free(*disk);
		fclose(fp);
		return -666;
	}

	strncpy((**disk).header.magic, IMAGE_MAGIC, 4);
	strncpy((**disk).header.creator, IMAGE_CR_XGS, 4);

	(**disk).header.header_len = sizeof(image_header);
	(**disk).header.version = 0x0000;
	(**disk).header.image_format = IMAGE_IF_RAW_DO;
	(**disk).header.flags =  (**disk).read_only? IMAGE_FL_LOCKED : 0;
	(**disk).header.num_blocks = size / 512;
	(**disk).header.data_offset = 0;
	(**disk).header.data_len = size;
	(**disk).header.cmnt_offset = 0;
	(**disk).header.cmnt_len = 0;
	(**disk).header.creator_offset = 0;
	(**disk).header.creator_len = 0;

	(**disk).pathname = strdup(pathname);
	(**disk).stream = fp;
	(**disk).image_type = IMAGE_TYPE_RAW;
	return 0;
}

int DSK_loadProdosOrder(char *pathname, disk_struct **disk, FILE *fp)
{
	long	size;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	if (size & 0x1FF) {
		free(*disk);
		fclose(fp);
		return -666;
	}

	strncpy((**disk).header.magic, IMAGE_MAGIC, 4);
	strncpy((**disk).header.creator, IMAGE_CR_XGS, 4);

	(**disk).header.header_len = sizeof(image_header);
	(**disk).header.version = 0x0000;
	(**disk).header.image_format = IMAGE_IF_RAW_PO;
	(**disk).header.flags =  (**disk).read_only? IMAGE_FL_LOCKED : 0;
	(**disk).header.num_blocks = size / 512;
	(**disk).header.data_offset = 0;
	(**disk).header.data_len = size;
	(**disk).header.cmnt_offset = 0;
	(**disk).header.cmnt_len = 0;
	(**disk).header.creator_offset = 0;
	(**disk).header.creator_len = 0;

	(**disk).pathname = strdup(pathname);
	(**disk).stream = fp;
	(**disk).image_type = IMAGE_TYPE_RAW;
	return 0;
}

int DSK_loadNibblized(char *pathname, disk_struct **disk, FILE *fp)
{
	long	size;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);

	if (size % 6656) {
		free(*disk);
		fclose(fp);
		return -666;
	}

	(**disk).read_only = 1;		/* writing not yet supported */

	strncpy((**disk).header.magic, IMAGE_MAGIC, 4);
	strncpy((**disk).header.creator, IMAGE_CR_XGS, 4);

	(**disk).header.header_len = sizeof(image_header);
	(**disk).header.version = 0x0000;
	(**disk).header.image_format = IMAGE_IF_NIBBLE;
	(**disk).header.flags =  (**disk).read_only? IMAGE_FL_LOCKED : 0;
	(**disk).header.num_blocks = size / 832;
	(**disk).header.data_offset = 0;
	(**disk).header.data_len = size;
	(**disk).header.cmnt_offset = 0;
	(**disk).header.cmnt_len = 0;
	(**disk).header.creator_offset = 0;
	(**disk).header.creator_len = 0;

	(**disk).pathname = strdup(pathname);
	(**disk).stream = fp;
	(**disk).image_type = IMAGE_TYPE_RAW;
	return 0;
}

int DSK_loadDiskCopy(char *pathname, disk_struct **disk, FILE *fp)
{
	diskcopy_header	dcimage;
	int		err;

	rewind(fp);
	if (fread(&dcimage, sizeof(diskcopy_header), 1, fp) != 1) {
		err = errno;
		free(*disk);
		fclose(fp);
		return err;
	}

#ifndef WORDS_BIGENDIAN
	DSK_swapField(&(dcimage.dataSize), 4);
	DSK_swapField(&(dcimage.tagSize), 4);
	DSK_swapField(&(dcimage.dataChecksum), 4);
	DSK_swapField(&(dcimage.tagChecksum), 4);
	DSK_swapField(&(dcimage.private), 2);
#endif /* WORDS_BIGENDIAN */

	if (dcimage.private != 0x0100) {
		free(*disk);
		fclose(fp);
		return -666;
	}

	strncpy((**disk).header.magic, IMAGE_MAGIC, 4);
	strncpy((**disk).header.creator, IMAGE_CR_XGS, 4);

	(**disk).header.header_len = sizeof(image_header);
	(**disk).header.version = 0x0000;
	(**disk).header.image_format = IMAGE_IF_RAW_PO;
	(**disk).header.flags =  (**disk).read_only? IMAGE_FL_LOCKED : 0;
	(**disk).header.num_blocks = dcimage.dataSize / 512;
	(**disk).header.data_offset = sizeof(diskcopy_header);
	(**disk).header.data_len = dcimage.dataSize;
	(**disk).header.cmnt_offset = 0;
	(**disk).header.cmnt_len = 0;
	(**disk).header.creator_offset = 0;
	(**disk).header.creator_len = 0;

	(**disk).pathname = strdup(pathname);
	(**disk).stream = fp;
	(**disk).image_type = IMAGE_TYPE_DC42;
	return 0;
}

/*
 * Read a 2IMG header from a file, automatically byte-swapping the
 * fields after reading if necessary.
 *
 * This routine does not check the magic number, header length, or
 * header version fields. It just reads a raw header into memory.
 */

int DSK_readUnivImageHeader(FILE *fp, image_header *image)
{
	image_header	our_image;

	if (fread(&our_image, sizeof(image_header), 1, fp) != 1) {
		return errno;
	}
#ifdef WORDS_BIGENDIAN
	DSK_swapField(&(our_image.header_len),2);
	DSK_swapField(&(our_image.version),2);
	DSK_swapField(&(our_image.image_format),4);
	DSK_swapField(&(our_image.flags),4);
	DSK_swapField(&(our_image.num_blocks),4);
	DSK_swapField(&(our_image.data_offset),4);
	DSK_swapField(&(our_image.data_len),4);
	DSK_swapField(&(our_image.cmnt_offset),4);
	DSK_swapField(&(our_image.cmnt_len),4);
	DSK_swapField(&(our_image.creator_offset),4);
	DSK_swapField(&(our_image.creator_len),4);
#endif /* WORDS_BIGENDIAN */
	memcpy(image,&our_image,sizeof(image_header));
	return 0;
}

/*
 * Write a 2IMG header to a file, automatically byte-swapping hte
 * fields before writing if necessary.
 */

int DSK_writeUnivImageHeader(FILE *fp, image_header *image)
{
	image_header	our_image;

	memcpy(&our_image,image,sizeof(image_header));
#ifdef WORDS_BIGENDIAN
	DSK_swapField(&(our_image.header_len),2);
	DSK_swapField(&(our_image.version),2);
	DSK_swapField(&(our_image.image_format),4);
	DSK_swapField(&(our_image.flags),4);
	DSK_swapField(&(our_image.num_blocks),4);
	DSK_swapField(&(our_image.data_offset),4);
	DSK_swapField(&(our_image.data_len),4);
	DSK_swapField(&(our_image.cmnt_offset),4);
	DSK_swapField(&(our_image.cmnt_len),4);
	DSK_swapField(&(our_image.creator_offset),4);
	DSK_swapField(&(our_image.creator_len),4);
#endif /* WORDS_BIGENDIAN */
	fwrite(&our_image, sizeof(image_header), 1, fp);
	return errno;
}
