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
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <unistd.h>

#include "app.h"
#include "emul.h"

void main(int argc, char *argv[])
{
    size_t len = strlen(XGS_DATA_DIR) + strlen(DATA_DIR) + 2;

    emul_path = malloc(len);

    if (emul_path == NULL) {
        perror("Unable to allocate memory for emul_path");

        exit(-1);
    }

    snprintf(emul_path, len, "%s/%s", DATA_DIR, XGS_DATA_DIR);

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
    int len = strlen(path) + strlen(emul_path) + 2;
    char *output = malloc(len);

    snprintf(output, len, "%s/%s", emul_path, path);

    return output;
}
