#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([xgs], [0.60], [joshua.thompson@gmail.com])
AC_INIT([FULL-PACKAGE-NAME], [VERSION], [BUG-REPORT-ADDRESS])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_PATH_X
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h stdlib.h string.h sys/file.h sys/ioctl.h sys/time.h unistd.h])

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
AC_ARG_WITH(pulseaudio, [  --with-pulseaudio       enable support for the PulseAudio driver], AC_DEFINE(WITH_PULSEAUDIO))
AC_ARG_WITH(mit-shm,    [  --with-mit-shm          enable MIT Shared Memory extension support], AC_DEFINE(WITH_MITSHM))
AC_ARG_WITH(debug,      [  --with-debug            enable debugging support], AC_DEFINE(DEBUG))

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
