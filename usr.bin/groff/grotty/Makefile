#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:43 polk Exp $

.include "../Makefile.defs"

PROG=	grotty
SRCS=   tty.cc
LDADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBGCC}

.include "../Makefile.common"
.include <bsd.prog.mk>
