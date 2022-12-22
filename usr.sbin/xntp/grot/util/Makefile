#
# Makefile.tmpl,v 3.1 1993/07/06 01:10:58 jbj Exp
#
PROGRAM=	tickadj
#
# Makefile for utilities
#
COMPILER= gcc
COPTS=      -g
BINDIR=	/usr/local/bin
INSTALL= install
DEFS=      -DSYS_BSDI
DEFS_OPT=-DDEBUG
DEFS_LOCAL= $(DEFS_OPT)  -DREFCLOCK #TEST -DPPSPPS -DKERNEL_PLL
DAEMONLIBS= -lkvm
RESLIB=
COMPAT=
#
INCL=		-I../include
CFLAGS=		$(COPTS) $(DEFS) $(DEFS_LOCAL) $(INCL)
CC=		$(COMPILER)
LIB=		../lib/libntp.a 
LINTLIB=	../lib/llib-llibntp.ln
MAKE=		make
TOP=../
#
SOURCE=		tickadj.c ntptime.c
TKOBJS=		tickadj.o
NTOBJS=		ntptime.o
EXECS=		ntptime jitter timetrim kern byteorder longsize precision

all:	$(PROGRAM)

tickadj: $(TKOBJS)
	$(CC) $(COPTS) -o $@ $(TKOBJS) $(DAEMONLIBS) $(RESLIB) $(COMPAT)

ntptime: $(NTOBJS)
	$(CC) $(COPTS) -o $@ $(NTOBJS) $(LIB)

precision: precision.o
	$(CC) $(COPTS) -o $@ $@.o

install:	$(BINDIR)/$(PROGRAM)

$(BINDIR)/$(PROGRAM): $(PROGRAM)
	$(INSTALL) -c -m 0755 $(PROGRAM) $(BINDIR)

tags:
	ctags *.c *.h

depend:
	mkdep $(CFLAGS) $(SOURCE)

clean:
	-@rm -f $(PROGRAM) $(EXECS) *.o *.out tags make.log Makefile.bak lint.errs

distclean:	clean
	-@rm -f *.orig *.rej .version Makefile

../lib/libntp.a:
	cd ../lib && $(MAKE) $(MFLAGS) $(MFLAGS) MFLAGS="$(MFLAGS)"

