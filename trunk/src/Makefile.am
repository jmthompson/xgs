## Process this file with automake to produce Makefile.in

AM_CFLAGS = -D DATA_DIR=\"$(datadir)\" -I ../include -I ../m65816
AM_LDFLAGS = -lrt

bin_PROGRAMS = xgs imgconv imgutil
imgconv_SOURCES = disks.c imgconv.c
imgutil_SOURCES = disks.c imgutil.c
xgs_SOURCES = adb.c clock.c disks.c emul.c fontutil.c hires.c iwm.c lores.c memory.c rw.c smtport.c sound.c super.c table.c text.c video.c

if WITH_POSIX
xgs_SOURCES += app-posix.c
endif

if WITH_PULSEAUDIO
xgs_SOURCES += sound-pulseaudio.c
AM_LDFLAGS  += -lpulse-simple
endif

if WITH_OSS
xgs_SOURCES += sound-oss.c
AM_LDFLAGS  += @OSS_LIBS@
endif

if WITH_SUNAUDIO
xgs_SOURCES += sound-sparc.c
endif

if WITH_X11
xgs_SOURCES += input-x11.c video-x11.c
AM_CFLAGS   += $(x_includes)
AM_LDFLAGS  += $(x_libraries) -lX11
endif

if HAVE_MITSHM
AM_LDFLAGS  += $(x_libraries) -lXext
endif

xgs_LDADD = ../m65816/libm65816.la
xgs_DEPENDNCIES = ../m65816/libm65816.la
