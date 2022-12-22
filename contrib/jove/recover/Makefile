#	BSDI $Id: Makefile,v 1.1.1.1 1992/02/19 05:00:05 polk Exp $

PROG=	recover
CFLAGS+=-I. -I${.CURDIR} -I${.CURDIR}/../include -I${.CURDIR}/../jove
SRCS=	recover.c tune.c
CLEANFILES+=	tune.c
NOMAN=noman
BINDIR=	/usr/contrib/lib/jove

.depend: tune.c

tune.c: 
	ln -s ${.CURDIR}/../jove/tune.c tune.c
	
.include <bsd.prog.mk>
