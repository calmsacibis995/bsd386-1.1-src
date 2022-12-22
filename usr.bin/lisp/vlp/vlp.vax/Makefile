#$Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/vlp/vlp.vax/Makefile,v 1.2 1992/01/04 19:00:41 kolstad Exp $
#
#			-[Thu Feb 10 13:15:17 1983 by jkf]-

# Makefile for VLP

# files required:
#	vlp.l	- lisp program to read source and add troff control
#		characters.
#	vlpmacs - troff macros used by vlp.
#
# vlp and vlpmacs are put in DESTDIR
# If DESTDIR is changed, then the source (vlp.l) must be modified since
# it explicitly points to vlpmacs.

DESTDIR =
.DEFAULT:vlp

vlp:	vlp.l
	/usr/ucb/liszt -qar -o vlp vlp.l

install: vlp vlpmacs
	install -o bin -g bin -m 755 vlp ${DESTDIR}/usr/bin
	install -c -o bin -g bin -m 664 vlpmacs ${DESTDIR}/usr/libdata

clean:
	rm -f vlp

depend:
