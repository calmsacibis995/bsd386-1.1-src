#	BSDI $Id: Makefile,v 1.1 1994/01/29 16:38:04 polk Exp $

.include "../../Makefile.inc"

PROG=	mkmanifest
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/libmtools.a
DPADD+=	${.CURDIR}/../lib/obj/libmtools.a

.include <bsd.prog.mk>
