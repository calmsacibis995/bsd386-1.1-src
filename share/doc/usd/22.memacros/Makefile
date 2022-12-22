#	@(#)Makefile	6.4 (Berkeley) 6/30/90

DIR=	usd/22.memacros
SRCS=	intro.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
