#	@(#)Makefile	4.3 (Berkeley) 6/30/90

DIR=	smm/05.fsck
SRCS=	0.t 1.t 2.t 3.t 4.t

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
