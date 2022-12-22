#	BSDI $Id: Makefile,v 1.2 1993/03/08 06:26:44 polk Exp $
#	@(#)Makefile	5.3 (Berkeley) 5/11/90

PROG=	init
MAN8=	init.0
CFLAGS+=-DSECURE
DPADD=	${LIBUTIL}
LDADD=	-lutil
BINMODE=500

.include <bsd.prog.mk>
