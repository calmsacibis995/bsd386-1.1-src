#	BSDI $Id: Makefile,v 1.4 1993/10/28 19:59:08 donn Exp $
#	@(#)Makefile	6.4 (Berkeley) 2/21/91

PROG=		cpp
BINDIR=		/usr/libexec
SRCS=		cpp.c cexp.y version.c obstack.c
CFLAGS+=	-I. -I$(.CURDIR)/../cc1 -I$(.CURDIR)/../cc1/config \
		-DGCC_INCLUDE_DIR=\"/usr/include/gcc\" \
		-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\"
YFLAGS=
.PATH:		$(.CURDIR)/../cc1

.include <bsd.prog.mk>

afterinstall:
	install -c -o $(BINOWN) -g $(BINGRP) -m $(BINMODE) \
		$(.CURDIR)/usr.bin.cpp.sh $(DESTDIR)/usr/bin/cpp

.include "../Makefile.symlinks"

.depend: config.h
