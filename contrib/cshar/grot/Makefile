##
##  SOURCE-SHIPPING TOOLS MAKEFILE
##  $Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/grot/Makefile,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $
##

## Grrrr...
SHELL	= /bin/sh

##  Edit appropriately, such as -g if you don't trust me...
CFLAGS	= -O

##  Where the readdir() and friends are, if not in your C library.
#DIRLIB	= -lndir

##  Use these two lines if you use ranlib...
AR_OBJ	= $(LIB_OBJ)
RANLIB	= ranlib lib.a
##  ...or use these two if you need tsort instead...
#AR_OBJ	= `lorder $(LIB_OBJ) | tsort`
#RANLIB	= @echo
##  ...or these two if you need neither.
#AR_OBJ	= $(LIB_OBJ)
#RANLIB	= @echo

##  Where executables should be put.
DESTDIR	= /usr/local/bin

##  The "foo" manpage will get put in $(MANDIR)/foo.$1
MANDIR	= /usr/man/man1
1	= 1
#MANDIR	= /usr/man/u_man/manl
#1	= 1L
#MANDIR	= /usr/man/local
#1	= 1
#MANDIR	= /usr/man/man.M
#1	= M

##
##  END OF CONFIGURATION SECTION
##

##  Header files.
HDRS	= shar.h config.h
LIB	= lib.a

##  Programs and documentation.
PROGRAMS= findsrc    makekit    shar    unshar    shell
MANPAGES= findsrc.$1 makekit.$1 shar.$1 unshar.$1 shell.$1

##  Our library of utility functions.
LIB_SRC	= glue.c parser.c lcwd.c lexec.c lfiles.c lhost.c llib.c lmem.c luser.c
LIB_OBJ	= glue.o parser.o lcwd.o lexec.o lfiles.o lhost.o llib.o lmem.o luser.o


all:		$(PROGRAMS) $(MANPAGES)
	touch all

##  You might want to change these actions...
install:	all
	cd $(DESTDIR) ; rm -f $(PROGRAMS)
	cp $(PROGRAMS) $(DESTDIR)
	cd $(DESTDIR) ; strip $(PROGRAMS) ; chmod 755 $(PROGRAMS)
#	cd $(DESTDIR) ; /etc/chown root $(PROGRAMS)
	cd $(MANDIR) ; rm -f $(MANPAGES)
	cp $(MANPAGES) $(MANDIR)
	touch install

clean:
	rm -f *.[oa] *.$1 *.BAK $(PROGRAMS) unshar.safe
	rm -f lint lib.ln tags core foo a.out Part?? all install


##  Creating manpages.
.SUFFIXES:	.$1 .man
.man.$1:
	@rm -f $@
	cp $< $@
	chmod 444 $@


##  Programs.
findsrc:	findsrc.o $(HDRS) $(LIB)
	@rm -f $@
	$(CC) $(CFLAGS) -o findsrc findsrc.o $(LIB) $(DIRLIB)

makekit:	makekit.o $(HDRS) $(LIB)
	@rm -f $@
	$(CC) $(CFLAGS) -o makekit makekit.o $(LIB) $(DIRLIB)

shar:		shar.o $(HDRS) $(LIB)
	@rm -f $@
	$(CC) $(CFLAGS) -o shar shar.o $(LIB) $(DIRLIB)

shell:		shell.o $(HDRS) $(LIB)
	@rm -f $@
	$(CC) $(CFLAGS) -o shell shell.o $(LIB) $(DIRLIB)

unshar:		unshar.o $(HDRS) $(LIB)
	@rm -f $@
	$(CC) $(CFLAGS) -o unshar unshar.o $(LIB) $(DIRLIB)


##  Special case, a version of unshar that uses the /bin/sh interpreter.
unshar.safe:	unshar.c $(HDRS) $(LIB)
	@rm -f $@
	@rm -f unshar.o
	$(CC) $(CFLAGS) -DUSE_MY_SHELL -o unshar unshar.c $(LIB) $(DIRLIB)
	@rm -f unshar.o


##  Support your local library.
lib.a:		$(LIB_OBJ)
	@rm -f $@
	ar r lib.a $(AR_OBJ)
	$(RANLIB)
$(LIB_OBJ):	$(HDRS)


##  Lint; this is probably only good for BSD-derived lints.
##  Ultrix 1.2 lint really hates the !var construct, for some reason.
#LINTF	= -p -ahb
LINTF	= -ahb
##  A slight speed hack...
X	= exec
lint:		$(PROGRAMS) lib.ln
	@rm -f $@
	$X lint $(LINTF) -u  >lint $(LIB_SRC)
	$X lint $(LINTF)    >>lint findsrc.c lib.ln
	$X lint $(LINTF)    >>lint makekit.c lib.ln
	$X lint $(LINTF)    >>lint shar.c    lib.ln
	$X lint $(LINTF)    >>lint shell.c   lib.ln
	$X lint $(LINTF)    >>lint unshar.c  lib.ln
#	$X lint $(LINTF) -DUSE_MY_SHELL >>lint unshar.c  lib.ln

lib.ln:		$(LIB_SRC)
	@rm -f $@
	lint -CX $(LIB_SRC)
	mv llib-lX.ln lib.ln
