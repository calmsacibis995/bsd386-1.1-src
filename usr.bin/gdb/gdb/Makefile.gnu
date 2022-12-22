# This file was generated automatically by configure.  Do not edit.
host_alias = sun4
host_cpu = sparc
host_vendor = sun
host_os = sunos411
target_alias = sun4
target_cpu = sparc
target_vendor = sun
target_os = sunos411
target_makefile_frag = ./config/sun4.mt
host_makefile_frag = ./config/sun4.mh
site_makefile_frag = 
links =  xm.h tm.h
VPATH = .
ALL=all.internal
#Copyright (C) 1989, 1990, 1991, 1992 Free Software Foundation, Inc.

# This file is part of GDB.

# This program is free software; you can redistribute it and/or modify
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
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

prefix = /usr/local

program_prefix = 
exec_prefix = $(prefix)
bindir = $(prefix)
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

xbindir = /usr/bin/X11
xlibdir = /usr/lib/X11

SHELL = /bin/sh

INSTALL = install -c
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL)

AR = ar
AR_FLAGS = qv
RANLIB = ranlib

# Flags that describe where you can find the termcap library.
# This can be overridden in the host Makefile fragment file.
TERMCAP = -ltermcap

# System V: If you compile gdb with a compiler which uses the coff
# encapsulation feature (this is a function of the compiler used, NOT
# of the m-?.h file selected by config.gdb), you must make sure that
# the GNU nm is the one that is used by munch.

# If you are compiling with GCC, make sure that either 1) You use the
# -traditional flag, or 2) You have the fixed include files where GCC
# can reach them.  Otherwise the ioctl calls in inflow.c
# will be incorrectly compiled.  The "fixincludes" script in the gcc
# distribution will fix your include files up.
#CC=cc
#CC=gcc -traditional
CC=gcc -O
GCC=gcc

# Directory containing source files.  Don't clean up the spacing,
# this exact string is matched for by the "configure" script.
srcdir = .

# It is also possible that you will need to add -I/usr/include/sys to the
# CFLAGS section if your system doesn't have fcntl.h in /usr/include (which 
# is where it should be according to Posix).

BISON=bison -y
BISONFLAGS=
YACC=$(BISON) $(BISONFLAGS)
# YACC=yacc
MAKE=make

# Documentation (gdb.dvi) needs either GNU m4 or SysV m4; 
# Berkeley/Sun don't have quite enough. 
#M4=/usr/5bin/m4
M4=gm4

# where to find texinfo; GDB dist should include a recent one
TEXIDIR=${srcdir}/../texinfo/fsf

# where to find makeinfo, preferably one designed for texinfo-2
MAKEINFO=makeinfo

# Set this up with gcc if you have gnu ld and the loader will print out
# line numbers for undefinded refs.
#CC-LD=gcc -static
CC-LD=${CC}

# Where is the "include" directory?  Traditionally ../include or ./include
INCLUDE_DIR =  ${srcdir}/../include
INCLUDE_DEP = $$(INCLUDE_DIR)

# Where is the source dir for the MMALLOC library? Traditionally ../mmalloc
# or ./mmalloc  (When we want the binary library built from it, we use
# ${MMALLOC_DIR}${subdir}.)
# Note that mmalloc can still be used on systems without mmap().
# To use your system malloc, comment out the following defines.
MMALLOC_DIR = ${srcdir}/../mmalloc
MMALLOC_DEP = $$(MMALLOC_DIR)
MMALLOC_LIB = ./../mmalloc${subdir}/libmmalloc.a
# To use your system malloc, uncomment MMALLOC_DISABLE.
#MMALLOC_DISABLE = -DNO_MMALLOC
# To use mmalloc but disable corruption checking, uncomment MMALLOC_CHECK
#MMALLOC_CHECK = -DNO_MMALLOC_CHECK
MMALLOC_CFLAGS = ${MMALLOC_CHECK} ${MMALLOC_DISABLE}

# Where is the source dir for the BFD library?  Traditionally ../bfd or ./bfd
# (When we want the binary library built from it, we use ${BFD_DIR}${subdir}.)
BFD_DIR =  ${srcdir}/../bfd
BFD_DEP = $$(BFD_DIR)
BFD_LIB = ./../bfd${subdir}/libbfd.a

# Where is the source dir for the READLINE library?  Traditionally in .. or .
# (For the binary library built from it, we use ${READLINE_DIR}${subdir}.)
READLINE_DIR = ${srcdir}/../readline
READLINE_DEP = $$(READLINE_DIR)
RL_LIB = ./../readline${subdir}/libreadline.a

# All the includes used for CFLAGS and for lint.
# -I. for config files.
# -I${srcdir} possibly for regex.h also.
INCLUDE_CFLAGS = -I. -I${srcdir} -I$(INCLUDE_DIR) -I$(READLINE_DIR) -I${srcdir}/vx-share

# {X,T}M_CFLAGS, if defined, has system-dependent CFLAGS.
# CFLAGS for GDB
MINUS_G=-g
GLOBAL_CFLAGS = ${MINUS_G} ${TM_CFLAGS} ${XM_CFLAGS}
#PROFILE_CFLAGS = -pg

# CFLAGS is the aggregate of several individual *_CFLAGS macros.
# USER_CFLAGS is specifically reserved for setting from the command line
# when running make.  I.E.  "make USER_CFLAGS=-Wmissing-prototypes".
CFLAGS = ${GLOBAL_CFLAGS} ${PROFILE_CFLAGS} ${MMALLOC_CFLAGS} ${INCLUDE_CFLAGS} ${USER_CFLAGS}
# None of the things in CFLAGS will do any harm, and on some systems
#  (e.g. SunOS4) it is important to use the M_CFLAGS.
LDFLAGS = $(CFLAGS)

# Where is the "-liberty" library, containing getopt and obstack?
LIBIBERTY_DIR = ${srcdir}/../libiberty
LIBIBERTY = ./../libiberty${subdir}/libiberty.a

# The config/mh-* file must define REGEX and REGEX1 on USG machines.
# If your sysyem is missing alloca(), or, more likely, it's there but
# it doesn't work, define ALLOCA & ALLOCA1 too.
# If your system is missing putenv(), add putenv.c to XM_ADD_FILES.

# Libraries and corresponding dependencies for compiling gdb.
# {X,T}M_CLIBS, defined in *config files, have host- and target-dependent libs.
# TERMCAP comes after readline, since readline depends on it.
CLIBS = ${BFD_LIB}  ${RL_LIB} ${TERMCAP} ${MMALLOC_LIB} ${LIBIBERTY} \
	${XM_CLIBS} ${TM_CLIBS}
CDEPS = ${XM_CDEPS} ${TM_CDEPS} ${BFD_LIB} ${MMALLOC_LIB} ${LIBIBERTY} \
	${RL_LIB} ${MMALLOC_LIB}

ADD_FILES = ${REGEX} ${ALLOCA} ${XM_ADD_FILES} ${TM_ADD_FILES}
ADD_DEPS = ${REGEX1} ${ALLOCA1} ${XM_ADD_FILES} ${TM_ADD_FILES}

VERSION = 4.5
DIST=gdb

LINT=/usr/5bin/lint
LINTFLAGS= -I${BFD_DIR}

# Host and target-dependent makefile fragments come in here.
####
# Target: Sun 4 or Sparcstation, running SunOS 4
TDEPFILES= exec.o sparc-tdep.o sparc-tcmn.o sparc-pinsn.o solib.o
TM_FILE= tm-sun4os4.h
TM_CFLAGS = -DKERNELDEBUG
TM_CLIBS= -lkvm
# Host: Sun 4 or Sparcstation, running SunOS 4
XDEPFILES= infptrace.o sparc-xdep.o
XM_FILE= xm-sun4os4.h
# End of host and target-dependent makefile fragments

LBL_SRC = kcore.c remote-sl.c remote-fp.c kernel.c cmdparse.c
LBL_OBJ = $(LBL_SRC:.c=.o)

# Source files in the main directory.
# Files which are included via a config/* Makefile fragment
# should *not* be specified here; they're in "ALLDEPFILES".
SFILES_MAINDIR = \
	 blockframe.c breakpoint.c command.c core.c \
	 environ.c eval.c expprint.c findvar.c infcmd.c inflow.c infrun.c \
	 main.c printcmd.c gdbtypes.c \
	 remote.c source.c stack.c symmisc.c symtab.c symfile.c \
	 utils.c valarith.c valops.c valprint.c values.c c-exp.y m2-exp.y \
	 signame.c cplus-dem.c mem-break.c target.c inftarg.c \
	 dbxread.c \
	 ieee-float.c language.c parse.c buildsym.c objfiles.c \
	 minsyms.c \
	$(LBL_SRC) xgdb.c

# Source files in subdirectories (which will be handled separately by
#  'make gdb.tar.Z').
# Files which are included via a config/* Makefile fragment
# should *not* be specified here; they're in "ALLDEPFILES".
SFILES_SUBDIR = \
	 ${srcdir}/vx-share/dbgRpcLib.h \
	 ${srcdir}/vx-share/ptrace.h \
	 ${srcdir}/vx-share/reg.h \
	 ${srcdir}/vx-share/vxTypes.h \
	 ${srcdir}/vx-share/vxWorks.h \
	 ${srcdir}/vx-share/wait.h \
	 ${srcdir}/vx-share/xdr_ld.h \
	 ${srcdir}/vx-share/xdr_ptrace.h \
	 ${srcdir}/vx-share/xdr_rdb.h \
	 ${srcdir}/vx-share/xdr_regs.h \
	 ${srcdir}/nindy-share/b.out.h \
	 ${srcdir}/nindy-share/block_io.h \
	 ${srcdir}/nindy-share/coff.h \
	 ${srcdir}/nindy-share/demux.h \
	 ${srcdir}/nindy-share/env.h \
	 ${srcdir}/nindy-share/stop.h \
	 ${srcdir}/nindy-share/ttycntl.h

# Non-source files in subdirs, that should go into gdb.tar.Z.
NONSRC_SUBDIR = \
	 ${srcdir}/nindy-share/Makefile \
	 ${srcdir}/nindy-share/VERSION

# All source files that go into linking GDB, except config-specified files.
SFILES = $(SFILES_MAINDIR) $(SFILES_SUBDIR)

# All source files that lint should look at
LINTFILES = $(SFILES) $(YYFILES) init.c

# Any additional files specified on these lines should also be added to
# the OTHERS = definition below, so they go in the tar files.
SFILES_STAND = $(SFILES) standalone.c
SFILES_KGDB  = $(SFILES) stuff.c kdb-start.c

# Header files that are not named in config/* Makefile fragments go here.
HFILES=	breakpoint.h buildsym.h call-cmds.h command.h defs.h environ.h \
	expression.h frame.h gdbcmd.h gdbcore.h gdbtypes.h \
	ieee-float.h inferior.h minimon.h objfiles.h partial-stab.h \
	signals.h signame.h symfile.h symtab.h solib.h xcoffsolib.h \
	target.h terminal.h tm-68k.h tm-i960.h tm-sunos.h tm-sysv4.h \
	xm-m68k.h xm-sysv4.h language.h parser-defs.h value.h xm-vax.h

REMOTE_EXAMPLES = m68k-stub.c i386-stub.c rem-multi.shar

POSSLIBS_MAINDIR = regex.c regex.h alloca.c
POSSLIBS = $(POSSLIBS_MAINDIR)

TESTS = testbpt.c testfun.c testrec.c testreg.c testregs.c

OTHERS = Makefile.in depend alldeps.mak createtags munch configure.in \
	 ChangeLog ChangeLog-9091 ChangeLog-3.x gdb.1 refcard.ps \
	 README TODO TAGS WHATS.NEW Projects \
	 .gdbinit COPYING $(YYFILES) \
	 copying.c Convex.notes copying.awk \
	 saber.suppress standalone.c stuff.c kdb-start.c \
	 putenv.c XGDB-README

# Subdirectories of gdb, which should be included in their entirety in
# gdb-xxx.tar.Z:
TARDIRS = doc # tests

# GDB "info" files, which should be included in their entirety
INFOFILES = gdb.info*

DEPFILES= ${TDEPFILES} ${XDEPFILES}

SOURCES=$(SFILES) $(ALLDEPFILES) $(YYFILES)
TAGFILES = $(SOURCES) ${HFILES} ${ALLPARAM} ${POSSLIBS} 
TAGFILES_MAINDIR = $(SFILES_MAINDIR) $(ALLDEPFILES_MAINDIR) \
             ${HFILES} ${ALLPARAM} ${POSSLIBS_MAINDIR} 
TARFILES = ${TAGFILES_MAINDIR} ${OTHERS} ${REMOTE_EXAMPLES}

OBS = main.o blockframe.o breakpoint.o findvar.o stack.o source.o \
    values.o eval.o valops.o valarith.o valprint.o printcmd.o \
    symtab.o symfile.o symmisc.o infcmd.o infrun.o remote.o \
    command.o utils.o expprint.o environ.o version.o gdbtypes.o \
    copying.o $(DEPFILES) signame.o cplus-dem.o mem-break.o target.o \
    inftarg.o ieee-float.o putenv.o parse.o language.o $(YYOBJ) \
    buildsym.o objfiles.o minsyms.o \
    dbxread.o \
    $(LBL_OBJ)

RAPP_OBS = rgdb.o rudp.o rserial.o serial.o udp.o $(XDEPFILES)

TSOBS = core.o inflow.o

NTSOBS = standalone.o

TSSTART = /lib/crt0.o

NTSSTART = kdb-start.o

SUBDIRS = doc

# For now, shortcut the "configure GDB for fewer languages" stuff.
YYFILES = c-exp.tab.c m2-exp.tab.c
YYOBJ = c-exp.tab.o m2-exp.tab.o

# Prevent Sun make from putting in the machine type.  Setting
# TARGET_ARCH to nothing works for SunOS 3, 4.0, but not for 4.1.
.c.o:
	${CC} -c ${CFLAGS} $<

all: gdb
	$(MAKE) subdir_do DO=all "DODIRS=$(SUBDIRS)"
check:
info: force
	$(MAKE) subdir_do DO=info "DODIRS=$(SUBDIRS)" "MAKEINFO=$(MAKEINFO)"
install-info: force
	$(MAKE) subdir_do DO=install-info "DODIRS=$(SUBDIRS)"
clean-info: force
	$(MAKE) subdir_do DO=clean-info "DODIRS=$(SUBDIRS)"

install.xgdb: xgdb
	rm -f $(xbindir)/xgdb
	install xgdb $(xbindir)
	install Xgdb.ad $(xlibdir)/app-defaults/Xgdb

gdb.z:gdb.1
	nroff -man $(srcdir)/gdb.1 | col -b > gdb.t 
	pack gdb.t ; rm -f gdb.t
	mv gdb.t.z gdb.z
	
install: gdb 
	$(INSTALL_PROGRAM) gdb $(bindir)/$(program_prefix)gdb
	$(INSTALL_DATA) $(srcdir)/gdb.1 $(man1dir)/$(program_prefix)gdb.1
	$(M_INSTALL)
	$(MAKE) subdir_do DO=install "DODIRS=$(SUBDIRS)"

xinit.c: $(srcdir)/munch $(OBS) $(TSOBS) xgdb.o
	rm -f xinit.c
	./munch ${MUNCH_DEFINE} $(OBS) $(TSOBS) xgdb.o > xinit.c

xgdb: $(OBS) $(TSOBS) xgdb.o xinit.o ${ADD_DEPS} ${CDEPS}
	$(CC-LD) $(LDFLAGS) -o xgdb xinit.o xgdb.o \
		$(OBS) $(TSOBS) $(ADD_FILES) $(CLIBS) $(LOADLIBES) \
		-lXaw -lXmu -lXt -lXext -lX11 -lXwchar -lX11

init.c: $(srcdir)/munch $(OBS) $(TSOBS)
	$(srcdir)/munch ${MUNCH_DEFINE} $(OBS) $(TSOBS) > init.c

gdb: $(OBS) $(TSOBS) ${ADD_DEPS} ${CDEPS} init.o
	${CC-LD} $(LDFLAGS) -o gdb init.o $(OBS) $(TSOBS) $(ADD_FILES) \
	  $(CLIBS) $(LOADLIBES)

saber_gdb: $(SFILES) $(DEPFILES) copying.c version.c
	#setopt load_flags $(CFLAGS) -I$(BFD_DIR) -DHOST_SYS=SUN4_SYS
	#load ./init.c $(SFILES)
	#unload ${srcdir}/c-exp.y ${srcdir}/m2-exp.y ${srcdir}/vx-share/*.h
	#unload ${srcdir}/nindy-share/[A-Z]*
	#load c-exp.tab.c m2-exp.tab.c
	#load copying.c version.c
	#load ../libiberty/libiberty.a
	#load ../bfd/libbfd.a
	#load ../readline/libreadline.a
	#load ../mmalloc/libmmalloc.a
	#load -ltermcap 
	#load `echo " "$(DEPFILES) | sed -e 's/\.o/.c/g' -e 's, , ../,g'`
	echo "Load .c corresponding to:" $(DEPFILES)


# This is useful when debugging GDB, because some Unix's don't let you run GDB
# on itself without copying the executable.  So "make gdb1" will make
# gdb and put a copy in gdb1, and you can run it with "gdb gdb1".
# Removing gdb1 before the copy is the right thing if gdb1 is open
# in another process.
gdb1: gdb
	rm -f gdb1
	cp gdb gdb1

# This is a remote stub which runs under unix and starts up an
# inferior process.  This is at least useful for debugging GDB's
# remote support.
rapp: $(RAPP_OBS)
	rm -f rapp_init.c
	${srcdir}/munch ${MUNCH_DEFINE} ${RAPP_OBS} > rapp_init.c
	${CC-LD} $(LDFLAGS) -o $@ rapp_init.c $(RAPP_OBS)
	
# Support for building Makefile out of configured pieces, automatically
# generated dependencies, etc.  alldeps.mak is a file that contains
# "make" variable definitions for all ALLDEPFILES, ALLDEPFILES_MAINDIR,
# ALLDEPFILES_SUBDIR, ALLPARAM, and ALLCONFIG, all cadged from the current
# contents of the config subdirectory.

alldeps.mak: ${srcdir}/config
	rm -f alldeps.mak alldeps.tmp allparam.tmp allconfig.tmp
	for i in `ls -d ${srcdir}/config/*.m[ht]` ; do \
	  echo $$i >>allconfig.tmp; \
	  awk <$$i ' \
	    $$1 == "TDEPFILES=" || $$1 == "XDEPFILES=" { \
	      for (i = 2; i <= NF; i++) \
	        print $$i >> "alldeps.tmp" ; \
            } \
	    $$1 == "TM_FILE=" || $$1 == "XM_FILE=" { \
	      print $$2 >> "allparam.tmp" }' ; \
	done
	sort <alldeps.tmp | uniq | \
	  sed -e 's/arm-convert.o/arm-convert.s/' \
	      -e 's!^Onindy.o!nindy-share/Onindy.c!' \
	      -e 's!^nindy.o!nindy-share/nindy.c!' \
	      -e 's!ttybreak.o!nindy-share/ttybreak.c!' \
	      -e 's!ttyflush.o!nindy-share/ttyflush.c!' \
	      -e 's!xdr_ld.o!vx-share/xdr_ld.c!' \
	      -e 's!xdr_ptrace.o!vx-share/xdr_ptrace.c!' \
	      -e 's!xdr_rdb.o!vx-share/xdr_rdb.c!' \
	      -e 's!xdr_regs.o!vx-share/xdr_regs.c!' \
	      -e 's/\.o/.c/' \
	    >alldeps2.tmp
	echo '# Start of "alldeps.mak" definitions' \
	    >>alldeps.mak;
	echo 'ALLDEPFILES = $$(ALLDEPFILES_MAINDIR) $$(ALLDEPFILES_SUBDIR)' \
	    >>alldeps.mak;
	grep -v / alldeps2.tmp | \
	  awk 'BEGIN {printf "ALLDEPFILES_MAINDIR="} \
	    NR == 0 {printf $$0;} \
	    NR != 0 {printf "\\\n" $$0} \
	    END {printf "\n\n"}' >>alldeps.mak;
	grep / alldeps2.tmp | \
	  awk 'BEGIN {printf "ALLDEPFILES_SUBDIR="} \
	    NR == 0 {printf $$0;} \
	    NR != 0 {printf "\\\n" $$0} \
	    END {printf "\n\n"}' >>alldeps.mak;
	sort <allparam.tmp | uniq | awk 'BEGIN {printf "ALLPARAM="} \
	    NR == 0 {printf $$0;} \
	    NR != 0 {printf "\\\n" $$0} \
	    END {printf "\n\n"}' >>alldeps.mak;
	sort <allconfig.tmp | uniq | awk 'BEGIN {printf "ALLCONFIG="} \
	    NR == 0 {printf $$0;} \
	    NR != 0 {printf "\\\n" $$0} \
	    END {printf "\n\n"}' >>alldeps.mak;
	echo '# End of "alldeps.mak" definitions' \
	    >>alldeps.mak;
	rm -f alldeps.tmp alldeps2.tmp allparam.tmp allconfig.tmp

ALL_OBJ = $(OBS) $(TSOBS) xgdb.o ${ADD_DEPS} ${CDEPS}
REALSOURCE = $(ALL_OBJ:.o=.c)
depend: 
	mkdep $(CFLAGS) $(REALSOURCE)

config.status:
	@echo "You must configure gdb.  Look at the README file for details."
	@false

# These are not generated by "make depend" because they only are there
# for some machines.
# But these rules don't do what we want; we want to hack the foo.o: tm.h
# dependency to do the right thing.
tm-isi.h tm-sun3.h tm-news.h tm-hp300bsd.h tm-altos.h: tm-68k.h
tm-hp300hpux.h tm-sun2.h tm-3b1.h: tm-68k.h
xm-news1000.h: xm-news.h
xm-i386-sv32.h: xm-i386.h
tm-i386gas.h: tm-i386.h
xm-sun4os4.h: xm-sparc.h
tm-sun4os4.h: tm-sparc.h
xm-vaxult.h: xm-vax.h
xm-vaxbsd.h: xm-vax.h

kdb: $(NTSSTART) $(OBS) $(NTSOBS) ${ADD_DEPS} ${CDEPS}
	rm -f init.c
	$(srcdir)/munch ${MUNCH_DEFINE} $(OBS) $(NTSOBS) > init.c
	$(CC) $(LDFLAGS) -c init.c $(CLIBS) 
	ld -o kdb $(NTSSTART) $(OBS) $(NTSOBS) init.o $(ADD_FILES) \
	  -lc $(CLIBS)

# Put the proper machine-specific files first.
# createtags will edit the .o in DEPFILES into .c
TAGS: ${TAGFILES}
	$(srcdir)/createtags $(TM_FILE) ${XM_FILE} $(DEPFILES) ${TAGFILES}
tags: TAGS

# Making distributions of GDB and friends.

# Make a directory `proto-gdb.dir' that contains an image of the GDB
# directory of the distribution, built up with symlinks.
make-proto-gdb.dir: force_update 
	$(MAKE) $(MFLAGS) -f Makefile.in setup-to-dist
	$(MAKE) $(MFLAGS) -f Makefile    make-proto-gdb-1

# Make a tar file containing the GDB directory of the distribution.
gdb.tar.Z: force_update
	$(MAKE) $(MFLAGS) -f Makefile.in setup-to-dist
	$(MAKE) $(MFLAGS) -f Makefile.in gdb-$(VERSION).tar.Z

# Set up the GDB directory for distribution, by building all files that
# are products of other files.
setup-to-dist: force_update
	../configure none
	rm -f alldeps.mak
	$(MAKE) $(MFLAGS) alldeps.mak
	../configure none
	rm -f depend
	$(MAKE) $(MFLAGS) depend
	../configure none
	(cd doc; $(MAKE) $(MFLAGS) gdbVN.m4)
	$(MAKE) $(MFLAGS) gdb.info
	$(MAKE) $(MFLAGS) refcard.ps

# Build a tar file from a proto-gdb.dir.
gdb-$(VERSION).tar.Z: force_update
	rm -f gdb.tar gdb-$(VERSION).tar.Z
	$(MAKE) $(MFLAGS) -f Makefile    make-proto-gdb-1
	ln -s proto-gdb.dir $(DIST)
	tar chf - $(DIST) | compress >gdb-$(VERSION).tar.Z
	rm -rf $(DIST) proto-gdb.dir

# Build a proto-gdb.dir after GDB has been set up for distribution.
# This stuff must be run in `Makefile', not `Makefile.in`; we use the makefile
# built in the setup-to-dist process, since it defines things like ALLCONFIG
# and ALLDEPFILES, that we need.
make-proto-gdb-1: ${TARFILES} ${TARDIRS} gdb.info
	rm -rf proto-gdb.dir
	mkdir proto-gdb.dir
	cd proto-gdb.dir ; for i in ${TARFILES} ; do ln -s ../$$i . ; done
	cd proto-gdb.dir ; ln -s ../${INFOFILES} .
	cd proto-gdb.dir ; for i in ${TARDIRS}; do \
	  (mkdir $$i; cd $$i; \
	  ln -s ../../$$i/* .; \
	  rm -rf SCCS CVS.adm RCS config.status); done
	mkdir proto-gdb.dir/config
	cd proto-gdb.dir/config ; \
	  for i in $(ALLCONFIG) ; do ln -s ../../$$i ../$$i ; done
	mkdir proto-gdb.dir/vx-share proto-gdb.dir/nindy-share
	cd proto-gdb.dir/config ; \
	  for i in $(SFILES_SUBDIR) $(NONSRC_SUBDIR) $(ALLDEPFILES_SUBDIR); \
	    do ln -s ../../$$i ../$$i ; done
	chmod og=u `find . -print`

clean:
	rm -f *.o ${ADD_FILES}
	rm -f init.c version.c
	rm -f gdb core gdb.tar gdb.tar.Z make.log
	rm -f gdb[0-9]
	rm -f xgdb.o xgdb
	$(MAKE) subdir_do DO=clean "DODIRS=$(SUBDIRS)"

distclean: clean c-exp.tab.c m2-exp.tab.c TAGS
	rm -f tm.h xm.h config.status
	rm -f y.output yacc.acts yacc.tmp
	rm -f ${TESTS} Makefile depend
	$(MAKE) subdir_do DO=distclean "DODIRS=$(SUBDIRS)"

realclean: clean
	rm -f c-exp.tab.c m2-exp.tab.c TAGS
	rm -f tm.h xm.h config.status
	rm -f Makefile depend
	$(MAKE) subdir_do DO=realclean "DODIRS=$(SUBDIRS)"

STAGESTUFF=${OBS} ${TSOBS} ${NTSOBS} ${ADD_FILES} init.c init.o version.c gdb

subdir_do: force
	for i in $(DODIRS); do \
		if [ -d ./$$i ] ; then \
			if (cd ./$$i; \
				$(MAKE) \
					"against=$(against)" \
					"AR=$(AR)" \
					"AR_FLAGS=$(AR_FLAGS)" \
					"CC=$(CC)" \
					"RANLIB=$(RANLIB)" \
					"MAKEINFO=$(MAKEINFO)" \
					"BISON=$(BISON)" $(DO)) ; then true ; \
			else exit 1 ; fi ; \
		else true ; fi ; \
	done

# Copy the object files from a particular stage into a subdirectory.
stage1: force
	-mkdir stage1
	-mv -f $(STAGESTUFF) stage1
	$(MAKE) subdir_do DO=stage1 "DODIRS=$(SUBDIRS)"

stage2: force
	-mkdir stage2
	-mv -f $(STAGESTUFF) stage2
	$(MAKE) subdir_do DO=stage2 "DODIRS=$(SUBDIRS)"

stage3: force
	-mkdir stage3
	-mv -f $(STAGESTUFF) stage3
	$(MAKE) subdir_do DO=stage3 "DODIRS=$(SUBDIRS)"

against=stage2

comparison: force
	for i in $(STAGESTUFF) ; do cmp $$i $(against)/$$i ; done
	$(MAKE) subdir_do DO=comparison "DODIRS=$(SUBDIRS)"

de-stage1: force
	-(cd stage1 ; mv -f * ..)
	-rmdir stage1
	$(MAKE) subdir_do DO=de-stage1 "DODIRS=$(SUBDIRS)"

de-stage2: force
	-(cd stage2 ; mv -f * ..)
	-rmdir stage2
	$(MAKE) subdir_do DO=de-stage2 "DODIRS=$(SUBDIRS)"

de-stage3: force
	-(cd stage3 ; mv -f * ..)
	-rmdir stage3
	$(MAKE) subdir_do DO=de-stage3 "DODIRS=$(SUBDIRS)"

Makefile: $(srcdir)/Makefile.in $(host_makefile_frag) $(target_makefile_frag)
	$(SHELL) ./config.status

force:

# Documentation!
# GDB QUICK REFERENCE (TeX dvi file, CM fonts)
refcard.dvi: $(srcdir)/doc/refcard.tex
	( cd ./doc; $(MAKE) refcard.dvi )
	mv ./doc/refcard.dvi .

# GDB QUICK REFERENCE (PostScript output, common PS fonts)
refcard.ps: $(srcdir)/doc/refcard.tex
	( cd ./doc; $(MAKE) refcard.ps )
	mv ./doc/refcard.ps .

# GDB MANUAL: TeX dvi file
gdb.dvi: ./doc/gdb-all.texi
	( cd ./doc; $(MAKE) M4=$(M4) gdb.dvi )
	mv ./doc/gdb.dvi .

# GDB MANUAL: info file
gdb.info: ./doc/gdb-all.texi
	( cd ./doc; $(MAKE) M4=$(M4) gdb.info )
	mv ./doc/gdb.info* .

./doc/gdb-all.texi:
	(cd $(srcdir)/doc; $(MAKE) M4=$(M4) gdb-all.texi)

# Make copying.c from COPYING
copying.c: ${srcdir}/COPYING ${srcdir}/copying.awk
	awk -f ${srcdir}/copying.awk < ${srcdir}/COPYING > copying.c

version.c: Makefile.in
	echo 'char *version = "$(VERSION)";' >version.c

# c-exp.tab.c is generated in target dir from c-exp.y if it doesn't exist
# in srcdir, then compiled in target dir to c-exp.tab.o.
c-exp.tab.o: c-exp.tab.c
c-exp.tab.c: $(srcdir)/c-exp.y
	@echo 'Expect 4 shift/reduce conflicts.'
	${YACC} $(srcdir)/c-exp.y
	-mv y.tab.c c-exp.tab.c

# m2-exp.tab.c is generated in target dir from m2-exp.y if it doesn't exist
# in srcdir, then compiled in target dir to m2-exp.tab.o.
m2-exp.tab.o: m2-exp.tab.c
m2-exp.tab.c: $(srcdir)/m2-exp.y
	${YACC} $(srcdir)/m2-exp.y
	-mv y.tab.c m2-exp.tab.c

xgdb.o: defs.h symtab.h frame.h
sparcbsd-tdep.o: sparcbsd-tdep.c
	${CC} -c -I/usr/src/4bsd/inc-hack ${CFLAGS} $<

# The symbol-file readers have dependencies on BFD header files.
dbxread.o: ${srcdir}/dbxread.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/dbxread.c

coffread.o: ${srcdir}/coffread.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/coffread.c

mipsread.o: ${srcdir}/mipsread.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/mipsread.c

elfread.o: ${srcdir}/elfread.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/elfread.c

dwarfread.o: ${srcdir}/dwarfread.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/dwarfread.c

xcoffread.o: ${srcdir}/xcoffread.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/xcoffread.c

xcoffexec.o: ${srcdir}/xcoffexec.c
	${CC} -c ${CFLAGS} -I$(BFD_DIR) ${srcdir}/xcoffexec.c

# Drag in the files that are in another directory.

xdr_ld.o: ${srcdir}/vx-share/xdr_ld.c
	${CC} -c ${CFLAGS} ${srcdir}/vx-share/xdr_ld.c

xdr_ptrace.o: ${srcdir}/vx-share/xdr_ptrace.c
	${CC} -c ${CFLAGS} ${srcdir}/vx-share/xdr_ptrace.c

xdr_rdb.o: ${srcdir}/vx-share/xdr_rdb.c
	${CC} -c ${CFLAGS} ${srcdir}/vx-share/xdr_rdb.c

xdr_regs.o: ${srcdir}/vx-share/xdr_regs.c
	${CC} -c ${CFLAGS} ${srcdir}/vx-share/xdr_regs.c

nindy.o: ${srcdir}/nindy-share/nindy.c
	${CC} -c ${CFLAGS} ${srcdir}/nindy-share/nindy.c

Onindy.o: ${srcdir}/nindy-share/Onindy.c
	${CC} -c ${CFLAGS} ${srcdir}/nindy-share/Onindy.c

ttybreak.o: ${srcdir}/nindy-share/ttybreak.c
	${CC} -c ${CFLAGS} ${srcdir}/nindy-share/ttybreak.c

ttyflush.o: ${srcdir}/nindy-share/ttyflush.c
	${CC} -c ${CFLAGS} ${srcdir}/nindy-share/ttyflush.c

lint: $(LINTFILES)
	$(LINT) $(INCLUDE_CFLAGS) $(LINTFLAGS) $(LINTFILES) \
	   `echo ${DEPFILES} | sed 's/\.o /\.c /g'

gdb.cxref: $(SFILES)
	cxref -I. $(SFILES) >gdb.cxref

force_update:

# When used with GDB, the demangler should never look for leading
# underscores because GDB strips them off during symbol read-in.  Thus
# -Dnounderscore.  

cplus-dem.o: cplus-dem.c
	${CC} -c ${CFLAGS} -Dnounderscore \
	  `echo ${srcdir}/cplus-dem.c | sed 's,^\./,,'`

# GNU Make has an annoying habit of putting *all* the Makefile variables
# into the environment, unless you include this target as a circumvention.
# Rumor is that this will be fixed (and this target can be removed)
# in GNU Make 4.0.
.NOEXPORT:

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

main.o: main.c /usr/include/stdio.h /usr/include/sys/param.h
main.o: /usr/include/machine/param.h /usr/include/sys/signal.h
main.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
main.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
main.o: /usr/include/sys/sysmacros.h defs.h /usr/include/stdio.h
main.o: ../include/ansidecl.h /usr/include/errno.h /usr/include/sys/errno.h
main.o: xm.h xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
main.o: tm-sparc.h tm-sunos.h solib.h gdbcmd.h command.h call-cmds.h gdbcore.h
main.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h symtab.h
main.o: ../include/obstack.h inferior.h symtab.h breakpoint.h frame.h value.h
main.o: symtab.h gdbtypes.h expression.h frame.h signals.h
main.o: /usr/include/signal.h /usr/include/sys/signal.h target.h
main.o: ../include/bfd.h breakpoint.h gdbtypes.h expression.h language.h
main.o: expression.h ../include/getopt.h ../readline/readline.h
main.o: ../readline/keymaps.h ../readline/chardefs.h ../readline/history.h
main.o: /usr/include/string.h /usr/include/sys/stdtypes.h
main.o: /usr/include/sys/file.h /usr/include/sys/types.h
main.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
main.o: /usr/include/sys/stat.h /usr/include/sys/types.h /usr/include/setjmp.h
main.o: /usr/include/machine/setjmp.h /usr/include/sys/stat.h
main.o: /usr/include/ctype.h /usr/include/sys/time.h /usr/include/time.h
main.o: /usr/include/sys/stdtypes.h /usr/include/sys/resource.h
main.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h /usr/include/sgtty.h
main.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
main.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
main.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
main.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
main.o: /usr/include/sys/sockio.h /usr/include/sys/ioccom.h
main.o: /usr/include/sys/ioctl.h
blockframe.o: blockframe.c defs.h /usr/include/stdio.h ../include/ansidecl.h
blockframe.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
blockframe.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
blockframe.o: tm-sunos.h solib.h symtab.h ../include/obstack.h ../include/bfd.h
blockframe.o: ../include/ansidecl.h ../include/obstack.h symfile.h objfiles.h
blockframe.o: frame.h gdbcore.h ../include/bfd.h value.h symtab.h gdbtypes.h
blockframe.o: expression.h target.h ../include/bfd.h inferior.h symtab.h
blockframe.o: breakpoint.h frame.h value.h frame.h
breakpoint.o: breakpoint.c defs.h /usr/include/stdio.h ../include/ansidecl.h
breakpoint.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
breakpoint.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
breakpoint.o: tm-sunos.h solib.h /usr/include/ctype.h symtab.h
breakpoint.o: ../include/obstack.h frame.h breakpoint.h frame.h value.h
breakpoint.o: symtab.h gdbtypes.h expression.h gdbtypes.h expression.h
breakpoint.o: gdbcore.h ../include/bfd.h ../include/ansidecl.h
breakpoint.o: ../include/obstack.h gdbcmd.h command.h value.h
breakpoint.o: /usr/include/ctype.h command.h inferior.h symtab.h breakpoint.h
breakpoint.o: frame.h target.h ../include/bfd.h language.h
breakpoint.o: /usr/include/string.h /usr/include/sys/stdtypes.h
findvar.o: findvar.c defs.h /usr/include/stdio.h ../include/ansidecl.h
findvar.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
findvar.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
findvar.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h frame.h
findvar.o: value.h symtab.h gdbtypes.h expression.h gdbcore.h ../include/bfd.h
findvar.o: ../include/ansidecl.h ../include/obstack.h inferior.h symtab.h
findvar.o: breakpoint.h frame.h value.h frame.h target.h ../include/bfd.h
stack.o: stack.c defs.h /usr/include/stdio.h ../include/ansidecl.h
stack.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
stack.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
stack.o: tm-sunos.h solib.h value.h symtab.h ../include/obstack.h gdbtypes.h
stack.o: expression.h symtab.h gdbtypes.h expression.h language.h frame.h
stack.o: gdbcmd.h command.h gdbcore.h ../include/bfd.h ../include/ansidecl.h
stack.o: ../include/obstack.h target.h ../include/bfd.h breakpoint.h frame.h
stack.o: value.h
source.o: source.c defs.h /usr/include/stdio.h ../include/ansidecl.h
source.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
source.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
source.o: tm-sunos.h solib.h symtab.h ../include/obstack.h expression.h
source.o: language.h command.h gdbcmd.h command.h frame.h /usr/include/string.h
source.o: /usr/include/sys/stdtypes.h /usr/include/sys/param.h
source.o: /usr/include/machine/param.h /usr/include/sys/signal.h
source.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
source.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
source.o: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
source.o: /usr/include/sys/types.h /usr/include/fcntl.h
source.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
source.o: /usr/include/sys/stat.h gdbcore.h ../include/bfd.h
source.o: ../include/ansidecl.h ../include/obstack.h regex.h symfile.h
source.o: objfiles.h
values.o: values.c defs.h /usr/include/stdio.h ../include/ansidecl.h
values.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
values.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
values.o: tm-sunos.h solib.h /usr/include/string.h /usr/include/sys/stdtypes.h
values.o: symtab.h ../include/obstack.h gdbtypes.h value.h symtab.h gdbtypes.h
values.o: expression.h gdbcore.h ../include/bfd.h ../include/ansidecl.h
values.o: ../include/obstack.h frame.h command.h gdbcmd.h command.h target.h
values.o: ../include/bfd.h
eval.o: eval.c defs.h /usr/include/stdio.h ../include/ansidecl.h
eval.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
eval.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
eval.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h value.h
eval.o: symtab.h gdbtypes.h expression.h expression.h target.h ../include/bfd.h
eval.o: ../include/ansidecl.h ../include/obstack.h frame.h
valops.o: valops.c defs.h /usr/include/stdio.h ../include/ansidecl.h
valops.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
valops.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
valops.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h value.h
valops.o: symtab.h gdbtypes.h expression.h frame.h inferior.h symtab.h
valops.o: breakpoint.h frame.h value.h frame.h gdbcore.h ../include/bfd.h
valops.o: ../include/ansidecl.h ../include/obstack.h target.h ../include/bfd.h
valops.o: /usr/include/errno.h
valarith.o: valarith.c defs.h /usr/include/stdio.h ../include/ansidecl.h
valarith.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
valarith.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
valarith.o: tm-sunos.h solib.h value.h symtab.h ../include/obstack.h gdbtypes.h
valarith.o: expression.h symtab.h gdbtypes.h expression.h target.h
valarith.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
valarith.o: /usr/include/string.h /usr/include/sys/stdtypes.h
valprint.o: valprint.c defs.h /usr/include/stdio.h ../include/ansidecl.h
valprint.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
valprint.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
valprint.o: tm-sunos.h solib.h /usr/include/string.h
valprint.o: /usr/include/sys/stdtypes.h symtab.h ../include/obstack.h
valprint.o: gdbtypes.h value.h symtab.h gdbtypes.h expression.h gdbcore.h
valprint.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
valprint.o: gdbcmd.h command.h target.h ../include/bfd.h ../include/obstack.h
valprint.o: language.h /usr/include/errno.h
printcmd.o: printcmd.c defs.h /usr/include/stdio.h ../include/ansidecl.h
printcmd.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
printcmd.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
printcmd.o: tm-sunos.h solib.h /usr/include/string.h
printcmd.o: /usr/include/sys/stdtypes.h frame.h symtab.h ../include/obstack.h
printcmd.o: gdbtypes.h value.h symtab.h gdbtypes.h expression.h language.h
printcmd.o: expression.h gdbcore.h ../include/bfd.h ../include/ansidecl.h
printcmd.o: ../include/obstack.h gdbcmd.h command.h target.h ../include/bfd.h
printcmd.o: breakpoint.h frame.h value.h
symtab.o: symtab.c defs.h /usr/include/stdio.h ../include/ansidecl.h
symtab.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
symtab.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
symtab.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h gdbcore.h
symtab.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h frame.h
symtab.o: target.h ../include/bfd.h value.h symtab.h gdbtypes.h expression.h
symtab.o: symfile.h objfiles.h gdbcmd.h command.h call-cmds.h regex.h
symtab.o: expression.h language.h ../include/obstack.h /usr/include/assert.h
symtab.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
symtab.o: /usr/include/sys/sysmacros.h /usr/include/fcntl.h
symtab.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
symtab.o: /usr/include/sys/stat.h /usr/include/sys/types.h
symtab.o: /usr/include/string.h /usr/include/sys/stdtypes.h
symtab.o: /usr/include/sys/stat.h /usr/include/ctype.h
symfile.o: symfile.c defs.h /usr/include/stdio.h ../include/ansidecl.h
symfile.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
symfile.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
symfile.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h
symfile.o: gdbcore.h ../include/bfd.h ../include/ansidecl.h
symfile.o: ../include/obstack.h frame.h target.h ../include/bfd.h value.h
symfile.o: symtab.h gdbtypes.h expression.h symfile.h objfiles.h gdbcmd.h
symfile.o: command.h breakpoint.h frame.h value.h ../include/obstack.h
symfile.o: /usr/include/assert.h /usr/include/sys/types.h
symfile.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
symfile.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
symfile.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
symfile.o: /usr/include/sys/types.h /usr/include/string.h
symfile.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
symfile.o: /usr/include/ctype.h
symmisc.o: symmisc.c defs.h /usr/include/stdio.h ../include/ansidecl.h
symmisc.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
symmisc.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
symmisc.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h
symmisc.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
symmisc.o: symfile.h objfiles.h breakpoint.h frame.h value.h symtab.h
symmisc.o: gdbtypes.h expression.h command.h ../include/obstack.h
symmisc.o: /usr/include/string.h /usr/include/sys/stdtypes.h
infcmd.o: infcmd.c defs.h /usr/include/stdio.h ../include/ansidecl.h
infcmd.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
infcmd.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
infcmd.o: tm-sunos.h solib.h /usr/include/signal.h /usr/include/sys/signal.h
infcmd.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
infcmd.o: /usr/include/sys/param.h /usr/include/machine/param.h
infcmd.o: /usr/include/sys/signal.h /usr/include/sys/types.h
infcmd.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
infcmd.o: /usr/include/string.h /usr/include/sys/stdtypes.h symtab.h
infcmd.o: ../include/obstack.h gdbtypes.h frame.h inferior.h symtab.h
infcmd.o: breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h frame.h
infcmd.o: environ.h value.h gdbcmd.h command.h gdbcore.h ../include/bfd.h
infcmd.o: ../include/ansidecl.h ../include/obstack.h target.h ../include/bfd.h
infrun.o: infrun.c defs.h /usr/include/stdio.h ../include/ansidecl.h
infrun.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
infrun.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
infrun.o: tm-sunos.h solib.h /usr/include/string.h /usr/include/sys/stdtypes.h
infrun.o: symtab.h ../include/obstack.h frame.h inferior.h symtab.h
infrun.o: breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h frame.h
infrun.o: breakpoint.h ../include/wait.h gdbcore.h ../include/bfd.h
infrun.o: ../include/ansidecl.h ../include/obstack.h signame.h command.h
infrun.o: terminal.h /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
infrun.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
infrun.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
infrun.o: /usr/include/sys/sysmacros.h /usr/include/sgtty.h
infrun.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
infrun.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
infrun.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
infrun.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
infrun.o: /usr/include/sys/sockio.h /usr/include/sys/ioccom.h
infrun.o: /usr/include/sys/ioctl.h target.h ../include/bfd.h
infrun.o: /usr/include/signal.h /usr/include/sys/signal.h
infrun.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
infrun.o: /usr/include/sys/file.h /usr/include/sys/types.h
infrun.o: /usr/include/sys/fcntlcom.h /usr/include/sys/time.h
infrun.o: /usr/include/time.h /usr/include/sys/stdtypes.h
infrun.o: /usr/include/sys/resource.h
remote.o: remote.c /usr/include/stdio.h /usr/include/varargs.h
remote.o: /usr/include/signal.h /usr/include/sys/signal.h
remote.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
remote.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
remote.o: /usr/include/sys/sysmacros.h /usr/include/sys/ioctl.h
remote.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
remote.o: /usr/include/sys/ttold.h /usr/include/sys/ioccom.h
remote.o: /usr/include/sys/ttycom.h /usr/include/sys/filio.h
remote.o: /usr/include/sys/ioccom.h /usr/include/sys/sockio.h
remote.o: /usr/include/sys/ioccom.h /usr/include/sys/file.h
remote.o: /usr/include/sys/types.h /usr/include/sys/fcntlcom.h
remote.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
remote.o: /usr/include/sys/types.h defs.h /usr/include/stdio.h
remote.o: ../include/ansidecl.h /usr/include/errno.h /usr/include/sys/errno.h
remote.o: xm.h xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
remote.o: tm-sparc.h tm-sunos.h solib.h target.h ../include/bfd.h
remote.o: ../include/ansidecl.h ../include/obstack.h frame.h inferior.h
remote.o: symtab.h ../include/obstack.h breakpoint.h frame.h value.h symtab.h
remote.o: gdbtypes.h expression.h frame.h ../include/wait.h gdbcmd.h command.h
remote.o: remote.h kgdb_proto.h
command.o: command.c defs.h /usr/include/stdio.h ../include/ansidecl.h
command.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
command.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
command.o: tm-sunos.h solib.h gdbcmd.h command.h symtab.h ../include/obstack.h
command.o: value.h symtab.h gdbtypes.h expression.h /usr/include/ctype.h
command.o: /usr/include/string.h /usr/include/sys/stdtypes.h
utils.o: utils.c defs.h /usr/include/stdio.h ../include/ansidecl.h
utils.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
utils.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
utils.o: tm-sunos.h solib.h /usr/include/sys/ioctl.h
utils.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
utils.o: /usr/include/sys/ttold.h /usr/include/sys/ioccom.h
utils.o: /usr/include/sys/ttycom.h /usr/include/sys/filio.h
utils.o: /usr/include/sys/ioccom.h /usr/include/sys/sockio.h
utils.o: /usr/include/sys/ioccom.h /usr/include/sys/param.h
utils.o: /usr/include/machine/param.h /usr/include/sys/signal.h
utils.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
utils.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
utils.o: /usr/include/sys/sysmacros.h /usr/include/pwd.h
utils.o: /usr/include/sys/types.h /usr/include/varargs.h /usr/include/ctype.h
utils.o: /usr/include/string.h /usr/include/sys/stdtypes.h signals.h
utils.o: /usr/include/signal.h /usr/include/sys/signal.h gdbcmd.h command.h
utils.o: terminal.h /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
utils.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
utils.o: /usr/include/sys/types.h /usr/include/sgtty.h /usr/include/sys/ioctl.h
utils.o: /usr/include/sys/ioctl.h ../include/bfd.h ../include/ansidecl.h
utils.o: ../include/obstack.h target.h ../include/bfd.h
expprint.o: expprint.c defs.h /usr/include/stdio.h ../include/ansidecl.h
expprint.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
expprint.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
expprint.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h
expprint.o: expression.h value.h symtab.h gdbtypes.h expression.h language.h
expprint.o: parser-defs.h
environ.o: environ.c defs.h /usr/include/stdio.h ../include/ansidecl.h
environ.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
environ.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
environ.o: tm-sunos.h solib.h environ.h /usr/include/string.h
environ.o: /usr/include/sys/stdtypes.h defs.h
version.o: version.c
gdbtypes.o: gdbtypes.c defs.h /usr/include/stdio.h ../include/ansidecl.h
gdbtypes.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
gdbtypes.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
gdbtypes.o: tm-sunos.h solib.h /usr/include/string.h
gdbtypes.o: /usr/include/sys/stdtypes.h ../include/bfd.h ../include/ansidecl.h
gdbtypes.o: ../include/obstack.h symtab.h ../include/obstack.h symfile.h
gdbtypes.o: objfiles.h gdbtypes.h expression.h language.h target.h
gdbtypes.o: ../include/bfd.h value.h symtab.h gdbtypes.h expression.h
copying.o: copying.c /usr/include/stdio.h defs.h /usr/include/stdio.h
copying.o: ../include/ansidecl.h /usr/include/errno.h /usr/include/sys/errno.h
copying.o: xm.h xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
copying.o: tm-sparc.h tm-sunos.h solib.h command.h gdbcmd.h command.h
exec.o: exec.c defs.h /usr/include/stdio.h ../include/ansidecl.h
exec.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
exec.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
exec.o: tm-sunos.h solib.h frame.h inferior.h symtab.h ../include/obstack.h
exec.o: breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h frame.h
exec.o: target.h ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
exec.o: gdbcmd.h command.h /usr/include/sys/param.h
exec.o: /usr/include/machine/param.h /usr/include/sys/signal.h
exec.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
exec.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
exec.o: /usr/include/sys/sysmacros.h /usr/include/fcntl.h
exec.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
exec.o: /usr/include/sys/stat.h /usr/include/sys/types.h /usr/include/string.h
exec.o: /usr/include/sys/stdtypes.h gdbcore.h ../include/bfd.h
exec.o: /usr/include/ctype.h /usr/include/sys/stat.h
sparc-tdep.o: sparc-tdep.c /usr/include/stdio.h defs.h /usr/include/stdio.h
sparc-tdep.o: ../include/ansidecl.h /usr/include/errno.h
sparc-tdep.o: /usr/include/sys/errno.h xm.h xm-sparc.h /usr/include/alloca.h
sparc-tdep.o: tm.h /usr/include/sun4/reg.h tm-sparc.h tm-sunos.h solib.h
sparc-tdep.o: frame.h target.h ../include/bfd.h ../include/ansidecl.h
sparc-tdep.o: ../include/obstack.h /usr/include/kvm.h /usr/include/sys/param.h
sparc-tdep.o: /usr/include/machine/param.h /usr/include/sys/signal.h
sparc-tdep.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
sparc-tdep.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
sparc-tdep.o: /usr/include/sys/sysmacros.h /usr/include/sys/dir.h
sparc-tdep.o: /usr/include/sys/user.h /usr/include/sys/param.h
sparc-tdep.o: /usr/include/machine/pcb.h /usr/include/machine/reg.h
sparc-tdep.o: /usr/include/machine/reg.h /usr/include/sys/time.h
sparc-tdep.o: /usr/include/time.h /usr/include/sys/stdtypes.h
sparc-tdep.o: /usr/include/sys/resource.h /usr/include/sys/exec.h
sparc-tdep.o: /usr/include/sys/proc.h /usr/include/sys/user.h
sparc-tdep.o: /usr/include/sys/session.h /usr/include/sys/types.h
sparc-tdep.o: /usr/include/sys/ucred.h /usr/include/sys/label.h
sparc-tdep.o: /usr/include/sys/audit.h /usr/include/errno.h
sparc-tdep.o: /usr/include/signal.h /usr/include/sys/signal.h
sparc-tdep.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
sparc-tdep.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
sparc-tdep.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
sparc-tdep.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
sparc-tdep.o: /usr/include/sys/sockio.h /usr/include/sys/ioccom.h
sparc-tdep.o: /usr/include/machine/reg.h /usr/include/fcntl.h
sparc-tdep.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
sparc-tdep.o: /usr/include/sys/stat.h /usr/include/sys/types.h symtab.h
sparc-tdep.o: ../include/obstack.h
sparc-tcmn.o: sparc-tcmn.c /usr/include/sys/param.h
sparc-tcmn.o: /usr/include/machine/param.h /usr/include/sys/signal.h
sparc-tcmn.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
sparc-tcmn.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
sparc-tcmn.o: /usr/include/sys/sysmacros.h /usr/include/stdio.h defs.h
sparc-tcmn.o: /usr/include/stdio.h ../include/ansidecl.h /usr/include/errno.h
sparc-tcmn.o: /usr/include/sys/errno.h xm.h xm-sparc.h /usr/include/alloca.h
sparc-tcmn.o: tm.h /usr/include/sun4/reg.h tm-sparc.h tm-sunos.h solib.h
sparc-tcmn.o: target.h ../include/bfd.h ../include/ansidecl.h
sparc-tcmn.o: ../include/obstack.h frame.h inferior.h symtab.h
sparc-tcmn.o: ../include/obstack.h breakpoint.h frame.h value.h symtab.h
sparc-tcmn.o: gdbtypes.h expression.h frame.h value.h
sparc-pinsn.o: sparc-pinsn.c defs.h /usr/include/stdio.h ../include/ansidecl.h
sparc-pinsn.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
sparc-pinsn.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
sparc-pinsn.o: tm-sunos.h solib.h symtab.h ../include/obstack.h
sparc-pinsn.o: ../include/opcode/sparc.h gdbcore.h ../include/bfd.h
sparc-pinsn.o: ../include/ansidecl.h ../include/obstack.h /usr/include/string.h
sparc-pinsn.o: /usr/include/sys/stdtypes.h target.h ../include/bfd.h
solib.o: solib.c defs.h /usr/include/stdio.h ../include/ansidecl.h
solib.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
solib.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
solib.o: tm-sunos.h solib.h /usr/include/sys/types.h
solib.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
solib.o: /usr/include/signal.h /usr/include/sys/signal.h
solib.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
solib.o: /usr/include/string.h /usr/include/sys/stdtypes.h /usr/include/link.h
solib.o: /usr/include/sys/param.h /usr/include/machine/param.h
solib.o: /usr/include/sys/signal.h /usr/include/sys/types.h
solib.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
solib.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
solib.o: /usr/include/sys/types.h /usr/include/a.out.h
solib.o: /usr/include/machine/a.out.h /usr/include/sys/exec.h symtab.h
solib.o: ../include/obstack.h ../include/bfd.h ../include/ansidecl.h
solib.o: ../include/obstack.h symfile.h objfiles.h gdbcore.h ../include/bfd.h
solib.o: command.h target.h ../include/bfd.h frame.h regex.h inferior.h
solib.o: symtab.h breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h
solib.o: frame.h
infptrace.o: infptrace.c defs.h /usr/include/stdio.h ../include/ansidecl.h
infptrace.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
infptrace.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
infptrace.o: tm-sunos.h solib.h frame.h inferior.h symtab.h
infptrace.o: ../include/obstack.h breakpoint.h frame.h value.h symtab.h
infptrace.o: gdbtypes.h expression.h frame.h target.h ../include/bfd.h
infptrace.o: ../include/ansidecl.h ../include/obstack.h
infptrace.o: /usr/include/sys/param.h /usr/include/machine/param.h
infptrace.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
infptrace.o: /usr/include/sys/stdtypes.h /usr/include/sys/types.h
infptrace.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
infptrace.o: /usr/include/sys/dir.h /usr/include/signal.h
infptrace.o: /usr/include/sys/signal.h /usr/include/sys/ioctl.h
infptrace.o: /usr/include/sys/ttychars.h /usr/include/sys/ttydev.h
infptrace.o: /usr/include/sys/ttold.h /usr/include/sys/ioccom.h
infptrace.o: /usr/include/sys/ttycom.h /usr/include/sys/filio.h
infptrace.o: /usr/include/sys/ioccom.h /usr/include/sys/sockio.h
infptrace.o: /usr/include/sys/ioccom.h /usr/include/sys/ptrace.h gdbcore.h
infptrace.o: ../include/bfd.h /usr/include/sys/file.h /usr/include/sys/types.h
infptrace.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
infptrace.o: /usr/include/sys/stat.h /usr/include/sys/types.h
infptrace.o: /usr/include/sys/stat.h
sparc-xdep.o: sparc-xdep.c /usr/include/stdio.h defs.h /usr/include/stdio.h
sparc-xdep.o: ../include/ansidecl.h /usr/include/errno.h
sparc-xdep.o: /usr/include/sys/errno.h xm.h xm-sparc.h /usr/include/alloca.h
sparc-xdep.o: tm-sparc.h inferior.h symtab.h ../include/obstack.h breakpoint.h
sparc-xdep.o: frame.h value.h symtab.h gdbtypes.h expression.h frame.h target.h
sparc-xdep.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
sparc-xdep.o: /usr/include/sys/param.h /usr/include/machine/param.h
sparc-xdep.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
sparc-xdep.o: /usr/include/sys/stdtypes.h /usr/include/sys/types.h
sparc-xdep.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
sparc-xdep.o: /usr/include/sys/ptrace.h /usr/include/machine/reg.h gdbcore.h
sparc-xdep.o: ../include/bfd.h /usr/include/sys/core.h /usr/include/sys/exec.h
sparc-xdep.o: /usr/include/machine/reg.h tm.h /usr/include/sun4/reg.h
sparc-xdep.o: tm-sparc.h tm-sunos.h solib.h
signame.o: signame.c defs.h /usr/include/stdio.h ../include/ansidecl.h
signame.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
signame.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
signame.o: tm-sunos.h solib.h /usr/include/signal.h /usr/include/sys/signal.h
signame.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h signame.h
cplus-dem.o: cplus-dem.c defs.h /usr/include/stdio.h ../include/ansidecl.h
cplus-dem.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
cplus-dem.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
cplus-dem.o: tm-sunos.h solib.h /usr/include/ctype.h /usr/include/strings.h
mem-break.o: mem-break.c defs.h /usr/include/stdio.h ../include/ansidecl.h
mem-break.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
mem-break.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
mem-break.o: tm-sunos.h solib.h symtab.h ../include/obstack.h breakpoint.h
mem-break.o: frame.h value.h symtab.h gdbtypes.h expression.h inferior.h
mem-break.o: symtab.h breakpoint.h frame.h target.h ../include/bfd.h
mem-break.o: ../include/ansidecl.h ../include/obstack.h
target.o: target.c defs.h /usr/include/stdio.h ../include/ansidecl.h
target.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
target.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
target.o: tm-sunos.h solib.h /usr/include/errno.h /usr/include/ctype.h target.h
target.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h gdbcmd.h
target.o: command.h symtab.h ../include/obstack.h inferior.h symtab.h
target.o: breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h frame.h
target.o: ../include/bfd.h symfile.h objfiles.h
inftarg.o: inftarg.c defs.h /usr/include/stdio.h ../include/ansidecl.h
inftarg.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
inftarg.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
inftarg.o: tm-sunos.h solib.h frame.h inferior.h symtab.h ../include/obstack.h
inftarg.o: breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h
inftarg.o: frame.h target.h ../include/bfd.h ../include/ansidecl.h
inftarg.o: ../include/obstack.h ../include/wait.h gdbcore.h ../include/bfd.h
inftarg.o: ieee-float.h
ieee-float.o: ieee-float.c defs.h /usr/include/stdio.h ../include/ansidecl.h
ieee-float.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
ieee-float.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
ieee-float.o: tm-sunos.h solib.h ieee-float.h /usr/include/math.h
ieee-float.o: /usr/include/floatingpoint.h /usr/include/sys/ieeefp.h
putenv.o: putenv.c /usr/include/stdio.h
parse.o: parse.c defs.h /usr/include/stdio.h ../include/ansidecl.h
parse.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
parse.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
parse.o: tm-sunos.h solib.h symtab.h ../include/obstack.h gdbtypes.h frame.h
parse.o: expression.h value.h symtab.h gdbtypes.h expression.h command.h
parse.o: language.h parser-defs.h
language.o: language.c defs.h /usr/include/stdio.h ../include/ansidecl.h
language.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
language.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
language.o: tm-sunos.h solib.h /usr/include/string.h
language.o: /usr/include/sys/stdtypes.h /usr/include/varargs.h symtab.h
language.o: ../include/obstack.h gdbtypes.h value.h symtab.h gdbtypes.h
language.o: expression.h gdbcmd.h command.h frame.h expression.h language.h
language.o: target.h ../include/bfd.h ../include/ansidecl.h
language.o: ../include/obstack.h parser-defs.h
c-exp.tab.o: c-exp.tab.c /usr/include/stdio.h /usr/include/string.h
c-exp.tab.o: /usr/include/sys/stdtypes.h defs.h /usr/include/stdio.h
c-exp.tab.o: ../include/ansidecl.h /usr/include/errno.h
c-exp.tab.o: /usr/include/sys/errno.h xm.h xm-sparc.h /usr/include/alloca.h
c-exp.tab.o: tm.h /usr/include/sun4/reg.h tm-sparc.h tm-sunos.h solib.h
c-exp.tab.o: symtab.h ../include/obstack.h gdbtypes.h frame.h expression.h
c-exp.tab.o: parser-defs.h value.h symtab.h gdbtypes.h expression.h language.h
c-exp.tab.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
c-exp.tab.o: symfile.h objfiles.h /usr/include/stdio.h
m2-exp.tab.o: m2-exp.tab.c /usr/include/stdio.h /usr/include/string.h
m2-exp.tab.o: /usr/include/sys/stdtypes.h defs.h /usr/include/stdio.h
m2-exp.tab.o: ../include/ansidecl.h /usr/include/errno.h
m2-exp.tab.o: /usr/include/sys/errno.h xm.h xm-sparc.h /usr/include/alloca.h
m2-exp.tab.o: tm.h /usr/include/sun4/reg.h tm-sparc.h tm-sunos.h solib.h
m2-exp.tab.o: symtab.h ../include/obstack.h gdbtypes.h frame.h expression.h
m2-exp.tab.o: language.h value.h symtab.h gdbtypes.h expression.h parser-defs.h
m2-exp.tab.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
m2-exp.tab.o: symfile.h objfiles.h /usr/include/stdio.h /usr/include/alloca.h
buildsym.o: buildsym.c defs.h /usr/include/stdio.h ../include/ansidecl.h
buildsym.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
buildsym.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
buildsym.o: tm-sunos.h solib.h ../include/obstack.h symtab.h
buildsym.o: ../include/obstack.h gdbtypes.h breakpoint.h frame.h value.h
buildsym.o: symtab.h gdbtypes.h expression.h gdbcore.h ../include/bfd.h
buildsym.o: ../include/ansidecl.h ../include/obstack.h symfile.h objfiles.h
buildsym.o: ../include/aout/stab_gnu.h ../include/aout/stab.def
buildsym.o: /usr/include/string.h /usr/include/sys/stdtypes.h
buildsym.o: /usr/include/ctype.h buildsym.h
objfiles.o: objfiles.c defs.h /usr/include/stdio.h ../include/ansidecl.h
objfiles.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
objfiles.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
objfiles.o: tm-sunos.h solib.h ../include/bfd.h ../include/ansidecl.h
objfiles.o: ../include/obstack.h symtab.h ../include/obstack.h symfile.h
objfiles.o: objfiles.h /usr/include/sys/types.h /usr/include/sys/stdtypes.h
objfiles.o: /usr/include/sys/sysmacros.h /usr/include/sys/stat.h
objfiles.o: /usr/include/sys/types.h /usr/include/fcntl.h
objfiles.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
objfiles.o: /usr/include/sys/stat.h ../include/obstack.h
minsyms.o: minsyms.c defs.h /usr/include/stdio.h ../include/ansidecl.h
minsyms.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
minsyms.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
minsyms.o: tm-sunos.h solib.h symtab.h ../include/obstack.h ../include/bfd.h
minsyms.o: ../include/ansidecl.h ../include/obstack.h symfile.h objfiles.h
dbxread.o: dbxread.c defs.h /usr/include/stdio.h ../include/ansidecl.h
dbxread.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
dbxread.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
dbxread.o: tm-sunos.h solib.h /usr/include/string.h /usr/include/sys/stdtypes.h
dbxread.o: ../include/obstack.h /usr/include/sys/param.h
dbxread.o: /usr/include/machine/param.h /usr/include/sys/signal.h
dbxread.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
dbxread.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
dbxread.o: /usr/include/sys/sysmacros.h /usr/include/sys/file.h
dbxread.o: /usr/include/sys/types.h /usr/include/sys/fcntlcom.h
dbxread.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
dbxread.o: /usr/include/sys/types.h /usr/include/sys/stat.h
dbxread.o: /usr/include/ctype.h symtab.h ../include/obstack.h breakpoint.h
dbxread.o: frame.h value.h symtab.h gdbtypes.h expression.h command.h target.h
dbxread.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h
dbxread.o: gdbcore.h ../include/bfd.h symfile.h objfiles.h buildsym.h
dbxread.o: ../include/aout/aout64.h ../include/aout/stab_gnu.h
dbxread.o: ../include/aout/stab.def partial-stab.h
kcore.o: kcore.c /usr/include/stdio.h /usr/include/errno.h
kcore.o: /usr/include/sys/errno.h /usr/include/signal.h
kcore.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
kcore.o: /usr/include/sys/stdtypes.h /usr/include/fcntl.h
kcore.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
kcore.o: /usr/include/sys/stat.h /usr/include/sys/types.h
kcore.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h defs.h
kcore.o: /usr/include/stdio.h ../include/ansidecl.h /usr/include/errno.h xm.h
kcore.o: xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
kcore.o: tm-sparc.h tm-sunos.h solib.h frame.h inferior.h symtab.h
kcore.o: ../include/obstack.h breakpoint.h frame.h value.h symtab.h gdbtypes.h
kcore.o: expression.h frame.h symtab.h command.h ../include/bfd.h
kcore.o: ../include/ansidecl.h ../include/obstack.h target.h ../include/bfd.h
kcore.o: gdbcore.h ../include/bfd.h /usr/include/kvm.h /usr/include/sys/stat.h
remote-sl.o: remote-sl.c /usr/include/signal.h /usr/include/sys/signal.h
remote-sl.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h
remote-sl.o: /usr/include/sys/param.h /usr/include/machine/param.h
remote-sl.o: /usr/include/sys/signal.h /usr/include/sys/types.h
remote-sl.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
remote-sl.o: /usr/include/sys/file.h /usr/include/sys/types.h
remote-sl.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
remote-sl.o: /usr/include/sys/stat.h /usr/include/sys/types.h
remote-sl.o: /usr/include/sys/time.h /usr/include/time.h
remote-sl.o: /usr/include/sys/stdtypes.h /usr/include/errno.h
remote-sl.o: /usr/include/sys/errno.h /usr/include/sgtty.h
remote-sl.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
remote-sl.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
remote-sl.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
remote-sl.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
remote-sl.o: /usr/include/sys/sockio.h /usr/include/sys/ioccom.h
remote-sl.o: /usr/include/sys/ioctl.h defs.h /usr/include/stdio.h
remote-sl.o: ../include/ansidecl.h /usr/include/errno.h xm.h xm-sparc.h
remote-sl.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
remote-sl.o: tm-sunos.h solib.h remote-sl.h remote.h
remote-fp.o: remote-fp.c /usr/include/stdio.h /usr/include/signal.h
remote-fp.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
remote-fp.o: /usr/include/sys/stdtypes.h defs.h /usr/include/stdio.h
remote-fp.o: ../include/ansidecl.h /usr/include/errno.h
remote-fp.o: /usr/include/sys/errno.h xm.h xm-sparc.h /usr/include/alloca.h
remote-fp.o: tm.h /usr/include/sun4/reg.h tm-sparc.h tm-sunos.h solib.h frame.h
remote-fp.o: inferior.h symtab.h ../include/obstack.h breakpoint.h frame.h
remote-fp.o: value.h symtab.h gdbtypes.h expression.h frame.h remote.h
remote-fp.o: ../include/wait.h /usr/include/a.out.h
remote-fp.o: /usr/include/machine/a.out.h /usr/include/sys/exec.h
remote-fp.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
remote-fp.o: /usr/include/sys/sysmacros.h /usr/include/sys/file.h
remote-fp.o: /usr/include/sys/types.h /usr/include/sys/fcntlcom.h
remote-fp.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
remote-fp.o: /usr/include/sys/types.h /usr/include/sys/socket.h
remote-fp.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
remote-fp.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
remote-fp.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
remote-fp.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
remote-fp.o: /usr/include/sys/sockio.h /usr/include/sys/ioccom.h
remote-fp.o: /usr/include/sys/time.h /usr/include/time.h
remote-fp.o: /usr/include/sys/stdtypes.h /usr/include/netinet/in.h
remote-fp.o: /usr/include/strings.h /usr/include/netdb.h kgdb_proto.h
kernel.o: kernel.c /usr/include/sys/param.h /usr/include/machine/param.h
kernel.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
kernel.o: /usr/include/sys/stdtypes.h /usr/include/sys/types.h
kernel.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h
kernel.o: /usr/include/ctype.h defs.h /usr/include/stdio.h
kernel.o: ../include/ansidecl.h /usr/include/errno.h /usr/include/sys/errno.h
kernel.o: xm.h xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
kernel.o: tm-sparc.h tm-sunos.h solib.h symtab.h ../include/obstack.h
cmdparse.o: cmdparse.c /usr/include/stdio.h defs.h /usr/include/stdio.h
cmdparse.o: ../include/ansidecl.h /usr/include/errno.h /usr/include/sys/errno.h
cmdparse.o: xm.h xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
cmdparse.o: tm-sparc.h tm-sunos.h solib.h ../readline/readline.h
cmdparse.o: ../readline/keymaps.h ../readline/chardefs.h ../readline/history.h
cmdparse.o: gdbcmd.h command.h symtab.h ../include/obstack.h expression.h
core.o: core.c defs.h /usr/include/stdio.h ../include/ansidecl.h
core.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
core.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
core.o: tm-sunos.h solib.h /usr/include/errno.h /usr/include/signal.h
core.o: /usr/include/sys/signal.h /usr/include/vm/faultcode.h
core.o: /usr/include/sys/stdtypes.h /usr/include/fcntl.h
core.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
core.o: /usr/include/sys/stat.h /usr/include/sys/types.h
core.o: /usr/include/sys/stdtypes.h /usr/include/sys/sysmacros.h frame.h
core.o: inferior.h symtab.h ../include/obstack.h breakpoint.h frame.h value.h
core.o: symtab.h gdbtypes.h expression.h frame.h symtab.h command.h
core.o: ../include/bfd.h ../include/ansidecl.h ../include/obstack.h target.h
core.o: ../include/bfd.h gdbcore.h ../include/bfd.h
inflow.o: inflow.c defs.h /usr/include/stdio.h ../include/ansidecl.h
inflow.o: /usr/include/errno.h /usr/include/sys/errno.h xm.h xm-sparc.h
inflow.o: /usr/include/alloca.h tm.h /usr/include/sun4/reg.h tm-sparc.h
inflow.o: tm-sunos.h solib.h frame.h inferior.h symtab.h ../include/obstack.h
inflow.o: breakpoint.h frame.h value.h symtab.h gdbtypes.h expression.h frame.h
inflow.o: command.h signals.h /usr/include/signal.h /usr/include/sys/signal.h
inflow.o: /usr/include/vm/faultcode.h /usr/include/sys/stdtypes.h terminal.h
inflow.o: /usr/include/fcntl.h /usr/include/sys/fcntlcom.h
inflow.o: /usr/include/sys/stdtypes.h /usr/include/sys/stat.h
inflow.o: /usr/include/sys/types.h /usr/include/sys/stdtypes.h
inflow.o: /usr/include/sys/sysmacros.h /usr/include/sgtty.h
inflow.o: /usr/include/sys/ioctl.h /usr/include/sys/ttychars.h
inflow.o: /usr/include/sys/ttydev.h /usr/include/sys/ttold.h
inflow.o: /usr/include/sys/ioccom.h /usr/include/sys/ttycom.h
inflow.o: /usr/include/sys/filio.h /usr/include/sys/ioccom.h
inflow.o: /usr/include/sys/sockio.h /usr/include/sys/ioccom.h
inflow.o: /usr/include/sys/ioctl.h target.h ../include/bfd.h
inflow.o: ../include/ansidecl.h ../include/obstack.h /usr/include/fcntl.h
inflow.o: /usr/include/sys/param.h /usr/include/machine/param.h
inflow.o: /usr/include/sys/signal.h /usr/include/sys/types.h
inflow.o: /usr/include/signal.h
xgdb.o: xgdb.c /usr/include/sys/types.h /usr/include/sys/stdtypes.h
xgdb.o: /usr/include/sys/sysmacros.h defs.h /usr/include/stdio.h
xgdb.o: ../include/ansidecl.h /usr/include/errno.h /usr/include/sys/errno.h
xgdb.o: xm.h xm-sparc.h /usr/include/alloca.h tm.h /usr/include/sun4/reg.h
xgdb.o: tm-sparc.h tm-sunos.h solib.h symtab.h ../include/obstack.h frame.h
xgdb.o: inferior.h symtab.h breakpoint.h frame.h value.h symtab.h gdbtypes.h
xgdb.o: expression.h frame.h /usr/include/stdio.h /usr/include/X11/IntrinsicP.h
xgdb.o: /usr/include/X11/Intrinsic.h /usr/include/X11/Xlib.h
xgdb.o: /usr/include/sys/types.h /usr/include/X11/X.h
xgdb.o: /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h
xgdb.o: /usr/include/stddef.h /usr/include/sys/stdtypes.h
xgdb.o: /usr/include/X11/Xutil.h /usr/include/X11/Xresource.h
xgdb.o: /usr/include/X11/Xfuncproto.h /usr/include/X11/Xosdefs.h
xgdb.o: /usr/include/string.h /usr/include/sys/stdtypes.h
xgdb.o: /usr/include/X11/Core.h /usr/include/X11/Composite.h
xgdb.o: /usr/include/X11/Constraint.h /usr/include/X11/Object.h
xgdb.o: /usr/include/X11/RectObj.h /usr/include/X11/CoreP.h
xgdb.o: /usr/include/X11/Core.h /usr/include/X11/CompositeP.h
xgdb.o: /usr/include/X11/Composite.h /usr/include/X11/ConstrainP.h
xgdb.o: /usr/include/X11/Constraint.h /usr/include/X11/ObjectP.h
xgdb.o: /usr/include/X11/Object.h /usr/include/X11/RectObjP.h
xgdb.o: /usr/include/X11/RectObj.h /usr/include/X11/ObjectP.h
xgdb.o: /usr/include/X11/StringDefs.h /usr/include/X11/Xaw/Text.h
xgdb.o: /usr/include/X11/Xfuncproto.h /usr/include/X11/Xaw/TextSink.h
xgdb.o: /usr/include/X11/Object.h /usr/include/X11/Xfuncproto.h
xgdb.o: /usr/include/X11/Xaw/TextSrc.h /usr/include/X11/Object.h
xgdb.o: /usr/include/X11/Xfuncproto.h /usr/include/X11/Xaw/AsciiSrc.h
xgdb.o: /usr/include/X11/Xaw/TextSrc.h /usr/include/X11/Xfuncproto.h
xgdb.o: /usr/include/X11/Xaw/AsciiSink.h /usr/include/X11/Xaw/TextSink.h
xgdb.o: /usr/include/X11/Xaw/AsciiSink.h /usr/include/X11/Xaw/AsciiText.h
xgdb.o: /usr/include/X11/Xaw/Text.h /usr/include/X11/Xaw/AsciiSrc.h
xgdb.o: /usr/include/X11/Xaw/Box.h /usr/include/X11/Xmu/Converters.h
xgdb.o: /usr/include/X11/Xfuncproto.h /usr/include/X11/Xaw/Command.h
xgdb.o: /usr/include/X11/Xaw/Label.h /usr/include/X11/Xaw/Simple.h
xgdb.o: /usr/include/X11/Xmu/Converters.h /usr/include/X11/Xaw/Label.h
xgdb.o: /usr/include/X11/Xaw/Paned.h /usr/include/X11/Constraint.h
xgdb.o: /usr/include/X11/Xmu/Converters.h /usr/include/X11/Xfuncproto.h
xgdb.o: /usr/include/ctype.h /usr/include/sys/file.h /usr/include/sys/types.h
xgdb.o: /usr/include/sys/fcntlcom.h /usr/include/sys/stdtypes.h
xgdb.o: /usr/include/sys/stat.h /usr/include/sys/types.h
xgdb.o: /usr/include/sys/errno.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
