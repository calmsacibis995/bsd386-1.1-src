# BSDI $Id: Makefile,v 1.2 1993/12/22 16:08:03 polk Exp $

PROG=	a2p
SRCS=	a2p.c hash.c str.c util.c walk.c
CFLAGS+= -I. -I${.CURDIR} -I${.CURDIR}/../perl
CLEANFILES+= a2p.c y.tab.h

.include "../../Makefile.inc"
.include <bsd.prog.mk>
