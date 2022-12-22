#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:44 polk Exp $

.include "../Makefile.defs"

PROG=	grodvi
SRCS=   dvi.cc
LDADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBGCC}

.include "../Makefile.common"
.include <bsd.prog.mk>
