# $Id: makefile.sol,v 1.1.1.1 1994/01/13 22:42:31 polk Exp $
#
#   Michael Seibel
#   Networks and Distributed Computing
#   Computing and Communications
#   University of Washington
#   Administration Builiding, AG-44
#   Seattle, Washington, 98195, USA
#   Internet: mikes@cac.washington.edu
#
#   Please address all bugs and comments to "pine-bugs@cac.washington.edu"
#
#   Copyright 1991-1993  University of Washington
#
#    Permission to use, copy, modify, and distribute this software and its
#   documentation for any purpose and without fee to the University of
#   Washington is hereby granted, provided that the above copyright notice
#   appears in all copies and that both the above copyright notice and this
#   permission notice appear in supporting documentation, and that the name
#   of the University of Washington not be used in advertising or publicity
#   pertaining to distribution of the software without specific, written
#   prior permission.  This software is made available "as is", and
#   THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
#   WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
#   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
#   NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
#   INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
#   LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
#   (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
#   WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#  
#   Pine and Pico are trademarks of the University of Washington.
#   No commercial use of these trademarks may be made without prior
#   written permission of the University of Washington.
#

#
# Makefile for SVR4 version of the PINE composer library and 
# stand-alone editor pico.
#
# [Port courtesy of Marc Unangst <mju@mudos.ann-arbor.mi.us> - mss, 921020]
#

#for GNU C
#CC= 		gcc
# LDCC= /usr/bin/cc
# If you don't have /usr/bin/cc (our Solaris 2.2 doesn't seem to have it,
# it only has /usr/ucb/cc) then change LDCC to the following line and
# give that a try.  This is still using the Solaris compiler but
# leaving out the UCB compatibility includes and libraries.
LDCC= cc5.sol

#CFLAGS=	-Dsv4 -DPOSIX -DJOB_CONTROL -ansi
#otherwise
CFLAGS=		-Dsv4 -DPOSIX -DJOB_CONTROL

#includes symbol info for debugging 
#DASHO=		-g
#for normal build
DASHO=		-O


# switches for library building
LIBCMD=		ar
LIBARGS=	ru
RANLIB=		true

LIB=		-ltermlib

OFILES=		attach.o ansi.o basic.o bind.o browse.o buffer.o \
		composer.o display.o file.o fileio.o line.o osdep.o \
		pico.o random.o region.o search.o spell.o tinfo.o \
		window.o word.o

CFILES=		attach.c ansi.c basic.c bind.c browse.c buffer.c \
		composer.c display.c file.c fileio.c line.c osdep.c \
		pico.c random.c region.c search.c spell.c tinfo.c \
		window.c word.c

HFILES=		estruct.h edef.h efunc.h ebind.h pico.h


#
# dependencies for the Unix versions of pico and libpico.a
#
all:		pico

osdep.c:	os_unix.c
		rm -f osdep.c
		cp os_unix.c osdep.c

osdep.h:	os_unix.h
		rm -f osdep.h
		cp os_unix.h osdep.h

libpico.a:	$& osdep.c osdep.h $(OFILES)
		$(LIBCMD) $(LIBARGS) libpico.a $(OFILES)
		$(RANLIB) libpico.a

pico:		main.c libpico.a
		$(LDCC) $(CFLAGS) main.c libpico.a $(LIB) -o pico

.c.o:		; $(CC) -c $(CFLAGS) $(DASHO) $*.c

$(OFILES):	$(HFILES)

clean:
		rm -f *.a *.o *~ osdep.c osdep.h
