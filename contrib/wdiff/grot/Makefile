# Generated automatically from Makefile.in by configure.
# Makefile for word diff.
# Copyright (C) 1992 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = .
VPATH = .
U = 

CC = gcc -O
DIFF = diff
PAGER = less
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
MAKEINFO = makeinfo
TEXI2DVI = texi2dvi

DEFS =  -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DHAVE_TPUTS=1
PROGDEFS = -DDIFF_PROGRAM="\"$(DIFF)\"" -DPAGER_PROGRAM="\"$(PAGER)\""
CFLAGS = -g
LDFLAGS = -g
LIBS =  -ltermcap
LIBOBJS = 

prefix = /usr/local
exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
infodir = $(prefix)/info
mandir = $(prefix)/man/man1
manext = .1

# You might add cbars to PROGS if you feel like it.
PROGS = wdiff

#### End of system configuration section. ####

.c.o:
	$(CC) -c -I$(srcdir) $(CFLAGS) $(DEFS) $<

HEADERS = getopt.h pathmax.h
SOURCES = wdiff.c readpipe.c writepipe.c \
error.c getopt.c getopt1.c alloca.c strstr.c
OBJECTS = $Uwdiff.o readpipe.o writepipe.o \
error.o getopt.o getopt1.o $(ALLOCA) $(LIBOBJS)
DISTFILES = README NEWS TODO THANKS COPYING INSTALL ChangeLog \
configure.in aclocal.m4 Makefile.in ansi2knr.c \
$(HEADERS) $(SOURCES) cbars.sh c-boxes.el check_it \
wdiff.texi gpl.texinfo texinfo.tex wdiff.1 \
configure wdiff.info

all: $(PROGS)

wdiff: $(OBJECTS)
	$(CC) $(LDFLAGS) -o wdiff $(OBJECTS) $(LIBS)

$Uwdiff.o: $Uwdiff.c
	$(CC) -c -I$(srcdir) $(CFLAGS) $(DEFS) $(PROGDEFS) $<
	
$Uwdiff.o: getopt.h pathmax.h
getopt.o getopt1.o: getopt.h

ansi2knr: ansi2knr.c
	$(CC) $(CFLAGS) $(DEFS) -o ansi2knr $(srcdir)/ansi2knr.c
_wdiff.c: wdiff.c ansi2knr
	./ansi2knr $(srcdir)/wdiff.c > _wdiff.c

cbars: cbars.sh
	sed -e 's|<bindir>|$(bindir)|' cbars.sh > cbars

info: wdiff.info

wdiff.info: wdiff.texi
	$(MAKEINFO) -o wdiff.info $(srcdir)/wdiff.texi

dvi: wdiff.dvi

wdiff.dvi: wdiff.texi
	$(TEXI2DVI) $(srcdir)/wdiff.texi

check: wdiff
	$(srcdir)/check_it

install: wdiff wdiff.info
	for file in $(PROGS); do \
	  $(INSTALL_PROGRAM) $$file $(bindir)/$$file; \
	done
	$(INSTALL_DATA) $(srcdir)/wdiff.info $(infodir)/wdiff.info
	-$(INSTALL_DATA) $(srcdir)/wdiff.1 $(mandir)/wdiff$(manext)

uninstall:
	for file in $(PROGS); do \
	  rm -f $(bindir)/$$file; \
	done
	rm -f $(infodir)/wdiff.info $(mandir)/wdiff$(manext)

tags: TAGS

TAGS: $(HEADERS) $(SOURCES)
	etags $(HEADERS) $(SOURCES)

texclean:
	rm -f *.aux *.cp *.dvi *.fn *.ky *.log *.pg *.toc *.tp *.vr

mostlyclean: texclean
	rm -f *.o *.elc core test.out *~

clean: mostlyclean
	rm -f $(PROGS) _*.c

distclean: clean
	rm -f Makefile config.status ansi2knr TAGS

realclean: distclean
	rm -f wdiff.info

dist: $(DISTFILES)
	echo `pwd` | sed 's|.*/||' > .fname
	rm -rf `cat .fname`
	mkdir `cat .fname`
	ln $(DISTFILES) `cat .fname`
	chmod -R a+r `cat .fname`
	tar chZf `cat .fname`.tar.Z `cat .fname`
	rm -rf `cat .fname` .fname

Makefile: Makefile.in config.status
	./config.status

config.status: configure
	$(srcdir)/configure --no-create

configure: configure.in aclocal.m4
	cd $(srcdir); autoconf

# Prevent GNU make v3 from overflowing arg limit on SysV.
.NOEXPORT:
