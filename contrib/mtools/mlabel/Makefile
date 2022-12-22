#	BSDI $Id: Makefile,v 1.1.1.1 1992/02/14 02:07:09 trent Exp $

.include "../../Makefile.inc"

PROG=	mlabel
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/libmtools.a
DPADD+=	${.CURDIR}/../lib/obj/libmtools.a

.include <bsd.prog.mk>
