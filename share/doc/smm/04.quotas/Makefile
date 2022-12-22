#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	smm/04.quotas
SRCS=	quotas.ms
MACROS=	-ms

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
