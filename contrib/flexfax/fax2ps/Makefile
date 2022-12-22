#	BSDI $Id: Makefile,v 1.2 1994/01/15 00:03:37 torek Exp $

PROG=	fax2ps
SRCS=	fax2ps.c faxdecode.c
CFLAGS+=${FAXINCS} -DSIGCLD=SIGCHLD
DPADD=	${FAXLIBS} ${LIBUTIL} ${LIBG++} ${LIBM}
LDADD=	${FAXLIBS} -lutil -lg++ -lm -lc

${PROG}.1: ${.CURDIR}/../man/${PROG}.1
	sed ${MANSED} < ${.ALLSRC} > ${.TARGET}
CLEANFILES+=${PROG}.1

.include <bsd.prog.mk>
