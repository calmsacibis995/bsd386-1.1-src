#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	ps1/13.rcs
SRCS=	rcs.ms
MACROS=	-ms

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
