#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	ps1/14.sccs
SRCS=	sccs.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
