#	BSDI $Id: Makefile,v 1.3 1993/11/28 00:00:46 donn Exp $
#	@(#)Makefile	5.3 (Berkeley) 6/29/90

PROG=	nfsd
CFLAGS+=-DNFS
MAN8=	nfsd.0
DPADD=	${LIBRPC} ${LIBUTIL}
LDADD=	-lrpc -lutil

.include <bsd.prog.mk>
