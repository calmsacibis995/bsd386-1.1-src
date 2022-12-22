#	BSDI $Id: Makefile,v 1.1.1.1 1993/12/22 06:27:33 polk Exp $

.include "../Makefile.defs"

PROG=	eqn
SRCS=   eqn.cc box.cc delim.cc lex.cc limit.cc list.cc main.cc mark.cc \
	other.cc over.cc pile.cc script.cc special.cc sqrt.cc text.cc
LDADD+=	${LIBGROFF} ${LIBGCC}
DPADD+=	${LIBGROFF} ${LIBGCC}
CLEANFILES+=eqn.tab.h eqn.cc

.depend: eqn.tab.h

eqn.tab.h: eqn.cc

.include "../Makefile.common"
.include <bsd.prog.mk>
