#	@(#)Makefile	5.1 (Berkeley) 5/3/91

PROG=	disklabel
SRCS=	disklabel.c dkcksum.c
CFLAGS+=-I/usr/src/sbin/disklabel
NOMAN=	noman
LDFLAGS=-N
LDADD+=	-lutil
.PATH:	/usr/src/sbin/disklabel

.include <bsd.prog.mk>
