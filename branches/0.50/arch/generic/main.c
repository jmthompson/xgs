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
 * File: arch/generic/main.c
 *
 * Generic main (startup) routine. This is simple a C main() function
 * that calls EMUL_init() and then EMUL_run(). The latter will never
 * return.
 */

#include "xgs.h"

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "emul.h"
#include "main.h"

void main(int argc, char *argv[])
{
	EMUL_init(argc, argv);
	EMUL_run();
}

void APP_shutdown()
{
	exit(0);
}

long EMUL_getCurrentTime()
{
#ifdef HAVE_GETTIMEOFDAY
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return (long) ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
#else
	return 0L;
#endif
}

char *EMUL_expandPath(const char *path)
{
	if ((path[0] == '.') || (path[0] == '/')) {
		strcpy(emul_path,path);
	} else {
		sprintf(emul_path,"%s/%s",XGS_DIR,path);
	}
	return emul_path;
}
