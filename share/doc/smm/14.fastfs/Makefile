#	@(#)Makefile	5.4 (Berkeley) 6/30/90

DIR=	smm/14.fastfs
SRCS=	0.t 1.t 2.t 3.t 4.t 5.t 6.t
MACROS=	-ms

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${TBL} ${SRCS} | ${EQN} | ${TROFF} > ${.TARGET}
