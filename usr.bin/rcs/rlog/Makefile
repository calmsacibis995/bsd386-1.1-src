#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/21 04:23:14 polk Exp $

.include "../../Makefile.inc"

PROG=	rlog
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/librcs.a
DPADD+=	${.CURDIR}/../lib/obj/librcs.a

.include <bsd.prog.mk>
