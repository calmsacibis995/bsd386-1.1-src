#************************************************************************
#*   IRC - Internet Relay Chat, ircd/Makefile
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

CC=gcc -traditional
RM=/bin/rm
CP=/bin/cp
TOUCH=touch
SHELL=/bin/sh
INSTALL=/usr/bin/install

# IRCDMODE given in top level Makefile, but added here to make sure
# compilation works if started in ircd subdirectory

IRCDMODE=711
INCLUDEDIR=../include

# CFLAGS/IRCDLIBS used in ../Makefile should also be used here.  The list
# given below is less than complete.
# For MIPS, use the following:
#CFLAGS= -g -systype bsd43 -DSYSTYPE_BSD43 -I${INCLUDEDIR}
# For Irix 4.x (SGI), use the following:
#CFLAGS= -g -cckr -I$(INCLUDEDIR)
# else just this
CFLAGS= -g -I${INCLUDEDIR}
#
# use the following on SUN OS without nameserver libraries inside libc
# NOTE: most dont have the resolver libraries inside libc
#IRCDLIBS=-L. -lefence
#
#on NeXT other than 2.0:
#IRCDLIBS=-lsys_s
#
# HPUX:
#IRCDLIBS=-lBSD
#
# Solaris 2
#IRCDLIBS= -lsocket -lnsl -lresolv
#
# ESIX
#CFLAGS=-O -I${INCLUDEDIR} -I/usr/ucbinclude
#IRCDLIBS=-L/usr/ucblib -L/usr/lib -lsocket -lucb -lresolv -lns -lnsl
#
#When profiling:
#IRCDLIBS=-lc_p

LINTFLAGS=-hba
#
# LDFLAGS - flags to send the loader (ld). SunOS users may want to add
# -Bstatic here.
#
#LDFLAGS=-Bstatic
#
# RES - if you are missing the resolv library (man 5 resolv.conf), or you
# experience probems with ip# to hostname lookups for local machines or
# the server wont compile because _res is missing, uncomment RES.
# For those who know what these are, if you normally use the resolv+
# libraries and have setup /etc/resolv.conf, these are fromm resolv+ if they
# are not part of your system libraries.  In all cases you should try your
# system libraries before these.
#
#RES=res_init.o res_comp.o res_mkquery.o

COMMONOBJS=../common/bsd.o ../common/dbuf.o \
           ../common/packet.o ../common/send.o\
           ../common/match.o ../common/parse.o ../common/support.o

OBJS=channel.o class.o hash.o ircd.o list.o res.o s_auth.o s_bsd.o s_conf.o \
	s_debug.o s_err.o s_misc.o s_numeric.o s_serv.o s_user.o whowas.o \
	note.o $(RES) $(COMMONOBJS)

SRC=$(OBJS:%.o=%.c)

COMMONSRC=$(COMMONOBJS:%.o=%.c)

MAKE = make 'CFLAGS=${CFLAGS}' 'CC=${CC}' 'IRCDLIBS=${IRCDLIBS}' \
	    'LDFLAGS=${LDFLAGS}' 'IRCDMODE=${IRCDMODE}'


all: build

build: ircd chkconf

ircd: $(OBJS) ../include/patchlevel.h
	$(SHELL) version.c.SH
	$(CC) $(CFLAGS) -c version.c
	$(CC) $(CFLAGS) $(OBJS) version.o $(LDFLAGS) $(IRCDLIBS) -o ircd
	chmod $(IRCDMODE) ircd

chkconf: ../include/struct.h ../include/config.h ../include/sys.h \
	 ../include/common.h chkconf.c
	$(CC) $(CFLAGS) chkconf.c $(LDFLAGS) $(IRCDLIBS) -o chkconf

saber: $(SRC)
	#load -I../include $(SRC) $(COMMONSRC) version.c $(IRCDLIBS)

lint:
	lint $(LINTFLAGS) -I../include $(SRC) | egrep -v 'sendto_|debug'

../common/parse.o: ../common/parse.c ../include/msg.h ../include/config.h\
		   ../include/struct.h ../include/sys.h ../include/numeric.h
	(cd ../common; $(MAKE) build);
../common/bsd.o: ../common/bsd.c ../include/config.h ../include/common.h\
		 ../include/struct.h ../include/sys.h
	(cd ../common; $(MAKE) build);
../common/dbuf.o: ../common/dbuf.c ../include/config.h ../include/common.h\
		  ../include/struct.h ../include/dbuf.h
	(cd ../common; $(MAKE) build);
../common/packet.o: ../common/packet.c ../include/config.h ../include/common.h\
		    ../include/struct.h ../include/msg.h
	(cd ../common; $(MAKE) build);
../common/send.o: ../common/send.c ../include/config.h ../include/common.h\
		  ../include/struct.h ../include/sys.h
	(cd ../common; $(MAKE) build);
../common/match.o: ../common/match.c ../include/config.h ../include/common.h\
		   ../include/sys.h
	(cd ../common; $(MAKE) build);
../common/support.o: ../common/support.c ../include/config.h ../include/sys.h\
		     ../include/common.h
	(cd ../common; $(MAKE) build);

install: all
	-if [ ! -d ${IRCDDIR} -a ! -f ${IRCDDIR} ] ; then \
		-mkdir ${IRCDDIR}; \
	fi
	../install -c -s -m ${IRCDMODE} ircd ${BINDIR}
	../install -c -s -m 700 chkconf ${BINDIR}
	$(CP) ../doc/example.conf ${IRCDDIR}
	$(TOUCH) ${IRCDDIR}/ircd.motd
	$(RM) -f ${IRCDDIR}/ircd.m4
	$(TOUCH) ${IRCDDIR}/ircd.m4
	chmod +x buildm4
	./buildm4 ${IRCDDIR}

clean:
	$(RM) -f *.o *~ core ircd version.c \#* *.bak chkconf

depend:
	makedepend -I${INCLUDEDIR} ${SRC}

channel.o: ../include/struct.h ../include/config.h ../include/dbuf.h \
	   ../include/numeric.h ../include/channel.h
	$(CC) $(CFLAGS) -c channel.c

class.o: ../include/struct.h ../include/class.h ../include/numeric.h \
	 ../include/common.h ../include/config.h
	$(CC) $(CFLAGS) -c class.c

ircd.o: ircd.c ../include/struct.h ../include/config.h \
	../include/dbuf.h ../include/numeric.h
	$(CC) $(CFLAGS) -c ircd.c

list.o: list.c ../include/struct.h ../include/config.h ../include/dbuf.h \
	../include/sys.h
	$(CC) $(CFLAGS) -c list.c

res.o: res.c ../include/struct.h ../include/config.h ../include/res.h
	$(CC) $(CFLAGS) -c res.c

s_bsd.o: s_bsd.c ../include/struct.h ../include/config.h ../include/dbuf.h \
	 ../include/sys.h
	$(CC) $(CFLAGS) -c s_bsd.c

s_auth.o: s_auth.c ../include/struct.h ../include/config.h ../include/dbuf.h \
	  ../include/sys.h
	$(CC) $(CFLAGS) -c s_auth.c

s_conf.o: s_conf.c ../include/struct.h ../include/config.h \
	  ../include/numeric.h  ../include/dbuf.h ../include/sys.h
	$(CC) $(CFLAGS) -c s_conf.c

s_debug.o: ../include/config.h ../include/struct.h ../include/common.h \
	   ../include/sys.h s_debug.c
	$(CC) $(CFLAGS) -c s_debug.c

s_misc.o: s_misc.c ../include/struct.h ../include/config.h ../include/dbuf.h
	$(CC) $(CFLAGS) -c s_misc.c

s_user.o: s_user.c ../include/struct.h ../include/config.h \
	  ../include/dbuf.h ../include/sys.h ../include/channel.h \
	  ../include/msg.h ../include/numeric.h ../include/whowas.h
	$(CC) $(CFLAGS) -c s_user.c

s_serv.o: s_serv.c ../include/struct.h ../include/config.h \
	  ../include/dbuf.h ../include/sys.h ../include/channel.h \
	  ../include/msg.h ../include/numeric.h ../include/whowas.h
	$(CC) $(CFLAGS) -c s_serv.c

s_numeric.o: s_numeric.c ../include/config.h ../include/sys.h \
	     ../include/struct.h ../include/dbuf.h ../include/numeric.h
	$(CC) $(CFLAGS) -c s_numeric.c

whowas.o: ../include/struct.h ../include/config.h \
	../include/dbuf.h ../include/sys.h ../include/numeric.h \
	../include/whowas.h whowas.c
	$(CC) $(CFLAGS) -c whowas.c

hash.o: ../include/struct.h ../include/sys.h ../include/hash.h hash.c \
	../include/config.h s_bsd.c s_serv.c s_user.c channel.c s_misc.c
	@crypt/sums
	$(CC) $(CFLAGS) -c hash.c
	@/bin/rm -f hash.c
	@/bin/mv -f hash.c.old hash.c
	@touch hash.o

# DO NOT DELETE THIS LINE -- make depend depends on it.

channel.o: ../include/struct.h ../include/config.h ../include/dbuf.h
channel.o: ../include/numeric.h ../include/channel.h channel.c
s_misc.o: ../include/struct.h ../include/config.h ../include/dbuf.h s_misc.c
ircd.o: ../include/struct.h ../include/config.h
ircd.o: ../include/dbuf.h ../include/numeric.h ircd.c
list.o: ../include/struct.h ../include/config.h ../include/dbuf.h
list.o: ../include/sys.h list.c
res.o: ../include/struct.h ../include/config.h ../include/res.h res.c
s_bsd.o: ../include/struct.h ../include/config.h ../include/dbuf.h
s_bsd.o: ../include/sys.h s_bsd.c
s_auth.o: ../include/struct.h ../include/config.h ../include/dbuf.h
s_auth.o: ../include/sys.h s_auth.c
s_debug.o: ../include/config.h ../include/struct.h ../include/common.h
s_debug.o: ../include/sys.h s_debug.c
s_debug.o: ../include/struct.h ../include/common.h ../include/sys.h
s_conf.o: ../include/struct.h ../include/config.h ../include/numeric.h
s_conf.o: ../include/dbuf.h ../include/sys.h s_conf.c
s_user.o: ../include/struct.h ../include/config.h
s_user.o: ../include/dbuf.h ../include/sys.h ../include/channel.h
s_user.o: ../include/msg.h ../include/numeric.h ../include/whowas.h s_user.c
s_serv.o: ../include/struct.h ../include/config.h
s_serv.o: ../include/dbuf.h ../include/sys.h ../include/channel.h
s_serv.o: ../include/msg.h ../include/numeric.h ../include/whowas.h s_serv.c
s_numeric.o: ../include/config.h ../include/sys.h ../include/struct.h
s_numeric.o: ../include/dbuf.h ../include/numeric.h s_numeric.c
whowas.o: ../include/struct.h ../include/config.h ../include/dbuf.h
whowas.o: ../include/numeric.h ../include/whowas.h ../include/sys.h whowas.c
class.o: ../include/struct.h ../include/class.h ../include/numeric.h
class.o: ../include/common.h ../include/config.h class.c
hash.o: ../include/config.h ../include/sys.h ../include/hash.h
hash.o: ../include/struct.h ../include/common.h s_serv.c s_user.c
hash.o: channel.c s_misc.c s_bsd.c ircd.c hash.c version.c.SH
version.o: version.c.SH version.c
