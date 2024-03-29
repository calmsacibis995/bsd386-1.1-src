# Generated automatically from Makefile.in by configure.
#
# Makefile template for screen 
#
# See machine dependant config.h for more configuration options.
#

srcdir = .
VPATH = .

# Where to install screen.

prefix= /usr/contrib
exec_prefix= $(prefix)

bindir= $(exec_prefix)/bin
mandir= $(prefix)/man
libdir= $(prefix)/lib

VERSION = 3.5.2

ETCSCREENRC= $(libdir)/screenrc

CC= gcc
CFLAGS= -O -DETCSCREENRC=\"$(ETCSCREENRC)\"
LDFLAGS=
LIBS= -ltermcap  -lutil

CPP_DEPEND= /usr/bin/cpp -MM

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = $(INSTALL) -m 4555 -o root -g bin
INSTALL_DATA = $(INSTALL) -m 444 -o bin -g bin

AWK = awk

### Chose some debug configuration options:
# -DDEBUG
#	Turn on really heavy debug output. This is written to 
#	/tmp/debug/screen.{front,back} Look at these files and quote 
#	questionable sections when sending bug-reports to the author.
# -DTMPTEST
#	Change the socket directory to a location that does not interfere
#	with the (suid-root) installed screen version. Use that in
#	combination with -DDEBUG
# -DDUMPSHADOW
#	With shadow-pw screen would never dump core. Use this option if you
#	still want to have a core. Use only for debugging.
# -DFORKDEBUG
#	Swap roles of father and son when forking the SCREEN process. 
#	Useful only for debugging.
OPTIONS=
#OPTIONS= -DDEBUG -DTMPTEST


######
######
###### The following lines should be obsolete because of the 'configure' script.
######
######


### If you choose to compile with the tried and true:
#CC= cc
#CFLAGS= -O
#CFLAGS= -g 
### gcc specific CFLAGS:
#CC= gcc
# If your system include files are bad, don't use -Wall
#CFLAGS= -O6 -g #-Wall 
#CFLAGS = -g -fstrength-reduce -fcombine-regs -finline-functions #-Wall

### On some machines special CFLAGS are required:
#M_CFLAGS=
#M_CFLAGS= -Dapollo -A cpu,mathchip -A nansi	# Apollo DN3000/4000/4500
M_CFLAGS= -DBSDI -traditional			# bsd386
#M_CFLAGS= -DISC -D_POSIX_SOURCE		# isc
#M_CFLAGS= -systype bsd43 -DMIPS		# mips
#M_CFLAGS= -fforce-mem -fforce-addr\
# -fomit-frame-pointer -finline-functions -bsd 	# NeXT
#M_CFLAGS= -qlanglvl=ansi			# RS6000/AIX
#M_CFLAGS= -qlanglvl=ansi -D_AIX32		# RS6000/AIX 3.2
#M_CFLAGS= -ansi				# sgi/IRIX 3.x ansi 
#M_CFLAGS= -xansi				# sgi/IRIX 4.x ext ansi 
#M_CFLAGS= -YBSD				# Ultrix 4.x
#M_CFLAGS= -DSVR4=1	# Bob Kline rvk@blink.att.com 80386 Unix SVR4.0
#M_CFLAGS= -D_CX_UX	# Ken Beal kbeal@amber.ssd.csd.harris.com Harris CX/UX

### Choose one of the LIBS setting below:
#LIBS= -ltermcap -lc -lsocket -linet -lsec -lseq	# Sequent/ptx
#LIBS= -ltermcap 			# SunOS, Linux, Apollo,
#					  gould_np1, NeXT, Ultrix
#LIBS= -ltermcap -lelf			# SVR4
#LIBS= -ltermlib -linet -lcposix	# isc
#LIBS= -ltermcap -lmld			# mips (nlist is in mld)
#LIBS= -ltermlib -lsun -lmld #-lc_s	# sgi/IRIX
#LIBS= -lcurses				# RS6000/AIX
#LIBS= -lcrypt_d -ltinfo		# sco32
#LIBS= -lcrypt_i -ltinfo		# sco32
#LIBS= -lcrypt -lsec			# sco322 (msilano@sra.com)
#LIBS= -ltermcap -lcrypt.o -ldir -lx	# SCO XENIX 2.3.4
#LIBS= -ltermcap -lcrypt -ldir -l2.3 -lx	# SCO UNIX XENIX cross dev.
#LIBS= -ltermcap -lelf -lcrypt -lsocket -lnet -lnsl	# Bob Kline SVR4

######
######
###### end of obsolete lines
######
######


SHELL=/bin/sh

CFILES=	screen.c ansi.c fileio.c mark.c misc.c resize.c socket.c \
	search.c tty.c term.c window.c utmp.c loadav.c putenv.c help.c \
	termcap.c input.c attacher.c pty.c process.c display.c comm.c acl.c
OFILES=	screen.o ansi.o fileio.o mark.o misc.o resize.o socket.o \
	search.o tty.o term.o window.o utmp.o loadav.o putenv.o help.o \
	termcap.o input.o attacher.o pty.o process.o display.o comm.o acl.o

all:	screen 

screen: $(OFILES)
	$(CC) $(LDFLAGS) -o $@ $(OFILES) $(LIBS)

obj objdir cleandir:
	@echo make target not supported: $@  



.c.o:
	$(CC) -c -I. -I$(srcdir) $(M_CFLAGS) $(DEFS) $(OPTIONS) $(CFLAGS) $<

install_bin: screen
	-mkdir -p $(bindir)
	$(INSTALL_PROGRAM) screen $(bindir)/screen
#	-chown root $(bindir)/screen-$(VERSION) && chmod 4755 $(bindir)/screen-$(VERSION)
# This doesn't work if $(bindir)/screen is a symlink
#	-if [ -f $(bindir)/screen ] && [ ! -f $(bindir)/screen.old ]; then mv $(bindir)/screen $(bindir)/screen.old; fi
#	rm -f $(bindir)/screen
#	ln -s $(bindir)/screen-$(VERSION) $(bindir)/screen

install: install_bin
	-mkdir -p $(mandir)/man1
	-mkdir -p $(libdir)
	$(INSTALL_DATA) $(srcdir)/etcscreenrc $(ETCSCREENRC)
	$(INSTALL_DATA) $(srcdir)/doc/screen.1 $(mandir)/man1/screen.1
#	-tic ${srcdir}/terminfo/screeninfo.src
# Better do this by hand. E.g. under RCS...
#	cat ${srcdir}/terminfo/screencap >> /etc/termcap
#	@echo termcap entry not installed.

installdirs: mkinstalldirs
# Path leading to ETCSCREENRC and Socketdirectory not checked.
	$(srcdir)/mkinstalldirs $(bindir) $(mandir)

uninstall:
	rm -f $(bindir)/screen-$(VERSION)
	rm -f $(bindir)/screen
	mv $(bindir)/screen.old $(bindir)/screen
	rm -f $(ETCSCREENRC)
	rm -f $(mandir)/man1/screen.1

shadow:
	mkdir shadow;
	cd shadow; ln -s ../*.[ch] ../*.in ../*.sh ../configure ../doc ../terminfo .
	rm -f shadow/term.h shadow/tty.c shadow/comm.h shadow/osdef.h

term.h: term.c term.sh
	AWK=$(AWK) srcdir=$(srcdir) . $(srcdir)/term.sh

tty.c:	tty.sh 
	sh $(srcdir)/tty.sh tty.c

comm.h: comm.c comm.sh config.h
	AWK=$(AWK) CC="$(CC)" srcdir=${srcdir} . $(srcdir)/comm.sh

osdef.h: osdef.sh config.h osdef.h.in
	CC="$(CC)" srcdir=${srcdir} . $(srcdir)/osdef.sh

docs:
	cd doc; $(MAKE) dvi info

dvi info:
	cd doc; $(MAKE) $@

mostlyclean:
	rm -f $(OFILES) screen

clean celan: mostlyclean
	rm -f tty.c term.h comm.h osdef.h

# Delete all files from the current directory that are created by 
# configuring or building the program.
# building of term.h/comm.h requires awk. Keep it in the distribution
# we keep config.h, as this file knows where 'make dist' finds the ETCSCREENRC.
distclean:	mostlyclean
	rm -f screen-$(VERSION).tar screen-$(VERSION).TZ
	rm -f config.status Makefile

# Delete everything from the current directory that can be
# reconstructed with this Makefile.
realclean:	distclean
	rm -f config.h

TAGS: $(CFILES)
	ctags $(CFILES) *.h
	ctags -e $(CFILES) *.h

dist: screen-$(VERSION).tar.z

screen-$(VERSION).tar: term.h comm.h tty.c
	-rm -rf dist
	mkdir dist
	mkdir dist/screen-$(VERSION)
	ln acl.h ansi.h display.h extern.h mark.h os.h overlay.h \
	   patchlevel.h rcs.h screen.h window.h osdef.h.in \
	   term.sh tty.sh comm.sh osdef.sh newsyntax \
	   acl.c ansi.c attacher.c comm.c display.c window.c fileio.c help.c \
	   input.c loadav.c mark.c misc.c process.c pty.c putenv.c \
	   screen.c search.c socket.c term.c termcap.c utmp.c resize.c \
	   ChangeLog COPYING INSTALL NEWS \
	   dist/screen-$(VERSION)
	ln configure.in configure dist/screen-$(VERSION)
	sed -e 's@"/local/screens@"/tmp/screens@' -e 's@"/local@"/usr/local@g' < config.h.in > dist/screen-$(VERSION)/config.h.in
	sed -e 's@[	 ]/local@ /usr/local@g' -e 's/^CFLAGS = -g/CFLAGS = -O/' < Makefile.in > dist/screen-$(VERSION)/Makefile.in
	ln term.h dist/screen-$(VERSION)/term.h.dist
	ln comm.h dist/screen-$(VERSION)/comm.h.dist
	ln tty.c dist/screen-$(VERSION)/tty.c.dist
	ln README dist/screen-$(VERSION)/README
	mkdir dist/screen-$(VERSION)/terminfo
	cd terminfo; ln 8bits README checktc.c screen-sco.mail screencap \
	  screeninfo.src test.txt tetris.c \
	  ../dist/screen-$(VERSION)/terminfo
	sed -e 's/^startup/#startup/' -e 's/^autodetach/#autodetach/' < $(ETCSCREENRC) > dist/screen-$(VERSION)/etcscreenrc 
	cp $(HOME)/.iscreenrc dist/screen-$(VERSION)/.iscreenrc
	mkdir dist/screen-$(VERSION)/doc
	ln doc/fdpat.ips dist/screen-$(VERSION)/doc/fdpat.ps
	sed -e 's@/local/emacs@/usr/local@g' < doc/Makefile.in > dist/screen-$(VERSION)/doc/Makefile.in
	cd doc; ln screen.1 screen.texinfo \
	  ../dist/screen-$(VERSION)/doc
	cd dist; tar chf ../screen-$(VERSION).tar screen-$(VERSION)
	rm -rf dist

screen-$(VERSION).tar.z: screen-$(VERSION).tar
	gzip -f screen-$(VERSION).tar

# Perform self-tests (if any).
check:

lint:
	lint -I. $(CFILES)

saber:
	#load $(CFLAGS) screen.c ansi.c $(LIBS)

mdepend: $(CFILES) term.h
	@rm -f DEPEND ; \
	for i in ${CFILES} ; do \
	  echo "$$i" ; \
	  echo `echo "$$i" | sed -e 's/.c$$/.o/'`": $$i" `\
            cc -E $$i |\
            grep '^# .*"\./.*\.h"' |\
            sort -t'"' -u +1 -2 |\
            sed -e 's/.*"\.\/\(.*\)".*/\1/'\
          ` >> DEPEND ; \
	done


depend: $(CFILES) term.h
	cp Makefile Makefile~
	sed -e '/\#\#\# Dependencies/q' < Makefile > tmp_make
	for i in $(CFILES); do echo $$i; $(CPP_DEPEND) $$i >> tmp_make; done 
	mv tmp_make Makefile

screen.o socket.o: Makefile

### Dependencies:
screen.o : screen.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h patchlevel.h extern.h 
ansi.o : ansi.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
fileio.o : fileio.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
mark.o : mark.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h mark.h extern.h 
misc.o : misc.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
resize.o : resize.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
socket.o : socket.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
search.o : search.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h mark.h extern.h 
tty.o : tty.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
term.o : term.c rcs.h term.h 
window.o : window.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
utmp.o : utmp.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
loadav.o : loadav.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
putenv.o : putenv.c rcs.h config.h 
help.o : help.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
termcap.o : termcap.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
input.o : input.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
attacher.o : attacher.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
pty.o : pty.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
process.o : process.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
display.o : display.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h \
  comm.h overlay.h term.h display.h window.h extern.h 
comm.o : comm.c rcs.h config.h acl.h comm.h 
acl.o : acl.c rcs.h config.h screen.h os.h osdef.h ansi.h acl.h comm.h \
  overlay.h term.h display.h window.h extern.h 
