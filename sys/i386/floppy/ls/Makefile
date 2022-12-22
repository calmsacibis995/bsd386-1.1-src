#	@(#)Makefile	5.1 (Berkeley) 5/3/91

PROG=	ls
SRCS=	cmp.c ls.c print.c util.c getbsize.c
DPADD=	${LIBUTIL}
LDADD=	-lutil
NOMAN=	noman
LDFLAGS=-N
.PATH:	/usr/src/bin/ls /usr/src/bin/df

.include <bsd.prog.mk>
