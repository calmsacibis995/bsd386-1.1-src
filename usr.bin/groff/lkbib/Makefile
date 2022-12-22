#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:51 polk Exp $

.include "../Makefile.defs"

PROG=	lkbib
SRCS=	lkbib.cc
LDADD+=	${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBGCC}

.include "../Makefile.common"
.include <bsd.prog.mk>
