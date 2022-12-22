#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	usd/34.trek
SRCS=	trek.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${ROFF} > ${.TARGET}
