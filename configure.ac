#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xgs], [0.60], [joshua.thompson@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR

# Checks for libraries.
LT_INIT

# Checks for header files.
AC_PATH_X
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h sys/file.h sys/ioctl.h sys/time.h X11/extensions/XShm.h],,, [ AC_INCLUDES_DEFAULT ])

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

dnl Check for optional features
AC_ARG_WITH(debug,
    AS_HELP_STRING([--with-debug],      [enable debugging support]),
    AC_DEFINE(DEBUG,           [], [Define to enable support for 65816 debugging])
)
AC_ARG_WITH(oss,
    AS_HELP_STRING([--with-oss], [enable support for the OSS driver]),
    AC_DEFINE(WITH_OSS, [], [Define to enable OSS support])
)
AC_ARG_WITH(pulseaudio,
    AS_HELP_STRING([--with-pulseaudio], [enable support for the PulseAudio driver]),
    AC_DEFINE(WITH_PULSEAUDIO, [], [Define to enable PulseAudio support])
)
AC_ARG_WITH(sparcaudio,
    AS_HELP_STRING([--with-sparcaudio], [enable support for the Sparc audio driver]),
    AC_DEFINE(WITH_SPARCAUDIO, [], [Define to enable Sparc audio support])
)

AS_IF([ test "x$no_x" != xyes ],
    AC_DEFINE(WITH_X11, [], [Define to enable X11 video support])

    AC_CHECK_LIB([Xext],
        [ XShmAttach ],
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
AM_CONDITIONAL([WITH_OSS],        [test "x$with_oss" = xyes])
AM_CONDITIONAL([WITH_POSIX],      [test "x$with_posix" = xyes])
AM_CONDITIONAL([WITH_PULSEAUDIO], [test "x$with_pulseaudio" = xyes])
AM_CONDITIONAL([WITH_SPARCAUDIO], [test "x$with_sparcaudio" = xyes])
AM_CONDITIONAL([WITH_X11],        [test "x$no_x" != xyes])

AC_CONFIG_FILES([Makefile m65816/Makefile src/Makefile])
AC_OUTPUT
