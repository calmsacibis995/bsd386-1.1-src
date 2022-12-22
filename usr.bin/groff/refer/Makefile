#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:46 polk Exp $

.include "../Makefile.defs"

PROG=	refer
SRCS=   label.cc command.cc ref.cc refer.cc token.cc
LDADD+=	${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBBIB} ${LIBGROFF} ${LIBM} ${LIBGCC}
CLEANFILES+=label.tab.h label.cc

.include "../Makefile.common"
.include <bsd.prog.mk>
