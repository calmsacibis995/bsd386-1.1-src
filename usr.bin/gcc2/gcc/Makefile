#	BSDI $Id: Makefile,v 1.1 1993/12/16 00:57:43 torek Exp $

# GCC2 gcc front-end.

PROG=	gcc2
SRCS=	gcc.c obstack.c version.c
.include "../Makefile.symlinks"
CFLAGS+=-I. -I${CFG} -I${.CURDIR}/../common \
	-DDEFAULT_TARGET_MACHINE=\"${MACHINE}\" \
	-DSTANDARD_EXEC_PREFIX=\"${LIBEXECDIR}/\" \
	-DTOOLDIR_BASE_PREFIX=\"./\"
.PATH: ${.CURDIR}/../common
NOMAN=	noman

.depend: config.h tm.h
CLEANFILES+=config.h tm.h

.include <bsd.prog.mk>
