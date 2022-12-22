#	@(#)Makefile	5.3 (Berkeley) 6/30/90

DIR=	smm/16.sendmail
SRCS=	intro.me
MACROS=	-me

all:	fmt

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${PIC} ${SRCS} | ${ROFF} > ${.TARGET}
