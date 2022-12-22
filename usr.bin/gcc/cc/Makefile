#	BSDI $Id: Makefile,v 1.3 1993/03/08 07:16:00 polk Exp $
#	@(#)Makefile	6.2 (Berkeley) 3/25/91

PROG=		cc
SRCS=		cc.c version.c obstack.c
CFLAGS+=	-I. -I$(.CURDIR)/../cc1 -I$(.CURDIR)/../cc1/config
MLINKS+=	cc.1 gcc.1
.PATH:		$(.CURDIR)/../cc1

.include "../../Makefile.inc"

afterinstall:
	-rm -f $(DESTDIR)$(BINDIR)/gcc
	cd $(DESTDIR)$(BINDIR); ln -s cc gcc

.include <bsd.prog.mk>
.include "../Makefile.symlinks"

.depend: config.h
