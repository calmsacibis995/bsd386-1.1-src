#	@(#)Makefile	5.1 (Berkeley) 5/3/91

PROG=	newfs
SRCS=	dkcksum.c newfs.c mkfs.c
CFLAGS+=-DMFS
NOMAN=	noman
LDFLAGS=-N
.PATH:	/usr/src/sbin/newfs /usr/src/sbin/disklabel

.include <bsd.prog.mk>
