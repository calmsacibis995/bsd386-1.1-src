# Generated automatically from Makefile.in by configure.
# Makefile for GNU DIFF
# Copyright (C) 1988, 1989, 1991, 1992, 1993 Free Software Foundation, Inc.
#
# This file is part of GNU DIFF.
#
# GNU DIFF is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GNU DIFF is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GNU DIFF; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

#### Start of system configuration section. ####

srcdir = .
VPATH = .

CC = gcc
INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644
MAKEINFO = makeinfo

CFLAGS = -g
LDFLAGS = -g
LIBS = 

# Some System V machines do not come with libPW.
# If this is true for you, use the GNU alloca.o here.
ALLOCA = 

prefix = /usr/local
exec_prefix = $(prefix)

# Prefix for each installed program, normally empty or `g'.
binprefix =

bindir = $(exec_prefix)/bin

infodir = $(prefix)/info

#### End of system configuration section. ####

SHELL = /bin/sh

# The source files for all of the programs.
srcs=diff.c analyze.c cmpbuf.c cmpbuf.h io.c context.c ed.c normal.c ifdef.c \
	util.c dir.c \
	version.c diff.h regex.c regex.h side.c system.h \
	diff3.c sdiff.c cmp.c error.c xmalloc.c getopt.c getopt1.c getopt.h \
	fnmatch.c fnmatch.h alloca.c
distfiles = $(srcs) README INSTALL NEWS diagmeet.note Makefile.in \
	config.h.in configure configure.in COPYING ChangeLog \
	diff.texi diff.info* texinfo.tex \
	mkinstalldirs

all: diff diff3 sdiff cmp diff.info

COMPILE = $(CC) -c -DHAVE_CONFIG_H $(CPPFLAGS) -I. -I$(srcdir) $(CFLAGS)

.c.o:
	$(COMPILE) $<

diff_o = diff.o analyze.o cmpbuf.o dir.o io.o util.o \
	context.o ed.o ifdef.o normal.o side.o \
	fnmatch.o getopt.o getopt1.o regex.o version.o $(ALLOCA)
diff: $(diff_o)
	$(CC) -o $@ $(LDFLAGS) $(diff_o) $(LIBS)

diff3_o = diff3.o getopt.o getopt1.o version.o
diff3: $(diff3_o)
	$(CC) -o $@ $(LDFLAGS) $(diff3_o) $(LIBS)

sdiff_o = sdiff.o getopt.o getopt1.o version.o
sdiff: $(sdiff_o)
	$(CC) -o $@ $(LDFLAGS) $(sdiff_o) $(LIBS)

cmp_o = cmp.o cmpbuf.o error.o getopt.o getopt1.o xmalloc.o version.o
cmp: $(cmp_o)
	$(CC) -o $@ $(LDFLAGS) $(cmp_o) $(LIBS)

diff.info: diff.texi
	$(MAKEINFO) $(srcdir)/diff.texi

$(diff_o): diff.h system.h
cmp.o diff3.o sdiff.o: system.h
context.o diff.o regex.o: regex.h
cmp.o diff.o diff3.o sdiff.o getopt.o getopt1.o: getopt.h
diff.o fnmatch.o: fnmatch.h
analyze.o cmpbuf.o cmp.o: cmpbuf.h

diff3.o: diff3.c
	$(COMPILE) -DDIFF_PROGRAM=\"$(bindir)/$(binprefix)diff\" $(srcdir)/diff3.c

sdiff.o: sdiff.c
	$(COMPILE) -DDIFF_PROGRAM=\"$(bindir)/$(binprefix)diff\" $(srcdir)/sdiff.c

TAGS: $(srcs)
	etags $(srcs)

clean:
	rm -f *.o diff diff3 sdiff cmp core

mostlyclean: clean

distclean: clean
	rm -f Makefile config.status

realclean: distclean
	rm -f TAGS *.info*

install: all installdirs
	for p in diff diff3 sdiff cmp; \
	do $(INSTALL_PROGRAM) $$p $(bindir)/$(binprefix)$$p; done
	cd $(srcdir); for f in diff.info*; \
	do $(INSTALL_DATA) $$f $(infodir)/$$f; done

installdirs:
	$(SHELL) ${srcdir}/mkinstalldirs $(bindir) $(infodir)

uninstall:
	for p in diff diff3 sdiff cmp; \
	do rm -f $(bindir)/$(binprefix)$$p; done
	rm -f $(infodir)/diff.info*

Makefile: Makefile.in config.status
	$(SHELL) config.status
config.status: configure
	$(SHELL) $(srcdir)/configure --no-create
configure: configure.in
	cd $(srcdir); autoconf
config.h.in: configure.in
	cd $(srcdir); autoheader

dist: $(distfiles)
	echo diffutils-`sed -e '/version_string/!d' -e 's/[^0-9]*\([0-9a-z.]*\).*/\1/' -e q version.c` > .fname
	rm -rf `cat .fname`
	mkdir `cat .fname`
	-ln $(distfiles) `cat .fname`
	for file in $(distfiles); do \
	  [ -r `cat .fname`/$$file ] || cp -p $$file `cat .fname` || exit; \
	done
	tar -chf - `cat .fname` | gzip >`cat .fname`.tar.gz
	rm -rf `cat .fname` .fname

# Prevent GNU make v3 from overflowing arg limit on SysV.
.NOEXPORT:
