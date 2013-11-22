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
 * File: fontutil.c
 *
 * This is the all-in-one XGS font manipulation utility.
 *
 */

#include "xgs.h"

#include <stdio.h>
#include <string.h>

/* Function prototypes */

int	buildFont40(char *, char *);
int	buildFont80(char *, char *);
int	showFont40(char *);
int	showFont80(char *);

int buildFont40(char *source, char *dest)
{
	FILE	*fp1,*fp2;
	int	i,j,k;
	byte	buffer[256];
	byte	line[14];

	if ((fp1 = fopen(source,"r")) == NULL) {
		perror("Can't open source file");
		return 1;
	}
	if ((fp2 = fopen(dest,"wb")) == NULL) {
		perror("Can't open destination file");
		fclose(fp1);
		return 1;
	}
	for (i = 0 ; i < 512 ; i++) {
		fgets(buffer,sizeof(buffer), fp1);
		fgets(buffer,sizeof(buffer), fp1);
		for (j = 0 ; j < 16 ; j++) {
			fgets(buffer,sizeof(buffer), fp1);
			for (k = 0 ; k < 14 ; k++) {
				if (buffer[k] == '*') {
					line[k] = 254;
				} else {
					line[k] = 253;
				}
			}
			fwrite(line,1,14,fp2);
		}
		fgets(buffer,sizeof(buffer), fp1);
	}
	fclose(fp1);
	fclose(fp2);
	return 0;
}

int buildFont80(char *source, char *dest)
{
	FILE	*fp1,*fp2;
	int	i,j,k;
	byte	buffer[256];
	byte	line[7];

	if ((fp1 = fopen(source,"r")) == NULL) {
		perror("Can't open source file");
		return 1;
	}
	if ((fp2 = fopen(dest,"wb")) == NULL) {
		perror("Can't open destination file");
		fclose(fp1);
		return 1;
	}
	for (i = 0 ; i < 512 ; i++) {
		fgets(buffer,sizeof(buffer), fp1);
		fgets(buffer,sizeof(buffer), fp1);
		for (j = 0 ; j < 16 ; j++) {
			fgets(buffer,sizeof(buffer), fp1);
			for (k = 0 ; k < 7 ; k++) {
				if (buffer[k] == '*') {
					line[k] = 254;
				} else {
					line[k] = 253;
				}
			}
			fwrite(line,1,7,fp2);
		}
		fgets(buffer,sizeof(buffer), fp1);
	}
	fclose(fp1);
	fclose(fp2);
	return 0;
}

int showFont40(char *filename)
{
	FILE	*fp;
	int	i,j,k;
	byte	line[15];

	line[14] = 0;
	if ((fp = fopen("xgs40.fnt","rb")) == NULL) {
		perror("Can't open source file");
		return 1;
	}
	for (i = 0 ; i < 512 ; i++) {
		printf("Character #$%02X\n\n",i);
		for (j = 0 ; j < 16 ; j++) {
			fread(line,1,14,fp);
			for (k = 0 ; k < 14 ; k++) {
				line[k] = (line[k] == 254) ? '*' : '.';
			}
			printf("%s\n",line);
		}
		printf("\n");
	}
	fclose(fp);
	return 0;
}

int showFont80(char *filename)
{
	FILE	*fp;
	int	i,j,k;
	byte	line[8];

	line[7] = 0;
	if ((fp = fopen("xgs40.fnt","rb")) == NULL) {
		perror("Can't open source file");
		return 1;
	}
	for (i = 0 ; i < 512 ; i++) {
		printf("Character #$%02X\n\n",i);
		for (j = 0 ; j < 16 ; j++) {
			fread(line,1,7,fp);
			for (k = 0 ; k < 7 ; k++) {
				line[k] = (line[k] == 254) ? '*' : '.';
			}
			printf("%s\n",line);
		}
		printf("\n");
	}
	fclose(fp);
	return 0;
}
