#       @(#)BSDI $Id: Makefile,v 1.1 1993/12/19 19:52:17 sanders Exp $

DIR=	sendmail/op
SRCS=	op.me
MACROS=	-C -pte -me

all:

.include <bsd.doc.mk>

fmt:	paper.${PRINTER}

paper.${PRINTER}: ${SRCS}
	${ROFF} ${SRCS} > ${.TARGET}
