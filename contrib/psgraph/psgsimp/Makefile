#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:17 polk Exp $

PROG=	psgsimp
CFLAGS+=-DPROLOG=\"/usr/contrib/lib/psgraph/psgraph.pro\"
LDADD+=	-lm
DPADD+=	${LIBM}

.include "../../Makefile.inc"
.include <bsd.prog.mk>
