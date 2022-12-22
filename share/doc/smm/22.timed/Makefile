#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	smm/22.timed
SRCS=	timed.ms
MACROS=	-ms

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${SOELIM} ${SRCS} | ${TBL} | ${ROFF} > ${.TARGET}
