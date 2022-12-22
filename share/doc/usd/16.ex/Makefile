#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/16.ex
SRCS=	ex.rm
MACROS=	-ms
CLEANFILES=summary.*

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER} summary.${PRINTER}

summary.${PRINTER}: ex.summary
	${TBL} ex.summary | ${ROFF} > ${.TARGET}

paper.${PRINTER}: ${SRCS} summary.${PRINTER}
	${ROFF} ${SRCS} > ${.TARGET}
