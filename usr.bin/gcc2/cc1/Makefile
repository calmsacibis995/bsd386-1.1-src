#	BSDI $Id: Makefile,v 1.1 1993/12/16 00:56:40 torek Exp $

# GCC2 C compiler.
.include "../Makefile.symlinks"

PROG=	cc1
BINDIR=	${LIBEXECDIR}
YFLAGS=	-d
.include "../Makefile.common"
SRCS+=	c-aux-info.c c-convert.c c-decl.c c-iterate.c c-lang.c c-lex.c \
	c-parse.y c-pragma.c c-typeck.c
NOMAN=	noman
CLEANFILES+=c-parse.c c-parse.h

.depend c-parse.h: c-parse.c

.include <bsd.prog.mk>
