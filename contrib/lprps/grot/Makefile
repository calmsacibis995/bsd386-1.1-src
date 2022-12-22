# Id: Makefile,v 1.16 1992/09/29 09:13:30 jjc Exp 
# Define A4 if you use A4 rather than letter size paper.
# Define IIg to enable a workaround for a bug in the serial
# driver of the Laserwriter IIf and IIg; this requires the POSIX
# tcdrain() function.  (I haven't tested this personally.)
DEFINES=-DA4 #-DIIg
PREFIX=/usr/local
# Where to install public executables.
BINDIR=$(PREFIX)/bin
# Where to install private executables.
LIBDIR=$(PREFIX)/lib
# Define REV_COMMAND to be empty if documents don't need to
# be printed in reverse order.
REV_COMMAND=$(BINDIR)/psrev |
# Where to install the filter for text files.
TEXT_FILTER=$(LIBDIR)/psif-text
# Where to install the filter for PostScript files.
PS_FILTER=$(LIBDIR)/psif-ps
# Where to install PostScript banner printing code
BANNER=$(LIBDIR)/banner.ps
# Filter to convert text to PostScript.  You can use your own instead.
# It must be able to read from standard input.
TEXT_TO_PS=$(BINDIR)/textps
MANROOT=$(PREFIX)/man
# Manual section for user commands.
MAN1EXT=1
MAN1DIR=$(MANROOT)/man$(MAN1EXT)
# Manual section for system maintenance and operation commands.
MAN8EXT=8
MAN8DIR=$(MANROOT)/man$(MAN8EXT)

CC=cc
CFLAGS=-O $(DEFINES)
#CFLAGS=-g -Bstatic $(DEFINES)
# Command for installing programs.
INSTALL_PROGRAM=install
# Command for installing man pages.
INSTALL_DATA=install -m 0444

SHELL=/bin/sh
# Uncomment the next line for Ultrix.
#SHELL=/bin/sh5

SHAR=shar
DIST1=README INSTALL TODO Makefile textps.man psrev.man lprps.man psif.man \
  psof.man banner.ps lpr.diff
DIST2=psrev.c lprps.c psif.c textps.c psof.c

PROGS=psrev psif lprps textps psof

.SUFFIXES: .man .n

.man.n:
	@echo Making $@ from $<
	@-rm -f $@
	@sed -e "s;@MAN1EXT@;$(MAN1EXT);" \
	-e "s;@MAN8EXT@;$(MAN8EXT);" \
	-e "s;@PS_FILTER@;$(PS_FILTER);" \
	-e "s;@TEXT_FILTER@;$(TEXT_FILTER);" \
	-e "s;@BANNER@;$(BANNER);" \
	$< >$@
	@chmod 444 $@

all: psif-text psif-ps $(PROGS) textps.n psrev.n lprps.n psif.n psof.n

install: all banner.ps
	-test -d $(LIBDIR) || mkdir $(LIBDIR)
	-test -d $(BINDIR) || mkdir $(BINDIR)
	$(INSTALL_PROGRAM) psif $(LIBDIR)/psif
	$(INSTALL_PROGRAM) lprps $(LIBDIR)/lprps
	$(INSTALL_PROGRAM) psof $(LIBDIR)/psof
	$(INSTALL_DATA) banner.ps $(BANNER)
	$(INSTALL_PROGRAM) textps $(BINDIR)/textps
	$(INSTALL_PROGRAM) psrev $(BINDIR)/psrev
	$(INSTALL_PROGRAM) psif-text $(TEXT_FILTER)
	$(INSTALL_PROGRAM) psif-ps $(PS_FILTER)
	$(INSTALL_DATA) textps.n $(MAN1DIR)/textps.$(MAN1EXT)
	$(INSTALL_DATA) psrev.n $(MAN1DIR)/psrev.$(MAN1EXT)
	$(INSTALL_DATA) lprps.n $(MAN8DIR)/lprps.$(MAN8EXT)
	$(INSTALL_DATA) psif.n $(MAN8DIR)/psif.$(MAN8EXT)
	$(INSTALL_DATA) psof.n $(MAN8DIR)/psof.$(MAN8EXT)
	@echo Now edit /etc/printcap

lprps: lprps.c
	$(CC) $(CFLAGS) -o $@ lprps.c

psif: psif.c
	$(CC) $(CFLAGS) -DTEXT_FILTER=\"$(TEXT_FILTER)\" \
	-DPS_FILTER=\"$(PS_FILTER)\" -o $@ psif.c

textps: textps.c
	$(CC) $(CFLAGS) -o $@ textps.c

psrev: psrev.c
	$(CC) $(CFLAGS) -o $@ psrev.c

psof: psof.c
	$(CC) $(CFLAGS) -DLPRPS=\"$(LIBDIR)/lprps\" \
	-DBANNER=\"$(BANNER)\" -o $@ psof.c

psif-text: Makefile
	@echo Creating $@
	@-rm -f $@
	@echo "#! $(SHELL)" > $@
	@echo "$(TEXT_TO_PS) | $(REV_COMMAND) $(LIBDIR)/lprps \"\$$@\"" >>$@
	@chmod +x $@

psif-ps: Makefile
	@echo Creating $@
	@-rm -f $@
	@echo "#! $(SHELL)" > $@
	@echo "$(REV_COMMAND) $(LIBDIR)/lprps \"\$$@\"" >>$@
	@chmod +x $@

dist: part01 part02

part01: $(DIST1)
	rm -f $@
	$(SHAR) -n1 -e2 $(DIST1) >$@

part02: $(DIST2)
	rm -f $@
	$(SHAR) -n2 -e2 $(DIST2) >$@

tar-dist: $(DIST1) $(DIST2)
	dir=lprps-`sed -ne '1s/.*version \([0-9.]*[0-9]\).*/\1/p' README`; \
	rm -fr $$dir; \
	mkdir $$dir; \
	cd $$dir; \
	for f in $(DIST1) $(DIST2); do \
	  ln -s ../$$f .; \
	done; \
	cd ..; \
	rm -f $$dir.tar.Z; \
	tar cfh - $$dir | compress >$$dir.tar.Z; \
	rm -fr $$dir
	
clean:
	-rm -f $(PROGS) psif-text psif-ps *.n
