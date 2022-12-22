#########################################################################
## (c) Copyright 1988, Hewlett-Packard Co.  All Rights Reserved.       ##
##                                                                     ##
## Author: Tai Jin, Hewlett-Packard Laboratories.                      ##
#########################################################################

## Makefile.tmpl,v 3.1 1993/07/06 01:04:40 jbj Exp

#
PROGRAM = adjtimed
COMPILER= gcc
CC= $(COMPILER)
BINDIR=	/usr/local/bin
COPTS=      -g
DEFS=      -DSYS_BSDI
DEFS_OPT=-DDEBUG
DEFS_LOCAL= $(DEFS_OPT)  -DREFCLOCK #TEST -DPPSPPS -DKERNEL_PLL
INCL= -I../include
LLIBS=	
INSTALL= install


CFLAGS=	$(COPTS) $(DEFS) $(DEFS_LOCAL) $(INCL)
CC=	$(COMPILER)
LDFLAGS= 
LIBS=	$(LLIBS) -lc
OBJ=	adjtime.o adjtimed.o
ALL=	libadjtime.a adjtimed

all:	$(ALL)

libadjtime.a: adjtime.o
	ar vr libadjtime.a $?

adjtimed: adjtimed.o ../lib/libntp.a
	$(CC) $(LDFLAGS) -o adjtimed adjtimed.o ../lib/libntp.a $(LIBS)

../lib/libntp.a:
	cd ../lib && $(MAKE) $(MFLAGS) MFLAGS="$(MFLAGS)" MAKE="$(MAKE)"

install:	$(BINDIR)/$(PROGRAM)

$(BINDIR)/$(PROGRAM): $(PROGRAM)
	$(INSTALL) -c -m 0755 $(PROGRAM) $(BINDIR)

clean:
	-@rm -f *.a *.o adjtimed

distclean:	clean
	-@rm -f *.orig *.rej .version Makefile

install:	$(PROGRAM)
	cp $(PROGRAM) $(BINDIR)
