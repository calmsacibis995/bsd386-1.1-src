#
# Makefile.tmpl,v 3.1 1993/07/06 01:04:48 jbj Exp
#
PROGRAM=	authcert authspeed md5
#
# authcert, authspeed - authentication utilities
#
COMPILER= gcc
COPTS=      -g
BINDIR=	/usr/local/bin
DEFS=      -DSYS_BSDI
DEFS_OPT=-DDEBUG
DEFS_LOCAL= $(DEFS_OPT)  -DREFCLOCK #TEST -DPPSPPS -DKERNEL_PLL
COMPAT=
RESLIB=
#
INCL=		-I../include
CFLAGS=		$(COPTS) $(DEFS) $(DEFS_LOCAL) $(INCL)
CC=		$(COMPILER)
LIB=		../lib/libntp.a
MAKE=		make
#
CRTOBJS=	authcert.o
SPDOBJS=	authspeed.o
PAROBJS=	keyparity.o
IFPOBJS=	makeIPFP.o
PC1OBJS=	makePC1.o
PC2OBJS=	makePC2.o
SPOBJS=		makeSP.o
RNDOBJS=	mkrandkeys.o
OIFOBJS=	omakeIPFP.o
UNXOBJS=	unixcert.o
MD5OBJS=	md5driver.o

SOURCE=	authcert.c authspeed.c keyparity.c makeIPFP.c makePC1.c \
	makePC2.c makeSP.c mkrandkeys.c omakeIPFP.c unixcert.c md5driver.c

all:	$(PROGRAM)

authcert: $(CRTOBJS) $(LIB)
	$(CC) $(COPTS) -o $@ $(CRTOBJS) $(LIB)

authspeed: $(SPDOBJS) $(LIB)
	$(CC) $(COPTS) -o $@ $(SPDOBJS) $(LIB) $(COMPAT) $(RESLIB)

keyparity: $(PAROBJS) $(LIB)
	$(CC) $(COPTS) -o $@ $(PAROBJS) $(LIB)

makeIPFP: $(IFPOBJS)
	$(CC) $(COPTS) -o $@ $(IFPOBJS)

makePC1: $(PC1OBJS)
	$(CC) $(COPTS) -o $@ $(PC1OBJS)

makePC2: $(PC2OBJS)
	$(CC) $(COPTS) -o $@ $(PC2OBJS)

makeSP: $(SPOBJS)
	$(CC) $(COPTS) -o $@ $(SPOBJS)

mkrandkeys: $(RNDOBJS) $(LIB)
	$(CC) $(COPTS) -o $@ $(RNDOBJS) $(LIB)

omakeIPFP: $(OIFBJS)
	$(CC) $(COPTS) -o $@ $(OIFBJS)

unixcert: $(UNXBJS)
	$(CC) $(COPTS) -o $@ $(UNXBJS)

md5:	$(MD5OBJS)
	$(CC) $(COPTS) -o $@ $(MD5OBJS) $(LIB)

tags:
	ctags *.c *.h

install:
	# Don't install any of this shit

depend:
	mkdep $(CFLAGS) $(SOURCE)

clean:
	-@rm -f $(PROGRAM) *.o *.out tags make.log Makefile.bak keyparity \
		makeIPFP makePC1 makePC2 makeSP mkrandkeys omakeIPFP unixcert \
		lint.errs md5cert

distclean:	clean
	-@rm -f *.orig *.rej .version Makefile

../lib/libntp.a:
	cd ../lib && $(MAKE) $(MFLAGS) MFLAGS="$(MFLAGS)"

