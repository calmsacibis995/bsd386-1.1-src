#	@(#)Makefile	5.2 (Berkeley) 6/30/90

DIR=	smm/10.newsop
SRCS=	tmac.n install.mn

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
