#	BSDI $Id: Makefile,v 1.1 1993/12/16 00:57:10 torek Exp $

# GCC2 preprocessor.
.include "../Makefile.symlinks"

PROG=	cpp
BINDIR=	${LIBEXECDIR}
SRCS=   cccp.c cexp.c obstack.c version.c
CFLAGS+=-I. -I${CFG} -I${.CURDIR}/../common \
	-DGCC_INCLUDE_DIR=\"/usr/include\" \
	-DGPLUSPLUS_INCLUDE_DIR=\"/usr/include/g++\" \
	-DTOOL_INCLUDE_DIR=\"/usr/include\"
YFLAGS=
.PATH: ${.CURDIR}/../common
NOMAN=	noman

CLEANFILES+=cexp.c

.depend: cexp.c config.h tm.h
CLEANFILES+=config.h tm.h

.include <bsd.prog.mk>
