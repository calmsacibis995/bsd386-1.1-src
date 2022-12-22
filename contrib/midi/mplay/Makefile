#	BSDI $Id: Makefile,v 1.1.1.1 1992/08/14 00:14:33 trent Exp $

.include "../../Makefile.inc"

PROG=	mplay
CFLAGS+=-I${.CURDIR}/../lib
LDADD+=	${.CURDIR}/../lib/obj/libmutil.a
DPADD+=	${.CURDIR}/../lib/obj/libmutil.a

.include <bsd.prog.mk>
