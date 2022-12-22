#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:48 polk Exp $

.include "../Makefile.defs"

PROG=	lookbib
SRCS=	lookbib.cc
LDADD+=	${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBGCC}

.include "../Makefile.common"
.include <bsd.prog.mk>
