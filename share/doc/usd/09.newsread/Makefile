#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/09.newsread
SRCS=	tmac.n howto.mn

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
