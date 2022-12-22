#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	smm/08.timedop
SRCS=	timed.ms
MACROS=	-ms

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
