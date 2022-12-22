#	BSDI $Id: Makefile,v 1.3 1994/01/08 18:31:59 donn Exp $

# GCC2 G++ front end.
.include "../Makefile.symlinks"

PROG=	g++2
SRCS=	g++.c
CFLAGS+=-I. -I${CFG} -I${.CURDIR}/../common \
	-DGCC_NAME=\"gcc2\"
NOMAN=	noman

LINKS=	${BINDIR}/g++2 ${BINDIR}/g++

.depend: config.h tm.h
CLEANFILES+=config.h tm.h

.include <bsd.prog.mk>
