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
AC_CHECK_FUNCS([bzero gettimeofday memmove memset random lrand48 select strdup strrchr])

dnl Command-line arguments
AC_ARG_WITH(debug,
    AS_HELP_STRING([--with-debug],      [enable debugging support]),
    AC_DEFINE(DEBUG,           [], [Define to enable support for 65816 debugging])
)
AC_ARG_WITH(sound,
    AS_HELP_STRING([--with-sound=DRIVER], [Select sound driver (sun, oss, or pulseaudio)]),
    [],
    [with_sound=check]
)

dnl Check for OSS
OSS_LIBS=
have_oss=no

AC_CHECK_HEADERS([ sys/soundcard.h ],[ have_oss=yes ])

case "$host_os" in
    openbsd*)
        OSS_LIBS="-lossaudio"
        ;;
esac

AC_SUBST(OSS_LIBS)

dnl Check for PulseAudio
AC_CHECK_LIB([pulse-simple],
    [pa_simple_new],
    [
        have_pulseaudio=yes
    ],
    [
        have_pulseaudio=no
    ]
)

dnl Check for Sun Audio
AC_MSG_CHECKING([for Sun audio support])

AC_COMPILE_IFELSE(
    [
        AC_LANG_SOURCE([[
#include <sys/types.h>
#include <sys/audioio.h>
}
        ]])
    ],
    [
        have_sunaudio=yes
    ],
    [
        have_sunaudio=no
    ]
)
AC_MSG_RESULT($have_sunaudio)

case "$with_sound" in
    check)
        AS_IF(
            [ test "x$have_sunaudio" = "xyes" ],
            [ with_sound=sun ]
        )
        AS_IF(
            [ test "x$have_oss" = "xyes" ],
            [ with_sound=oss ]
        )
        AS_IF(
            [ test "x$have_pulseaudio" = "xyes" ],
            [ with_sound=pulseaudio ]
        )
        dnl in case nothing was chosen
        AS_IF(
            [ test "$with_sound" = "check" ],
            [
                AC_MSG_ERROR([Unable to choose a sound driver. Try choosing one manually with --with-sound=DRIVER])
            ]
        )
        ;;
    oss)
        AS_IF(
            [ test "x$have_oss" != "xyes" ],
            [
                AC_MSG_ERROR([The OSS sound driver was selected, but it is not available on this platform])
            ]
        )
        ;;
    pulseaudio)
        AS_IF(
            [ test "x$have_pulseaudio" != "xyes" ],
            [
                AC_MSG_ERROR([The PulseAudio driver selected, but it is not available on this platform])
            ]
        )
        ;;
    sun)
        AS_IF(
            [ test "x$have_sunaudio" != "xyes" ],
            [
                AC_MSG_ERROR([The Sun audio driver selected, but it is not available on this platform])
            ]
        )
        ;;
    *)
        AC_MSG_ERROR([Unknown audio driver: $with_sound])
        ;;
esac

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

    if ((ts.tv_sec == 0) && (ts.tv_nsec < 32)) {
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
    ]
)

AC_MSG_RESULT([$have_hrtimers])
LIBS="$temp_LIBS"

dnl Check for the timerfd_* functions
AS_IF(
    [ test "x$have_hrtimers" = "xyes" ],
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

dnl Check for the MIT-SHM extension
AS_IF(
    [ test "x$no_x" != xyes ],
    AC_DEFINE(WITH_X11, [], [Define to enable X11 video support])

    AC_CHECK_LIB([Xext],
        [XShmAttach],
        [
            AC_DEFINE([HAVE_MITSHM], [], [Define if your system supports the MIT-SHM extension])
            AM_CONDITIONAL([HAVE_MITSHM], [true])
        ],
        [
            AM_CONDITIONAL([HAVE_MITSHM], [false])
        ]
    )
)

with_posix=yes

dnl Automake conditonals
AM_CONDITIONAL([WITH_OSS],        [test "$with_sound" = oss])
AM_CONDITIONAL([WITH_PULSEAUDIO], [test "$with_sound" = pulseaudio])
AM_CONDITIONAL([WITH_SUNAUDIO],   [test "$with_sound" = sun])
AM_CONDITIONAL([WITH_POSIX],      [test "x$with_posix" = xyes])
AM_CONDITIONAL([WITH_X11],        [test "x$no_x" != xyes])

AC_CONFIG_FILES([Makefile m65816/Makefile src/Makefile])
AC_OUTPUT

echo
echo "Configured XGS to build with the following options:"
echo 
echo "Sound driver: $with_sound"
echo
echo "Run \"make\" and \"make install\" to compile and install."
echo
