# This file was generated automatically by configure.  Do not edit.
host_alias = sun4
host_cpu = sparc
host_vendor = sun
host_os = sunos411
target_alias = sun4
target_cpu = sparc
target_vendor = sun
target_os = sunos411
target_makefile_frag = 
host_makefile_frag = 
site_makefile_frag = 
links = 
VPATH = .
ALL=all.internal
#
# Makefile for directory with subdirs to build.
#   Copyright (C) 1990, 1991, 1992 Free Software Foundation
#
# This file is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */
#

CC = gcc -O4
srcdir = .

prefix = /usr/local

exec_prefix = $(prefix)
bindir = $(exec_prefix)/bin
libdir = $(exec_prefix)/lib

datadir = $(prefix)/lib
mandir = $(prefix)/man
man1dir = $(mandir)/man1
man2dir = $(mandir)/man2
man3dir = $(mandir)/man3
man4dir = $(mandir)/man4
man5dir = $(mandir)/man5
man6dir = $(mandir)/man6
man7dir = $(mandir)/man7
man8dir = $(mandir)/man8
man9dir = $(mandir)/man9
infodir = $(prefix)/info
includedir = $(prefix)/include
docdir = $(datadir)/doc

SHELL = /bin/sh

INSTALL = install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)

AR = ar
AR_FLAGS = qc
RANLIB = ranlib

BISON = `if [ -d $${rootme}/bison ] ; \
	then echo $${rootme}/bison/bison -L $${rootme}/bison/ -y ; \
	else echo bison -y ; fi`

MAKEINFO = `if [ -d $${rootme}/texinfo/C ] ; \
	then echo $${rootme}/texinfo/C/makeinfo ; \
	else echo makeinfo ; fi`

SUBDIRS = mmalloc libiberty bfd binutils byacc bison gcc readline glob ld gas gdb emacs ispell make grep diff rcs cvs patch send_pr libg++ newlib gprof
OTHERS = 

ALL = all.normal
INSTALL_TARGET = install.all

### for debugging
#GCCVERBOSE=-v


#### host and target specific makefile fragments come in here.
###

.PHONY: all info install-info clean-info

all:	$(ALL)

info:	cfg-paper.info configure.info
	rootme=`pwd` ; export rootme ; $(MAKE) subdir_do DO=info "DODIRS=$(SUBDIRS)" "MAKEINFO=$(MAKEINFO)"

check:; rootme=`pwd` ; export rootme ; $(MAKE) subdir_do DO=check \
	"DODIRS=`echo $(SUBDIRS) | sed -e \"s/libg\+\+//\"" \
	"MAKEINFO=$(MAKEINFO)"
	if [ -d libg++ ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd libg++ ; \
		 $(MAKE) check "CC=$${rootme}/gcc/gcc \
		 -B$${rootme}/gcc/") ; \
	fi
		

clean-info:
	$(MAKE) subdir_do DO=clean-info "DODIRS=$(SUBDIRS)"
	rm -f cfg-paper.info* configure.info*

cfg-paper.info: cfg-paper.texi
	rootme=`pwd` ; export rootme ; $(MAKEINFO) -o cfg-paper.info $(srcdir)/cfg-paper.texi

configure.info: configure.texi
	rootme=`pwd` ; export rootme ; $(MAKEINFO) -o configure.info $(srcdir)/configure.texi

install-info: install-info-dirs force
	[ -d $(infodir) ] || mkdir $(infodir)
	$(MAKE) subdir_do DO=install-info "DODIRS=$(SUBDIRS)"
	$(INSTALL_DATA) cfg-paper.info $(infodir)/cfg-paper.info
	$(INSTALL_DATA) configure.info $(infodir)/configure.info
	$(MAKE) dir.info install-dir.info

install-dir.info:
	$(INSTALL_DATA) dir.info $(infodir)/dir.info

all.normal: all-libiberty all-mmalloc all-bison \
	all-byacc all-bfd all-ld all-gas all-gcc \
	all-binutils all-libg++ all-readline all-gdb \
	all-make all-rcs all-cvs all-diff all-grep \
	all-patch all-emacs all-ispell all-fileutils \
	all-newlib all-gprof all-send_pr
all.cross: all-libiberty all-mmalloc all-gas all-bison all-ld \
	all-bfd all-libgcc all-readline all-gdb
#	$(MAKE) subdir_do DO=all "DODIRS=$(SUBDIRS) $(OTHERS)"

clean: clean-stamps clean-libiberty clean-mmalloc clean-bfd \
	clean-newlib clean-binutils \
	clean-bison clean-byacc clean-ld clean-gas \
	clean-gcc clean-libgcc clean-readline clean-glob clean-gdb \
	clean-make clean-diff clean-grep clean-rcs \
	clean-cvs clean-patch clean-emacs clean-ispell clean-fileutils \
	clean-libg++ clean-gprof clean-send_pr
	-rm -rf *.a TEMP errs core *.o *~ \#* TAGS *.E

clean-stamps:
	-rm -f all-*

install: $(INSTALL_TARGET) $(srcdir)/configure.man
	$(INSTALL_DATA) $(srcdir)/configure.man $(man1dir)/configure.1


install.all: install-dirs install-libiberty install-mmalloc \
	install-bfd install-binutils install-bison install-byacc \
	install-ld install-gas install-gcc install-gprof \
	install-libgcc install-readline install-glob install-gdb \
	install-make install-cvs install-patch install-emacs \
	install-ispell install-fileutils install-libg++ install-newlib \
	install-send_pr

install.cross: install-dirs install-libiberty install-mmalloc install-binutils \
	install-bison install-byacc install-ld install-gas install-libgcc \
	install-readline install-glob install-gdb install-mmalloc install-gprof

### libiberty
all-libiberty: force
	@if [ -d ./libiberty ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libiberty; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-libiberty: force
	@if [ -d ./libiberty ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libiberty; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-libiberty: force
	@if [ -d ./libiberty ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libiberty; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### mmalloc
all-mmalloc: force
	@if [ -d ./mmalloc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./mmalloc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-mmalloc: force
	@if [ -d ./mmalloc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./mmalloc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-mmalloc: force
	@if [ -d ./mmalloc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./mmalloc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### texinfo
all-texinfo: all-libiberty
	@if [ -d ./texinfo ] ; then \
		rootme=`pwd` ; export rootme ; \
		rootme=`pwd` ; export rootme ; \
		(cd ./texinfo; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-texinfo: force
	@if [ -d ./texinfo ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./texinfo; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-texinfo: force
	@if [ -d ./texinfo ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./texinfo; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### bfd
all-bfd: force
	@if [ -d ./bfd ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./bfd; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-bfd: force
	@if [ -d ./bfd ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./bfd; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-bfd: force
	@if [ -d ./bfd ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./bfd; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### binutils
all-binutils: all-libiberty all-bfd
	@if [ -d ./binutils ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./binutils; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-binutils: force
	@if [ -d ./binutils ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./binutils; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-binutils: force
	@if [ -d ./binutils ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./binutils; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### newlib
all-newlib: force
	@if [ -d ./newlib ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./newlib; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-newlib: force
	@if [ -d ./newlib ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./newlib; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-newlib: force
	@if [ -d ./newlib ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./newlib; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### gprof
all-gprof: all-libiberty all-bfd
	@if [ -d ./gprof ] ; then \
		(cd ./gprof; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-gprof: force
	@if [ -d $(unsubdir)/gprof ] ; then \
		(cd $(unsubdir)/gprof$(subdir); \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-gprof: force
	@if [ -d $(unsubdir)/gprof ] ; then \
		(cd $(unsubdir)/gprof$(subdir); \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### byacc
all-byacc: force
	@if [ -d ./byacc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./byacc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-byacc: force
	@if [ -d ./byacc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./byacc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-byacc: force
	@if [ -d ./byacc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./byacc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### bison
all-bison: all-libiberty
	@if [ -d ./bison ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./bison; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-bison: force
	@if [ -d ./bison ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./bison; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-bison: force
	@if [ -d ./bison ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./bison; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### gcc
all-gcc: all-libiberty all-bison
	@if [ -d ./gcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-gcc: force
	@if [ -d ./gcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-gcc: force
	@if [ -d ./gcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### readline
all-readline: force
	@if [ -d ./readline ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./readline; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-readline: force
	@if [ -d ./readline ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./readline; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-readline: force
	@if [ -d ./readline ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./readline; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### glob
all-glob: force
	@if [ -d ./glob ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./glob; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-glob: force
	@if [ -d ./glob ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./glob; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-glob: force
	@if [ -d ./glob ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./glob; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### gas
all-gas: all-libiberty all-bfd
	@if [ -d ./gas ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gas; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-gas: force
	@if [ -d ./gas ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gas; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-gas: force
	@if [ -d ./gas ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gas; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### ld
all-ld: all-libiberty all-bfd all-bison
	@if [ -d ./ld ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./ld; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-ld: force
	@if [ -d ./ld ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./ld; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-ld: force
	@if [ -d ./ld ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./ld; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### libgcc (and libgcc1)
all-libgcc1: all-gas all-binutils
	@if [ -d ./libgcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libgcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			libgcc1.a) ; \
	else \
		true ; \
	fi

clean-libgcc1: force
	@if [ -d ./libgcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libgcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean-libgcc1) ; \
	else \
		true ; \
	fi

install-libgcc1: force
	echo libgcc1 is a component, not an installable target

all-libgcc: all-gas all-gcc all-binutils
	true
	@if [ -d ./libgcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libgcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-libgcc: force
	@if [ -d ./libgcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libgcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-libgcc: force
	@if [ -d ./libgcc ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libgcc; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### gdb
all-gdb: all-bfd all-libiberty all-mmalloc all-readline all-glob all-bison
	@if [ -d ./gdb ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gdb; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-gdb: force
	@if [ -d ./gdb ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gdb; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-gdb: force
	@if [ -d ./gdb ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./gdb; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### make
all-make: all-libiberty
	@if [ -d ./make ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./make; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-make: force
	@if [ -d ./make ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./make; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-make: force
	@if [ -d ./make ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./make; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### diff
all-diff: force
	@if [ -d ./diff ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./diff; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-diff: force
	@if [ -d ./diff ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./diff; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-diff: force
	@if [ -d ./diff ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./diff/; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### grep
all-grep: force
	@if [ -d ./grep ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./grep; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-grep: force
	@if [ -d ./grep ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./grep; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-grep: force
	@if [ -d ./grep ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./grep; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### rcs
all-rcs: force
	@if [ -d ./rcs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./rcs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-rcs: force
	@if [ -d ./rcs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./rcs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-rcs: force
	@if [ -d ./rcs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./rcs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### cvs
all-cvs: force
	@if [ -d ./cvs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./cvs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-cvs: force
	@if [ -d ./cvs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./cvs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-cvs: force
	@if [ -d ./cvs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./cvs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### patch
all-patch: force
	@if [ -d ./patch ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./patch; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-patch: force
	@if [ -d ./patch ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./patch; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-patch: force
	@if [ -d ./patch ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./patch; \
		$(MAKE) \
			bindir=$(bindir) \
			man1dir=$(man1dir) \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### emacs
all-emacs: force
	@if [ -d ./emacs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./emacs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-emacs: force
	@if [ -d ./emacs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./emacs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-emacs: force
	@if [ -d ./emacs ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./emacs; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### ispell
all-ispell: all-emacs
	@if [ -d ./ispell ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./ispell; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-ispell: force
	@if [ -d ./ispell ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./ispell; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-ispell: force
	@if [ -d ./ispell ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./ispell; \
		$(MAKE) \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### fileutils
all-fileutils: force
	@if [ -d ./fileutils ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./fileutils; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-fileutils: force
	@if [ -d ./fileutils ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./fileutils; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-fileutils: force
	@if [ -d ./fileutils ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./fileutils; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### send_pr
all-send_pr: force
	@if [ -d ./send_pr ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./send_pr; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-send_pr: force
	@if [ -d ./send_pr ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./send_pr; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-send_pr: force
	@if [ -d ./send_pr ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./send_pr; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### libg++
GXX = `if [ -d $${rootme}/gcc ] ; \
	then echo $${rootme}/gcc/gcc -B$${rootme}/gcc/ ; \
	else echo gcc ; fi`

XTRAFLAGS = `if [ -d $${rootme}/gcc ] ; \
	then echo -I$${rootme}/gcc/include ; \
	else echo ; fi`

all-libg++: all-gas all-ld all-gcc
	@if [ -d ./libg++ ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libg++; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=${GXX}" \
			"XTRAFLAGS=${XTRAFLAGS}" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			all) ; \
	else \
		true ; \
	fi

clean-libg++: force
	@if [ -d ./libg++ ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libg++; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			clean) ; \
	else \
		true ; \
	fi

install-libg++: force
	@if [ -d ./libg++ ] ; then \
		rootme=`pwd` ; export rootme ; \
		(cd ./libg++; \
		$(MAKE) \
			"prefix=$(prefix)" \
			"datadir=$(datadir)" \
			"mandir=$(mandir)" \
			"against=$(against)" \
			"AR=$(AR)" \
			"AR_FLAGS=$(AR_FLAGS)" \
			"CC=$(CC)" \
			"RANLIB=$(RANLIB)" \
			"LOADLIBES=$(LOADLIBES)" \
			"LDFLAGS=$(LDFLAGS)" \
			"BISON=$(BISON)" \
			"MAKEINFO=$(MAKEINFO)" \
			install) ; \
	else \
		true ; \
	fi

### other supporting targets
# this is a bad hack.
all.xclib:	all.normal
	if [ -d clib ] ; then \
		(cd clib ; $(MAKE)) ; \
	fi

subdir_do:
	for i in $(DODIRS); do \
		if [ -f ./$$i/localenv ] ; then \
			if (rootme=`pwd` ; export rootme ; cd ./$$i; \
				$(MAKE) \
					"against=$(against)" \
					"BISON=$(BISON)" \
					"MAKEINFO=$(MAKEINFO)" \
					$(DO)) ; then true ; \
				else exit 1 ; fi ; \
		else if [ -d ./$$i ] ; then \
			if (rootme=`pwd` ; export rootme ; cd ./$$i; \
				$(MAKE) \
					"against=$(against)" \
					"AR=$(AR)" \
					"AR_FLAGS=$(AR_FLAGS)" \
					"CC=$(CC)" \
					"RANLIB=$(RANLIB)" \
					"LOADLIBES=$(LOADLIBES)" \
					"LDFLAGS=$(LDFLAGS)" \
					"BISON=$(BISON)" \
					"MAKEINFO=$(MAKEINFO)" \
					$(DO)) ; then true ; \
			else exit 1 ; fi ; \
		else true ; fi ; \
	fi ; \
	done

bootstrap:
	$(MAKE) all info
	$(MAKE) stage1
	$(MAKE) pass "stagepass=stage1"
	$(MAKE) stage2
	$(MAKE) pass "stagepass=stage2"
	$(MAKE) comparison

bootstrap2:
	$(MAKE) pass "stagepass=stage1"
	$(MAKE) stage2
	$(MAKE) pass "stagepass=stage2"
	$(MAKE) comparison

bootstrap3:
	$(MAKE) pass "stagepass=stage2"
	$(MAKE) comparison

pass:
	cp $(srcdir)/gcc/gstdarg.h ./gas/stdarg.h
	$(MAKE) subdir_do "DO=all info" "DODIRS=$(SUBDIRS)" \
		"CC=`pwd`/gcc/$(stagepass)/gcc \
		-O $(GCCVERBOSE) -I`pwd`/gcc/include \
		-B`pwd`/gcc/$(stagepass)/ \
		-B`pwd`/gas/$(stagepass)/ \
		-B`pwd`/ld/$(stagepass)/" \
		"AR=`pwd`/binutils/$(stagepass)/ar" \
		"LD=`pwd`/gcc/$(stagepass)/gcc $(GCCVERBOSE)" \
		"RANLIB=`pwd`/binutils/$(stagepass)/ranlib" \
		"LOADLIBES=`pwd`/libgcc/$(stagepass)/libgcc.a /lib/libc.a" \
		"LDFLAGS=-nostdlib /lib/crt0.o \
		-L`pwd`/libgcc/$(stagepass)/ \
		-B`pwd`/ld/$(stagepass)/"


stage1:
	$(MAKE) subdir_do DO=stage1 "DODIRS=$(SUBDIRS)"

stage2:
	$(MAKE) subdir_do DO=stage2 "DODIRS=$(SUBDIRS)"

stage3:
	$(MAKE) subdir_do DO=stage3 "DODIRS=$(SUBDIRS)"

stage4:
	$(MAKE) subdir_do DO=stage4 "DODIRS=$(SUBDIRS)"

against=stage2

comparison:; $(MAKE) subdir_do DO=comparison against=$(against) "DODIRS=$(SUBDIRS)"

de-stage1:; $(MAKE) subdir_do DO=de-stage1 "DODIRS=$(SUBDIRS)"
de-stage2:; $(MAKE) subdir_do DO=de-stage2 "DODIRS=$(SUBDIRS)"
de-stage3:; $(MAKE) subdir_do DO=de-stage3 "DODIRS=$(SUBDIRS)"
de-stage4:; $(MAKE) subdir_do DO=de-stage4 "DODIRS=$(SUBDIRS)"

# The "else true" stuff is for Ultrix; the shell returns the exit code
# of the "if" command, if no commands are run in the "then" or "else" part,
# causing Make to quit.

MAKEDIRS= \
	$(prefix) \
	$(exec_prefix) \
	$(bindir) \
	$(libdir) \
	$(tooldir) \
	$(includedir) \
	$(datadir) \
	$(docdir) \
	$(mandir) \
	$(man1dir) \
	$(man5dir)

#	$(man2dir) \
#	$(man3dir) \
#	$(man4dir) \
#	$(man6dir) \
#	$(man7dir) \
#	$(man8dir)

install-dirs:
	for i in $(MAKEDIRS) ; do \
		echo Making $$i... ; \
		[ -d $$i ] || mkdir $$i || exit 1 ; \
	done

MAKEINFODIRS= \
	$(prefix) \
	$(infodir)

install-info-dirs:
	if [ -d $(prefix) ] ; then true ; else mkdir $(prefix) ; fi
	if [ -d $(datadir) ] ; then true ; else mkdir $(datadir) ; fi
	if [ -d $(infodir) ] ; then true ; else mkdir $(infodir) ; fi

dir.info:
	$(srcdir)/texinfo/gen-info-dir $(infodir) > dir.info.new
	mv -f dir.info.new dir.info

etags tags: TAGS

TAGS:
	etags `$(MAKE) ls`

ls:
	@echo Makefile
	@for i in $(SUBDIRS); \
	do \
		(cd $$i; \
			pwd=`pwd`; \
			wd=`basename $$pwd`; \
			for j in `$(MAKE) ls`; \
			do \
				echo $$wd/$$j; \
			done) \
	done

force:

# with the gnu make, this is done automatically.

Makefile: $(srcdir)/Makefile.in $(host_makefile_frag) $(target_makefile_frag)
	$(SHELL) ./config.status

#
# Build GDB distributions that contain BFD, Include, Libiberty, Readline, etc

DEVO_SUPPORT= README cfg-paper.texi Makefile.in configure configure.in \
	config.sub config configure.man
GDB_SUPPORT_DIRS= bfd include libiberty mmalloc readline glob
GDB_SUPPORT_FILES= $(GDB_SUPPORT_DIRS) texinfo/fsf/texinfo.tex

setup-dirs: force_update
	./configure sun4
	make clean
	./configure -rm sun4
	chmod og=u `find $(DEVO_SUPPORT) $(GDB_SUPPORT_FILES) -print`

bfd.ilrt.tar.Z: setup-dirs
	rm -f bfd.ilrt.tar.Z
	tar cf - $(DEVO_SUPPORT) $(GDB_SUPPORT_FILES) \
		| compress -v >bfd.ilrt.tar.Z

gdb.tar.Z: setup-dirs
	(cd gdb; $(MAKE) -f Makefile.in make-proto-gdb.dir)
	$(MAKE) $(MFLAGS) -f Makefile.in make-gdb.tar.Z

make-gdb.tar.Z: $(DEVO_SUPPORT) $(GDB_SUPPORT_DIRS) gdb texinfo/fsf/texinfo.tex
	rm -rf proto-toplev; mkdir proto-toplev
	ln -s ../gdb/proto-gdb.dir proto-toplev/gdb
	(cd proto-toplev; for i in $(DEVO_SUPPORT) $(GDB_SUPPORT_DIRS); do \
		ln -s ../$$i . ; \
	done)
	# Put only one copy (four hard links) of COPYING in the tar file.
	rm                          proto-toplev/bfd/COPYING
	ln proto-toplev/gdb/COPYING proto-toplev/bfd/COPYING
	rm                          proto-toplev/include/COPYING
	ln proto-toplev/gdb/COPYING proto-toplev/include/COPYING
	rm                          proto-toplev/readline/COPYING
	ln proto-toplev/gdb/COPYING proto-toplev/readline/COPYING
	# Take out texinfo from configurable dirs
	rm proto-toplev/configure.in
	sed '/^configdirs=/s/texinfo //' <configure.in >proto-toplev/configure.in
	# Take out glob from buildable dirs
	rm proto-toplev/Makefile.in
	sed '/^SUBDIRS =/s/glob //' <Makefile.in >proto-toplev/Makefile.in
	# Take out texinfo from buildable dirs
	cp proto-toplev/Makefile.in temp.$$
	sed '/^all\.normal: /s/\all-texinfo //' <temp.$$ >temp1.$$
	sed '/^clean: /s/clean-texinfo //' <temp1.$$ >temp.$$
	sed '/^install\.all: /s/install-texinfo //' <temp.$$ >proto-toplev/Makefile.in
	rm temp.$$ temp1.$$
	mkdir proto-toplev/texinfo
	mkdir proto-toplev/texinfo/fsf
	ln -s ../../../texinfo/fsf/texinfo.tex proto-toplev/texinfo/fsf/
	chmod og=u `find proto-toplev -print`
	(VER=`sed <gdb/Makefile.in -n 's/VERSION = //p'`; \
		echo "==> Making gdb-$$VER.tar.Z"; \
		ln -s proto-toplev gdb-$$VER; \
		tar cfh - gdb-$$VER \
		| compress -v >gdb-$$VER.tar.Z)

force_update:

nothing:

# end of Makefile.in
