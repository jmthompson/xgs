## Process this file with automake to produce Makefile.in

AM_CFLAGS = -D DATA_DIR=\"$(datadir)\" -I ../include -I ../m65816 @SDL_CFLAGS@
AM_LDFLAGS = @SDL_LIBS@

bin_PROGRAMS = xgs imgconv imgutil
imgconv_SOURCES = disks.c imgconv.c
imgutil_SOURCES = disks.c imgutil.c
xgs_SOURCES = adb.c clock.c disks.c fontutil.c global.c hardware.c hires.c iwm.c lores.c memory.c rw.c smtport.c scheduler.c sound.c super.c table.c text.c video.c

xgs_LDADD = ../m65816/libm65816.la
xgs_DEPENDNCIES = ../m65816/libm65816.la
