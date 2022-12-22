#	@(#) BSDI $Id: Makefile,v 1.2 1993/03/23 02:25:39 polk Exp $

# The Makefile was converted to the standard BSD style Makefile.
# Here is the original copyright information:

# Makefile for gzip (GNU zip)    -*- Indented-Text -*-
# Copyright (C) 1992-1993 Jean-loup Gailly and the Free Software Foundation

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

# Id: Makefile.in,v 0.11 1993/02/04 13:29:42 jloup Exp

#### Start of system configuration section. ####

# Things you might add to DEFS (configure usually figures out what to do):
# -DDIRENT              Use <dirent.h>  for recursion (-r)
# -DSYSDIR              Use <sys/dir.h> for recursion (-r)
# -DSYSNDIR             Use <sys/ndir.h> for recursion (-r)
# -DNDIR                Use <ndir.h> for recursion (-r)
# -DSTDC_HEADERS        Use <stdlib.h>
# -DHAVE_UNISTD_H	Use <unistd.h>
# -DHAVE_UTIME_H	Use <utime.h>
# -DHAVE_SYSUTIME_H	Use <sys/utime.h>
# -DNEED_MEMORY_H       Use <memory.h>
# -DHAVE_STRING_H       Use <string.h> (otherwise use strings.h)
# -DRETSIGTYPE=int      Define this if signal handlers must return an int.
# -DNO_SYMLINK          OS defines S_IFLNK but does not support symbolic links
# -DNO_MULTIPLE_DOTS    System does not allow file names with multiple dots
# -DNO_UTIME		System does not support setting file modification time
# -DNO_CHOWN		System does not support setting file owner
# -DNO_DIR		System does not support readdir()
# -DPROTO		Force function prototypes even if __STDC__ not defined
# -DASMV		Use asm version match.S
# -DMSDOS		MSDOS specific
# -DOS2			OS/2 specific
# -DVMS			Vax/VMS specific
# -DDEBUG		Debug code
# -DDYN_ALLOC		Use dynamic allocation of large data structures
# -DMAXSEG_64K		Maximum array size is 64K (for 16 bit system)
# -DRECORD_IO           read() and write() are rounded to record sizes.
# -DNO_STDIN_FSTAT      fstat() is not available on stdin
# -DNO_SIZE_CHECK       stat() does not give a reliable file size

all:	gzip 
# zcmp zdiff zmore znew gzexe

CFLAGS+=-DASMV -DSTDC_HEADERS=1 -DHAVE_UNISTD_H=1 -DHAVE_STDLIB_H=1 \
	-DHAVE_FCNTL_H=1 -DHAVE_STRING_H=1 -DHAVE_MEMORY_H=1 -DHAVE_UTIME_H=1 \
	-DDIRENT=1 -DRETSIGTYPE=void
PROG=	gzip
SEDCMD=	1d
#MAN1=	gzip.0 zcmp.0 zmore.0 znew.0 gzexe.0
#MLINKS=	gzip.1 gzcat.1 gzip.1 gunzip.1
LINKS=	${BINDIR}/gzip ${BINDIR}/gunzip

NOMAN=noman
#LDADD+=-N
.PATH:	/usr/src/contrib/gzip

SRCS=	gzip.c zip.c deflate.c trees.c bits.c unzip.c inflate.c util.c \
	crypt.c lzw.c unlzw.c unpack.c getopt.c alloca.c match.S

match.o: match.S
	$(CPP) /usr/src/contrib/gzip/match.S > _match.s
	$(CC) -c _match.s
	mv _match.o match.o
	rm -f _match.s

zcmp: zcmp.in
	sed "$(SEDCMD)" $(.CURDIR)/zcmp.in > zcmp
	chmod 775 zcmp

zdiff: zdiff.in
	sed "$(SEDCMD)" $(.CURDIR)/zdiff.in > zdiff
	chmod 775 zdiff

zmore: zmore.in
	sed "$(SEDCMD)" $(.CURDIR)/zmore.in > zmore
	chmod 775 zmore

znew: znew.in
	sed "$(SEDCMD)" $(.CURDIR)/znew.in > znew
	chmod 775 znew

gzexe: gzexe.in
	sed "$(SEDCMD)" $(.CURDIR)/gzexe.in > gzexe
	chmod 775 gzexe

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} zcmp zdiff zmore znew gzexe ${DESTDIR}/${BINDIR}

.include <bsd.prog.mk>
