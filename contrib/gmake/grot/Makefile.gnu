# Generated automatically from Makefile.in by configure.
# NOTE: If you have no `make' program at all to process this makefile, run
# `build.sh' instead.
#
# Copyright (C) 1988, 1989, 1991, 1992, 1993 Free Software Foundation, Inc.
# This file is part of GNU Make.
# 
# GNU Make is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# GNU Make is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Make; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

#
#	Makefile for GNU Make
#

# Ultrix 2.2 make doesn't expand the value of VPATH.
VPATH = .
srcdir = $(VPATH)

CC = gcc

CFLAGS = -g
LDFLAGS = -g

# How to invoke ranlib.  This is only used by the `glob' subdirectory.
RANLIB = ranlib

# Define these for your system as follows:
#	-DNO_ARCHIVES		To disable `ar' archive support.
#	-DNO_FLOAT		To avoid using floating-point numbers.
#	-DENUM_BITFIELDS	If the compiler isn't GCC but groks enum foo:2.
#				Some compilers apparently accept this
#				without complaint but produce losing code,
#				so beware.
# NeXT 1.0a uses an old version of GCC, which required -D__inline=inline.
# See also `config.h'.
defines = -DHAVE_CONFIG_H -DLIBDIR=\"$(libdir)\" -DINCLUDEDIR=\"$(includedir)\"

# Which flavor of remote job execution support to use.
# The code is found in `remote-$(REMOTE).c'.
REMOTE = stub

# If you are using the GNU C library, or have the GNU getopt functions in
# your C library, you can comment these out.
GETOPT = getopt.o getopt1.o
GETOPT_SRC = $(srcdir)/getopt.c $(srcdir)/getopt1.c $(srcdir)/getopt.h

# If you are using the GNU C library, or have the GNU glob functions in
# your C library, you can comment this out.  GNU make uses special hooks
# into the glob functions to be more efficient (by using make's directory
# cache for globbing), so you must use the GNU functions even if your
# system's C library has the 1003.2 glob functions already.  Also, the glob
# functions in the AIX and HPUX C libraries are said to be buggy.
GLOB = glob/libglob.a

# If your system doesn't have alloca, or the one provided is bad, define this.
ALLOCA = 
ALLOCA_SRC = $(srcdir)/alloca.c

# If your system needs extra libraries loaded in, define them here.
# System V probably need -lPW for alloca.  HP-UX 7.0's alloca in
# libPW.a is broken on HP9000s300 and HP9000s400 machines.  Use
# alloca.c instead on those machines.
LOADLIBES =  -lutil -lkvm

# Any extra object files your system needs.
extras = 

# Common prefix for machine-independent installed files.
prefix = /usr/local
# Common prefix for machine-dependent installed files.
exec_prefix = $(prefix)

# Name under which to install GNU make.
instname = make
# Directory to install `make' in.
bindir = $(exec_prefix)/bin
# Directory to find libraries in for `-lXXX'.
libdir = $(exec_prefix)/lib
# Directory to search by default for included makefiles.
includedir = $(prefix)/include
# Directory to install the Info files in.
infodir = $(prefix)/info
# Directory to install the man page in.
mandir = $(prefix)/man/man$(manext)
# Number to put on the man page filename.
manext = 1

# Whether or not make needs to be installed setgid.
# The value should be either `true' or `false'.
# On many systems, the getloadavg function (used to implement the `-l'
# switch) will not work unless make is installed setgid kmem.
install_setgid = false
# Install make setgid to this group so it can read /dev/kmem.
group = 

# Program to install `make'.
INSTALL_PROGRAM = $(INSTALL)
# Program to install the man page.
INSTALL_DATA = $(INSTALL) -m 644
# Generic install program.
INSTALL = /usr/bin/install -c

# Program to format Texinfo source into Info files.
MAKEINFO = makeinfo
# Program to format Texinfo source into DVI files.
TEXI2DVI = texi2dvi

# Programs to make tags files.
ETAGS = etags -tw
CTAGS = ctags -tw

objs = commands.o job.o dir.o file.o misc.o main.o read.o remake.o	\
       rule.o implicit.o default.o variable.o expand.o function.o	\
       vpath.o version.o ar.o arscan.o signame.o remote-$(REMOTE).o	\
       $(GLOB) $(GETOPT) $(ALLOCA) $(extras)
srcs = $(srcdir)/commands.c $(srcdir)/job.c $(srcdir)/dir.c		\
       $(srcdir)/file.c $(srcdir)/getloadavg.c $(srcdir)/misc.c		\
       $(srcdir)/main.c $(srcdir)/read.c $(srcdir)/remake.c		\
       $(srcdir)/rule.c $(srcdir)/implicit.c $(srcdir)/default.c	\
       $(srcdir)/variable.c $(srcdir)/expand.c $(srcdir)/function.c	\
       $(srcdir)/vpath.c $(srcdir)/version.c				\
       $(srcdir)/remote-$(REMOTE).c					\
       $(srcdir)/ar.c $(srcdir)/arscan.c				\
       $(srcdir)/signame.c $(srcdir)/signame.h $(GETOPT_SRC)		\
       $(srcdir)/commands.h $(srcdir)/dep.h $(srcdir)/file.h		\
       $(srcdir)/job.h $(srcdir)/make.h $(srcdir)/rule.h		\
       $(srcdir)/variable.h $(ALLOCA_SRC) $(srcdir)/config.h.in


.SUFFIXES:
.SUFFIXES: .o .c .h .ps .dvi .info .texinfo

all: make
check: # No tests.
info: make.info
dvi: make.dvi
# Some makes apparently use .PHONY as the default goal is it is before `all'.
.PHONY: all check info dvi

make.info: make.texinfo
	$(MAKEINFO) -I$(srcdir) $(srcdir)/make.texinfo -o make.info

make.dvi: make.texinfo
	$(TEXI2DVI) $(srcdir)/make.texinfo

make.ps: make.dvi
	dvi2ps make.dvi > make.ps

make: $(objs)
	$(CC) $(LDFLAGS) $(objs) $(LOADLIBES) -o make.new
	mv -f make.new make

# -I. is needed to find config.h in the build directory.
.c.o:
	$(CC) $(defines) -c -I. -I$(srcdir) -I$(srcdir)/glob \
	      $(CFLAGS) $< $(OUTPUT_OPTION)

# For some losing Unix makes.
SHELL = /bin/sh
MAKE = make

glob/libglob.a: FORCE config.h
	cd glob; $(MAKE) CC='$(CC)' CFLAGS='$(CFLAGS) -I..' \
			 CPPFLAGS='$(CPPFLAGS) -DHAVE_CONFIG_H' \
			 RANLIB='$(RANLIB)' \
			 libglob.a
FORCE:

tagsrcs = $(srcs) $(srcdir)/remote-*.c
TAGS: $(tagsrcs)
	$(ETAGS) $(tagsrcs)
tags: $(tagsrcs)
	$(CTAGS) $(tagsrcs)

.PHONY: install installdirs
install: installdirs \
	 $(bindir)/$(instname) $(infodir)/make.info \
	 $(mandir)/$(instname).$(manext)

installdirs:
	$(SHELL) ${srcdir}/mkinstalldirs $(bindir) $(infodir) $(mandir)

$(bindir)/$(instname): make
	$(INSTALL_PROGRAM) make $@.new
	@if $(install_setgid); then \
	   if chgrp $(group) $@.new && chmod g+s $@.new; then \
	     echo "chgrp $(group) $@.new && chmod g+s $@.new"; \
	   else \
	     echo "$@ needs to be owned by group $(group) and setgid;"; \
	     echo "otherwise the \`-l' option will probably not work."; \
	     echo "You may need special priveleges to install $@."; \
	   fi; \
	 else true; fi
# Some systems can't deal with renaming onto a running binary.
	-rm -f $@.old
	-mv $@ $@.old
	mv $@.new $@

$(infodir)/make.info: make.info
	if [ -r ./make.info ]; then dir=.; else dir=$(srcdir); fi; \
	for file in $${dir}/make.info*; do \
	  name="`basename $$file`"; \
	  $(INSTALL_DATA) $$file \
	    `echo $@ | sed "s,make.info\$$,$$name,"`; \
	done
# Run install-info only if it exists.
# Use `if' instead of just prepending `-' to the
# line so we notice real errors from install-info.
# We use `$(SHELL) -c' because some shells do not
# fail gracefully when there is an unknown command.
	if $(SHELL) -c 'install-info --version' >/dev/null 2>&1; then \
	  install-info --infodir=$(infodir) $$dir/make.info; \
	else true; fi

$(mandir)/$(instname).$(manext): make.man
	$(INSTALL_DATA) $(srcdir)/make.man $@


loadavg: getloadavg.c config.h
	$(CC) $(defines) -DTEST -I. -I$(srcdir) $(CFLAGS) $(LDFLAGS) \
	      $(srcdir)/getloadavg.c $(LOADLIBES) -o $@
check-loadavg: loadavg
	@echo The system uptime program believes the load average to be:
	-uptime
	@echo The GNU load average checking code believes:
	./loadavg
check: check-loadavg


.PHONY: clean realclean distclean mostlyclean
clean: glob-clean
	-rm -f make loadavg *.o core make.info* make.dvi
distclean: clean glob-realclean
	-rm -f Makefile config.h config.status build.sh stamp-config
	-rm -f TAGS tags
	-rm -f make.?? make.??s make.log make.toc make.*aux
realclean: distclean
mostlyclean: clean

.PHONY: glob-clean glob-realclean
glob-clean glob-realclean:
	cd glob; $(MAKE) $@

Makefile: config.status $(srcdir)/Makefile.in
	$(SHELL) config.status
config.h: stamp-config ;
stamp-config: config.status $(srcdir)/config.h.in
	$(SHELL) config.status
	touch stamp-config

configure: configure.in
	autoconf $(ACFLAGS)
config.h.in: configure.in
	autoheader $(ACFLAGS)

# This tells versions [3.59,3.63) of GNU make not to export all variables.
.NOEXPORT:

# The automatically generated dependencies below may omit config.h
# because it is included with ``#include <config.h>'' rather than
# ``#include "config.h"''.  So we add the explicit dependency to make sure.
$(objs): config.h

# Automatically generated dependencies will be put at the end of the file.

# Automatically generated dependencies.
commands.o : commands.c make.h config.h dep.h commands.h file.h variable.h job.h 
job.o : job.c make.h config.h commands.h job.h file.h variable.h 
dir.o : dir.c make.h config.h 
file.o : file.c make.h config.h commands.h dep.h file.h variable.h 
misc.o : misc.c make.h config.h dep.h 
main.o : main.c make.h config.h commands.h dep.h file.h variable.h job.h getopt.h 
read.o : read.c make.h config.h commands.h dep.h file.h variable.h glob/glob.h 
remake.o : remake.c make.h config.h commands.h job.h dep.h file.h 
rule.o : rule.c make.h config.h commands.h dep.h file.h variable.h rule.h 
implicit.o : implicit.c make.h config.h rule.h dep.h file.h 
default.o : default.c make.h config.h rule.h dep.h file.h commands.h variable.h 
variable.o : variable.c make.h config.h commands.h variable.h dep.h file.h 
expand.o : expand.c make.h config.h commands.h file.h variable.h 
function.o : function.c make.h config.h variable.h dep.h commands.h job.h 
vpath.o : vpath.c make.h config.h file.h variable.h 
version.o : version.c 
ar.o : ar.c make.h config.h file.h dep.h 
arscan.o : arscan.c make.h config.h 
signame.o : signame.c config.h signame.h 
remote-stub.o : remote-stub.c make.h config.h commands.h 
getopt.o : getopt.c config.h getopt.h 
getopt1.o : getopt1.c config.h getopt.h 
getloadavg.o : getloadavg.c config.h 
