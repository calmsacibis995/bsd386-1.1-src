#	@(#)Makefile	5.1 (Berkeley) 6/30/90

DIR=	usd/04.csh
MISSING= csh.a
SRCS=	tabs csh.1 csh.2 csh.3 csh.4 csh.g

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} > ${.TARGET}
