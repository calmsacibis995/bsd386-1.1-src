#************************************************************************
#*   IRC - Internet Relay Chat, Makefile
#*   Copyright (C) 1990, Jarkko Oikarinen
#*
#*   This program is free software; you can redistribute it and/or modify
#*   it under the terms of the GNU General Public License as published by
#*   the Free Software Foundation; either version 1, or (at your option)
#*   any later version.
#*
#*   This program is distributed in the hope that it will be useful,
#*   but WITHOUT ANY WARRANTY; without even the implied warranty of
#*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#*   GNU General Public License for more details.
#*
#*   You should have received a copy of the GNU General Public License
#*   along with this program; if not, write to the Free Software
#*   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#*/

CC=cc
RM=/bin/rm
INCLUDEDIR=../include

# Default flags:
CFLAGS= -I$(INCLUDEDIR) -O
IRCDLIBS=
IRCLIBS=-lcurses -ltermcap
#
# use the following on MIPS:
#CFLAGS= -systype bsd43 -DSYSTYPE_BSD43 -I$(INCLUDEDIR)
# For Irix 4.x (SGI), use the following:
#CFLAGS= -O -cckr -I$(INCLUDEDIR)
#
# on NEXT use:
#CFLAGS=-bsd -I$(INCLUDEDIR)
#on NeXT other than 2.0:
#IRCDLIBS=-lsys_s
#
# AIX 370 flags
#CFLAGS=-D_BSD -Hxa -I$(INCLUDEDIR)
#IRCDLIBS=-lbsd
#IRCLIBS=-lcurses -lcur
#
# Dynix/ptx V2.0.x
#CFLAGS= -I$(INCLUDEDIR) -O -Xo
#IRCDLIBS= -lsocket -linet -lnsl -lseq
#IRCLIBS=-ltermcap -lcurses -lsocket -linet -lnsl -lseq
# 
# Dynix/ptx V1.x.x
#IRCDLIBS= -lsocket -linet -lnsl -lseq
#
#use the following on SUN OS without nameserver libraries inside libc
#IRCDLIBS= -lresolv
#
# Solaris 2
#IRCDLIBS= -lresolv -lsocket -lnsl
#IRCLIBS=-lcurses -ltermcap -lsocket -lnsl
#
# ESIX
#CFLAGS=-O -I$(INCLUDEDIR) -I/usr/ucbinclude
#IRCDLIBS=-L/usr/ucblib -L/usr/lib -lsocket -lucb -lresolv -lns -lnsl
#
# LDFLAGS - flags to send the loader (ld). SunOS users may want to add
# -Bstatic here.
#
#LDFLAGS=-Bstatic

# IRCDMODE is the mode you want the binary to be.
# The 4 at the front is important (allows for setuidness)
#
# WARNING: if you are making ircd SUID or SGID, check config.h to make sure
#          you are not defining CMDLINE_CONFIG 
IRCDMODE = 711

# IRCDDIR must be the same as DPATH in include/config.h
#
IRCDDIR=/usr/local/lib/ircd

SHELL=/bin/sh
SUBDIRS=common ircd irc
BINDIR=$(IRCDDIR)
MANDIR=/usr/local/man
INSTALL=/usr/bin/install

MAKE=make 'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
	'LDFLAGS=${LDFLAGS}' 'IRCDMODE=${IRCDMODE}' 'BINDIR=${BINDIR}' \
	'INSTALL=${INSTALL}' 'IRCLIBS=${IRCLIBS}' 'INCLUDEDIR=${INCLUDEDIR}' \
	'IRCDDIR=${IRCDDIR}'

all:	build

server:
	@echo 'Making server'; cd ircd; ${MAKE} build; cd ..;

client:
	@echo 'Making client'; cd irc; ${MAKE} build; cd ..;

build:
	@for i in $(SUBDIRS); do \
		echo "Building $$i";\
		cd $$i;\
		${MAKE} build; cd ..;\
	done

clean:
	${RM} -f *~ #* core
	@for i in $(SUBDIRS); do \
		echo "Cleaning $$i";\
		cd $$i;\
		${MAKE} clean; cd ..;\
	done

depend:
	@for i in $(SUBDIRS); do \
		echo "Making dependencies in $$i";\
		cd $$i;\
		${MAKE} depend; cd ..;\
	done

install: all
	chmod +x ./install
	@for i in ircd irc; do \
		echo "Installing $$i";\
		cd $$i;\
		${MAKE} install; cd ..;\
	done
	./install -c -m 644 doc/ircd.8 ${MANDIR}/man8
	./install -c -m 644 doc/irc.1 ${MANDIR}/man1
	-./install -c doc/ircd.8 ${MANDIR}/man8
	-./install -c doc/irc.1 ${MANDIR}/man1


rcs:
	cii -H -R Makefile common include ircd

