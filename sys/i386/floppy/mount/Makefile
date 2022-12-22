#	@(#)Makefile	5.5 (Berkeley) 5/11/90

PROG=	mount
SRCS=	mount.c
CFLAGS+=-DNFS
NOMAN=noman
DPADD=	${LIBRPC}
LDADD=	-lrpc
LDFLAGS+=-N

.PATH:	/usr/src/sbin/mount

.include <bsd.prog.mk>
