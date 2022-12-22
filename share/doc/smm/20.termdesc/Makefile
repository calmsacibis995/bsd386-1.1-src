#	@(#)Makefile	6.2 (Berkeley) 6/30/90

DIR=	smm/20.termdesc
SRCS=	termdesc.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
