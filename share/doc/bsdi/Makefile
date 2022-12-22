#       @(#)BSDI $Id: Makefile,v 1.2 1994/01/31 09:09:16 donn Exp $

DIR=	bsdi
SRCS=	config.n driver.n
MACROS=	-pte -me

all:

.include <bsd.doc.mk>

fmt:	config.${PRINTER} driver.${PRINTER}

config.${PRINTER}: config.n
	${ROFF} config.n > ${.TARGET}

driver.${PRINTER}: driver.n
	${ROFF} driver.n > ${.TARGET}
