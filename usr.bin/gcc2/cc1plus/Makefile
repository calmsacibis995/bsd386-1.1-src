#	BSDI $Id: Makefile,v 1.1 1993/12/16 00:56:53 torek Exp $

# GCC2 C++ compiler.
.include "../Makefile.symlinks"

PROG=	cc1plus
BINDIR=	${LIBEXECDIR}
YFLAGS=	-d
.include "../Makefile.common"
SRCS+=	cp-call.c cp-class.c cp-cvt.c cp-decl.c cp-decl2.c cp-edsel.c \
	cp-errfn.c cp-error.c cp-except.c cp-expr.c cp-gc.c cp-init.c \
	cp-lex.c cp-method.c cp-parse.c cp-pt.c cp-ptree.c cp-search.c \
	cp-spew.c cp-tree.c cp-type2.c cp-typeck.c cp-xref.c
NOMAN=	noman
CLEANFILES+=cp-parse.c cp-parse.h

.depend cp-parse.h: cp-parse.c

.include <bsd.prog.mk>
