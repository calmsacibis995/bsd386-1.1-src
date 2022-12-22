#	BSDI:	$Id: Makefile,v 1.1.1.1 1994/01/03 23:13:48 polk Exp $

PROG=	basica
CFLAGS+=-I. -I${.CURDIR}
SRCS=	parser.c expr.c mat.c run.c stmt.c temps.c lex.c \
	lines.c linpack.c linpackc.c main.c misc.c strings.c \
	symbols.c trees.c typecheck.c ops.c renum.c globals.c
MLINKS= basica.1 basic.1
LDADD+=	-lm
DPADD+=	${LIBMATH}
CLEANFILES+= parser.c tokens.h y.tab.h

parser.c: parser.y parser.sed
	yacc -d ${.CURDIR}/parser.y
	sed -f ${.CURDIR}/parser.sed < y.tab.c > parser.c && rm y.tab.c
	sed -f ${.CURDIR}/parser.sed < y.tab.h > y.tab.h.new
	${.CURDIR}/update y.tab.h.new tokens.h

.include <bsd.prog.mk>
