SUBDIRS = xkbcomp

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS) $(XMALLOC_ZERO_CFLAGS)

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_LIBADD = xkbcomp/libxkbcomp.la
libxkbcommon_la_LDFLAGS = -no-undefined
libxkbcommon_la_SOURCES = \
	XKBcommonint.h \
	alloc.c \
	atom.c \
	galloc.c \
	geom.c \
	imKStoUCS.c \
	keysym.c \
	malloc.c \
	maprules.c \
	misc.c \
	text.c \
	xkb.c \
	xkballoc.h \
	xkbgeom.h \
	xkbmisc.h \
	xkbrules.h

BUILT_SOURCES = ks_tables.h
CLEANFILES = $(BUILT_SOURCES)

ks_tables.h: $(top_builddir)/makekeys/makekeys$(EXEEXT) $(KEYSYMDEF_H) $(XF86KEYSYM_H)
	$(AM_V_GEN)cat $(KEYSYMDEF_H) $(XF86KEYSYM_H) | $(top_builddir)/makekeys/makekeys$(EXEEXT) > $@
