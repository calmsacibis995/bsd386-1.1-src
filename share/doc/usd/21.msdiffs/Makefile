#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/21.msdiffs
SRCS=	ms.diffs
MACROS=	-ms

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
