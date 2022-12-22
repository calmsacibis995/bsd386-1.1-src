#	BSDI $Id: Makefile,v 1.2 1992/11/22 22:41:18 polk Exp $

.include "../../Makefile.inc"

PROG=	mcopy
CFLAGS+=-I${.CURDIR}/../lib -fwritable-strings
LDADD+=	${.CURDIR}/../lib/obj/libmtools.a
DPADD+=	${.CURDIR}/../lib/obj/libmtools.a

.include <bsd.prog.mk>
