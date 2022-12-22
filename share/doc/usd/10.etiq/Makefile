#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/10.etiq
SRCS=	tmac.n manner.mn

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} > ${.TARGET}
