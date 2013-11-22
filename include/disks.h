#ifndef _GSDISKS_H_
#define _GSDISKS_H_

/* The universal Apple II Disk Image Header */

typedef struct {
	char	magic[4];	/* "2IMG"                                  */
	char	creator[4];	/* Creator signature                       */
	word16	header_len;	/* Length of header in bytes            k  */
	word16	version;	/* Image version                           */
	word32	image_format;	/* Image data format (see below)           */
	word32	flags;		/* Format flags (see below)                */
	word32	num_blocks;	/* Number of 512-byte blocks in this image */
	word32	data_offset;	/* File offset to start of data            */
	word32	data_len;	/* Length of data in bytes                 */
	word32	cmnt_offset;	/* File offset to start of comment         */
	word32	cmnt_len;	/* Length of comment in bytes              */
	word32	creator_offset;	/* File offset to start of creator data    */
	word32	creator_len;	/* Length of creator data in bytes         */
	word32	spare[4];	/* Spare words (pads header to 64 bytes)   */
} image_header;

/* Image header magic number */

#define IMAGE_MAGIC	"2IMG"

/* Image creator types */

#define IMAGE_CR_XGS	"XGS!"

/* Image data format numbers */

#define IMAGE_IF_RAW_DO	0
#define IMAGE_IF_RAW_PO	1
#define IMAGE_IF_NIBBLE	2

/* Image flags */

#define IMAGE_FL_LOCKED	0x80000000

/* In-memory-only structure containing the universal header, the path	*/
/* to the image, and the file pointer for the open image file.		*/

typedef struct {
	image_header	header;
	int		image_type;
	int		read_only;
	char		*pathname;
	FILE		*stream;
} disk_struct;

/* Image types */

#define	IMAGE_TYPE_2IMG	0	/* 2IMG Universal Format	*/
#define IMAGE_TYPE_RAW	1	/* Raw (headerless) image, with	*/
				/* data format depending on the	*/
				/* format type in the in-memory	*/
				/* image_header structure.	*/
#define IMAGE_TYPE_DC42	2	/* DiskCopy 4.2			*/

/* The DiskCopy 4.2 header structure */

typedef struct {
	char	diskName[64];	/* Disk name, as a Pascal string	*/
	word32	dataSize;	/* Size of block data in bytes		*/
	word32	tagSize;	/* Size of tag information in bytes	*/
	word32	dataChecksum;	/* Checksum of block data		*/
	word32	tagChecksum;	/* Checksum of tag data			*/
	char	diskFormat;	/* 0 = 400K, 1 = 800K, 2 = 720K,	*/
				/* 3 = 1440K				*/
	char	formatByte;	/* 0x12 = 400K, 0x22 = >400K Mac,	*/
				/* 0x24 = 800k Apple II			*/
	word16	private;	/* reserved				*/
} diskcopy_header;

void	DSK_swapField(void *, int);

int	DSK_openImage(char *, disk_struct **);
int	DSK_closeImage(disk_struct *);
int	DSK_updateImage(disk_struct *);
int	DSK_readChunk(disk_struct *, byte *, int, int);
int	DSK_writeChunk(disk_struct *, byte *, int, int);

int	DSK_loadUnivImage(char *, disk_struct **, FILE *);
int	DSK_loadDosOrder(char *, disk_struct **, FILE *);
int	DSK_loadProdosOrder(char *, disk_struct **, FILE *);
int	DSK_loadNibblized(char *, disk_struct **, FILE *);
int	DSK_loadDiskCopy(char *, disk_struct **, FILE *);

int	DSK_readUnivImageHeader(FILE *, image_header *);
int	DSK_writeUnivImageHeader(FILE *, image_header *);

#endif
