#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xgs], [0.60], [joshua.thompson@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])
AC_CANONICAL_HOST

# Checks for programs.
AC_PROG_CC
AM_PROG_AR

# Checks for libraries.
LT_INIT

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h sys/file.h sys/ioctl.h sys/time.h],,, [ AC_INCLUDES_DEFAULT ])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_TIME
AC_C_BIGENDIAN
AC_C_CONST
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_CHECK_SIZEOF(int,4)
AC_CHECK_SIZEOF(short,2)
AC_CHECK_SIZEOF(long,4)

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gettimeofday memmove memset random lrand48 select strdup strrchr])

dnl Command-line arguments
AC_ARG_WITH(debug,
    AS_HELP_STRING([--with-debug],      [enable debugging support]),
    AC_DEFINE(DEBUG, [], [Define to enable support for 65816 debugging])
)

dnl Check for SDL2
AM_PATH_SDL2(2.0.0,, [ AC_MSG_ERROR([sdl2-config not found. SDL2 is required in order to build XGS]) ])

dnl Check for high-resolution POSIX timers
temp_LIBS="$LIBS"
LIBS="-lrt $LIBS"

AC_MSG_CHECKING([for high-res POSIX timers])
AC_RUN_IFELSE(
    [
        AC_LANG_SOURCE([[
#include <time.h>

int main(int argc, char **argv) {
    struct timespec ts;

    if (clock_getres(CLOCK_MONOTONIC, &ts) < 0) {
        return 1;
    }

    if ((ts.tv_sec == 0) && (ts.tv_nsec < 30000)) {
        return 0;
    }
    else {
        return 1;
    }
}
        ]])
    ],
    [
        have_hrtimers=yes
    ],
    [
        have_hrtimers=no
        AC_MSG_ERROR([ XGS requires support for POSIX timers with a minimum resolution of 30 microseconds ])
    ]
)

AC_MSG_RESULT([$have_hrtimers])
LIBS="$temp_LIBS"

dnl Check for the timerfd_* functions
AS_IF(
    [ test "$have_hrtimers" = "yes" ],
    [
        AC_CHECK_LIB([c],
            [timerfd_create],
            [
                AC_DEFINE([HAVE_TIMERFD], [], [Define if your system supports timerfd_* syscalls ])
            ],
            [
            ]
        )
    ]
)

AC_CONFIG_FILES([Makefile m65816/Makefile src/Makefile])
AC_OUTPUT
