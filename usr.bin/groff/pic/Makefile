#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:31 polk Exp $

.include "../Makefile.defs"

PROG=	pic
SRCS=   pic.cc common.cc lex.cc main.cc object.cc tex.cc troff.cc
LDADD+=	${LIBGROFF} ${LIBM} ${LIBGCC}
DPADD+=	${LIBGROFF} ${LIBM} ${LIBGCC}
CLEANFILES+=pic.tab.h pic.cc

.depend: pic.tab.h

pic.tab.h: pic.cc

.include "../Makefile.common"
.include <bsd.prog.mk>
