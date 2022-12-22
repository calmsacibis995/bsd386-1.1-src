#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/21 04:23:11 polk Exp $

.include "../../Makefile.inc"

PROG=	rcsmerge
SRCS=	rcsmerge.c merger.c
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

.PATH:	${.CURDIR}/../merge

.include <bsd.prog.mk>
