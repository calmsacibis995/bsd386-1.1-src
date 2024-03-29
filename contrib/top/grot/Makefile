# Makefile for "top", a top 10 process display for Unix
#
# This makefile is for top, version 3
#
# Written by William LeFebvre, Northwestern University
#			(formerly Rice University)

# DO NOT EDIT "Makefile"!!!!  Make changes to "Makefile.X" and rerun
# Configure.

# Executables (these should be obvious):

SHELL   = /bin/sh
CC      = cc
AWK     = awk
INSTALL = install

# installation information:
#	OWNER	- name (or uid) for the installed executable's owner
#	GROUP	- group name (or gid) for the installed executable's group
#	MODE	- mode for the installed executable (should start with a 0)
#	BINDIR	- directory where the executable should live
#	MANDIR	- directory where the manual page should live
#	MANEXT	- installed man pages end in .$(MANEXT)
#	MANSTY	- "man" or "catman" depending on what's to be installed
#	SIGNAL	- <signal.h> or <sys/signal.h>; the one with signal definitions
#	TROFF	- most appropriate troff command

OWNER  = root
GROUP  = kmem
MODE   = 2755
BINDIR = /usr/contrib/bin
MANDIR = /usr/contrib/man/cat1
MANEXT = 0
MANSTY = catman
SIGNAL = /usr/include/signal.h

# Values for the two defaults in "top":
#	TOPN	- default number of processes to display
#	DELAY	- default delay between updates
#
# set TOPN to -1 to indicate infinity (so that top will display as many
# as the screen will hold).

TOPN = 15
DELAY = 5

TARFILES = README INSTALL FAQ Changes Configure Porting \
	   Makefile.X Make.desc.X getans \
	   top.c commands.c display.c screen.c username.c \
	   utils.c version.c getopt.c \
	   boolean.h display.h layout.h loadavg.h screen.h \
	   machine.h patchlevel.h top.h top.local.H os.h utils.h \
	   sigconv.awk top.X m-template metatop \
	   machine
CFILES = top.c commands.c display.c screen.c username.c \
	 utils.c version.c getopt.c machine.c
OBJS = top.o commands.o display.o screen.o username.o \
       utils.o version.o getopt.o machine.o

CDEFS = -O
LIBS = -lkvm

CFLAGS =  $(CDEFS)
LINTFLAGS = -x $(CDEFS)

all: Makefile top.local.h top

Makefile: Makefile.X
	@echo 'You need to run the script "Configure" before running "make".'
	exit 10

top.local.h: top.local.H
	@echo 'You need to run the script "Configure" before running "make".'
	exit 10

top: $(OBJS)
	rm -f top
	$(CC) -o top $(OBJS) -ltermcap -lm $(LIBS)

lint: sigdesc.h
	$(LINT) $(LINTFLAGS) $(CFILES)

# include file dependencies
top.o: boolean.h display.h screen.h top.h top.local.h utils.h machine.h 
commands.o: boolean.h sigdesc.h utils.h
display.o: boolean.h display.h layout.h screen.h top.h top.local.h utils.h
machine.o: top.h machine.h
screen.o: boolean.h screen.h
utils.o: top.h
version.o: top.h patchlevel.h
username.o: top.local.h utils.h

# automatically built include file
sigdesc.h: sigconv.awk $(SIGNAL)
	$(AWK) -f sigconv.awk $(SIGNAL) >sigdesc.h

tar:
	rm -f top.tar machine/*.desc
	tar cvf top.tar $(TARFILES)

shar:
	rm -f top.shar* machine/*.desc
	makekit -ntop.shar. -t"Now read README and INSTALL, then run Configure" machine $(TARFILES)/*

clean:
	rm -f *.o top core core.* sigdesc.h

veryclean: clean
	rm -f Make.desc machine/*.desc .defaults top.tar SYNOPSIS Makefile top.local.h top.1 machine.c

install: top top.1 install-top install-$(MANSTY)

install-top:
	$(INSTALL) -s -o $(OWNER) -m $(MODE) -g $(GROUP) top $(BINDIR)

install-man:
	$(INSTALL) -c top.1 $(MANDIR)/top.$(MANEXT)

install-catman:
	tbl top.1 | nroff -man > $(MANDIR)/top.$(MANEXT)

installmeta: top top.1
	$(INSTALL) -o $(OWNER) -m 755 -g $(GROUP) metatop $(BINDIR)/top
	@echo $(INSTALL) -s -o $(OWNER) -m $(MODE) -g $(GROUP) top $(BINDIR)/top-`uname -m`-`uname -r`
	@$(INSTALL) -s -o $(OWNER) -m $(MODE) -g $(GROUP) \
		top $(BINDIR)/top-`uname -m`-`uname -r`
	$(INSTALL) -c top.1 $(MANDIR)/top.$(MANEXT)
