#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:41 polk Exp $

.include "../Makefile.defs"

PROG=	grops
SRCS=   ps.cc psrm.cc
LDADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBDRIVER} ${LIBGROFF} ${LIBM} ${LIBGCC}

.include "../Makefile.common"
.include <bsd.prog.mk>
