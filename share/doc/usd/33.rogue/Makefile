#	@(#)Makefile	6.3 (Berkeley) 6/30/90

DIR=	usd/33.rogue
SRCS=	rogue.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}
