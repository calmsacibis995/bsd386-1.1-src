#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/21 04:23:09 polk Exp $

.include "../../Makefile.inc"

PROG=	rcs
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

MAN1=rcs.0 rcsintro.0
MAN5=rcsfile.0

.include <bsd.prog.mk>
