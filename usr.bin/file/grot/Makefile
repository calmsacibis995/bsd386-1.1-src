# Makefile for file(1) cmd. 
# Copyright (c) Ian F. Darwin 86/09/01 - see LEGAL.NOTICE.
# @(#)=Id: Makefile,v 1.42 1994/01/21 01:28:18 christos Exp =
#
# This software is not subject to any license of the American Telephone
# and Telegraph Company or of the Regents of the University of California.
#
# Permission is granted to anyone to use this software for any purpose on
# any computer system, and to alter it and redistribute it freely, subject
# to the following restrictions:
#
# 1. The author is not responsible for the consequences of use of this
#    software, no matter how awful, even if they arise from flaws in it.
#
# 2. The origin of this software must not be misrepresented, either by
#    explicit claim or by omission.  Since few users ever read sources,
#    credits must appear in the documentation.
#
# 3. Altered versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.  Since few users
#    ever read sources, credits must appear in the documentation.
#
# 4. This notice may not be removed or altered.
VERSION	= 3.13
SHELL	= /bin/sh
#MAGIC	= /etc/magic
MAGIC	= /usr/local/etc/magic
DEFS	= -DMAGIC='"$(MAGIC)"' # -Dvoid=int
CC	= cc
COPTS	= -O -g		# newer compilers allow both; else drop -O
# For truly antique environments, use this for (dummy) include files:
COPTS	= -O # -Ilocalinc
CFLAGS	= $(COPTS) $(DEFS)
LDFLAGS	= $(COPTS) # -Bstatic	# older gdb couldn't handle shared libs
SHAR	= bundle
OFILE	= /usr/bin/file		# old or distributed version, for comparison
# Where new binary lives; typically /usr/local (BSD), /usr/lbin (USG).
BINDIR	= /usr/local/bin
# For installing our man pages; 
# MANCxxx is manual section for Commands, MANFxxx is section for file formats.
# MANxDIR is directory names; MANxEXT is the filename extention. Usual values:
# Variable	V7		4BSD		Sys V
# MANCDIR 	/usr/man/man1	/usr/man/man1	/usr/man/u_man/man1
# MANFDIR 	/usr/man/man5	/usr/man/man5	/usr/man/u_man/man4
# MANCEXT	1		1		1
# MANFEXT	5		5		4
# --- possible alternative for 4BSD ---
# MANCDIR			/usr/local/man/man1
# MANCEXT			1
# or
# MANCDIR			/usr/man/manl
# MANCEXT			l
# --- possible alternative for USG ---
# MANCDIR			/usr/man/local/man1
# MANCEXT			1

MANCDIR	= /usr/local/man/man1
MANCEXT	= 1
MANFDIR	= /usr/local/man/man4
MANFEXT	= 4

# There are no system-dependant configuration options (except maybe CFLAGS).
# Uncomment any of these that is missing from your "standard" library.
LOCALSRCS = # localsrc/getopt.c localsrc/strtol.c \
#		localsrc/strtok.c localsrc/strchr.c
LOCALOBJS = # localsrc/getopt.o localsrc/strtol.o \
#		localsrc/strtok.o localsrc/strchr.o
# These are not compiled in unless you use -Ilocalinc, but
# are not commented out as "make dist" &c use them.
LOCALINC = # localinc/*.h localinc/sys/*.h

SRCS = file.c apprentice.c fsmagic.c softmagic.c ascmagic.c \
	compress.c is_tar.c \
	print.c $(LOCALSRCS) $(LOCALINC)
OBJS = file.o apprentice.o fsmagic.o softmagic.o ascmagic.o \
	compress.o is_tar.o \
	print.o $(LOCALOBJS)

ALLSRC = LEGAL.NOTICE README MAINT PORTING $(SRCS) *.h \
	 Makefile file.man magic.man 
ALLMAGIC =   Magdir/Makefile Magdir/Localstuff Magdir/Header Magdir/[a-z]*

all:		file magic file.${MANCEXT} magic.${MANFEXT}

TESTFILES = * tst/*
try:		all $(OFILE)
		cd tst; $(MAKE)
		time $(OFILE) $(TESTFILES) >/tmp/t1 # can't use ./magic
		time ./file -m ./magic $(TESTFILES) >/tmp/t2
		-diff -b /tmp/t[12]
		what ./file >lastnocore

file:		$(OBJS)
		$(CC) $(CFLAGS) $(OBJS) -o $@
lint:		$(SRCS)
		lint -ha $(DEFS) $(SRCS) | tee $@
magic:		Magdir
#		Magdir/Makefile's "install" mv's magic to here for testing.
		cd Magdir; $(MAKE) install

ascmagic.o:	names.h

compress.o apprentice.o ascmagic.o file.o fsmagic.o print.o softmagic.o: file.h

install:	file magic
		cp file	$(BINDIR)/file
		cp magic $(MAGIC)

install.man: file.${MANCEXT} magic.${MANFEXT}
		cp file.${MANCEXT} $(MANCDIR)/file.$(MANCEXT)
		cp magic.${MANFEXT} $(MANFDIR)/magic.$(MANFEXT)

clean:
		rm -f *.o core file magic lint dist.* MANIFEST \
		      magic.${MANFEXT} file.${MANCEXT}
clobber:
		cd tst; $(MAKE) clean


magic.${MANFEXT} :	Makefile magic.man
		@rm -f $@
		sed -e s@__SECTION__@${MANFEXT}@g \
		    -e s@__MAGIC__@${MAGIC}@g magic.man > $@

file.${MANCEXT} :	Makefile file.man
		@rm -f $@
		sed -e s@__SECTION__@${MANCEXT}@g \
		    -e s@__MAGIC__@${MAGIC}@g file.man > $@

send:		dist
		ftp ftp.cs

dist:		dist.src dist.magic
		@echo Now check this patchlevel!
		ident patchlevel.h

dist.src:	$(ALLSRC) MANIFEST
#		Some versions of shar can't handle a single file from
#		a subdirectory, so we manually insert mkdir as needed.
#		The point is to exclude all the generable targets in tst.
		(echo mkdir localinc localinc/sys localsrc tst; \
			$(SHAR) $(ALLSRC) MANIFEST) > $@

rcsdiff:	$(ALLSRC)
		rcsdiff -q RCS/*

MANIFEST:	$(ALLSRC)
		ident $(ALLSRC) > MANIFEST
dist.magic:	Magdir
#		As above, but to exclude Magdir/RCS from being shipped.
		(echo mkdir Magdir; $(SHAR) $(ALLMAGIC)) >$@

tar:		$(ALLSRC) $(ALLMAGIC)
		-rm -fr file-${VERSION}
		-mkdir file-${VERSION} file-${VERSION}/Magdir
		ln $(ALLSRC) file-${VERSION}
		ln ${ALLMAGIC} file-${VERSION}/Magdir
		tar cvf file-${VERSION}.tar file-${VERSION}
		-rm -fr file-${VERSION}
