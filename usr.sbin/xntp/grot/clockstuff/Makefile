#
# Makefile.tmpl,v 3.1 1993/07/06 01:05:19 jbj Exp
#
PROGRAM=	propdelay
#
# Makefile for clock miscellany
#
COMPILER= gcc
COPTS=      -g
BINDIR=	/usr/local/bin
DEFS=      -DSYS_BSDI
DEFS_OPT=-DDEBUG
DEFS_LOCAL= $(DEFS_OPT)  -DREFCLOCK #TEST -DPPSPPS -DKERNEL_PLL
COMPAT=
#
INCL=		-I../include
CFLAGS=		$(COPTS) $(DEFS) $(DEFS_LOCAL) $(INCL)
CC=		$(COMPILER)
LIB=		../lib/libntp.a
LINTLIB=	../lib/llib-llibntp.ln
MAKE=		make
INSTALL= install
#
SOURCE=	chutest.c propdelay.c
OBJS=	propdelay.o
CHUOBJS=	chutest.o
CLKOBJS=	clktest.o

all:	$(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(COPTS) -o $@ $(OBJS) -lm $(COMPAT)

chutest: $(CHUOBJS) $(LIB)
	$(CC) $(COPTS) -o $@ $(CHUOBJS) $(LIB)

clktest: $(CLKOBJS) $(LIB)
	$(CC) $(COPTS) -o $@ $(CLKOBJS) $(LIB)

install:	$(BINDIR)/$(PROGRAM)

$(BINDIR)/$(PROGRAM): $(PROGRAM)
#	$(INSTALL) -c -m 0755 $(PROGRAM) $(BINDIR)

tags:
	ctags *.c *.h

depend:
	mkdep $(CFLAGS) $(SOURCE)

clean:
	-@rm -f $(PROGRAM) *.o *.out tags make.log Makefile.bak chutest clktest \
		lint.errs

distclean:	clean
	-@rm -f *.orig *.rej .version Makefile

../lib/libntp.a:
	cd ../lib && $(MAKE) $(MFLAGS) MFLAGS="$(MFLAGS)"

