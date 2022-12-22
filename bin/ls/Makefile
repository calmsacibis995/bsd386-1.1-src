#	@(#)Makefile	5.3 (Berkeley) 5/11/90

PROG=	ls
SRCS=	cmp.c ls.c print.c util.c getbsize.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
.PATH:	${.CURDIR}/../df

.include <bsd.prog.mk>
