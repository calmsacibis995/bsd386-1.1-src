#************************************************************************
#*   IRC - Internet Relay Chat, irc/Makefile
#*   Copyright (C) 1990 Jarkko Oikarinen
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

# 17 Jun 1990 - Armin
# Now, common/debug.c has changed to irc/c_debug.c, for the benefit of
# displaying wrong server stuff

# 30 May 1990 - Jto
# swear.o isn't needed when compiling irc.
# Removed it (as suggested by Kim).

# Check the curses libraries if your systems wants them differently...

INCLUDEDIR=../include
DFLAGS=-DCLIENT_COMPILE
INSTALL=/usr/bin/install
CFLAGS = -g -I${INCLUDEDIR}

#For MIPS, use this:
#CFLAGS =-O -systype bsd43 -DSYSTYPE_BSD43 -I${INCLUDEDIR} ${DFLAGS}

#Libs...default is
IRCLIBS=-lcurses -ltermcap
# For AIX, use the following:
#IRCLIBS=-lcurses -lcur
#
# Solaris 2
#IRCLIBS=-lcurses -ltermcap -lsocket -lnsl
#
# Sequent Dynix 1.4/2.0
#IRCLIBS=-lcurses -ltermcap -lsocket -linet -lnsl -lseq

#CC=/bin/cc
CC=cc
RM=/bin/rm
OBJS=c_bsd.o c_conf.o c_msg.o c_numeric.o c_version.o edit.o help.o ignore.o\
     irc.o screen.o str.o c_debug.o ctcp.o

COMMONOBJS=bsd.o dbuf.o packet.o send.o match.o parse.o support.o
COMMONSRC=../common/bsd.c ../common/dbuf.c ../common/packet.c \
	 ../common/send.c ../common/match.c ../common/parse.c \
	 ../common/support.c
SRC=$(OBJS:%.o=%.c) $(COMMONSRC)

MAKE = make 'CFLAGS=${CFLAGS}' 'CC=${CC}'

all: build

build: irc

install: all
	../install -c -s -m 711 irc $(BINDIR)

irc: $(OBJS) $(COMMONOBJS)
	$(CC) $(OBJS) $(COMMONOBJS) -o irc $(IRCLIBS) $(LIBFLAGS)

parse.o: ../common/parse.c ../include/msg.h
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/parse.c

bsd.o: ../common/bsd.c
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/bsd.c

dbuf.o: ../common/dbuf.c
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/dbuf.c

packet.o: ../common/packet.c
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/packet.c

send.o: ../common/send.c
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/send.c

match.o: ../common/match.c
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/match.c

support.o: ../common/support.c
	${CC} ${CFLAGS} ${DFLAGS} -c ../common/support.c

c_bsd.o: ../include/struct.h ../include/config.h ../include/dbuf.h c_bsd.c
	${CC} ${CFLAGS} ${DFLAGS} -c c_bsd.c

c_conf.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	  ../include/sys.h c_conf.c
	${CC} ${CFLAGS} ${DFLAGS} -c c_conf.c

c_msg.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	 ../include/msg.h c_msg.c
	${CC} ${CFLAGS} ${DFLAGS} -c c_msg.c

c_numeric.o: ../include/config.h ../include/sys.h ../include/struct.h \
	     ../include/dbuf.h ../include/numeric.h ../include/struct.h \
	     c_numeric.c
	${CC} ${CFLAGS} ${DFLAGS} -c c_numeric.c

c_version.o: ../include/patchlevel.h c_version.c ../include/config.h \
	     ../include/dbuf.h
	${CC} ${CFLAGS} ${DFLAGS} -c c_version.c

edit.o: ../include/struct.h ../include/config.h ../include/dbuf.h edit.c
	${CC} ${CFLAGS} ${DFLAGS} -c edit.c

help.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	 /usr/include/pwd.h ../include/sys.h help.c help.h
	${CC} ${CFLAGS} ${DFLAGS} -c help.c

ignore.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	 /usr/include/pwd.h ../include/sys.h ignore.c
	${CC} ${CFLAGS} ${DFLAGS} -c ignore.c

irc.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	 ../include/msg.h ../include/sys.h irc.c irc.h
	${CC} ${CFLAGS} ${DFLAGS} -c irc.c

screen.o: ../include/struct.h ../include/config.h ../include/dbuf.h screen.c
	${CC} ${CFLAGS} ${DFLAGS} -c screen.c

str.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	 ../include/sys.h str.c
	${CC} ${CFLAGS} ${DFLAGS} -c str.c

swear.o: ../include/struct.h ../include/config.h \
	 ../include/dbuf.h ../include/sys.h /usr/include/pwd.h swear.c
	${CC} ${CFLAGS} ${DFLAGS} -c swear.c

c_debug.o: ../include/struct.h c_debug.c
	${CC} ${CFLAGS} ${DFLAGS} -c c_debug.c

clean:
	rm -f *.o *~ irc core \#* *.bak

depend:
	makedepend -I${INCLUDEDIR} ${SRC}

# DO NOT DELETE THIS LINE -- make depend depends on it.

c_bsd.o: ../include/struct.h ../include/config.h ../include/dbuf.h
c_conf.o: ../include/struct.h ../include/config.h
c_conf.o: ../include/dbuf.h ../include/sys.h
c_msg.o: ../include/struct.h ../include/config.h ../include/dbuf.h
c_msg.o: ../include/msg.h
c_numeric.o: ../include/config.h ../include/sys.h
c_numeric.o: ../include/struct.h
c_numeric.o: ../include/dbuf.h ../include/numeric.h
c_version.o: ../include/struct.h ../include/config.h ../include/dbuf.h
c_version.o: ../include/patchlevel.h
edit.o: ../include/struct.h
edit.o: ../include/config.h ../include/dbuf.h
help.o: ../include/struct.h ../include/config.h ../include/dbuf.h
help.o: ../include/sys.h
help.o: /usr/include/pwd.h help.h
ignore.o: ../include/struct.h ../include/config.h ../include/dbuf.h
ignore.o: ../include/sys.h
ignore.o: /usr/include/pwd.h
irc.o: ../include/struct.h ../include/config.h ../include/dbuf.h
irc.o: ../include/msg.h ../include/sys.h
irc.o: irc.h
screen.o: ../include/struct.h ../include/config.h ../include/dbuf.h
str.o: ../include/struct.h ../include/config.h ../include/dbuf.h
str.o: ../include/sys.h
swear.o: ../include/struct.h ../include/config.h
swear.o: ../include/dbuf.h ../include/sys.h /usr/include/pwd.h
c_debug.o: ../include/struct.h
