### Makefile.am
include $(top_builddir)/version.mk

AM_CFLAGS = $(EXTRA_CFLAGS)
AM_CPPFLAGS = -D_POSIX_C_SOURCE=201001L -D_XOPEN_SOURCE=700 -D_BSD_SOURCE
AM_CPPFLAGS += -DHAVE_VERSION_H
AM_LDFLAGS = $(XCCLDFLAGS)

bin_PROGRAMS =
noinst_PROGRAMS =
noinst_LIBRARIES =
noinst_HEADERS =
BUILT_SOURCES =
EXTRA_DIST = $(BUILT_SOURCES)
CLEANFILES = 
SUFFIXES =

CLEANFILES += version.c
EXTRA_DIST += version.c.in

noinst_LIBRARIES += libgeo2t.a
libgeo2t_a_SOURCES =
libgeo2t_a_SOURCES += instant.c instant.h
libgeo2t_a_SOURCES += range.c range.h
libgeo2t_a_SOURCES += dt-strpf.c dt-strpf.h
libgeo2t_a_SOURCES += boobs.h
libgeo2t_a_SOURCES += version.c version.h

bin_PROGRAMS += geo2t
geo2t_SOURCES = geo2t.c geo2t.yuck
geo2t_CPPFLAGS = $(AM_CPPFLAGS)
geo2t_CPPFLAGS += -D_GNU_SOURCE
geo2t_LDADD = libgeo2t.a -lm
BUILT_SOURCES += geo2t.yucc

noinst_PROGRAMS += tbox-norm
tbox_norm_SOURCES = tbox-norm.c tbox-norm.yuck
tbox_norm_CPPFLAGS = $(AM_CPPFLAGS)
tbox_norm_CPPFLAGS += -D_GNU_SOURCE
tbox_norm_LDADD = libgeo2t.a -lm
BUILT_SOURCES += tbox-norm.yucc


## version rules
version.c: version.c.in $(top_builddir)/.version
	$(AM_V_GEN) PATH="$(top_builddir)/build-aux:$${PATH}" \
		yuck$(EXEEXT) scmver --ignore-noscm --force -o $@ \
			--use-reference --reference $(top_builddir)/.version $<

## yuck rule
SUFFIXES += .yuck
SUFFIXES += .yucc
.yuck.yucc:
	$(AM_V_GEN) PATH=$(top_builddir)/build-aux:"$${PATH}" \
		yuck$(EXEEXT) gen -o $@ $<

## Create preprocessor output (debugging purposes only)
.c.i:
	$(COMPILE) -E -o $@ $<

## Create assembler output (debugging purposes only)
.c.s:
	$(COMPILE) -S -c $(AM_CFLAGS) $<
